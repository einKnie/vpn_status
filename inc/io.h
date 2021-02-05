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

#ifndef _VPN_IO_H_
#define _VPN_IO_H_

#include <curses.h>
#include "vpn_status.h"
#include "log.h"

typedef struct {
	int startx;
	int starty;
	int height;
	int width;
	WINDOW *win;
} windata_t;

// initialize screen with curses lib
int initscreen(void);
int initwindows(int x, int y);
int exitscreen(void);
int updatewindow(ifdata_t *head);

int resizewindows();

#endif // _VPN_IO_H_
