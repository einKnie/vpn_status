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

#include <linux/rtnetlink.h>
#include <string.h>
#include <poll.h>
#include "vpn_status.h"
#include "log.h"

int main(void) {

	ifdata_t *p_head = NULL;
	if (init(&p_head) != 0) {
		printf("error: failed to initialize\n");
		exit(1);
	}

	log_notice("Got current interface data:");
	print_ifinfo(p_head);

	// open and bind netlink socket
	int nl_sock = -1;
	struct sockaddr_nl sa = {AF_NETLINK, 0, 0, RTNLGRP_LINK};

	if ((nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
		log_error("Failed to create socket");
		exit(1);
	}

	if (bind(nl_sock, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		log_error("Failed to bind socket: %s", strerror(errno));
		close(nl_sock);
		exit(1);
	}

	// prepare polling
	int buflen = -1;
	char nl_buf[4096];
	struct pollfd fds[1];
	int nfds = 1;
	fds[0].fd = nl_sock;
	fds[0].events  = POLLIN;
	fds[0].revents = 0;

	while (1) {
		log_debug("polling...");
		if (poll(fds, nfds, -1) <= 0) {
			if (errno != EINTR) {
				log_error("poll() : %s", strerror(errno));
			}
		} else {
			if ((buflen = recv(nl_sock, nl_buf, sizeof(nl_buf)-1 , 0)) <= 0) {
				log_error("error receiving uevent message: %s", strerror(errno));
			} else {
				nl_buf[buflen-1] = '\0';
				parse_nlmsg(nl_buf, buflen, p_head);
			}
		}
	}

	// cleanup
	for (ifdata_t *p_tmp = p_head; p_tmp != NULL; ) {
		ifdata_t *p_del = p_tmp;
		p_tmp = p_tmp->next;
		free(p_del);
	}

	close(nl_sock);
	return 0;
}
