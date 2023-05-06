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
 *	17-Apr-2005	Added configurability of the browser. (Russ Allbery)
 *	28-Feb-2003	Added project create/edit window.
 *	21-Feb-2003	Added project pulldown in task create/edit.
 *	30-Apr-1999	Fixed bug where \n chars could be included at the
 *			end of a task name (which would screw up the task
 *			data file format and make the task unreachable).
 *	25-Mar-1999	Fixed bug where adding new tasks messes up the
 *			hide/unhide stuff
 *	18-Mar-1999	Internationalization
 *	18-Mar-1998	Added calls to gtk_window_set_wmclass so the windows
 *			behave better for window managers.
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
#include "config.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

extern TaskData **visible_tasks;
extern int num_visible_tasks;
extern TaskData **tasks;
extern int num_tasks;
extern GtkWidget *status;

typedef struct {
  Project *p;
  GtkWidget *window;
  GtkWidget *name;
} ProjectEditData;

typedef struct {
  TaskData *taskdata;
  GtkWidget *window;
  GtkWidget *name;
  GtkWidget *projectMenu;
} TaskEditData;

typedef struct {
  GtkWidget *window;
  GtkWidget *name;
} BrowserEditData;


static void ok_project_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ProjectEditData *ed = (ProjectEditData *) data;
  Project *p;
  char *str, *ptr;
  GtkWidget *item = NULL;
  int project_updated = 0, loop;

  /* Update existing project */
  if ( ed->p ) {
    str = gtk_entry_get_text ( GTK_ENTRY(ed->name) );
    if ( strcmp ( str, ed->p->name ) ) {
      free ( ed->p->name );
      /* remove trailing white space */
      for ( ptr = str + strlen ( str ) - 1; isspace ( *ptr ) && ptr > str;
        ptr-- )
        *ptr = '\0';
      ed->p->name = (char *) malloc ( strlen ( str ) + 1 );
      strcpy ( ed->p->name, str );
      project_updated = 1;
    }
    showMessage ( gettext("Project updated") );
  }

  /* New Project */
  else {
    p = (Project *) malloc ( sizeof ( Project ) );
    memset ( p, '\0', sizeof ( Project ) );
    str = gtk_entry_get_text ( GTK_ENTRY(ed->name) );
    /* remove trailing white space */
    ptr = (char *) malloc ( strlen ( str ) + 1 );
    strcpy ( ptr, str );
    str = ptr;
    for ( ptr = str + strlen ( str ) - 1; isspace ( *ptr ) && ptr > str;
      ptr-- )
      *ptr = '\0';
    p = projectCreate ( str );
    projectAdd ( p );
    showMessage ( gettext("Project added") );
  }

  /* redraw the task list only if we changed a project name */
  if ( project_updated ) {
    for ( loop = 0; ed->p && loop < num_visible_tasks; loop++ ) {
      if ( visible_tasks[loop]->task->project_id == ed->p->number ) {
        visible_tasks[loop]->name_updated = 1;
        visible_tasks[loop]->project_name = ed->p->name;
      }
    }
    update_list ();
  }

  gtk_grab_remove ( ed->window );
  gtk_widget_destroy ( ed->window );
  free ( ed );

}


static void cancel_project_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ProjectEditData *ed = (ProjectEditData *) data;
  gtk_grab_remove ( ed->window );
  gtk_widget_destroy ( ed->window );
  free ( ed );
}




/*
** Create the add/edit project window.
** It's an add if project is NULL.
*/
GtkWidget *create_project_edit_window ( project )
Project *project;
{
  GtkWidget *edit_window;
  GtkWidget *table;
  /*GtkTooltips *tooltips;*/
  GtkWidget *label, *name_text, *ok_button, *cancel_button;
  GtkWidget *option, *option_menu, *menuitem;
  ProjectEditData *ed;
  char msg[100], *ptr = NULL;

  ed = (ProjectEditData *) malloc ( sizeof ( ProjectEditData ) );
  memset ( ed, 0, sizeof ( ProjectEditData ) );
  ed->p = project;
  ed->window = edit_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( ed->window ), "GTimer", "gtimer" );
  if ( project )
    sprintf ( msg, "GTimer: %s", gettext ("Edit Project") );
  else
    sprintf ( msg, "GTimer: %s", gettext ("Add Project") );
  gtk_window_set_title (GTK_WINDOW (edit_window), msg );
  gtk_window_position ( GTK_WINDOW(edit_window), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( edit_window );
  gtk_widget_realize ( edit_window );

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (edit_window)->vbox ),
    table, TRUE, FALSE, 5 );

  sprintf ( msg, "%s: ", gettext ( "Project name" ) );
  label = gtk_label_new ( msg );
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
    0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show ( label );

  ed->name = name_text = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), name_text, 1, 3,
    0, 1);
  if ( project )
    gtk_entry_set_text ( GTK_ENTRY(name_text),
      project->name );
  else {
    gtk_entry_set_text ( GTK_ENTRY(name_text), gettext ("Unnamed Project") );
    gtk_entry_select_region ( GTK_ENTRY(name_text), 0, 
      strlen ( gettext ("Unnamed Project") ) );
  }
  gtk_window_set_focus ( &GTK_DIALOG ( edit_window )->window,
    name_text );
  gtk_widget_show ( name_text );

  gtk_widget_show ( table );
  
  /* add command buttons */
  /*tooltips = gtk_tooltips_new ();*/

  ok_button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_window)->action_area),
    ok_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
    GTK_SIGNAL_FUNC (ok_project_callback), ed);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);
  gtk_widget_show (ok_button);
  /*gtk_tooltips_set_tips (tooltips, ok_button,
    "Save this task" );*/

  cancel_button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_window)->action_area),
    cancel_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
    GTK_SIGNAL_FUNC (cancel_project_callback), ed);
  gtk_widget_show (cancel_button);
  /*gtk_tooltips_set_tips (tooltips, cancel_button,
    "Edit the selected task" );*/

/*
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
*/
  gtk_widget_show (edit_window);

  return ( edit_window );
}



static int getMenuSelectionIndex ( GtkWidget *w )
{
  int count = 0;
  GList *child;
  GtkMenuShell *menu_shell;
  GtkBin *bin ;

  menu_shell = GTK_MENU_SHELL ( gtk_option_menu_get_menu (
    GTK_OPTION_MENU( w) ) );
  child = menu_shell->children;
  while ( child ) {
    bin = GTK_BIN ( child->data );
    if ( !bin->child )
      return count;
    child = child->next;
    count++;
  }
  return ( -1 );
}


static void ok_task_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  TaskEditData *ed = (TaskEditData *) data;
  TaskData *td;
  char *str, *ptr;
  GtkWidget *item = NULL;
  Project *p = NULL, *selp = NULL;
  int cnt, loop, new_project_id;

  /* Determine project */
  if ( ed->projectMenu ) {
    cnt = getMenuSelectionIndex ( ed->projectMenu );
    for ( loop = 0, p = projectGetFirst(); p != NULL && selp == NULL;
      p = projectGetNext(), loop++ ) {
      if ( loop + 1 == cnt ) {
        selp = p;
      }
    }
  }

  /* Update existing task */
  if ( ed->taskdata ) {
    str = gtk_entry_get_text ( GTK_ENTRY(ed->name) );
    if ( strcmp ( str, ed->taskdata->task->name ) ) {
      free ( ed->taskdata->task->name );
      /* remove trailing white space */
      for ( ptr = str + strlen ( str ) - 1; isspace ( *ptr ) && ptr > str;
        ptr-- )
        *ptr = '\0';
      ed->taskdata->task->name = (char *) malloc ( strlen ( str ) + 1 );
      strcpy ( ed->taskdata->task->name, str );
      ed->taskdata->name_updated = 1;
    }
    if ( ed->projectMenu ) {
      new_project_id = selp ? selp->number : -1;     
      if ( ed->taskdata->task->project_id != new_project_id ) {
        if ( selp == NULL )
          ed->taskdata->task->project_id = -1; /* none */
        else
          ed->taskdata->task->project_id = selp->number;
        ed->taskdata->name_updated = 1;
        ed->taskdata->project_name = selp ? selp->name : "";
      }
    }
    showMessage ( gettext("Task updated") );
  }

  /* New Task */
  else {
    td = (TaskData *) malloc ( sizeof ( TaskData ) );
    memset ( td, '\0', sizeof ( TaskData ) );
    td->new_task = 1;
    str = gtk_entry_get_text ( GTK_ENTRY(ed->name) );
    /* remove trailing white space */
    ptr = (char *) malloc ( strlen ( str ) + 1 );
    strcpy ( ptr, str );
    str = ptr;
    for ( ptr = str + strlen ( str ) - 1; isspace ( *ptr ) && ptr > str;
      ptr-- )
      *ptr = '\0';
    td->task = taskCreate ( str );
    taskAdd ( td->task );
    tasks = (TaskData **) realloc ( tasks,
      ( num_tasks + 1 ) * sizeof ( TaskData * ) );
    tasks[num_tasks] = td;
    visible_tasks = (TaskData **) realloc ( visible_tasks,
      ( num_visible_tasks + 1 ) * sizeof ( TaskData * ) );
    visible_tasks[num_visible_tasks] = td;
    num_visible_tasks++;
    num_tasks++;
    new_project_id = selp ? selp->number : -1;     
    td->task->project_id = new_project_id;
    td->project_name = selp ? selp->name : "";
    showMessage ( gettext("Task updated") );
  }

  gtk_grab_remove ( ed->window );
  gtk_widget_destroy ( ed->window );
  free ( ed );

  /* redraw the task list */
  update_list ();
}


static void cancel_task_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  TaskEditData *ed = (TaskEditData *) data;
  gtk_grab_remove ( ed->window );
  gtk_widget_destroy ( ed->window );
  free ( ed );
}




/*
** Create the add/edit window.
** It's an add if taskdata is NULL.
*/
GtkWidget *create_task_edit_window ( taskdata )
TaskData *taskdata;
{
  GtkWidget *edit_window;
  GtkWidget *table;
  /*GtkTooltips *tooltips;*/
  GtkWidget *label, *name_text, *ok_button, *cancel_button;
  GtkWidget *option, *option_menu, *menuitem;
  TaskEditData *ed;
  char msg[100], *ptr = NULL;
  int use_project, loop, selitem;
  Project *p;

  ed = (TaskEditData *) malloc ( sizeof ( TaskEditData ) );
  memset ( ed, 0, sizeof ( TaskEditData ) );
  ed->taskdata = taskdata;
  ed->window = edit_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( ed->window ), "GTimer", "gtimer" );
  if ( taskdata )
    sprintf ( msg, "GTimer: %s", gettext ("Edit Task") );
  else
    sprintf ( msg, "GTimer: %s", gettext ("Add Task") );
  gtk_window_set_title (GTK_WINDOW (edit_window), msg );
  gtk_window_position ( GTK_WINDOW(edit_window), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( edit_window );
  gtk_widget_realize ( edit_window );

  use_project = 0;
  if ( configGetAttribute ( CONFIG_USE_PROJECTS, &ptr ) == 0 &&
    ptr && ptr[0] == '1' )
    use_project = 1;

  table = gtk_table_new (1, use_project ? 4 : 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (edit_window)->vbox ),
    table, TRUE, FALSE, 5 );

  if ( use_project ) {
    sprintf ( msg, "%s: ", gettext ( "Project " ) );
    label = gtk_label_new ( msg );
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
      GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show ( label );

    option = gtk_option_menu_new ();
    gtk_widget_show ( option );
    gtk_table_attach_defaults ( GTK_TABLE (table), option, 1, 3, 0, 1);

    option_menu = gtk_menu_new ();

    menuitem = gtk_menu_item_new_with_label ( "None" );
    gtk_menu_append ( GTK_MENU ( option_menu ), menuitem );
    gtk_widget_show ( menuitem );

    selitem = 0;
    loop = 1;
    for ( p = projectGetFirst(); p != NULL; p = projectGetNext(), loop++ ) {
      menuitem = gtk_menu_item_new_with_label ( p->name );
      gtk_menu_append ( GTK_MENU ( option_menu ), menuitem );
      gtk_widget_show ( menuitem );
      gtk_object_set_data ( GTK_OBJECT ( menuitem ), "Project", p );
      if ( taskdata && taskdata->task->project_id == p->number )
        selitem = loop;
    }
    gtk_option_menu_set_menu ( GTK_OPTION_MENU ( option ), option_menu );
    if ( selitem > 0 )
      gtk_option_menu_set_history ( GTK_OPTION_MENU ( option ), selitem );
    ed->projectMenu = option;
  }

  sprintf ( msg, "%s: ", gettext ( "Task name" ) );
  label = gtk_label_new ( msg );
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
    0 + use_project, 1 + use_project,
    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show ( label );

  ed->name = name_text = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), name_text, 1, 3,
    0 + use_project, 1 + use_project);
  if ( taskdata )
    gtk_entry_set_text ( GTK_ENTRY(name_text),
      taskdata->task->name );
  else {
    gtk_entry_set_text ( GTK_ENTRY(name_text), gettext ("Unnamed Task") );
    gtk_entry_select_region ( GTK_ENTRY(name_text), 0,
      strlen ( gettext ("Unnamed Task") ) );
  }
  gtk_window_set_focus ( &GTK_DIALOG ( edit_window )->window,
    name_text );
  gtk_widget_show ( name_text );

  gtk_widget_show ( table );
  
  /* add command buttons */
  /*tooltips = gtk_tooltips_new ();*/

  ok_button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_window)->action_area),
    ok_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
    GTK_SIGNAL_FUNC (ok_task_callback), ed);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);
  gtk_widget_show (ok_button);
  /*gtk_tooltips_set_tips (tooltips, ok_button,
    "Save this task" );*/

  cancel_button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_window)->action_area),
    cancel_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
    GTK_SIGNAL_FUNC (cancel_task_callback), ed);
  gtk_widget_show (cancel_button);
  /*gtk_tooltips_set_tips (tooltips, cancel_button,
    "Edit the selected task" );*/

/*
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
*/
  gtk_widget_show (edit_window);

  return ( edit_window );
}



static void ok_browser_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  BrowserEditData *ed = (BrowserEditData *) data;
  char *browser;

  browser = gtk_entry_get_text ( GTK_ENTRY(ed->name) );
  configSetAttribute ( CONFIG_BROWSER, browser );
  gtk_grab_remove ( ed->window );
  gtk_widget_destroy ( ed->window );
  free ( ed );
}


static void cancel_browser_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  BrowserEditData *ed = (BrowserEditData *) data;
  gtk_grab_remove ( ed->window );
  gtk_widget_destroy ( ed->window );
  free ( ed );
}

/*
** Create the add/edit browser window.
*/
GtkWidget *create_browser_edit_window ()
{
  GtkWidget *browser_window;
  GtkWidget *table;
  GtkWidget *info, *label, *name_text, *ok_button, *cancel_button;
  BrowserEditData *ed;
  char msg[500], *browser;

  ed = (BrowserEditData *) malloc ( sizeof ( BrowserEditData ) );
  memset ( ed, 0, sizeof ( BrowserEditData ) );
  ed->window = browser_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( ed->window ), "GTimer", "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext ("Change Browser") );
  gtk_window_set_title ( GTK_WINDOW ( browser_window ), msg );
  gtk_window_position ( GTK_WINDOW ( browser_window ), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( browser_window );
  gtk_widget_realize ( browser_window );

  sprintf ( msg, "%s", "The string \"%s\" in this command, if present, will"
    " be replaced with the URL to open; otherwise, the URL will be added to"
    " the end of the command.");
  info = gtk_label_new ( msg );
  gtk_label_set_line_wrap ( GTK_LABEL (info), 1 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (browser_window)->vbox ),
    info, TRUE, FALSE, 5 );

  table = gtk_table_new ( 1, 3, FALSE );
  gtk_table_set_row_spacings ( GTK_TABLE (table), 4 );
  gtk_table_set_col_spacings ( GTK_TABLE (table), 8 );
  gtk_container_border_width ( GTK_CONTAINER (table), 6 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (browser_window)->vbox ),
    table, TRUE, FALSE, 5 );

  sprintf ( msg, "%s: ", gettext ( "Command" ) );
  label = gtk_label_new ( msg );
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1,
    0, 1, GTK_FILL, GTK_FILL, 0, 0);

  ed->name = name_text = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), name_text, 1, 3,
    0, 1);
  if ( configGetAttribute ( CONFIG_BROWSER, &browser ) == 0 )
    gtk_entry_set_text ( GTK_ENTRY(name_text), browser );
  else {
    gtk_entry_set_text ( GTK_ENTRY(name_text),
      "mozilla -raise -remote 'openURL(file:%s)'" );
    gtk_entry_select_region ( GTK_ENTRY(name_text), 0, 
      strlen ( "mozilla -raise -remote 'openURL(file:%s)'" ) );
  }
  gtk_window_set_focus ( &GTK_DIALOG ( browser_window )->window,
    name_text );

  ok_button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (browser_window)->action_area),
    ok_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
    GTK_SIGNAL_FUNC (ok_browser_callback), ed);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (ok_button);

  cancel_button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (browser_window)->action_area),
    cancel_button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
    GTK_SIGNAL_FUNC (cancel_browser_callback), ed);

  gtk_widget_show_all ( browser_window );

  return ( browser_window );
}
