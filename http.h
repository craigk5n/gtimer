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
 *	Routines for using HTTP.
 *
 * Author:
 *	Craig Knudsen, craig@k5n.us, https://www.k5n.us/gtimer
 *
 * Home Page:
 *	https://www.k5n.us/gtimer/
 *
 * History:
 *	19-May-1999	Stole from another project to use on GTimer :-)
 *	15-Aug-1995	Createdandle box to menu.
 *
 ****************************************************************************/


#ifndef _HTTP_H
#define _HTTP_H

#include "tcpt.h"	/* provides a little portability between UNIX/Win32 */

#define HTTP_PORT		80
#define HTTP_MAX_LINE_LEN	2048


/*
** Errors
*/
typedef enum {
  HTTP_NO_ERROR = 0,		/* success */
  HTTP_INVALID_HOST = 1,	/* invalid host specified */
  HTTP_SYSTEM_ERROR = 2,	/* error returned from system call */
				/* error value set in errno */
  HTTP_SOCKET_ERROR = 3,	/* error returned from socket function */
				/* winsock does not use error for errors */
  HTTP_HTTP_ERROR = 4,		/* error returned from server */
  HTTP_NO_REQUESTS = 5,		/* no requests on queue */
  HTTP_TOO_MANY_REQUESTS = 6,	/* too many requests queued */
  HTTP_OTHER_ERROR = 7,		/* other error */
  HTTP_HOST_LOOKUP_FAILED = 8,	/* unable to resolve name */
  HTTP_UNKNOWN_ERROR = 9	/* unknown error */
} httpError;


/*
** Encode text suitable for use in a URL.
*/
char *url_encode (
#ifndef _NO_PROTO
  char *str
#endif
);

/*
** httpErrorString - Translate a httpError value into a text string.
** If HTTP_SYSTEM_ERROR, then lookup the error using errno.  If
** HTTP_SOCKET_ERROR, then use winsock to get an error value.
*/
char *httpErrorString (
#ifndef _NO_PROTO
  httpError error_num		/* Error value */
#endif
);

/*
** httpProcessRead - Perform what's been queued up now that data is
** ready to be read from the socket.
*/
httpError httpProcessRead (
#ifndef _NO_PROTO
  sockfd connection
#endif
);

/*
** httpConnect - Attempt to connect to a server.
*/
httpError httpOpenConnection (
#ifndef _NO_PROTO
  char *servername,		/* in: hostname of http server */
  int port,			/* in: port to use (80) */
  sockfd *connection		/* return: socket (if successful) */
#endif
);



/*
** httpKillConnnection - Just close the socket immediately.
** Also removes all requests from the queue.
*/
httpError httpKillConnection (
#ifndef _NO_PROTO
  sockfd connection		/* in: connection to server */
#endif
);


/*
** Enable an HTTP proxy
*/
void httpEnableProxy (
#ifndef _NO_PROTO
  char *server,
  int port
#endif
);

/*
** Disable an HTTP proxy
*/
void httpDisableProxy ();

/*
** Generic http request
*/
httpError httpGet (
#ifndef _NO_PROTO
  sockfd connection,		/* in: connection to server */
  char *virthost,		/* in: hostname of server */
  char *path,			/* in: path to CGI */
  char *qs_names[],		/* in: names in form */
  char *qs_values[],		/* in: values in form */
  int num,			/* in: size of above arrays */
  void (*callback)(char *,int)	/* in: callback to call with data */
#endif
);


#endif /* _HTTP_H */
