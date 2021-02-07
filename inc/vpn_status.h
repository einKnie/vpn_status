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

#ifndef _VPN_STATUS_H_
#define _VPN_STATUS_H_

#include <linux/rtnetlink.h>
#include <net/if.h>

#define PROCNAME "vpn_status"
#define VERSION 0
#define MINVERSION 2

#define VPN_UP 1
#define VPN_DOWN 0

#define MACSIZE 18
#define IPSIZE 100

#define IFDATA_GET 0
#define IFDATA_SET 1

/// interfaces linked list
typedef struct ifdata {
	int  ifidx;
	char ifname[IFNAMSIZ];
	char mac[MACSIZE];
	char ip[IPSIZE];
	int  up;
	struct ifdata *next;
	struct ifdata *prev;
} ifdata_t;

/// Initialize vpn_status
int init(ifdata_t **head, const char *upscript, const char *downscript);

/// Parse a netlink message
/// @todo: see how i can get rid of this
int parse_nlmsg(char *nlmsg_buf, ssize_t buflen, ifdata_t *p_head);

/// request info on all interfaces && store data in linked list
/// if head == NULL, the list is created, otherwise appended
int fetch_ifinfo(ifdata_t **head);

/// send a request for data to netlink socket
/// type may be RTM_GETLINK or RTM_GETADDR
int request_ifdata(int sock, int type);

/// parse a response to a getlink or getaddr request
int receive_ifdata(int sock, ifdata_t **head);

/// parse a getlink response specifically
ifdata_t *get_ifdata(struct nlmsghdr *hdr, ifdata_t **head, int op);

/// parse a getaddr response specifically
ifdata_t *get_ifaddr(struct nlmsghdr *hdr, ifdata_t **head, int op);

/// print all info on interfaces
void print_ifinfo(ifdata_t *head);

/// add an entry to the interfaces linked list
void add_ifdata(ifdata_t *p_new, ifdata_t **head);

/// remove an entry from the interfaces linked list
void del_ifdata(ifdata_t *p_del, ifdata_t **head);

/// find interface by index
ifdata_t *find_ifdata(ifdata_t *head, int idx);

// check if device is tun or tap
// return 1 if yes, 0 if no
int is_tun_or_tap(const char *ifname);

/// call a user defined command
int call_script(int op);

#endif // !_VPN_STATUS_H_
