#include <linux/rtnetlink.h>
#include <net/if.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "vpn_status.h"

///////////////////////
// VPN STATUS /////////
///////////////////////
//
//	monitor the status
//	of VPN connections
//
//

//	todo:
//		* allow arbitrary action
//		* maybe also check whether
//			interfaces are up

char g_datfile[PATH_MAX] = {'\0'};	///< path to data file

/// Initialize vpn_status
int init() {
	char logfile[PATH_MAX];
  char *user = NULL;
  if ((user = getlogin()) == NULL) {
    printf("error: cannot determine current user\n");
    return 1;
  }
	snprintf(logfile,  sizeof(logfile),  "/tmp/%s.log", PROCNAME);
  snprintf(g_datfile,  sizeof(g_datfile), "/home/%s/.%s", user, PROCNAME);

	// init logging
	return !log_init(ELogVerbose, ELogStyleVerbose, logfile);
}

/// Parse a given netlink message
int parse_nlmsg(char *nlmsg_buf, ssize_t buflen, ifdata_t *p_head) {
	struct nlmsghdr *nh;

	// Loop over the netlink header contained in the message
	for (nh = (struct nlmsghdr *) nlmsg_buf; NLMSG_OK(nh, buflen); nh = NLMSG_NEXT(nh, buflen)) {

		if (nh->nlmsg_type == RTM_NEWLINK) {
			// new link

			ifdata_t *p_dat = get_ifdata(nh);
			if (p_dat == NULL) {
				log_debug("invalid interface data gotten: ignoring");
				continue;
			}

			ifdata_t *p_res = find_ifdata(p_dat, p_head);
			if (p_res == NULL) {
				add_ifdata(p_dat, &p_head);
				if ((strncmp(p_dat->ifname, "tun", 3) == 0) ||
						(strncmp(p_dat->ifname, "tap", 3) == 0)) {
					log_notice("\n--\nA VPN connection was added\n--");
					if (file_write(VPN_UP) != 0) {
						log_error("Failed to write to file");
					}
				}

				log_notice("Interface added:");
				log_notice("%-12s: %s", p_dat->ifname, p_dat->mac);

				log_notice("New state:");
				print_ifinfo(p_head);
			} else {
				log_debug("Changes detected on interface %s:%s", p_res->ifname, p_res->mac);
				free(p_dat);
				continue;
			}
		} else if (nh->nlmsg_type == RTM_DELLINK) {
			// link deleted

			ifdata_t *p_dat = get_ifdata(nh);
			if (p_dat == NULL) {
				log_debug("invalid interface data gotten: ignoring");
				continue;
			}

			ifdata_t *p_res = find_ifdata(p_dat, p_head);
			if (p_res == NULL) {
				log_debug("no data found on downed interface!");
			} else {
				if ((strncmp(p_res->ifname, "tun", 3) == 0) ||
						(strncmp(p_res->ifname, "tap", 3) == 0)) {
					log_notice("\n--\nA VPN connection was severed\n--");
					if (file_write(VPN_DOWN) != 0) {
						log_error("Failed to write to file");
					}
				}
				del_ifdata(p_res, &p_head);
			}

			log_notice("Interface removed:");
			log_notice("%-12s: %s", p_dat->ifname, p_dat->mac);
			free(p_dat);

			log_notice("New state:");
			print_ifinfo(p_head);
		}
	}
	return 0;
}

ifdata_t *get_ifdata(struct nlmsghdr *hdr) {
	struct ifinfomsg *ifi;
	struct rtattr *attr;
	ssize_t attr_len;
	unsigned char *ptr = NULL;

	// prepare new ifdata struct
	ifdata_t *p_new = (ifdata_t*)malloc(sizeof(ifdata_t));
	memset(p_new->ifname, '\0', sizeof(p_new->ifname));
	memset(p_new->mac, '\0', sizeof(p_new->mac));
	p_new->next = NULL;
	p_new->prev = NULL;

	ifi = (struct ifinfomsg *) NLMSG_DATA(hdr);
	attr = IFLA_RTA(ifi);
	attr_len = hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi));
	for (; RTA_OK(attr, attr_len); attr = RTA_NEXT(attr, attr_len)) {
		switch(attr->rta_type) {
			case IFLA_ADDRESS:
				ptr = (unsigned char *) RTA_DATA(attr);
				snprintf(p_new->mac, sizeof(p_new->mac),
									"%02x:%02x:%02x:%02x:%02x:%02x",
									ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
				break;
			case IFLA_IFNAME:
				snprintf(p_new->ifname, sizeof(p_new->ifname),
									"%s", (char *)RTA_DATA(attr));
				break;
			default:
				break;
		}
	}

	if (strlen(p_new->ifname) == 0) {
		log_debug("no usable interface data gatered, ignoring %7s:%s",
								p_new->ifname, p_new->mac);
		free(p_new);
		p_new = NULL;
	}
	return p_new;
}

/// Add info (ifname && mac) for all current interfaces to ifdata linked list
int fetch_ifinfo(ifdata_t **head) {
	int nl_sock = -1;
	struct sockaddr_nl src_addr = {AF_NETLINK, 0, (__u32) getpid(), 0};
	struct sockaddr_nl dest_addr = {AF_NETLINK, 0, 0, 0};

	// create socket
	if ((nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
		log_error("Failed to create socket");
		return 1;
	}

	// bind socket
	if (bind(nl_sock, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
		log_error("Failed to bind fetcher socket: %s", strerror(errno));
		close(nl_sock);
		return 1;
	}

	struct {
		struct nlmsghdr nh;
		struct rtgenmsg rtg;
	} request = {0};
	char buffer[4096];
	struct iovec iov = {0};
	struct msghdr msg = {0};
	struct nlmsghdr *nh;
	ssize_t n;

	// Prepare address request for the kernel
	request.nh.nlmsg_len 			= NLMSG_LENGTH(sizeof(request.rtg));
	request.nh.nlmsg_type 		= RTM_GETLINK;
	request.nh.nlmsg_flags 		= NLM_F_REQUEST | NLM_F_DUMP;
	request.rtg.rtgen_family 	= AF_PACKET;

	iov.iov_base 		= &request;
	iov.iov_len 		= request.nh.nlmsg_len;
	msg.msg_name 		= &dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov 		= &iov;
	msg.msg_iovlen 	= 1;

	// Send the request message
	if (sendmsg(nl_sock, (struct msghdr *) &msg, 0) < 0) {
		log_error("Error sending interface request to netlink: %s", strerror(errno));
		close(nl_sock);
		return 1;
	}

	// Prepare iovec for the response
	memset(&iov, 0, sizeof(iov));
	iov.iov_base 	= buffer;
	iov.iov_len 	= sizeof(buffer);

	int done = 0;
	int got_vpn = 0;
	while (!done) {
		// Receive the response from netlink
		if ((n = recvmsg(nl_sock, &msg, 0)) < 0) {
			log_error("Error receiving message from netlink: %s", strerror(errno));
			close(nl_sock);
			return 1;
		}

		// go though messages
		for (nh = (struct nlmsghdr *) buffer; NLMSG_OK(nh, n); nh = NLMSG_NEXT(nh, n)) {
			if (nh->nlmsg_type == RTM_NEWLINK) {
				ifdata_t *p_new = get_ifdata(nh);
				add_ifdata(p_new, head);
				if ((strncmp(p_new->ifname, "tun", 3) == 0) ||
						(strncmp(p_new->ifname, "tap", 3) == 0)) {
					got_vpn = 1;
				}
			} else if (nh->nlmsg_type == NLMSG_DONE) {
				log_debug("all interfaces received");
				done = 1;
			} else {
				log_debug("Got another type message: type %d", nh->nlmsg_type);
			}
		}
	}

	if (file_write(got_vpn) != 0) {
		log_error("Failed to write to file");
	}
	close(nl_sock);
	return 0;
}

ifdata_t *find_ifdata(ifdata_t *ifquery, ifdata_t *head) {
	// if contains ifname or mac
	// if either matches, return this interface's full data
	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
		if ((strncmp(p_tmp->ifname, ifquery->ifname, sizeof(p_tmp->ifname)) == 0) ||
				(strncmp(p_tmp->mac, ifquery->mac, sizeof(p_tmp->mac)) == 0)) {
			log_debug("found matching interface: %s", p_tmp->ifname);
			return p_tmp;
		}
	}
	return NULL;
}

void print_ifinfo(ifdata_t *head) {
		for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
			log_notice("%-12s: %s", p_tmp->ifname, p_tmp->mac);
		}
}

void add_ifdata(ifdata_t *p_new, ifdata_t **head) {
	if (*head == NULL) {
		*head = p_new;
	} else {
		ifdata_t *p_tmp = *head;
		while (p_tmp->next != NULL) {
			p_tmp = p_tmp->next;
		}

		p_tmp->next = p_new;
		p_new->prev = p_tmp;
	}
}

void del_ifdata(ifdata_t *p_del, ifdata_t **head) {
	if (p_del->prev != NULL) {
		p_del->prev->next = p_del->next;
	} else {
		// head!
		*head = p_del->next;
	}

	if (p_del->next != NULL) {
		p_del->next->prev = p_del->prev;
	}
	free(p_del);
}

int file_write(int op) {
	FILE *fd = fopen(g_datfile, "w");
	if (fd == NULL) {
		log_error("Failed to open file at %s", g_datfile);
		return 1;
	}

	if (op == VPN_UP) {
		fprintf(fd, "%s", VPN_SYMBOL);
	} else if (op == VPN_DOWN) {
		fprintf(fd, "%s", "");
	}

	fclose(fd);
	return 0;
}