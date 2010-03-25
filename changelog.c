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
 *	Display the ChangeLog
 *
 * Author:
 *	Craig Knudsen, cknudsen@cknudsen.com, http://www.cknudsen.com/
 *
 * Home Page:
 *	http://www.cknudsen.com/gtimer/
 *
 * History:
 *	13-May-1999	Created
 *
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>
#include <memory.h>
#include <ctype.h>

#include <gtk/gtk.h>

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#else
#define gettext(a)      a
#endif

#include "project.h"
#include "task.h"
#include "gtimer.h"
#include "changelog.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

extern GdkPixmap *appicon2;
extern GdkPixmap *appicon2_mask;

static GtkWidget *changelog_window = NULL;

static void ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  gtk_widget_hide ( changelog_window );
}




/*
** Create the window.
*/
static void create_changelog_window ()
{
  /*GtkTooltips *tooltips;*/
  GtkWidget *hbox, *textarea, *vscrollbar, *button;
  GtkStyle *style;
  char msg[100];

  changelog_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( changelog_window ), "GTimer", "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext ("Change Log") );
  gtk_window_set_title (GTK_WINDOW (changelog_window), msg );
  gtk_window_position ( GTK_WINDOW(changelog_window), GTK_WIN_POS_MOUSE );
  gtk_widget_realize ( changelog_window );
  gdk_window_set_icon ( GTK_WIDGET ( changelog_window )->window,
    NULL, appicon2, appicon2_mask );

  hbox = gtk_hbox_new ( FALSE, 2 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (changelog_window)->vbox ),
    hbox, TRUE, TRUE, 2 );

  style = gtk_style_new ();
  gdk_font_unref ( style->font );
  style->font = gdk_font_load (
    "-adobe-courier-medium-r-*-*-12-*-*-*-*-*-*-*" );
  if ( style->font == NULL )
    style->font = gdk_font_load ( "fixed" );
  gtk_widget_push_style ( style );

  textarea = gtk_text_new ( NULL, NULL );
  gtk_widget_set_usize ( textarea, 300, 100 );
  gtk_box_pack_start ( GTK_BOX ( hbox ),
    textarea, TRUE, TRUE, 2 );
  gtk_text_set_word_wrap ( GTK_TEXT ( textarea ), 1 );
  gtk_widget_show ( textarea );
  gtk_widget_realize ( textarea );
  gtk_text_freeze ( GTK_TEXT ( textarea ) );
  gtk_text_set_point ( GTK_TEXT ( textarea ), 0 );
  gtk_text_insert ( GTK_TEXT ( textarea ), NULL, NULL, NULL,
    changelog_text, strlen ( changelog_text ) );
  gtk_text_thaw ( GTK_TEXT ( textarea ) );

  gtk_widget_pop_style ();

  vscrollbar = gtk_vscrollbar_new ( GTK_TEXT ( textarea )->vadj );
  gtk_box_pack_start ( GTK_BOX ( hbox ),
    vscrollbar, FALSE, FALSE, 2 );
  gtk_widget_show ( vscrollbar );

  gtk_widget_show ( hbox );

  button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (changelog_window)->action_area),
    button, TRUE, FALSE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (ok_callback), NULL);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_set_usize ( changelog_window, 600, 400 );
  /*gdk_window_resize ( GTK_WIDGET ( changelog_window )->window, 600, 400 );*/
  gtk_widget_show (changelog_window);
}



void display_changelog () {
  if ( changelog_window == NULL )
    create_changelog_window ();
  else
    gtk_widget_show (changelog_window);
}

