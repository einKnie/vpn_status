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

#include "vpn_status.h"
#include "log.h"

// initialize screen with curses lib
int initscreen(void);
int exitscreen(void);
int updatescreen(ifdata_t *head);

#endif // _VPN_IO_H_
