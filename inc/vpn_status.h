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

#include <net/if.h>

// #define _DEBUG

#define PROCNAME "vpn_status"
#define VERSION 0.1

#define VPN_UP 1
#define VPN_DOWN 0
#define VPN_SYMBOL ""

/// interfaces linked list
typedef struct ifdata {
	char ifname[IFNAMSIZ];
	char mac[64];
	int  up;
	struct ifdata *next;
	struct ifdata *prev;
} ifdata_t;

/// Initialize vpn_status
int init(ifdata_t **head);
/// Parse a netlink message
int parse_nlmsg(char *nlmsg_buf, ssize_t buflen, ifdata_t *p_head);
/// request info on all interfaces && store data
int fetch_ifinfo(ifdata_t **head);
/// fetch single interface data from a netlink message
ifdata_t *get_ifdata(struct nlmsghdr *hdr);

/// print info on interfaces
void print_ifinfo(ifdata_t *head);
/// add an entry to the interfaces linked list
void add_ifdata(ifdata_t *p_new, ifdata_t **head);
/// remove an entry from the interfaces linked list
void del_ifdata(ifdata_t *p_del, ifdata_t **head);
/// find full data of query in interfaces linked list
ifdata_t *find_ifdata(ifdata_t *ifquery, ifdata_t *head);

/// write the symbol to the file
/// or remove it
int file_write(int op);

#endif // !_VPN_STATUS_H_
