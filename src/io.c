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

	if (g_mainwin.win != NULL) {
		delwin(g_mainwin.win);
		g_mainwin.win = NULL;
	}

	if (g_innerwin.win != NULL) {
		delwin(g_innerwin.win);
		g_innerwin.win = NULL;
	}

	g_mainwin.startx = 0;
	g_mainwin.starty = 0;
	g_mainwin.height = y;
	g_mainwin.width  = x;
	g_mainwin.win = newwin(g_mainwin.height, g_mainwin.width,
		g_mainwin.starty, g_mainwin.startx);
	box(g_mainwin.win, 0, 0);
	wrefresh(g_mainwin.win);

	g_innerwin.startx = g_mainwin.startx + 2;
	g_innerwin.starty = g_mainwin.starty + 1;
	g_innerwin.height = g_mainwin.height - 2;
	g_innerwin.width  = g_mainwin.width  - 4;
	g_innerwin.win = subwin(g_mainwin.win, g_innerwin.height, g_innerwin.width,
		g_innerwin.starty, g_innerwin.startx);
	wrefresh(g_innerwin.win);

	return 0;
}

// is all of this really necessary?
// todo: all the error checking
int resizewindows() {
	int x,y;
	exitscreen();
	initscreen();
	getmaxyx(stdscr, y, x);
	resizeterm(y, x);
	log_debug("prev window size: %d/%d", g_mainwin.width, g_mainwin.height);
	log_debug("new window size: %d/%d", x, y);
	initwindows(x, y);
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

	g_initialized = 0;
	log_debug("exit curses: success");
	return 0;
}

int updatewindow(ifdata_t *head) {
	if (!g_initialized) {
		log_debug("curses is not initialized");
		return 0;
	}

	for (ifdata_t *p_tmp = head; p_tmp != NULL; p_tmp = p_tmp->next) {
		if (p_tmp->up) {
			waddch(g_innerwin.win, 'o' | A_BOLD | COLOR_PAIR(2));
		} else {
			waddch(g_innerwin.win, 'x' | A_BOLD | COLOR_PAIR(1));
		}
		wprintw(g_innerwin.win, " | %-12s: %s\n", p_tmp->ifname, p_tmp->mac);
	}

	wrefresh(g_mainwin.win);
	wrefresh(g_innerwin.win);
	log_debug("screen updated");
	return 0;
}
