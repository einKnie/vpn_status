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
#include <poll.h>
#include <signal.h>
#include "vpn_status.h"
#include "log.h"
#include "io.h"

// these need to be global for cleanup
ifdata_t *g_head = NULL;
int g_sock = -1;

void cleanup(void);
void sigHdl(const int signum);
void do_resize(int sig);

void print_help() {
	log_notice("%s v.%d.%d", PROCNAME, VERSION, MINVERSION);
	log_notice("keep track of your network interfaces");
	log_notice("usage:");
	log_notice("\t%s [options]", PROCNAME);
	log_notice("-f <path>  ... log to file at path");
	log_notice("-v <level> ... set loglevel (0...4) [default 3]");
	log_notice("-q         ... quiet mode; all logging goes to /tmp/%s.log", PROCNAME);
	log_notice("-u <cmd>   ... set script to call when vpn is detected up");
	log_notice("-d <cmd>   ... set script to call when vpn is detected down");
	log_notice("-m         ... monitor mode; logging to /tmp/%s.log, if info to stdout w/ ncurses", PROCNAME);
	log_notice("-h         ... print this help message and exit");
	log_notice("");
}

int main(int argc, char *argv[]) {

	// set exit handlers
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sigHdl;
	sigaction(SIGINT, &act, 0);
	sigaction(SIGTERM, &act, 0);
	atexit(cleanup);

	char upscript[PATH_MAX] = {'\0'};
	char downscript[PATH_MAX] = {'\0'};

	// init preliminary logger
	char logfile[PATH_MAX] = {'\0'};
	logLevel_e loglevel = ELogVerbose;
	log_init(loglevel, ELogStyleNone, NULL);


	int f_daemon  = 0;
	int f_monitor = 0;

	int opt;
	while ((opt = getopt(argc, argv, "f:v:u:d:qmh")) != -1) {
		switch(opt) {
			case 'f':
				strncpy(logfile, optarg, sizeof(logfile));
				log_notice("disabling output to stdout.");
				log_notice("logfile may be read at %s", logfile);
				break;
			case 'v':
				loglevel = (logLevel_e)atoi(optarg);
				break;
			case 'q':
				f_daemon = 1;
				snprintf(logfile,  sizeof(logfile),  "/tmp/%s.log", PROCNAME);
				log_notice("disabling output to stdout.");
				log_notice("logfile may be read at %s", logfile);
				break;
			case 'u':
				strncpy(upscript, optarg, sizeof(upscript));
				log_notice("will call %s when vpn is up", upscript);
				break;
			case 'd':
				strncpy(downscript, optarg, sizeof(downscript));
				log_notice("will call %s when vpn is down", downscript);
			case 'm':
				f_monitor = 1;
				snprintf(logfile,  sizeof(logfile),  "/tmp/%s.log", PROCNAME);
				log_notice("enabling monitor mode");
				log_notice("logfile may be read at %s", logfile);
				break;
			case 'h':
				print_help();
				exit(0);
				break;
			default:
				log_error("invalid parameter: %c", opt); exit(1);
		}
	}

	// check params
	if (f_daemon && f_monitor) {
		log_error("cannot be daemon and monitor at the same time");
		return 1;
	}

	// init logging
	if (!log_init(loglevel, ELogStyleVerbose, logfile)) {
		printf("error: failed to initialize logging\n");
		return 1;
	}

	if (f_daemon) {
		// todo: daemonize
	} else if (f_monitor) {
		// init curses lib
		if (initscreen() != 0) {
			log_error("failed to initialize monitoring screen");
			return 1;
		}
		// set signal handler for terminal resize
		signal(SIGWINCH, do_resize);
	}

	g_head = NULL;
	if (init(&g_head, upscript, downscript) != 0) {
		printf("error: failed to initialize\n");
		exit(1);
	}

	log_notice("Got current interface data:");
	print_ifinfo(g_head);
	updatewindow(g_head);

	// open and bind netlink socket
	g_sock = -1;
	struct sockaddr_nl sa = {AF_NETLINK, 0, 0, RTNLGRP_LINK};

	if ((g_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
		log_error("Failed to create socket");
		exit(1);
	}

	if (bind(g_sock, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		log_error("Failed to bind socket: %s", strerror(errno));
		close(g_sock);
		exit(1);
	}

	// prepare polling
	int buflen = -1;
	char nl_buf[4096];
	struct pollfd fds[1];
	int nfds = 1;
	fds[0].fd = g_sock;
	fds[0].events  = POLLIN;
	fds[0].revents = 0;

	while (1) {
		log_debug("polling...");
		if (poll(fds, nfds, -1) <= 0) {
			if (errno != EINTR) {
				log_error("poll() : %s", strerror(errno));
			}
		} else {
			if ((buflen = recv(g_sock, nl_buf, sizeof(nl_buf)-1 , 0)) <= 0) {
				log_error("error receiving uevent message: %s", strerror(errno));
			} else {
				nl_buf[buflen-1] = '\0';
				parse_nlmsg(nl_buf, buflen, g_head);
			}
		}
		updatewindow(g_head);
	}

	return 0;
}

void do_resize(int sig) {
	(void)sig;
	log_debug("window size changed");
	// exitscreen();
	// initscreen();
	resizewindows();
	updatewindow(g_head);
}

// ----- cleanup ---

void sigHdl(const int signum) {
	exit(signum);
}

void cleanup(void) {
	// exit ncursrs if necessary
	exitscreen();

	for (ifdata_t *p_tmp = g_head; p_tmp != NULL; ) {
		ifdata_t *p_del = p_tmp;
		p_tmp = p_tmp->next;
		free(p_del);
	}

	close(g_sock);
	log_notice("safe exit");
	log_exit();
}
