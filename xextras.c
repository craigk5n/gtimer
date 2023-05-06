/*
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
 *	X utilities that I cannot find a way to implement using GTK.
 *	(Maybe they'll appear in a future GTK release.)
 *
 * History:
 *	18-Mar-1999	I18N
 *	18-Mar-1999	Added support for X11 screen saver extension
 *			for idle detect.  (This will detect keyboard
 *			usage rather than just mouse usage.)
 *	07-Apr-1998	Added call to XSetIOErrorHandler.
 *	28-Mar-1998	Created
 *
 ****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#ifdef HAVE_SCREEN_SAVER_EXT
#include <X11/extensions/scrnsaver.h>
#endif

#include "gtimeri18n.h"

extern void save_all ();


/*
** Handle X errors.
*/
static int x_error_handler ( display, event )
Display *display;
XErrorEvent *event;
{
  fprintf ( stderr, "%s\n", gettext ("Received X error.  See ya!") );
  save_all ();
  exit ( 0 );
}

/*
** Handle X IO errors.  This could be from xkill or the window manager
** dying and closing X, etc.
*/
static int x_io_error_handler ( display )
Display *display;
{
  fprintf ( stderr, "%s\n", gettext("Received X error.  See ya!") );
  save_all ();
  exit ( 0 );
}



/*
** Tell X to use the above function for X errors so we can catch
** them before we exit.
*/
void set_x_error_handler ()
{
  XSetErrorHandler ( x_error_handler );
  XSetIOErrorHandler ( x_io_error_handler );
}


/*
** Get the idle time
*/
#ifdef HAVE_SCREEN_SAVER_EXT
int get_x_idle_time ( display )
Display *display;
{
  int idle;
  static XScreenSaverInfo *ss_info = NULL;

  if ( ss_info == NULL )
    ss_info = XScreenSaverAllocInfo ();
  XScreenSaverQueryInfo ( display, DefaultRootWindow ( display ),
    ss_info );
  idle = ss_info->idle / 1000;

  return ( idle );
}
#endif

