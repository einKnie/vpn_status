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

#include <curses.h>
#include "io.h"
/*
	implement output w/ ncurses
	* logging should still go to file
	* only the current interface data should be shown
		and updated when necessary
*/

int g_initialized = 0;

int initscreen() {
	log_debug("initializing ncurses library");
	if (g_initialized) {
		log_warning("curses is already initialized");
		return 1;
	}

	if (initscr() == NULL) {
		log_error("err: initscr()");
		return 1;
	}

	cbreak();             // do not capture control sequences line crtl-c
	noecho();             // dont echo user input
	keypad(stdscr, TRUE); // enable f-keys and arrows, etc
	curs_set(0);          // disable the cursor

	log_debug("init curses: success");
	g_initialized = 1;
	return 0;
}

int exitscreen() {
	if (! g_initialized) {
		log_debug("curses is not initialized");
		return 0;
	}

	if (endwin() == ERR) {
		log_error("err: endwin()");
		return 1;
	}

	log_debug("exit curses: success");
	return 0;
}

// this is hacky, use a window insteasd
// but i want to try this first
int updatescreen(ifdata_t *head) {
	if (!g_initialized) {
		log_debug("curses is not initialized");
		return 0;
	}

	clear();
	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
		printw("%c | %-12s: %s\n", (p_tmp->up ? 'o' : 'x'), p_tmp->ifname, p_tmp->mac);
	}
	refresh();
	log_debug("screen updated");
	return 0;
}
