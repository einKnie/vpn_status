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

void print_help() {
	log_notice("%s v.%d.%d", PROCNAME, VERSION, MINVERSION);
	log_notice("keep track of your network interfaces");
	log_notice("usage:");
	log_notice("\tPROCNAME [options]");
	log_notice("-f <path> ... log to file");
	log_notice("-v        ... print debug output");
	log_notice("-q        ... quiet, produces no output");
	log_notice("-d        ... daemon mode; all logging goes to /tmp/%s.log", PROCNAME);
	log_notice("-h        ... print this help message and exit");
	log_notice("");
}

int main(int argc, char *argv[]) {
	char logfile[PATH_MAX] = {'\0'};
	logLevel_e loglevel = ELogVerbose;
	log_init(loglevel, ELogStyleNone, NULL);

	int opt;
	while ((opt = getopt(argc, argv, "f:vdqh")) != -1) {
		switch(opt) {
			case 'f':
				strncpy(logfile, optarg, sizeof(logfile));
				log_notice("disabling output to stdout.\nlogfile may be read at %s\n", logfile);
				break;
			case 'v':
				loglevel = ELogDebug;
				break;
			case 'd':
				snprintf(logfile,  sizeof(logfile),  "/tmp/%s.log", PROCNAME);
				log_notice("disabling output to stdout.\nlogfile may be read at %s\n", logfile);
				break;
			case 'q':
				loglevel = ELogDisable;
				break;
			case 'h':
				print_help();
				exit(0);
				break;
			default:
				log_error("invalid parameter: %c\n", opt); exit(1);
		}
	}

	// init logging

	if (!log_init(loglevel, ELogStyleVerbose, logfile)) {
		printf("error: failed to initialize logging\n");
		return 1;
	}

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
	log_exit();
	return 0;
}
