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
 *	Display the ChangeLog
 *
 * Author:
 *	Craig Knudsen, craig@k5n.us, https://www.k5n.us/gtimer/
 *
 * Home Page:
 *	https://www.k5n.us/gtimer/
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

static void changelog_ok_callback ( GtkWidget *widget, gpointer data )
{
  gtk_widget_hide ( changelog_window );
}

static gboolean changelog_X_callback ( GtkWidget *widget, gpointer data )
{
  changelog_ok_callback( widget, data );
  return (TRUE);    // do NOT continue destroying window
}



/*
** Create the window.
*/
static void create_changelog_window ()
{
  /*GtkTooltips *tooltips;*/
  GtkWidget *hbox, *textarea, *vscrollbar, *button;
  GtkWidget *vbox, *textview, *swindow, *ok_button;    // PV: +
  GtkTextBuffer *txbuf;                                // PV: +
  int txlen;                                           // PV: +
  GtkStyle *style;
  char msg[100];

  changelog_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_wmclass ( GTK_WINDOW ( changelog_window ), "GTimer", "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext ("Change Log") );
  gtk_window_set_title (GTK_WINDOW (changelog_window), msg );
  gtk_window_set_position ( GTK_WINDOW(changelog_window), GTK_WIN_POS_MOUSE );
  gtk_widget_realize ( changelog_window );
  gdk_window_set_icon ( GTK_WIDGET ( changelog_window )->window,
    NULL, appicon2, appicon2_mask );

  vbox = gtk_vbox_new ( FALSE, 2 );
  gtk_container_add ( GTK_CONTAINER (changelog_window), vbox);

  hbox = gtk_hbox_new ( FALSE, 2 );
  gtk_box_pack_start ( GTK_BOX ( vbox ), hbox, FALSE, FALSE, 2 );

  swindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(swindow),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start ( GTK_BOX(vbox), swindow, TRUE, TRUE, 0);

//  style = gtk_style_new ();
// PV: -
/*
  gdk_font_unref ( style->font );
  style->font = gdk_font_load (
    "-adobe-courier-medium-r-*-*-12-*-*-*-*-*-*-*" );
  if ( style->font == NULL )
    style->font = gdk_font_load ( "fixed" );
*/
//  gtk_widget_push_style ( style );

  txbuf = gtk_text_buffer_new(NULL);
  txlen = strlen(changelog_text);
  gtk_text_buffer_set_text (GTK_TEXT_BUFFER(txbuf), changelog_text, txlen);
  textview = gtk_text_view_new_with_buffer(txbuf);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(textview),0);                   
  gtk_container_add (GTK_CONTAINER (swindow), textview);
  ok_button = gtk_button_new_with_label(gettext("Ok"));
  gtk_box_pack_start(GTK_BOX(vbox), ok_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(ok_button), "clicked",
                   G_CALLBACK (changelog_ok_callback), NULL);
  g_signal_connect(G_OBJECT(changelog_window), "delete-event",
                   G_CALLBACK (changelog_X_callback), NULL);


//  gtk_widget_show (button);
  gtk_window_set_default_size ( changelog_window, 600, 600 );
  /*gdk_window_resize ( GTK_WIDGET ( changelog_window )->window, 600, 400 );*/
  gtk_widget_show (changelog_window);
  gtk_widget_show_all(changelog_window);

  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);

}



void display_changelog () {
  if ( changelog_window == NULL )
    create_changelog_window ();
  else
    gtk_widget_show (changelog_window);
}

