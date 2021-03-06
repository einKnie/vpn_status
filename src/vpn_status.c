/*                          _        _
*                         | |      | |
* __   ___ __  _ __    ___| |_ __ _| |_ _   _ ___
* \ \ / / '_ \| '_ \  / __| __/ _` | __| | | / __|
*  \ V /| |_) | | | | \__ \ || (_| | |_| |_| \__ \
*   \_/ | .__/|_| |_| |___/\__\__,_|\__|\__,_|___/
*       | |
*       |_|  <einKnie@gmx.at>
*
*/

#include <string.h>
#include <errno.h>
#include "log.h"
#include "vpn_status.h"

// for some reason this ifi flag is not defined even though it should be
#ifndef IFF_LOWER_UP
#	define IFF_LOWER_UP (1<<16)
#endif
#ifndef IFF_DORMANT
#	define IFF_DORMANT (1<<17)
#endif
///////////////////////
// VPN STATUS /////////
///////////////////////
//
//	monitor the status
//	of VPN connections
//
//

//	todo:
//		* allow arbitrary action ~ kinda done
//		* actual ui (maybe w/ curses)
//		* actions settable for different interfaces (per name or mac))
//		* config via configfile
//

char g_datfile[PATH_MAX]  = {'\0'};  ///< path to data file
char g_upcmd[PATH_MAX]    = {'\0'};  ///< up script
char g_downcmd[PATH_MAX]  = {'\0'};  ///< down script

/// Initialize vpn_status
int init(ifdata_t **head, const char *upscript, const char *downscript) {
	char *user = NULL;

	// prepare data file
	if ((user = getlogin()) == NULL) {
		log_error("error: cannot determine current user\n");
		return 1;
	}
	snprintf(g_datfile,  sizeof(g_datfile), "/home/%s/.%s", user, PROCNAME);

	// get current interface data
	if (fetch_ifinfo(head) != 0) {
		log_error("Failed to fetch interface information");
		return 1;
	}

	// set up && down actions
	strncpy(g_upcmd, upscript, sizeof(g_upcmd));
	strncpy(g_downcmd, downscript, sizeof(g_downcmd));

	return 0;
}

/// Parse a given netlink message
int parse_nlmsg(char *nlmsg_buf, ssize_t buflen, ifdata_t *p_head) {
	struct nlmsghdr *nh = NULL;

	// Loop over the netlink header contained in the message
	for (nh = (struct nlmsghdr *) nlmsg_buf; NLMSG_OK(nh, buflen); nh = NLMSG_NEXT(nh, buflen)) {

		ifdata_t *p_dat = get_ifdata(nh);
		if (p_dat == NULL) {
			log_debug("invalid interface data gotten: ignoring");
			continue;
		}

		if (nh->nlmsg_type == RTM_NEWLINK) {
			// new link

			// check if interface already exists
			ifdata_t *p_res = find_ifdata(p_dat, p_head);
			if (p_res == NULL) {
				add_ifdata(p_dat, &p_head);
				if (is_tun_or_tap(p_dat->ifname)) {
					log_notice("\n--\nA VPN interface was added\n--");
					if (p_dat->up) {
						if (call_script(p_dat->up) != 0) {
							log_error("Failed to execute cmd");
						}
					}
				}

				log_notice("Interface added:");
				log_notice("%-12s: %s", p_dat->ifname, p_dat->mac);

			} else {
				log_notice("Changes detected on interface %s:%s (%c)",
					p_res->ifname, p_res->mac, p_dat->up ? 'o' : 'x');

				if (p_res->up != p_dat->up) {
					p_res->up = p_dat->up;
					if(is_tun_or_tap(p_res->ifname)) {
						if (call_script(p_dat->up) != 0) {
							log_error("Failed to execute cmd");
						}
					}
					log_notice("New state:");
					print_ifinfo(p_head);
				}
				free(p_dat);
				continue;
			}

		} else if (nh->nlmsg_type == RTM_DELLINK) {
			// link deleted

			ifdata_t *p_res = find_ifdata(p_dat, p_head);
			if (p_res == NULL) {
				log_debug("no data found on removed interface!");
			} else {
				if (is_tun_or_tap(p_res->ifname)) {
					log_notice("\n--\nA VPN interface was removed\n--");
					if (call_script(VPN_DOWN) != 0) {
						log_error("Failed to execute cmd");
					}
				}
				del_ifdata(p_res, &p_head);
			}

			log_notice("Interface removed:");
			log_notice("%-12s: %s", p_dat->ifname, p_dat->mac);
			free(p_dat);


		} else if (nh->nlmsg_type == RTM_GETLINK) {
			log_error("got a getlink meesage!\n\n");
		}
		log_notice("New state:");
		print_ifinfo(p_head);
	}
	return 0;
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
	request.nh.nlmsg_len      = NLMSG_LENGTH(sizeof(request.rtg));
	request.nh.nlmsg_type     = RTM_GETLINK;
	request.nh.nlmsg_flags    = NLM_F_REQUEST | NLM_F_DUMP;
	request.rtg.rtgen_family  = AF_PACKET;

	iov.iov_base    = &request;
	iov.iov_len     = request.nh.nlmsg_len;
	msg.msg_name    = &dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov     = &iov;
	msg.msg_iovlen  = 1;

	// Send the request message
	if (sendmsg(nl_sock, (struct msghdr *) &msg, 0) < 0) {
		log_error("Error sending interface request to netlink: %s", strerror(errno));
		close(nl_sock);
		return 1;
	}

	// Prepare iovec for the response
	memset(&iov, 0, sizeof(iov));
	iov.iov_base  = buffer;
	iov.iov_len   = sizeof(buffer);

	int done    = 0;
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
				if (p_new->up && (is_tun_or_tap(p_new->ifname))) {
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

	if (call_script(got_vpn) != 0) {
		log_error("Failed to execute command");
	}
	close(nl_sock);
	return 0;
}

ifdata_t *get_ifdata(struct nlmsghdr *hdr) {
	struct ifinfomsg *ifi = NULL;
	struct rtattr *attr   = NULL;
	ssize_t attr_len      = 0;
	unsigned char *ptr    = NULL;

	// prepare new ifdata struct
	ifdata_t *p_new = (ifdata_t*)malloc(sizeof(ifdata_t));
	memset(p_new->ifname, '\0', sizeof(p_new->ifname));
	memset(p_new->mac, '\0', sizeof(p_new->mac));
	p_new->up = -1;
	p_new->next = NULL;
	p_new->prev = NULL;

	char ifname[IFNAMSIZ] = {'\0'};

	ifi = (struct ifinfomsg *) NLMSG_DATA(hdr);
	log_debug("~~~ interface %s ~~~ \n", if_indextoname(ifi->ifi_index, ifname));
	log_debug("flags:");
	log_debug("IFF_UP: %s", (ifi->ifi_flags & IFF_UP) ? "yes" : "no");
	log_debug("IFF_LOWER_UP: %s", (ifi->ifi_flags & IFF_LOWER_UP) ? "yes" : "no");
	log_debug("!IFF_DORMANT: %s", (ifi->ifi_flags ^ IFF_DORMANT) ? "yes" : "no");
	log_debug("IFF_RUNNING: %s", (ifi->ifi_flags & IFF_RUNNING) ? "yes" : "no");

	p_new->up = (ifi->ifi_flags & IFF_RUNNING) ? 1 : 0;
	log_debug("Device %d (%s) is  %s!", ifi->ifi_index, ifname, p_new->up ? "up" : "down");

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
			case IFLA_LINK:
				log_debug("got link status (maybe): %d", (char*)RTA_DATA(attr));
				break;
			case IFLA_TXQLEN:
				log_debug("got TX queue length");
				break;
			case IFLA_OPERSTATE: {
				int oper_state = *(int*)RTA_DATA(attr);
				log_debug("got operation state %d", oper_state);
				if (oper_state == 6) {
					log_debug("interface %s is operational", ifname);
				} else if (oper_state == 5) {
					log_debug("interface %s might be dormant", ifname);
				} else {
					log_debug("interface %s may not be operational", ifname);
				}
				break;
			}
			case IFLA_PROMISCUITY:
				log_debug("got promiscuity: %d", *(int*)RTA_DATA(attr));
				break;
			case IFLA_CARRIER:
				log_debug("got carrier: %s", (char*)RTA_DATA(attr));
				break;
			case IFLA_CARRIER_CHANGES:
				log_debug("got carrier changes(?): %s", (char*)RTA_DATA(attr));
				break;
			case IFLA_LINKMODE:
				log_debug("got link: %d", *(int*)RTA_DATA(attr));
				break;
			case IFLA_PROTO_DOWN:
				log_debug("got proto down: %d", *(int*)RTA_DATA(attr));
				break;
			case IFLA_MTU:
				log_debug("got MTU");
				break;
			case IFLA_GROUP:
				log_debug("got group: %s", (char *)RTA_DATA(attr));
				break;
			case IFLA_STATS:
				log_debug("Got stats!");
				struct rtnl_link_stats *stats = (struct rtnl_link_stats*)RTA_DATA(attr);
				log_debug("\nrx packets: %ul\ntx packets: %ul\nrx bytes: %ul\n" \
					"tx bytes: %ul\nrx errors: %ul\ntx errors: %ul\nrx dropped: %ul\n" \
					"tx dropped: %ul", stats->rx_packets, stats->tx_packets,
					stats->rx_bytes, stats->tx_bytes, stats->rx_errors, stats->tx_errors,
					stats->rx_dropped, stats->tx_dropped);
				break;
			default:
				// log_debug("got something else: rta_type: %d : %d", attr->rta_type, (char*)RTA_DATA(attr));
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

void print_ifinfo(ifdata_t *head) {
	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
		log_notice("%c | %-12s: %s", (p_tmp->up ? 'o' : 'x'), p_tmp->ifname, p_tmp->mac);
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

ifdata_t *find_ifdata(ifdata_t *ifquery, ifdata_t *head) {
	// ifquery contains ifname or mac
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

int is_tun_or_tap(const char *ifname) {
	return ((strncmp(ifname, "tun", 3) == 0) ||
		(strncmp(ifname, "tap", 3) == 0));
}

int call_script(int op) {
	int ret = -1;

	if (op == VPN_UP) {
		ret = system(g_upcmd);
	} else if (op == VPN_DOWN) {
		ret = system(g_downcmd);
	} else {
		log_warning("invalid action requested in call_script");
		ret = 1;
	}

	if (ret < 0) {
		log_warning("Something went wrong when calling script");
		return 1;
	}
	return ret;
}
