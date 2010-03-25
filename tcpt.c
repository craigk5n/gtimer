/*
 * GTimer
 *
 * Copyright:
 *	(C) 1999 Craig Knudsen, cknudsen@cknudsen.com
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
 *	Craig Knudsen, cknudsen@cknudsen.com, http://www.cknudsen.com
 *
 * Home Page:
 *	http://www.cknudsen.com/gtimer/
 *
 * History:
 *	19-May-1999	Created
 *
 * Limitations:
 *
 *	1. Winsock defines the type "SOCKET" to use with sockets
 *	   while we use "int" on UNIX.  We defined sockfd which can
 *	   be used for either.
 *	2. Winsock must be initialized and cleaned up before exit.
 *	3. Winsock does not user errno for system errors like
 *	   UNIX does.
 *	4. Winsock must use send() and recv() to read and write
 *	   sockets.  (UNIX can also use read() and write().)
 *      5. In summary, winsock sucks, and we most code accordingly.
 *
 *****************************************************************************/


/* system include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef SYSV
#include <strings.h>
#endif
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <winsock.h>    /* winsock API */
#else
#include <unistd.h>
#include <netdb.h>      /* for gethostbyaddr() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#ifdef AIX
#include <sys/select.h>
#endif
#include <pwd.h>
#include <errno.h>

extern int errno;

/* local include files */
#include "tcpt.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif


/* local variables */
typedef struct {
  unsigned char version;
  unsigned char command;
  unsigned short dst_port;
  unsigned long dst_addr;
} socks_struct;

static char socks_server[256];
static int socks_port = 1080;
static int socks_enabled = 0;

/*
** tcptErrorString - Translate a tcptError into a text string.
*/
char *tcptErrorString ( error_num )
tcptError error_num;
{
  switch ( error_num ) {
    case TCPT_NO_ERROR:
      return ( "No error" );
    case TCPT_SOCKET_ERROR:
      return ( "Socket connection error" );
    case TCPT_INIT_ERROR:
      return ( "Error initializing TCP/IP" );
    case TCPT_CONNECT_ERROR:
      return ( "Error connecting socket" );
    case TCPT_SOCKS_CONNECT_ERROR:
      return ( "Error connecting to SOCKS server" );
    case TCPT_SOCKS_CONNECT_REFUSED:
      return ( "SOCKS access refused" );
    case TCPT_INVALID_SOCKS_HOST:
      return ( "Invalid SOCKS host" );
    case TCPT_SOCKS_CONNECT_TIMEOUT:
      return ( "SOCKS connection timed out" );
    case TCPT_SOCKS_CONNECTION_REFUSED:
      return ( "SOCKS connection refused" );
    case TCPT_CONNECTION_REFUSED:
      return ( "Server connection refused" );
    default:
      return ( "Unknown error" );
  }
}

/*
** tcptInit - Initialize the TCP/IP stack.  Not needed in UNIX.
*/
tcptError tcptInit ()
{
#ifdef WIN32
  WSADATA WSAData;

  if ( WSAStartup ( MAKEWORD ( 1, 1 ), &WSAData ) != 0 ) {
    WSACleanup ();
    return ( TCPT_INIT_ERROR );
  }
#endif

  return ( TCPT_NO_ERROR );
}


/*
** tcptEnableSocks - Enable SOCKS usage.
*/
tcptError tcptEnableSocks ( server, port )
char *server;
int port;
{
  strcpy ( socks_server, server );
  socks_port = port;
  socks_enabled = 1;

  return ( TCPT_NO_ERROR );
}


/*
** tcptDisableSocks - Disable SOCKS usage
*/
tcptError tcptDisableSocks ()
{
  socks_enabled = 0;

  return ( TCPT_NO_ERROR );
}

/*
** tcptConnect - Connect a TCP/IP socket.  Uses SOCKS if enabled.
*/
tcptError tcptConnect ( sock, server, size )
sockfd sock;
struct sockaddr_in *server;
int size;
{
  int count, rval;
  struct timeval tv;
  struct hostent *hp;
  unsigned long addr;
  socks_struct data;
  char temp[100];
  fd_set fds;
  uid_t uid;
  struct passwd *pw;
  int datasize;

  if ( socks_enabled ) {
    hp = gethostbyname ( socks_server );
    if ( ! hp ) {
      /* might be an IP address */
      addr = inet_addr ( socks_server );
      hp = gethostbyaddr ( (char *)&addr, sizeof ( struct in_addr ), AF_INET );
      if ( ! hp )
        return ( TCPT_INVALID_SOCKS_HOST );
    }
    /* save destination port */
    data.version = 4;
    data.command = 1;
    data.dst_port = server->sin_port;
    memcpy ( (char *)&data.dst_addr, (char *)&server->sin_addr, hp->h_length );
    /* setup struct for socks server */
    server->sin_family = AF_INET;
    memcpy ( (char *)&server->sin_addr, (char*)hp->h_addr, hp->h_length );
    server->sin_port = htons ( socks_port );
  }

  tv.tv_sec = 0;
  tv.tv_usec = 10000;   /* in microseconds = 1/100 second */
  count = errno = 0;
  while ( errno != EISCONN && count < 5 ) {
    if ( ( rval = connect ( sock, (struct sockaddr *)server, size ) ) ) {
      if ( errno == ECONNREFUSED ) {
        return ( socks_enabled ? TCPT_SOCKS_CONNECTION_REFUSED :
          TCPT_CONNECTION_REFUSED );
      }
      /* use select since all systems (AIX) don't have usleep() */
      select ( 32, NULL, NULL, NULL, &tv );
      rval = errno;
    }
    count++;
  }
  if ( count >= 5 ) {
    closesocket ( sock );
    tcptCleanup ();
    return ( socks_enabled ? TCPT_SOCKS_CONNECT_ERROR :
      TCPT_CONNECT_ERROR );
  }

  /*
  ** Now connected to end host or socks host
  */
  if ( socks_enabled ) {
    /* 5 second timeout waiting for socks host */
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    FD_ZERO ( &fds );
    FD_SET ( sock, &fds );
    if ( select( 32, NULL, &fds, NULL, &tv ) > 0 ) {
      datasize = sizeof ( data );
      memcpy ( temp, (char *)&data, sizeof ( data ) );
      if ( ( pw = getpwuid ( uid = getuid () ) ) == NULL )
        sprintf ( temp + datasize, "Unknown user-id %d", (int)uid );
      else
        strcpy ( temp + datasize, pw->pw_name );
      datasize += strlen ( temp + datasize ) + 1;
      rval = send ( sock, temp, datasize, 0 );
      if ( rval < 0 )
        return ( TCPT_SOCKET_ERROR );
    }
    else
      return ( TCPT_SOCKS_CONNECT_TIMEOUT );
    /* get response from socks server */
    FD_ZERO ( &fds );
    FD_SET ( sock, &fds );
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    memset ( &data, '\0', sizeof ( data ) );
    /* wait to data is ready to read */
    if ( ( rval = select ( 32, &fds, NULL, NULL, &tv ) ) > 0 ) {
      if ( FD_ISSET ( sock, &fds ) ) {
        rval = recv ( sock, (char *)&data, sizeof ( data ), 0 );
        if ( rval < 0 )
          return ( TCPT_SOCKET_ERROR );
        else if ( ! rval )
          return ( TCPT_SOCKS_CONNECT_TIMEOUT );
        else if ( data.command != 90 )
          return ( TCPT_SOCKS_CONNECT_REFUSED );
      }
      else {
        return ( TCPT_SOCKS_CONNECT_TIMEOUT );
      }
    }
    else {
      return ( TCPT_SOCKS_CONNECT_TIMEOUT );
    }
    /* successful connection via socks */
  }

  return ( TCPT_NO_ERROR );
}

/*
** tcptCleanup - Shutdown the TCP/IP stack.  Does nothing in UNIX.
*/
tcptError tcptCleanup ()
{
#ifdef WIN32
  WSACleanup ();
#endif
  return ( TCPT_NO_ERROR );
}

