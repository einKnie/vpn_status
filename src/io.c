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

#include "io.h"
#include <string.h>
/*
	implement output w/ ncurses
	* logging should still go to file
	* only the current interface data should be shown
		and updated when necessary
*/

int g_initialized = 0;
WINDOW *g_win = NULL;
windata_t g_mainwin;
windata_t g_innerwin;

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

	use_default_colors(); // necessary to keep terminal background
	start_color();
	init_pair(1, COLOR_RED, -1);   // set background to -1 to use terminal's
	init_pair(2, COLOR_GREEN, -1);

	g_mainwin.win = NULL;
	g_innerwin.win = NULL;
	initwindows(COLS, LINES);

	log_debug("init curses: success");
	g_initialized = 1;
	return 0;
}

int initwindows(int x, int y) {

	g_mainwin.startx = 0;
	g_mainwin.starty = 0;
	g_mainwin.height = y;
	g_mainwin.width  = x;

	if (g_mainwin.win == NULL) {
		g_mainwin.win = newwin(g_mainwin.height, g_mainwin.width,
			g_mainwin.starty, g_mainwin.startx);
	} else {
		wresize(g_mainwin.win, g_mainwin.height, g_mainwin.width);
	}

	box(g_mainwin.win, 0, 0);
	wrefresh(g_mainwin.win);

	g_innerwin.startx = g_mainwin.startx + 2;
	g_innerwin.starty = g_mainwin.starty + 1;
	g_innerwin.height = g_mainwin.height - 2;
	g_innerwin.width  = g_mainwin.width  - 4;

	if (g_innerwin.win == NULL) {
		g_innerwin.win = subwin(g_mainwin.win, g_innerwin.height, g_innerwin.width,
			g_innerwin.starty, g_innerwin.startx);
	} else {
		wresize(g_innerwin.win, g_innerwin.height, g_innerwin.width);
	}

	wrefresh(g_innerwin.win);
	return 0;
}

// is all of this really necessary?
// todo: all the error checking
int resizewindows() {
	endwin();
	refresh();
	initwindows(COLS, LINES); // after endwin && refresh, COLS and LINES are updated
	return 0;
}

int exitscreen() {
	if (! g_initialized) {
		log_debug("curses is not initialized");
		return 0;
	}

	if ((g_mainwin.win != NULL) || (g_innerwin.win != NULL)) {
		// delwin(g_innerwin.win); // not sure if I need this, valgrind does not notice a difference
		// delwin(g_mainwin.win);
		// g_mainwin.win = g_innerwin.win = NULL;
	}

	if (endwin() == ERR) {
		log_error("err: endwin()");
		return 1;
	}

	g_initialized = 0;
	log_debug("exit curses: success");
	return 0;
}

int updatewindow(ifdata_t *head) {
	if (!g_initialized) {
		log_debug("curses is not initialized");
		return 0;
	}

	// pretty sure this can be optimized
	// but i need a wy to erase mainwin in case of  screen resize
	werase(g_mainwin.win);
	werase(g_innerwin.win);
	box(g_mainwin.win, 0, 0);

	writewindow(head);

	wnoutrefresh(g_mainwin.win);
	wnoutrefresh(g_innerwin.win);
	doupdate();
	log_debug("screen updated");
	return 0;
}

int writewindow(ifdata_t *head) {

	// check win size.
	int x, y;
	x = getmaxx(g_innerwin.win);
	y = 0;
	int macstart = x - MACSIZE;

	attron(A_BOLD);
	mvwprintw(g_innerwin.win, y, LINESTART, "%s", "UP");
	mvwprintw(g_innerwin.win, y, NAMESTART, "%s", "NAME");
	mvwprintw(g_innerwin.win, y++, macstart, "%s", "MAC");
	attroff(A_BOLD);

	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {

		if (p_tmp->up) {
			mvwaddch(g_innerwin.win, y, LINESTART, 'o' | A_BOLD | COLOR_PAIR(2));
		} else {
			mvwaddch(g_innerwin.win, y, LINESTART, 'x' | A_BOLD | COLOR_PAIR(1));
		}

		mvwprintw(g_innerwin.win, y, NAMESTART, "%s", p_tmp->ifname);
		mvwprintw(g_innerwin.win, y, macstart, "%s", p_tmp->mac);

		y++;
	}
	return 0;
}
