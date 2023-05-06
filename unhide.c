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
 *      07-Sep-2007	Updated to use GtkTreeView 
 *	09-Mar-2000	Updated call to create_confirm_window()
 *	18-Mar-1999	Internationalization
 *	16-Mar-1999	Add back support for GTK 1.0
 *	16-Feb-1999	Created unhide window
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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include "project.h"
#include "task.h"
#include "gtimer.h"
#include "config.h"
// PV:
#include "custom-list.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

#define NO_TOOLTIPS	1

extern TaskData **visible_tasks, **tasks;
extern int num_visible_tasks, num_tasks;
extern GtkWidget *main_window;

typedef struct {
  GtkWidget *window;
  GtkWidget *task_list;     // PV: -
//GtkTreeView *task_list;   // PV: +
  TaskData **tasks;
  int num_tasks;
//GtkWidget **list_items;   // PV: -
  GtkTreePath **list_items; // PV: +
} HideData;






static void ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  HideData *hd = (HideData *) data;
  GList *selected, *item;
  int loop;
  // PV:
  GtkTreeSelection *select;
#if PV_DEBUG
  volatile const char *s1;
  volatile const char *s2;
#else
  const char *s1, *s2;
#endif

//  selected = GTK_LIST ( hd->task_list ) ->selection;

  select = gtk_tree_view_get_selection ( GTK_TREE_VIEW(hd->task_list) );
  selected = gtk_tree_selection_get_selected_rows ( GTK_TREE_SELECTION(select), NULL );
  // selected contains list of paths

  if ( ! selected ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected any tasks"),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
    return;
  }

  /* which tasks were selected... */
  for ( item = selected; item != NULL; item = item->next ) {
    s1 = gtk_tree_path_to_string(item->data);
    for ( loop = 0; loop < hd->num_tasks; loop++ ) {
      s2 = gtk_tree_path_to_string( hd->list_items[loop] );
//    if ( item->data == hd->list_items[loop] ) {
      if ( strcmp (s1, s2) == 0 ) {
        /* don't need to realloc visible_tasks[] */
        visible_tasks[num_visible_tasks] = hd->tasks[loop];
        visible_tasks[num_visible_tasks]->moved = 1;
        visible_tasks[num_visible_tasks]->new_task = 1; /* add back */
        taskUnsetOption ( visible_tasks[num_visible_tasks]->task,
          GTIMER_TASK_OPTION_HIDDEN );
        num_visible_tasks++;
      }
    }
  }

  gtk_grab_remove ( hd->window );
  gtk_widget_destroy ( hd->window );

  /* Free resources */
  free ( hd->tasks );
  free ( hd->list_items );
  free ( hd );

  /* redraw the task list */
  update_list ();
}


static void cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  HideData *hd = (HideData *) data;
  gtk_grab_remove ( hd->window );
  gtk_widget_destroy ( hd->window );
  free ( hd->tasks );
  free ( hd->list_items );
  free ( hd );
}



static void select_all_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  HideData *hd = (HideData *) data;

  gtk_tree_selection_select_all (
    gtk_tree_view_get_selection ( GTK_TREE_VIEW(hd->task_list) ) );

}



static void select_none_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  HideData *hd = (HideData *) data;

  gtk_tree_selection_unselect_all (
    gtk_tree_view_get_selection ( GTK_TREE_VIEW(hd->task_list) ) );

}





/*
** Create the unhide window.
*/
GtkWidget *create_unhide_window ()
{
  GtkWidget *hide_window;
  /*GtkTooltips *tooltips;*/
  GtkWidget *label, *button, *scrolled;
  HideData *hd;
  GList *items = NULL;
  int loop, count;
  char msg[100];
  // PV:
  char temp[512];
  CustomList *customlist;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkTreeSelection *select;
  gchar * xtaskname;

  customlist = custom_list_new();

  hd = (HideData *) malloc ( sizeof ( HideData ) );
  memset ( hd, '\0', sizeof ( HideData ) );
  hd->window = hide_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( hd->window ), "GTimer", "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext("Unhide") );
  gtk_window_set_title (GTK_WINDOW (hide_window), msg );
  gtk_window_position ( GTK_WINDOW(hide_window), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( hide_window );
  gtk_widget_realize ( hide_window );

  label = gtk_label_new ( gettext("Tasks to include:") );
  gtk_label_set_justify ( GTK_LABEL ( label ), GTK_JUSTIFY_LEFT );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (hide_window)->vbox ),
    label, FALSE, FALSE, 2 );
  gtk_widget_show ( label );

  /* list of tasks */
  scrolled = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrolled, 300, 200 );
  gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW (scrolled),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (hide_window)->vbox ),
    scrolled, TRUE, TRUE, 2 );

  hd->tasks = (TaskData **) malloc ( sizeof ( TaskData * ) *
    num_tasks ); /* more than we need... */
  hd->list_items = (GtkWidget **) malloc ( sizeof ( GtkTreePath * ) *
    num_tasks ); /* more than we need... */
  for ( loop = 0, count = 0; loop < num_tasks; loop++ ) {
    if ( taskOptionEnabled ( tasks[loop]->task, GTIMER_TASK_OPTION_HIDDEN ) ) {

      if (tasks[loop]->task->project_id < 0 )
        snprintf ( temp, sizeof (temp), "%s", tasks[loop]->task->name );
      else
        snprintf ( temp, sizeof (temp), "[%s] %s", tasks[loop]->project_name,
	  tasks[loop]->task->name );
      items = g_list_append ( items, hd->list_items[count] );
      hd->tasks[count] = tasks[loop];
      xtaskname = g_strdup_printf ("%s", temp);
      hd->list_items[count] = custom_list_append_record( customlist, xtaskname);
      count++;
    }
  }
  hd->num_tasks = count;
// PV: TreeView
  hd->task_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(customlist));
  g_object_unref(customlist);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new();

  gtk_tree_view_column_pack_start ( col, renderer, TRUE );
  gtk_tree_view_column_add_attribute ( col, renderer, "text", CUSTOM_LIST_COL_NAME );
  gtk_tree_view_column_set_title ( col, gettext("[Project] Task") );
  gtk_tree_view_append_column ( GTK_TREE_VIEW(hd->task_list), col );
  select = gtk_tree_view_get_selection ( GTK_TREE_VIEW(hd->task_list) );
  gtk_tree_selection_set_mode( select, GTK_SELECTION_MULTIPLE );

  gtk_scrolled_window_add_with_viewport ( GTK_SCROLLED_WINDOW ( scrolled ),
    hd->task_list );

  gtk_tree_selection_select_all ( GTK_TREE_SELECTION(select) );
  gtk_widget_show ( scrolled );
  gtk_widget_show ( hd->task_list );
  
  /* add command buttons */
  /*tooltips = gtk_tooltips_new ();*/

  gtk_box_set_homogeneous ( GTK_BOX ( GTK_DIALOG (hide_window)->action_area ),
    1 );
  button = gtk_button_new_with_label (gettext("Ok"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (hide_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_box_set_spacing ( GTK_BOX (GTK_DIALOG (hide_window)->action_area),
    2 );
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (ok_callback), hd);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button, "Uhide selected tasks" );*/

  button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (hide_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (cancel_callback), hd);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button,
    "Cancel without unhiding taskst" );*/

  button = gtk_button_new_with_label ( gettext("Select all") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (hide_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (select_all_callback), hd);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button, "Select all tasks" );*/

  button = gtk_button_new_with_label ( gettext("Select none") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (hide_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (select_none_callback), hd);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button, "Unselect all tasks" );*/

  gtk_widget_show_all (hide_window);

  return ( hide_window );
}




