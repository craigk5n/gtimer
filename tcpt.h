/*
 * GTimer
 *
 * Copyright:
 *	(C) 1999-2023 Craig Knudsen, craig@k5n.us
 *	See accompanying file "COPYING".
 * 
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version 2
 *	of the License, or (at your option) any later version.
 * 
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the
 *	Free Software Foundation, Inc., 59 Temple Place,
 *	Suite 330, Boston, MA  02111-1307, USA
 *
 * Description:
 *	Helps you keep track of time spent on different tasks.
 *
 * Author:
 *	Craig Knudsen, craig@k5n.us https://www.k5n.us/gtimer/
 *
 * Home Page:
 *	https://www.k5n.us/gtimer
 *
 * History:
 *	25-Feb-1998	Created
 *
 ****************************************************************************/

#ifndef _TCPT_H
#define _TCPT_H

/*
** Define some handy definitions.
*/
#ifdef WIN32
#define sockfd          SOCKET
#else
#define closesocket	close
#define sockfd          int
#endif

/*
** Error values
*/
typedef enum {
  TCPT_NO_ERROR = 0,
  TCPT_INIT_ERROR = 1,
  TCPT_SOCKET_ERROR = 2,
  TCPT_CONNECT_ERROR = 3,
  TCPT_SOCKS_CONNECT_ERROR = 4,
  TCPT_SOCKS_CONNECT_REFUSED = 5,
  TCPT_INVALID_SOCKS_HOST = 6,
  TCPT_SOCKS_CONNECT_TIMEOUT = 7,
  TCPT_SOCKS_CONNECTION_REFUSED = 8,
  TCPT_CONNECTION_REFUSED = 9
} tcptError;


/*
** tcptErrorString - Translate a tcptError into a text string.
*/
char *tcptErrorString (
#ifndef _NO_PROTO
  tcptError error_num
#endif
);

/*
** tcptInit - Initialize the TCP/IP stack.  Not needed in UNIX.
*/
tcptError tcptInit ();

/*
** tcptConnect - Connect a TCP/IP socket.  Uses SOCKS if enabled.
*/
/*tcptError tcptConnect ( int sock, struct sockaddr *server, int size );*/

/*
** tcptCleanup - Shutdown the TCP/IP stack.  Does nothing in UNIX.
*/
tcptError tcptCleanup ();

/*
** tcptEnableSocks - Enable the SOCKS server
*/
tcptError tcptEnableSocks (
#ifndef _NO_PROTO
  char *server, int port
#endif
);

/*
** tcptDisableSocks - Disable the SOCKS server
*/
tcptError tcptDisableSocks ();

tcptError tcptConnect ( sockfd sock, struct sockaddr_in *server, int size );

#endif /* _TCPT_H */
