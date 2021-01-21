#ifndef _VPN_STATUS_H_
#define _VPN_STATUS_H_

#define PROCNAME "vpn_status"
#define VERSION 0.1

#define VPN_UP 1
#define VPN_DOWN 0
#define VPN_SYMBOL ""

typedef struct ifdata {
	char ifname[IFNAMSIZ];
	char mac[64];
	struct ifdata *next;
	struct ifdata *prev;
} ifdata_t;

int init(void);
int parse_nlmsg(char *nlmsg_buf, ssize_t buflen, ifdata_t *p_head);
int fetch_ifinfo(ifdata_t **head);
void print_ifinfo(ifdata_t *head);
void add_ifdata(ifdata_t *p_new, ifdata_t **head);
void del_ifdata(ifdata_t *p_del, ifdata_t **head);
ifdata_t *get_ifdata(struct nlmsghdr *hdr);
ifdata_t *find_ifdata(ifdata_t *ifquery, ifdata_t *head);
int file_write(int op);

#endif // !_VPN_STATUS_H_
