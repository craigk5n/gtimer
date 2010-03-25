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
 *	18-Mar-1999	Internationalization
 *	16-May-1999	Added support for GTK 1.0.
 *	10-May-1998	Removed ifdef for GTK versions before 1.0.0
 *	02-Apr-1998	Fixed bug that caused garbage to appear at the
 *			end of annotations.
 *	18-Mar-1998	Added calls to gtk_window_set_wmclass so the windows
 *			behave better for window managers.
 *	15-Mar-1998	Created
 *
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#include <time.h>
#include <memory.h>
#include <ctype.h>

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#else
#define gettext(a)      a
#endif

#include <gtk/gtk.h>

#include "project.h"
#include "task.h"
#include "gtimer.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif


extern char *taskdir;
extern GtkWidget *status;
extern guint status_id;

typedef struct {
  TaskData *taskdata;
  GtkWidget *window;
  GtkWidget *text;
} AnnotateData;


static void ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  AnnotateData *td = (AnnotateData *) data;
  char *str;
  int len;

  /* Add annotation to task */
  if ( td->taskdata ) {
    len = GTK_TEXT ( td->text )->gap_position;
    str = (char *) malloc ( len + 1 );
#if GTK_VERSION < 10100
    strncpy ( str, (char *) GTK_TEXT ( td->text )->text, len );
#else
    strncpy ( str, (char *) GTK_TEXT ( td->text )->text.wc, len );
#endif
    str[len] = '\0';
    if ( strlen ( str ) )
      taskAddAnnotation ( td->taskdata->task, taskdir, str );
    free ( str );
    gtk_statusbar_push ( GTK_STATUSBAR ( status ), status_id,
      gettext("Annotation added") );
  }

  gtk_grab_remove ( td->window );
  gtk_widget_destroy ( td->window );
  free ( td );
}


static void cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  AnnotateData *td = (AnnotateData *) data;
  gtk_grab_remove ( td->window );
  gtk_widget_destroy ( td->window );
  free ( td );
}




/*
** Create the add annotation window.
*/
GtkWidget *create_annotate_window ( taskdata )
TaskData *taskdata;
{
  GtkWidget *annotate_window;
  GtkWidget *label, *ok_button, *cancel_button, *hbox;
  GtkStyle *style;
  AnnotateData *td;
  char *str, msg[100];

  if ( taskdata == NULL )
    return ( NULL );

  td = (AnnotateData *) malloc ( sizeof ( AnnotateData ) );
  td->taskdata = taskdata;
  td->window = annotate_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( td->window ), "GTimer",
    "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext("Add Annotation") );
  gtk_window_set_title (GTK_WINDOW (annotate_window), msg );
  gtk_window_position ( GTK_WINDOW(annotate_window), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( annotate_window );
  gtk_widget_realize ( annotate_window );

  hbox = gtk_hbox_new ( TRUE, 5 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (annotate_window)->vbox ),
    hbox, FALSE, FALSE, 5 );

  str = (char *) malloc ( strlen ( taskdata->task->name ) + 100 );
  sprintf ( str, "%s\n%s", gettext("Enter annotation for"),
    taskdata->task->name );
  label = gtk_label_new ( str );
  free ( str );
  gtk_box_pack_start ( GTK_BOX ( hbox ), label, FALSE, FALSE, 5 );
  gtk_widget_show ( label );

  gtk_widget_show ( hbox );

  style = gtk_style_new ();
  gdk_font_unref ( style->font );
  style->font = gdk_font_load (
    "-adobe-courier-medium-r-*-*-12-*-*-*-*-*-*-*" );
  if ( style->font == NULL )
    style->font = gdk_font_load ( "fixed" );
  gtk_widget_push_style ( style );

  td->text = gtk_text_new ( NULL, NULL );
  gtk_text_set_word_wrap ( GTK_TEXT ( td->text ), 1 );
  gtk_widget_set_usize ( td->text, 340, 50 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (annotate_window)->vbox ),
    td->text, TRUE, TRUE, 5 );
  gtk_text_set_editable ( GTK_TEXT ( td->text ), TRUE );
  gtk_widget_show ( td->text );

  gtk_widget_pop_style ();

  /* add command buttons */
  /*tooltips = gtk_tooltips_new ();*/

  ok_button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (annotate_window)->action_area),
    ok_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
    GTK_SIGNAL_FUNC (ok_callback), td);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);
  gtk_widget_show (ok_button);
  /*gtk_tooltips_set_tips (tooltips, ok_button,
    "Save this task" );*/

  cancel_button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (annotate_window)->action_area),
    cancel_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
    GTK_SIGNAL_FUNC (cancel_callback), td);
  gtk_widget_show (cancel_button);
  /*gtk_tooltips_set_tips (tooltips, cancel_button,
    "Edit the selected task" );*/

  gtk_widget_show (annotate_window);

  return ( annotate_window );
}


