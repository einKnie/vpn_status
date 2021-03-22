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

#include <arpa/inet.h>
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

		ifdata_t *p_dat = NULL;

		switch(nh->nlmsg_type) {
			case RTM_NEWLINK:
			{
				log_debug("RTM_NEWLINK");
				// check if interface already exists
				p_dat = get_ifdata(nh, NULL, IFDATA_GET);
				ifdata_t *p_res = find_ifdata(p_head, p_dat->ifidx);
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
			} break;

			case RTM_DELLINK:
			{
				log_debug("RTM_DELLINK");
				p_dat = get_ifdata(nh, NULL, IFDATA_GET);
				ifdata_t *p_res = find_ifdata(p_head, p_dat->ifidx);
				if (p_res == NULL) {
					log_debug("no data found on removed interface!");
				} else {
					if (is_tun_or_tap(p_res->ifname)) {
						log_notice("\n--\nA VPN interface was removed\n--");
						if (call_script(p_dat->up) != 0) {
							log_error("Failed to execute cmd");
						}
					}
					del_ifdata(p_res, &p_head);
				}

				log_notice("Interface removed:");
				log_notice("%-12s: %s", p_dat->ifname, p_dat->mac);
				free(p_dat);
			} break;

			case RTM_NEWADDR:
			{
				log_debug("RTM_NEWADDR");
				p_dat = get_ifaddr(nh, NULL, IFDATA_GET);
				ifdata_t *p_res = find_ifdata(p_head, p_dat->ifidx);
				if (p_res == NULL) {
					log_debug("no data found on changed interface!");
				} else {
					if (strncmp(p_dat->ip, p_res->ip, strlen(p_res->ip)) != 0) {
						log_debug("ip address has changed! %s vs. %s", p_dat->ip, p_res->ip);
						strncpy(p_res->ip, p_dat->ip, sizeof(p_res->ip));
					} else {
						log_debug("no changes detected");
					}
				}
			} break;

			case RTM_DELADDR:
			{
				log_debug("RTM_DELADDR");
				p_dat = get_ifaddr(nh, NULL, IFDATA_GET);
				ifdata_t *p_res = find_ifdata(p_head, p_dat->ifidx);
				if (p_res == NULL) {
					log_debug("no data found on changed interface!");
				} else {
					if (strncmp(p_dat->ip, p_res->ip, strlen(p_res->ip)) != 0) {
						log_debug("ip address has changed! %s vs. %s", p_dat->ip, p_res->ip);
						snprintf(p_res->ip, sizeof(p_res->ip), "%s", p_dat->ip);
					} else {
						log_debug("no changes detected");
					}
				}
			} break;

			case RTM_NEWROUTE: log_debug("RTM_NEWROUTE"); continue;
			case RTM_DELROUTE: log_debug("RTM_DELROUTE"); continue;
			case RTM_NEWNEIGH: log_debug("RTM_NEWNEIGH"); continue; // why am i getting these messages?
			case RTM_DELNEIGH: log_debug("RTM_DELNEIGH"); continue;
			default: log_debug("other type received: %d", nh->nlmsg_type); continue;
		}

		log_notice("New state:");
		print_ifinfo(p_head);
	}
	return 0;
}

/// Add info (ifname, mac, ip) for all current interfaces to ifdata linked list
int fetch_ifinfo(ifdata_t **head) {
	int nl_sock = -1;
	int got_vpn = 0;

	// create socket
	if ((nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
		log_error("Failed to create socket");
		return 1;
	}

	// get linklevel info (state, ifname, mac)
	if (request_ifdata(nl_sock, RTM_GETLINK) < 0) {
		log_error("failed to send netlink request");
	} else if (receive_ifdata(nl_sock, head) != 0) {
		log_error("failed to parse netlink response");
	}

	// get ip address
	if (request_ifdata(nl_sock, RTM_GETADDR) < 0) {
		log_error("failed to send netlink request");
	} else if (receive_ifdata(nl_sock, head) != 0) {
		log_error("failed to parse netlink response");
	}

	// check if we have a vpn link already
	for (ifdata_t *tmp = *head; tmp != NULL; tmp = tmp->next) {
		if (tmp->up && (is_tun_or_tap(tmp->ifname))) {
			got_vpn = 1;
			break;
		}
	}

	if (call_script(got_vpn) != 0) {
		log_error("Failed to execute command");
	}

	close(nl_sock);
	return 0;
}

int request_ifdata(int sock, int type) {
	struct {
		struct nlmsghdr nh;
		struct rtgenmsg rtg;
	} request = {0};

	// Prepare address request for the kernel
	request.nh.nlmsg_len      = NLMSG_LENGTH(sizeof(request.rtg));
	request.nh.nlmsg_type     = type;
	request.nh.nlmsg_flags    = NLM_F_REQUEST | NLM_F_DUMP;
	request.rtg.rtgen_family  = AF_PACKET;

	return send(sock, &request, request.nh.nlmsg_len, 0);
}

int receive_ifdata(int sock, ifdata_t **head) {
	char buffer[4096];
	struct nlmsghdr *nh;
	ssize_t n;
	int done    = 0;

	while (!done) {

		if ((n = recv(sock, buffer, sizeof(buffer), 0)) < 0) {
			log_error("Error receiving message from netlink: %s", strerror(errno));
			close(sock);
			return 1;
		}

		// go though messages
		for (nh = (struct nlmsghdr *) buffer; NLMSG_OK(nh, n); nh = NLMSG_NEXT(nh, n)) {
			if (nh->nlmsg_type == RTM_NEWLINK) {
				log_debug("NEWLINK received");
				get_ifdata(nh, head, IFDATA_SET);
			} else if (nh->nlmsg_type == RTM_NEWADDR) {
				log_debug("NEWADDR received");
				get_ifaddr(nh, head, IFDATA_SET);
			} else if (nh->nlmsg_type == NLMSG_DONE) {
				log_debug("all interfaces received");
				done = 1;
			} else {
				log_debug("Got another type message: type %d", nh->nlmsg_type);
			}
		}
	}

	return 0;
}

ifdata_t *get_ifdata(struct nlmsghdr *hdr, ifdata_t **head, int op) {
	struct ifinfomsg *ifi = NULL;
	struct rtattr *attr   = NULL;
	ssize_t attr_len      = 0;
	unsigned char *ptr    = NULL;
	char ifname[IFNAMSIZ] = {'\0'};

	ifi = (struct ifinfomsg *) NLMSG_DATA(hdr);

	ifdata_t *p_new = NULL;
	// if op == get:
	// dont even look at head, create new, return new
	// if op == set:
	// check if index in head
	//	if yes: add data to existing, return existing
	//	if no: create new, add to list, add data, return new
	if ( (op == IFDATA_GET) || ((op == IFDATA_SET) && ((p_new = find_ifdata(*head, ifi->ifi_index)) == NULL)) ) {
		// create p_new
		p_new = (ifdata_t*)malloc(sizeof(ifdata_t));
		memset(p_new->ifname, '\0', sizeof(p_new->ifname));
		memset(p_new->mac, '\0', sizeof(p_new->mac));
		memset(p_new->ip, '\0', sizeof(p_new->ip));
		p_new->ifidx = ifi->ifi_index;
		p_new->up = -1;
		p_new->next = NULL;
		p_new->prev = NULL;

		if (op == IFDATA_SET) {
			add_ifdata(p_new, head);
		}
	}

	log_debug("~~~ interface %s ~~~ \n", if_indextoname(ifi->ifi_index, ifname));

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
			default:
				// log_debug("got something else: rta_type: %d : %d", attr->rta_type, (char*)RTA_DATA(attr));
				break;
		}
	}

	if (op == IFDATA_GET) {
		if (strlen(p_new->ifname) == 0) {
			log_debug("no usable interface data gatered, ignoring %7s:%s",
				p_new->ifname, p_new->mac);
			free(p_new);
			p_new = NULL;
		}
	}

	return p_new;
}

ifdata_t *get_ifaddr(struct nlmsghdr *hdr, ifdata_t **head, int op) {
	struct ifaddrmsg *ifa = NULL;
	struct rtattr *attr   = NULL;
	ssize_t attr_len      = 0;
	char tmpip[IPSIZE]    = {'\0'};

	ifa = (struct ifaddrmsg *) NLMSG_DATA(hdr);
	attr = IFA_RTA(ifa);
	attr_len = hdr->nlmsg_len - NLMSG_LENGTH(sizeof(*ifa));

	ifdata_t *p_new = NULL;
	if ( (op == IFDATA_GET) || ((op == IFDATA_SET) && ((p_new = find_ifdata(*head, ifa->ifa_index)) == NULL)) ) {
		// create p_new
		p_new = (ifdata_t*)malloc(sizeof(ifdata_t));
		memset(p_new->ifname, '\0', sizeof(p_new->ifname));
		memset(p_new->mac, '\0', sizeof(p_new->mac));
		memset(p_new->ip, '\0', sizeof(p_new->ip));
		p_new->ifidx = ifa->ifa_index;
		p_new->up = -1;
		p_new->next = NULL;
		p_new->prev = NULL;

		if (op == IFDATA_SET) {
			add_ifdata(p_new, head);
		}
	}

	for (; RTA_OK(attr, attr_len); attr = RTA_NEXT(attr, attr_len)) {
		switch(attr->rta_type) {
			case IFA_ADDRESS:
				inet_ntop(AF_INET, RTA_DATA(attr), tmpip, sizeof(tmpip));
				if (strlen(p_new->ip) == 0) {
					// only overwrite in case we don't already have a local address
					snprintf(p_new->ip, sizeof(p_new->ip), "%s", tmpip);
				}
				log_debug("got an ipaddress for %s: %s", p_new->ifname, tmpip);
				break;
			case IFA_LOCAL:
				// pretty sue this is the one we want primarily
				inet_ntop(AF_INET, RTA_DATA(attr), p_new->ip, sizeof(p_new->ip));
				log_debug("got a local ipaddress for %s: %s", p_new->ifname, p_new->ip);
				break;
			default:
				break;
		}
	}

	if (op == IFDATA_GET) {
		if (strlen(p_new->ip) == 0) {
			log_debug("no usable interface data gatered, ignoring %7s:%s",
				p_new->ifname, p_new->mac);
			free(p_new);
			p_new = NULL;
		}
	}
	return p_new;
}

void print_ifinfo(ifdata_t *head) {
	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
		log_notice("%c | %2d | %-12s: %-17s %s", (p_tmp->up ? 'o' : 'x'), p_tmp->ifidx, p_tmp->ifname, p_tmp->ip, p_tmp->mac);
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

ifdata_t *find_ifdata(ifdata_t *head, int idx) {
	// find interface in list by given index
	// if found, return this interface's full data, else NULL
	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
		if (p_tmp->ifidx == idx) {
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
