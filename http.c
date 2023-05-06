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
 *	Craig Knudsen, craig@k5n.us, https://www.k5n.us/gtimer
 *
 * Home Page:
 *	https://www.k5n.us/gtimer/
 *
 * History:
 *	19-May-1999	Created (based on an 1995 app I wrote)
 *
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef AIX
#include <sys/select.h>
#endif
#include <sys/ioctl.h>
#ifdef WIN32
#include <winsock.h>	/* winsock API */
#else
#include <unistd.h>
#include <netdb.h>	/* for gethostbyaddr() */
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <fcntl.h>
#include <time.h>

#include "project.h"
#include "task.h"
#include "gtimer.h"

#include "tcpt.h"
#include "http.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif


static char *my_strtok (
#ifndef _NO_PROTO
  char *ptr1, char *tok
#endif
);


/*
** Max number of bytes to read at once.
*/
#define MAX_READ_SIZE		1024

/*
** Structure for states.
*/
typedef struct _httpRequest {
  sockfd connection;			/* connection to server */
  char *request;			/* text message to send */
  httpError (*read_function)();		/* read function */
#ifdef _NO_PROTO
  void (*callback)();			/* callback when results received */
#else
  void (*callback)(char *);		/* callback when results received */
#endif
#ifdef _NO_PROTO
  void (*gen_callback)();		/* callback when results received */
#else
  void (*gen_callback)(char *,int);	/* callback when results received */
#endif
  int sent;				/* has request been sent */
  char *data_read;			/* data that was read */
  int data_len;				/* size of above data */
  int content_length;			/* length according to header */
  int pass;				/* no. reads done */
} httpRequest;

#define MAX_REQUESTS_QUEUED	2048

static char http_other_error[256];
static httpRequest *requests[MAX_REQUESTS_QUEUED];
static int num_requests = 0;
static char *http_proxy = NULL;
static char http_proxy_string[1024];
static int http_proxy_port;


static char *user_agent () {
  static char ret[100];

  sprintf ( ret, "GTimer/%s", GTIMER_VERSION );
  return ( ret );
}

/*
** Local functions.
*/
static char *httpTruncateHeader (
#ifndef _NO_PROTO
  char *text,
  int *len
#endif
);
static httpError httpReadGet (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static httpError httpReadGetArticles (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static httpError httpReadGetAds (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static httpError httpReadList (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static httpError httpReadGetUserId (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static httpError httpReadGetSessionId (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static httpError httpSendRequest (
#ifndef _NO_PROTO
  httpRequest *request
#endif
);
static char *encode_for_use_in_url (
#ifndef _NO_PROTO
  char *str
#endif
);





/*
** URL-encode a string
** See RFC 1738 (http://andrew2.andrew.cmu.edu/rfc/rfc1738.html)
** Caller should free result.
*/
static char *encode_for_use_in_url ( str )
char *str;
{
  char *ret, *ptr1, *ptr2, *ptr3;
  static char unsafe_chars[] = {
    ' ', '{', '}', '|', '/', '\\', '^', '~', '[', ']', '`', '%', '<', '>', '"', '#', ')', '(', '\0',
  };
  int unsafe;

  /* just to be safe... */
  ret = (char *) malloc ( strlen ( str ) * 3 + 1 );

  for ( ptr1 = str, ptr2 = ret; *ptr1 != '\0'; ptr1++ ) {
    unsafe = 0;
    /* is unsafe??? */
    for ( ptr3 = unsafe_chars; *ptr3 != '\0'; ptr3++ ) {
      if ( *ptr3 == *ptr1 ) {
        unsafe = 1;
        break;
      }
    }
/*
    if ( ! unsafe ) {
      if ( ( (unsigned int)*ptr1 < 0x80 || (unsigned int)*ptr1 > 0xFF ) ||
        ( (unsigned int)*ptr1 >= 0x00 && (unsigned int)*ptr1 <= 0x1F ) ||
        ( (unsigned int)*ptr1 == 0x7F ) )
        unsafe = 1;
    }
*/
    if ( unsafe ) {
      sprintf ( ptr2, "%%%02X", (unsigned int)*ptr1 );
      ptr2 += 3;
    }
    else {
      *ptr2 = *ptr1;
      ptr2++;
    }
  }
  *ptr2 = '\0';

  return ( ret );
}



void httpEnableProxy ( server, port )
char *server;
int port;
{
  if ( http_proxy )
    free ( http_proxy );
  http_proxy = (char *) malloc ( strlen ( server ) + 1 );
  strcpy ( http_proxy, server );
  http_proxy_port = port;
}


void httpDisableProxy ()
{
  if ( http_proxy )
    free ( http_proxy );
  http_proxy_string[0] = '\0';
  http_proxy = NULL;
}


/*
** Put a request onto the end of the queue.
** Current request is always reqeusts[0].
*/
httpError httpEnqueueRequest ( connection, msg, read_function, callback,
  gen_callback )
sockfd connection;
char *msg;
#ifdef _NO_PROTO
httpError (*read_function)();
void (*callback)();
void (*gen_callback)();
#else
httpError (*read_function)(sockfd);
void (*callback)(char *);
void (*gen_callback)(char *,int);
#endif
{
  httpRequest *request;

  if ( num_requests >= MAX_REQUESTS_QUEUED - 1 ) {
    fprintf ( stderr, "Too many errors...\n" );
    return ( HTTP_TOO_MANY_REQUESTS );
  }

  /* put request at end of queue. */
  request = (httpRequest *) malloc ( sizeof ( httpRequest ) );
  memset ( request, '\0', sizeof ( httpRequest ) );
  if ( msg ) {
    request->request = (char *) malloc ( strlen ( msg ) + 1 );
    strcpy ( request->request, msg );
    request->connection = connection;
  }
  else {
    /* NULL msg indicates no message to send */
    request->request = NULL;
    request->sent = 1;
  }
  request->connection = connection;
  request->read_function = read_function;
  request->callback = callback;
  request->gen_callback = gen_callback;
  request->data_read = NULL;
  requests[num_requests++] = request;

  /* now send the request if this is the only request */
  if ( num_requests == 1 ) {
    httpSendRequest ( requests[0] );
  }

  return ( HTTP_NO_ERROR );
}



/*
** Send a request to the server.
** This should not be called directly, only from httpDequeueRequest and
** httpEnqueueRequest.
*/
static httpError httpSendRequest ( request )
httpRequest *request;
{
  int rval;

  if ( ! request->sent ) {
    rval = send ( request->connection, request->request,
      strlen ( request->request ), 0 );

    request->sent = 1;

    if ( rval >= 0 )
      return ( HTTP_NO_ERROR );
    else
      return ( HTTP_SOCKET_ERROR );
  }
  else
    return ( HTTP_NO_ERROR );
}



/*
** Remove the most recent request from the queue.
** It is not an error to call this function with nothing in the queue.
*/
httpError httpDequeueRequest ()
{
  int loop;

  if ( num_requests ) {
    if ( requests[0]->data_read )
      free ( requests[0]->data_read );
    if ( requests[0]->request )
      free ( requests[0]->request );
    free ( requests[0] );
    /* move each request up one slot */
    for ( loop = 1; loop < num_requests; loop++ )
      requests[loop-1] = requests[loop];
    num_requests--;
  }

  if ( num_requests )
    httpSendRequest ( requests[0] );

  return ( HTTP_NO_ERROR );
}



/*
** Calling app calls this when select() indicates data is ready to be
** read on socket.  This will figure out where we left off and
** continue from there.
*/
httpError httpProcessRead ( connection )
sockfd connection;
{
  int loop;
  int num = -1;

  for ( loop = 0; loop < num_requests; loop++ ) {
    if ( requests[loop]->connection == connection ) {
      num = loop;
      break;
    }
  }

  if ( num < 0 )
    return ( HTTP_NO_REQUESTS );

  return ( requests[loop]->read_function ( requests[loop] ) );
}



/*
** Connect to an HTTP server.
** Returns socket file descriptor.
** Note that this is the one place where we must wait for a response
** that could cause things to hang a bit.
*/
httpError httpOpenConnection ( http_host, port, connection )
char *http_host;
int port;
sockfd *connection;
{
  sockfd sock;
  static struct sockaddr_in server;
  struct hostent *hp, *gethostbyname ();
  tcptError ret;

  memset ( &server, '\0', sizeof ( struct sockaddr_in ) );

  /* Verify all necessary data is available */
  if ( ! http_host || ! strlen ( http_host ) )
    return ( HTTP_INVALID_HOST );

  if ( http_proxy ) {
    /* save info for next request */
    sprintf ( http_proxy_string, "http://%s:%d", http_host, port );
    /* now change to http proxy */
    http_host = http_proxy;
    port = http_proxy_port;
  }

  hp = gethostbyname ( http_host );

  if ( ! hp ) {
    return ( HTTP_HOST_LOOKUP_FAILED );
  }

  /* Init windows winsock DLL */
  if ( tcptInit () ) {
    return ( HTTP_SOCKET_ERROR );
  }

  sock = socket ( AF_INET, SOCK_STREAM, 0 );
  if ( sock < 0 ) {
    tcptCleanup ();
    return ( HTTP_SOCKET_ERROR );
  }

  server.sin_family = AF_INET;
  memcpy((char *)&server.sin_addr, (void *)hp->h_addr_list[0], hp->h_length);

  /* Get HTTP port (80) */
  if ( port )
    server.sin_port = htons ( port );
  else
    server.sin_port = htons ( HTTP_PORT );

  if ( ( ret = tcptConnect ( sock, (struct sockaddr *)&server,
     sizeof ( server ) ) ) ) {
    strcpy ( http_other_error, tcptErrorString ( ret ) );
    return ( HTTP_OTHER_ERROR );
  }

  /* success.  return socket */
  *connection = sock;

  /* Set to non-blocking */
  /*ioctl ( *connection, FIONBIO, 1 );*/

  return ( HTTP_NO_ERROR );
}





/*
** Close the connection and remove all requests from the queue.
*/
httpError httpKillConnection ( connection )
sockfd connection;
{
  int loop, newnum;

  /*
  ** Remove all requests in the queue that reference this connection.
  */
  for ( loop = 0, newnum = 0; loop < num_requests; loop++ ) {
    if ( requests[loop]->connection == connection ) {
      /* NULL out requests we are killing */
      if ( requests[loop]->data_read )
        free ( requests[loop]->data_read );
      if ( requests[loop]->request )
        free ( requests[loop]->request );
      free ( requests[loop] );
      requests[loop] = NULL;
    }
    else {
      requests[newnum] = requests[loop];
    }
  }

  for ( loop = 0, newnum = 0; loop < num_requests; loop++ ) {
    if ( requests[loop] && loop != newnum ) {
      requests[newnum] = requests[loop];
      requests[loop] = NULL;
      newnum++;
    }
  }

  num_requests = newnum;

  closesocket ( connection );
  return ( HTTP_NO_ERROR );
}



/*
** Do a generic get request.
*/
httpError httpGet ( connection, virthost, path, qs_names, qs_values,
  num, callback )
sockfd connection;
char *virthost;
char *path;
char *qs_names[];
char *qs_values[];
int num;
#ifdef _NO_PROTO
void (*callback)();
#else
void (*callback)(char *,int);
#endif
{
  char *command, *temp, temp2[256], *ret1, *ret2;
  int loop;

  temp = (char *) malloc ( strlen ( path ) + strlen ( http_proxy_string ) +
    5 );
  sprintf ( temp, "GET %s%s", http_proxy_string, path );
  command = (char *) malloc ( strlen ( temp ) + 2 );
  strcpy ( command, temp );
  free ( temp );
  if ( num ) {
    strcat ( command, "?" );
    for ( loop = 0; loop < num; loop++ ) {
      ret1 = encode_for_use_in_url ( qs_names[loop] );
      ret2 = encode_for_use_in_url ( qs_values[loop] );
      command = (char *) realloc ( command, strlen ( command ) +
        strlen ( ret1 ) + strlen ( ret2 ) + 3 );
      if ( loop )
        strcat ( command, "&" );
      strcat ( command, ret1 );
      strcat ( command, "=" );
      strcat ( command, ret2 );
      free ( ret1 );
      free ( ret2 );
    }
  }
  strcpy ( temp2, " HTTP/1.0\r\n" );
  strcat ( temp2, "User-Agent: " );
  strcat ( temp2, user_agent() );
  strcat ( temp2, "\r\n" );
  if ( virthost != NULL ) {
    strcat ( temp2, "Host: " );
    strcat ( temp2, virthost );
    strcat ( temp2, "\r\n" );
  }
  strcat ( temp2, "\r\n" );
  command = (char *) realloc ( command, strlen ( command ) +
    strlen ( temp2 ) + 1 );
  strcat ( command, temp2 );

  httpEnqueueRequest ( connection, command, httpReadGet, NULL, callback );

  return ( HTTP_NO_ERROR );
}

static int my_strncasecmp ( str1, str2, len )
char *str1, *str2;
int len;
{
  char *a1, *a2, *ptr;
  int ret, loop;

  a1 = (char *) malloc ( len + 1 );
  strncpy ( a1, str1, len );
  a1[len] = '\0';
  a2 = (char *) malloc ( len + 1 );
  strncpy ( a2, str2, len );
  a2[len] = '\0';
  for ( ptr = a1, loop = 0; *ptr != '\0' && loop < len; loop++, ptr++ )
    *ptr = toupper ( *ptr );
  for ( ptr = a2, loop = 0; *ptr != '\0' && loop < len; loop++, ptr++ )
    *ptr = toupper ( *ptr );

  ret = strncmp ( a1, a2, len );
  free ( a1 );
  free ( a2 );

  return ( ret );
}

static httpError httpReadGet ( request )
httpRequest *request;
{
  int rval;
  char data[MAX_READ_SIZE], *alldata, *ptr;
  int len;

  memset ( data, '\0', MAX_READ_SIZE );
  rval = recv ( request->connection, data, MAX_READ_SIZE, 0 );
  if ( rval < 0 )
    return ( HTTP_SOCKET_ERROR );
  else if ( rval == 0 ) {
    /* we are done. */
    request->gen_callback ( request->data_read, request->data_len );
    request->gen_callback ( NULL, 0 );
    httpDequeueRequest ();
    return ( HTTP_NO_ERROR );
  }
  if ( request->data_read ) {
    alldata = (char *) malloc ( request->data_len + rval );
    memcpy ( alldata, request->data_read, request->data_len );
    memcpy ( alldata + request->data_len, data, rval );
    len = request->data_len + rval;
  }
  else {
    alldata = (char *) malloc ( rval );
    memcpy ( alldata, data, rval );
    len = rval;
  }
  if ( request->data_read )
    free ( request->data_read );
  if ( ! request->content_length ) {
    request->data_read = httpTruncateHeader ( alldata, &len );
    if ( request->data_read )
      request->data_len = len;
    if ( request->data_read ) {
      /* complete header found (starts in alldata) */
      ptr = strtok ( alldata, "\r\n" );
      while ( ptr ) {
        if ( strncmp ( ptr, "HTTP/1.0 200", 12 ) == 0 ||
          strncmp ( ptr, "HTTP/1.1 200", 12 ) == 0 ) {
          /* ok status. ignore */
        }
        else if ( strncmp ( ptr, "HTTP/1.", 7 ) == 0 ) {
          /* some other status -- error */
          request->gen_callback ( ptr, strlen ( ptr ) );   /* indicates error */
          request->gen_callback ( NULL, 0 );        /* indicates error */
          httpDequeueRequest ();
          free ( alldata );
          return ( HTTP_HTTP_ERROR );
        }
        else if ( my_strncasecmp ( ptr, "Content-Length:", 15 ) == 0 ) {
          /* specifies length of content */
          request->content_length = atoi ( ptr + 15 );
          break;
        }
        else {
           /* ignore all others.... */
        }
        ptr = strtok ( NULL, "\r\n" );
      }
    }
  }
  else {
    if ( request->content_length == len ) {
      /* we're done */
      request->gen_callback ( alldata, len );
      request->gen_callback ( NULL, 0 );
      httpDequeueRequest ();
      free ( alldata );
      return ( HTTP_HTTP_ERROR );
    }
  }

  free ( alldata );
  return ( HTTP_NO_ERROR );
}






char *httpErrorString ( num )
httpError num;
{
  static char msg[200];

  switch ( num ) {
    case HTTP_NO_ERROR:
      return ( "No error." );
    case HTTP_INVALID_HOST:
      return ( "Unable to resolve server name." );
    case HTTP_SYSTEM_ERROR:
      sprintf ( msg, "System error (%d)", errno );
      return ( msg );
    case HTTP_SOCKET_ERROR:
      sprintf ( msg, "System error (%d)", errno );
      return ( msg );
    case HTTP_HTTP_ERROR:
      return ( "HTTP protocol error." );
    case HTTP_NO_REQUESTS:
      return ( "No requests on queue." );
    case HTTP_TOO_MANY_REQUESTS:
      return ( "Too many requests on queue." );
    case HTTP_HOST_LOOKUP_FAILED:
      return ( "Unable to resolve server hostname" );
    case HTTP_OTHER_ERROR:
      return ( http_other_error );
    case HTTP_UNKNOWN_ERROR:
    default:
      return ( "Unknown error." );
  }
}









/*
** Take the given text and truncate after the http header
*/
static char *httpTruncateHeader ( text, len )
char *text;
int *len;
{
  char *ptr;
  char *ret;
  char *end_header1 = "\n\n";
  char *end_header2 = "\r\n\r\n";
  int loop;
  int newlen;

  /* check to see it we've reached the end. */
  if ( strlen ( text ) < 10 )
    return ( NULL );

  /* back up to the last separator */
  for ( loop = 0, ptr = text; loop < *len; loop++, ptr++ ) {
    if ( strncmp ( ptr, end_header1, strlen ( end_header1 ) ) == 0 ) {
      /* end of http header found */
      *ptr = '\0';
      ptr += strlen ( end_header1 );
      newlen = *len - strlen ( text ) - strlen ( end_header1 );
      ret = (char *) malloc ( newlen );
      memcpy ( ret, ptr, newlen );
      *len = newlen;
      return ( ret );
    }
    else if ( strncmp ( ptr, end_header2, strlen ( end_header2 ) ) == 0 ) {
      /* end of http header found */
      *ptr = '\0';
      ptr += strlen ( end_header2 );
      newlen = *len - strlen ( text ) - strlen ( end_header2 );
      ret = (char *) malloc ( newlen );
      memcpy ( ret, ptr, newlen );
      *len = newlen;
      return ( ret );
    }
  }

  return ( NULL );
}




static char *my_strtok ( ptr1, tok )
char *ptr1;
char *tok;
{
  static char *last;
  char *p;

  if ( ! ptr1 )
    ptr1 = last;
  else
    last = ptr1;

  if ( ! ptr1 )
    return ( NULL );

  for ( p = ptr1; *p != '\0'; p++ ) {
    if ( strncmp ( p, tok, strlen ( tok ) ) == 0 ) {
      *p = '\0';
      last = p + strlen ( tok );
      return ( ptr1 );
    }
  }

  last = NULL;
  return ( ptr1 );
}

