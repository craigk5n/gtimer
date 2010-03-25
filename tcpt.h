/*
**
**  Classification:     UNCLASSIFIED
**
**  Copyright:          (c) Copyright 1995 IBM Corporation
**                      This program is protected by the copyright law as an
**                      unpublished work.
**
**  RCS Info:           $Date: 1995/11/27 15:27:46 $ $Revision: 1.1 $
**
**                      $Id: Header.h,v 1.1 1995/11/27 15:27:46 baseline Exp $
**
**  Filename:           $Source: /Minerva/testing/minerva/src/templates/RCS/Header.h,v $
**
**  Originator:         Craig Knudsen
**                      t/l 335-6068
**                      knudsen@dev.infomkt.ibm.com
**
**  Organization:       IBM Corporation / infoMarket
**                      3190 Fairview Park Drive
**                      Falls Church, VA  22042
**                      Ph: (703)205-5600       FAX: (703)205-5691
**
**  Description:
**
**      NewsTicker include file.
**	Include file for using tcpt routines.
**	Attempt to resolve some of the issues with dealing with
**	winsock.  See tcpt.c for more info on usage.
**	Define sockfd to be an int on UNIX and SOCKET on Win32.
**
**  Limitations:
**
**      1.  None.
**
**  Modification History:
**
**      12-Mar-96  cek  Created
**
******************************************************************************/

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

#endif /* _TCPT_H */
