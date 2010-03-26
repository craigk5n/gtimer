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
 *	09-Mar-2000	Added optional third button for confirm windows.
 *	07-Mar-2000	Added create_confirm_toplevel ()
 *	25-Mar-1999	Added use of gtk_window_set_transient_for() for
 *			GTK 1.1 or later.
 *	18-Mar-1998	Added calls to gtk_window_set_wmclass so the windows
 *			behave better for window managers.
 *	16-Mar-1998	Changed application/about icon.
 *	03-Mar-1998	Created
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

#include "project.h"
#include "task.h"
#include "gtimer.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

/* icons */
#include "icons/error.xpm"
#include "icons/confirm.xpm"
#include "icons/about.xpm"

extern GtkWidget *main_window;

static GdkPixmap *error_icon = NULL, *confirm_icon = NULL,
  *gtimer_icon = NULL;
static GdkBitmap *error_mask, *confirm_mask, *gtimer_mask;

typedef struct {
  char *callback_data;
  GtkWidget *window;
  void (*ok_callback)();
  void (*cancel_callback)();
  void (*third_callback)();
} ConfirmData;


static void confirm_ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ConfirmData *cd = (ConfirmData *) data;

  gtk_grab_remove ( cd->window );
  if ( cd->ok_callback )
    cd->ok_callback ( widget, (gpointer)cd->callback_data );
  gtk_widget_destroy ( cd->window );
  free ( cd );
}


static void confirm_cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ConfirmData *cd = (ConfirmData *) data;

  gtk_grab_remove ( cd->window );
  if ( cd->cancel_callback )
    cd->cancel_callback ( widget, (gpointer)cd->callback_data );
  gtk_widget_destroy ( cd->window );
  free ( cd );
}


static void confirm_third_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ConfirmData *cd = (ConfirmData *) data;

  gtk_grab_remove ( cd->window );
  if ( cd->third_callback )
    cd->third_callback ( widget, (gpointer)cd->callback_data );
  gtk_widget_destroy ( cd->window );
  free ( cd );
}







/*
** Create the confirm window.
*/
static GtkWidget *create_window ( type, is_modal, title, text,
  ok_text, cancel_text, third_text,
  ok_callback, cancel_callback, third_callback,
  callback_data )
enum_confirm_type type;
int is_modal;
char *title;
char *text;
char *ok_text, *cancel_text, *third_text;
void (*ok_callback)();
void (*cancel_callback)();
void (*third_callback)();
char *callback_data;
{
  GtkWidget *window;
  /*GtkWidget *vbox, *hbox, *table;*/
  GtkWidget *table;
  GtkWidget *pixmap, *label, *ok_button, *cancel_button, *third_button;
  ConfirmData *cd;
  GdkPixmap *icon = NULL, *mask = NULL;
  char *title2;

  cd = (ConfirmData *) malloc ( sizeof ( ConfirmData ) );
  cd->callback_data = callback_data;
  cd->ok_callback = ok_callback;
  cd->cancel_callback = cancel_callback;
  cd->third_callback = third_callback;
  cd->window = window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( cd->window ), "GTimer", "gtimer" );
  title2 = (char *) malloc ( strlen ( title ) + 10 );
  sprintf ( title2, "GTimer: %s", title );
  gtk_window_set_title (GTK_WINDOW (window), title2 );
  free ( title2 );
  gtk_window_position ( GTK_WINDOW(window), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( window );
  gtk_widget_realize ( window );

#if OLD_GTK
#else
  if ( is_modal )
    gtk_window_set_transient_for ( GTK_WINDOW ( window ),
      GTK_WINDOW ( main_window ) );
#endif

  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (window)->vbox ),
    table, TRUE, TRUE, 5 );

  /* Add pixmap */
  switch ( type ) {
    case CONFIRM_ABOUT:
      if ( gtimer_icon == NULL )
        gtimer_icon = gdk_pixmap_create_from_xpm_d (
          GTK_WIDGET ( window )->window, &gtimer_mask,
          &window->style->white, about_xpm );
      icon = gtimer_icon;
      mask = gtimer_mask;
      break;
    case CONFIRM_ERROR:
    case CONFIRM_WARNING:
      if ( error_icon == NULL )
        error_icon = gdk_pixmap_create_from_xpm_d (
          GTK_WIDGET ( window )->window, &error_mask,
          &window->style->white, error_xpm );
      icon = error_icon;
      mask = error_mask;
      break;
    case CONFIRM_CONFIRM:
      if ( confirm_icon == NULL )
        confirm_icon = gdk_pixmap_create_from_xpm_d (
          GTK_WIDGET ( window )->window, &confirm_mask,
          &window->style->white, confirm_xpm );
      icon = confirm_icon;
      mask = confirm_mask;
      break;
    case CONFIRM_MESSAGE:
      break;
  }
  if ( icon ) {
    pixmap = gtk_pixmap_new ( icon, mask );
    gtk_misc_set_alignment (GTK_MISC (pixmap), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), pixmap, 0, 1, 0, 1,
      GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show ( pixmap );
  }

  /* Add message */
  label = gtk_label_new ( text );
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  gtk_widget_show ( label );

  gtk_widget_show ( table );
  
  /* add command buttons */

  if ( ok_text ) {
    ok_button = gtk_button_new_with_label ( ok_text );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
      ok_button, FALSE, FALSE, 10);
    gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
      GTK_SIGNAL_FUNC (confirm_ok_callback), cd);
    GTK_WIDGET_SET_FLAGS ( ok_button, GTK_CAN_DEFAULT );
    gtk_widget_grab_default ( ok_button );
    gtk_widget_show (ok_button);
  }

  if ( cancel_text ) {
    cancel_button = gtk_button_new_with_label ( cancel_text );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
      cancel_button, FALSE, FALSE, 10);
    gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
      GTK_SIGNAL_FUNC (confirm_cancel_callback), cd);
    if ( ! ok_text ) {
      GTK_WIDGET_SET_FLAGS ( cancel_button, GTK_CAN_DEFAULT );
      gtk_widget_grab_default ( cancel_button );
    }
    gtk_widget_show (cancel_button);
  }

  if ( third_text ) {
    third_button = gtk_button_new_with_label ( third_text );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
      third_button, FALSE, FALSE, 10);
    if ( confirm_third_callback != NULL )
      gtk_signal_connect (GTK_OBJECT (third_button), "clicked",
        GTK_SIGNAL_FUNC (confirm_third_callback), cd);
    gtk_widget_show (third_button);
  }

  gtk_widget_show (window);

  return ( window );
}




/*
** Create a toplevel confirm window.
*/
GtkWidget *create_confirm_toplevel ( type, title, text,
  ok_text, cancel_text, third_text,
  ok_callback, cancel_callback, third_callback,
  callback_data )
enum_confirm_type type;
char *title;
char *text;
char *ok_text, *cancel_text, *third_text;
void (*ok_callback)();
void (*cancel_callback)();
void (*third_callback)();
char *callback_data;
{
  return create_window ( type, FALSE, title, text, ok_text,
    cancel_text, third_text, ok_callback, cancel_callback, third_callback,
    callback_data );
}

/*
** Create a modal confirm window.
*/
GtkWidget *create_confirm_window ( type, title, text,
  ok_text, cancel_text, third_text,
  ok_callback, cancel_callback, third_callback,
  callback_data )
enum_confirm_type type;
char *title;
char *text;
char *ok_text, *cancel_text, *third_text;
void (*ok_callback)();
void (*cancel_callback)();
void (*third_callback)();
char *callback_data;
{
  return create_window ( type, TRUE, title, text,
    ok_text, cancel_text, third_text,
    ok_callback, cancel_callback, third_callback, callback_data );
}

