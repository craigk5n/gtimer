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
 *	03-Mar-2003	Added support for rounding.
 *	28-Feb-2003	Include project name or "none" in reports
 *	09-Mar-2000	Updated calls to create_confirm_window()
 *	25-Mar-1999	Added use of gtk_window_set_transient_for() for
 *			GTK 1.1 or later.
 *	18-Mar-1999	Internationalization
 *	02-Feb-1999	gtk 1.1 support
 *	04-Jan-1999	Eek...  Fixed bug for "last month" reports when
 *			run in Jan.
 *	10-May-1998	Removed ifdef for GTK versions before 1.0.0
 *	05-Apr-1998	Added word wrap for report display
 *	04-Apr-1998	Added print
 *	28-Mar-1998	Finished adding support for HTML.
 *			Tell Netscape where to look for the file.
 *	26-Mar-1998	Added O_TRUNC option when saving to a file.
 *	26-Mar-1998	Added preliminary support for HTML output.
 *			Must save to a file and open with Netscape.
 *	22-Mar-1998	Added config_midnight_offset as an parameter to
 *			TaskGetAnnotationEntries.  Entries after midnight
 *			were showing up on the day after the hours were
 *			recorded.
 *	20-Mar-1998	Replaced label widget with a text widget.
 *	18-Mar-1998	Added calls to gtk_window_set_wmclass so the windows
 *			behave better for window managers.
 *	18-Mar-1998	Fixed bug where all weekdays show up as "Mon" in
 *			reports.
 *	18-Mar-1998	Fixed bug where for loop went past array boundaries
 *			and called free on invalid address.
 *	17-Mar-1998	Changed font in report window to be courier so
 *			that text would line up correctly.
 *	17-Mar-1998	Added option of including annotations and make
 *			including hours worked optional.
 *	16-Mar-1998	Removed reference to sys_errlist[] for
 *			compatibility with glibc.
 *	13-Mar-1998	Use config_midnight_offset when figuring out what the
 *			current date is.
 *	13-Mar-1998	Include seconds in reports
 *	12-Mar-1998	Remove toggle buttons for report type.  Now set
 *			by calling routine.
 *	10-Mar-1998	Added save to file
 *	10-Mar-1998	Added additional report options:
 *			  allow hours for each day to be optional
 *			  include summary for day
 *			  include summary for week
 *			  include summary for month
 *			  include days with no hours
 *	10-Mar-1998	Commented out all tooltips since the API is
 *			different for GTK-0.99.3 and GTK-0.99.5.
 *	08-Mar-1998	Created
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

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#else
#define gettext(a)      a
#endif

#include "project.h"
#include "task.h"
#include "gtimer.h"
#include "config.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

#define NO_TOOLTIPS	1

/* CSS stylesheet */
#define CSS_STYLE "\
<style>\n\
body {\n\
  background: #ffffff;\n\
}\n\
th {\n\
  background: #c0c0c0;\n\
}\n\
td {\n\
  background: #ffffff;\n\
}\n\
</style>\n"

#define ONE_DAY	(3600*24)
static int month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static int lmonth_days[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

extern TaskData **visible_tasks;
extern int num_visible_tasks;
extern int config_midnight_offset;
extern GtkWidget *main_window;
extern GdkPixmap *appicon2;
extern GdkPixmap *appicon2_mask;

/* Report types */
#define REPORT_RANGE_TODAY              0
#define REPORT_RANGE_THIS_WEEK          1
#define REPORT_RANGE_LAST_WEEK          2
#define REPORT_RANGE_THIS_AND_LAST_WEEK 3
#define REPORT_RANGE_LAST_TWO_WEEKS     4
#define REPORT_RANGE_THIS_MONTH         5
#define REPORT_RANGE_LAST_MONTH         6
#define REPORT_RANGE_THIS_YEAR          7
static char *time_options[] = { "Today", "This Week", "Last Week",
  "This Week & Last Week", "Last Two Weeks",
  "This Month", "Last Month", "This Year", NULL };

/* Output types */
#define REPORT_OUTPUT_TEXT		0
#define REPORT_OUTPUT_HTML		1
/* LaTeX next? */
static char *output_options[] = { "Text", "HTML", NULL };

/* Data to include */
#define REPORT_DATA_HOURS		0
#define REPORT_DATA_ANNOTATIONS		1
#define REPORT_DATA_BOTH		2
static char *data_options[] = { "Hours worked", "Annotations",
  "Hours & Annotations", NULL };

/* Rounding options */
#define REPORT_ROUND_NONE		0
#define REPORT_ROUND_MINUTE		60
#define REPORT_ROUND_5_MINUTES		300
#define REPORT_ROUND_10_MINUTES		600
#define REPORT_ROUND_15_MINUTES		900
#define REPORT_ROUND_30_MINUTES		1800
#define REPORT_ROUND_HOUR		3600
static char *round_options[] = { "None", "Minute", "5 Minutes",
  "10 Minutes", "15 minutes", "30 Minutes", "Hour", NULL };
static int round_values[]= { REPORT_ROUND_NONE,
  REPORT_ROUND_MINUTE, REPORT_ROUND_5_MINUTES,
  REPORT_ROUND_10_MINUTES, REPORT_ROUND_15_MINUTES,
  REPORT_ROUND_30_MINUTES, REPORT_ROUND_HOUR, -1 };
  

typedef struct {
  TaskData *taskdata;
  time_t weekly_total;
  time_t monthly_total;
  time_t yearly_total;
  time_t total;
  char *todays_annotations;
  char *weekly_annotations;
  char *monthly_annotations;
  char *yearly_annotations;
  char *total_annotations;
  char week_start[20];
} ReportTaskData;

typedef struct {
  GtkWidget *window;
  GtkWidget *time_menu;
  GtkWidget *time_menu_items[8];
  GtkWidget *output_menu;
  GtkWidget *output_menu_items[2];
  GtkWidget *data_menu;
  GtkWidget *data_menu_items[3];
  GtkWidget *round_menu;
  GtkWidget *round_menu_items[8];
  GtkWidget *task_list;
  TaskData **tasks;
  int num_tasks;
  GtkWidget **list_items;
  report_type type;
  int include_hours;
  int include_annotations;
} ReportData;

typedef struct {
  GtkWidget *window;
  char *text;
  GtkWidget *filesel;
  GtkWidget *printwin;
  GtkWidget *printentry;
} DisplayReportData;


/* Local functions */
static char *get_client_path ( char *file );





/*
** Append an annotation to the given pointer after we realloc some
** space for it.  optionally include the date and time and indent
** all lines to make it look nice.
*/
static void concat_annotation ( ptr, annotation, include_date,
  include_time, indentation, newline )
char **ptr;
TaskAnnotation *annotation;
int include_date, include_time;
char *indentation;
char *newline;
{
  struct tm *tm;
  char date_time_str[30];
  char padding[30], *p;
  char *anntext, *newtext;
  int first;

  strcpy ( date_time_str, indentation );
  tm = localtime ( &annotation->text_time );
  if ( include_date )
    sprintf ( date_time_str + strlen ( date_time_str ),
      "%02d/%02d/%02d ", tm->tm_mon + 1,
      tm->tm_mday, tm->tm_year % 100 );

  if ( include_time )
    sprintf ( date_time_str + strlen ( date_time_str ),
      "%02d:%02d ", tm->tm_hour, tm->tm_min );

  strcpy ( padding, date_time_str );
  for ( p = padding; *p != '\0'; p++ )
    *p = ' ';

  /* make a copy of the annotation text because strtok() stomps on it */
  anntext = (char *) malloc ( strlen ( annotation->text ) + 1 );
  strcpy ( anntext, annotation->text );

  if ( *ptr ) {
    newtext = (char *) malloc ( strlen ( *ptr ) + 1 );
    strcpy ( newtext, *ptr );
  } else {
    newtext = (char *) malloc ( 1 );
    newtext[0] = '\0';
  }
  first = 1;
  p = strtok ( anntext, "\n" );
  while ( p ) {
    newtext = (char *) realloc ( newtext,
      strlen ( newtext ) + strlen ( p ) + strlen ( padding ) + 2 +
      strlen ( newline ) );
    if ( first ) {
      first = 0;
      strcat ( newtext, date_time_str );
    } else {
      strcat ( newtext, padding );
    }
    strcat ( newtext, p );
    strcat ( newtext, newline );
    p = strtok ( NULL, "\n" );
  }
  free ( anntext );

  if ( *ptr )
    free ( *ptr );
  *ptr = newtext;
}



static void display_text_results_ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;
  gtk_grab_remove ( drd->window );
  gtk_widget_destroy ( drd->window );
  if ( drd->filesel ) {
    gtk_grab_remove ( drd->filesel );
    gtk_widget_destroy ( drd->filesel );
  }
  if ( drd->printwin ) {
    gtk_grab_remove ( drd->printwin );
    gtk_widget_destroy ( drd->printwin );
  }
  free ( drd->text );
  free ( drd );
}

static void display_text_results_save_cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;
  gtk_grab_remove ( drd->filesel );
  gtk_widget_destroy ( drd->filesel );
  drd->filesel = NULL;
}

static void display_text_results_save_ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;
  char *file;
  int fd;
  char *msg;

  file = gtk_file_selection_get_filename (
    GTK_FILE_SELECTION ( drd->filesel ) );
  fd = open ( file, O_WRONLY | O_CREAT | O_TRUNC, 0644 );
  if ( fd <= 0 ) {
    msg = (char *) malloc ( strlen ( file ) + 100 );
    sprintf ( msg, "%s %d %s\n%s", gettext ("Error"),
      errno, gettext("writing to file"), file );
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"), msg, gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
    free ( msg );
  } else {
    write ( fd, drd->text, strlen ( drd->text ) );
    close ( fd );
    gtk_grab_remove ( drd->filesel );
    gtk_widget_destroy ( drd->filesel );
    drd->filesel = NULL;
  }
}

static void display_text_results_save_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;
  char msg[100];

  if ( drd->filesel )
    return;

  sprintf ( msg, "GTimer: %s", gettext("Save Report") );
  drd->filesel = gtk_file_selection_new ( msg );

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(drd->filesel)->ok_button),
    "clicked", GTK_SIGNAL_FUNC (display_text_results_save_ok_callback), drd);
  gtk_signal_connect (
    GTK_OBJECT (GTK_FILE_SELECTION(drd->filesel)->cancel_button),
    "clicked", GTK_SIGNAL_FUNC (display_text_results_save_cancel_callback), drd);

  gtk_widget_show ( drd->filesel );
}



static void print_ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;
  int fd;
  char tempfile[100];
  char cmd[500], *ptr, *msg;

  ptr = gtk_entry_get_text ( GTK_ENTRY(drd->printentry) );
  if ( ! ptr || ! strlen ( ptr ) ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You must enter a print command"),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
    return;
  }

  sprintf ( tempfile, "/tmp/gtimer-%d.txt", (int) random () % 1000 );
  fd = open ( tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0644 );
  if ( fd <= 0 ) {
    msg = (char *) malloc ( strlen ( tempfile ) + 100 );
    sprintf ( msg, "%s %d %s\n%s", gettext("Error"),
      errno, gettext("writing to file"), tempfile );
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"), msg, gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
    free ( msg );
  } else {
    write ( fd, drd->text, strlen ( drd->text ) );
    close ( fd );
    sprintf ( cmd, "cat %s | %s", tempfile, ptr );
    if ( system ( cmd ) ) {
      create_confirm_window ( CONFIRM_ERROR,
        gettext("Error"), gettext("Error printing"),
        gettext("Ok"), NULL, NULL,
        NULL, NULL, NULL, NULL );
    } else {
      configSetAttribute ( CONFIG_PRINT, ptr );
      gtk_grab_remove ( drd->printwin );
      gtk_widget_destroy ( drd->printwin );
      drd->printwin = NULL;
    }
  }
}


static void print_cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;

  gtk_grab_remove ( drd->printwin );
  gtk_widget_destroy ( drd->printwin );
  drd->printwin = NULL;
}


static void display_text_results_print_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  DisplayReportData *drd = (DisplayReportData *)data;
  char *ptr, msg[100];
  GtkWidget *hbox, *label, *button;

  if ( drd->printwin )
    return;

  drd->printwin = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( drd->printwin ), "GTimer", "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext("Print Report") );
  gtk_window_set_title (GTK_WINDOW (drd->printwin), msg );
  gtk_window_position ( GTK_WINDOW(drd->printwin), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( drd->printwin );
  gtk_widget_realize ( drd->printwin );
  gdk_window_set_icon ( GTK_WIDGET ( drd->printwin )->window,
    NULL, appicon2, appicon2_mask );

#if GTK_VERSION >= 10100
  gtk_window_set_transient_for ( GTK_WINDOW ( drd->printwin ),
    GTK_WINDOW ( drd->window ) );
#endif

  hbox = gtk_hbox_new ( TRUE, 5 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (drd->printwin)->vbox ),
    hbox, FALSE, FALSE, 5 );

  sprintf ( msg, "%s: ", gettext("Print Command") );
  label = gtk_label_new ( msg );
  gtk_box_pack_start ( GTK_BOX ( hbox ), label, FALSE, FALSE, 5 );
  gtk_widget_show ( label );

  drd->printentry = gtk_entry_new ();
  gtk_box_pack_start ( GTK_BOX ( hbox ), drd->printentry, FALSE, FALSE, 5 );
  if ( configGetAttribute ( CONFIG_PRINT, &ptr ) == 0 )
    gtk_entry_set_text ( GTK_ENTRY( drd->printentry ), ptr );
  gtk_widget_show ( drd->printentry );

  gtk_widget_show ( hbox );

  button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (drd->printwin)->action_area),
    button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (print_ok_callback), drd);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (drd->printwin)->action_area),
    button, TRUE, TRUE, 5);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (print_cancel_callback), drd);
  gtk_widget_show (button);

  gtk_widget_show ( drd->printwin );
}




static void display_text_results ( text )
char *text;
{
  DisplayReportData *drd;
  GtkWidget *window, *hbox, *button, *textarea, *vscrollbar;
  GtkStyle *style;
  /*GtkTooltips *tooltips;*/
  char msg[100];

  drd = (DisplayReportData *) malloc ( sizeof ( DisplayReportData ) );
  memset ( drd, '\0', sizeof ( DisplayReportData ) );
  drd->text = text;

  drd->window = window = gtk_dialog_new ();
  sprintf ( msg, "GTimer: %s", gettext ("Report") );
  gtk_window_set_title (GTK_WINDOW (window), msg );
  gtk_window_set_wmclass ( GTK_WINDOW ( drd->window ), "GTimer", "gtimer" );
  gtk_widget_realize ( drd->window );
  gdk_window_set_icon ( GTK_WIDGET ( drd->window )->window,
    NULL, appicon2, appicon2_mask );

  hbox = gtk_hbox_new ( FALSE, 2 );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (window)->vbox ),
    hbox, TRUE, TRUE, 2 );

  style = gtk_style_new ();
  gdk_font_unref ( style->font );
  style->font = gdk_font_load (
    "-adobe-courier-medium-r-*-*-12-*-*-*-*-*-*-*" );
  if ( style->font == NULL )
    style->font = gdk_font_load ( "fixed" );
  gtk_widget_push_style ( style );

  textarea = gtk_text_new ( NULL, NULL );
  gtk_widget_set_usize ( textarea, 300, 200 );
  gtk_box_pack_start ( GTK_BOX ( hbox ),
    textarea, TRUE, TRUE, 2 );
  gtk_text_set_word_wrap ( GTK_TEXT ( textarea ), 1 );
  gtk_widget_show ( textarea );
  gtk_widget_realize ( textarea );
  gtk_text_freeze ( GTK_TEXT ( textarea ) );
  gtk_text_set_point ( GTK_TEXT ( textarea ), 0 );
  gtk_text_insert ( GTK_TEXT ( textarea ), NULL, NULL, NULL,
    text, strlen ( text ) );
  gtk_text_thaw ( GTK_TEXT ( textarea ) );

  gtk_widget_pop_style ();

  vscrollbar = gtk_vscrollbar_new ( GTK_TEXT ( textarea )->vadj );
  gtk_box_pack_start ( GTK_BOX ( hbox ),
    vscrollbar, FALSE, FALSE, 2 );
  gtk_widget_show ( vscrollbar );

  gtk_widget_show ( hbox );

  button = gtk_button_new_with_label ( gettext("Save") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
    button, TRUE, FALSE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (display_text_results_save_callback), drd);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( gettext("Print") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
    button, TRUE, FALSE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (display_text_results_print_callback), drd);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
    button, TRUE, FALSE, 10);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (display_text_results_ok_callback), drd);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show ( window );
}


static time_t do_round ( time_in, round_incr )
time_t time_in;
int round_incr;
{
  int t, n, r;

  if ( round_incr == 0 )
    return ( time_in );

  t = (int) time_in;
  r = t % round_incr;
  t -= r;
  if ( r > ( round_incr / 2 ) )
    t += round_incr;

  return ( (time_t) t );
}



static time_t summarize_day ( fp, seltasks, num_seltasks, then, type,
  format, include_hours, include_annotations, round_incr, is_last )
FILE *fp;
ReportTaskData **seltasks;
int num_seltasks;
time_t then;
report_type type;
int format;
int include_hours;
int include_annotations;
int round_incr;
int is_last;
{
  struct tm *tm;
  int found = 0;
  int loop, loop2;
  int mon, mday, year, wday;
  TaskTimeEntry *entry;
  TaskAnnotation **anns;
  int num_anns = 0;
  int h, m, s, rounded;
  static char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri",
    "Sat" };
  time_t ret = 0;
  char indentation[30];
  int ncols = 0;
  char *newline = "\n";

  switch ( format ) {
    case REPORT_OUTPUT_TEXT:
      newline = "\n";
      break;
    case REPORT_OUTPUT_HTML:
      newline = "<br>\n";
      break;
  }

  if ( include_hours )
    ncols++;
  if ( include_annotations )
    ncols++;

  tm = localtime ( &then );
  mon = tm->tm_mon + 1;
  mday = tm->tm_mday;
  year = tm->tm_year + 1900;
  wday = tm->tm_wday;

  if ( include_hours )
    strcpy ( indentation, "            " );
  else
    strcpy ( indentation, "  " );

  for ( loop = 0; loop < num_seltasks; loop++ ) {
    if ( seltasks[loop]->week_start[0] == '\0' || tm->tm_wday == 0 )
      sprintf ( seltasks[loop]->week_start, "%02d/%02d/%d %s", mon, mday,
        year, weekdays[wday] );
    if ( include_hours )
      entry = taskGetTimeEntry ( seltasks[loop]->taskdata->task,
        year, mon, mday );
    else
      entry = NULL;
    if ( include_annotations ) {
      anns = TaskGetAnnotationEntries ( seltasks[loop]->taskdata->task,
        year, mon, mday, config_midnight_offset, &num_anns );
      for ( loop2 = 0; loop2 < num_anns; loop2++ ) {
        if ( type == REPORT_TYPE_DAILY )
          concat_annotation ( &seltasks[loop]->todays_annotations,
            anns[loop2], 0, 0, indentation, newline );
        else if ( type == REPORT_TYPE_WEEKLY )
          concat_annotation ( &seltasks[loop]->weekly_annotations,
            anns[loop2], 0, 0, indentation, newline );
        else if ( type == REPORT_TYPE_MONTHLY )
          concat_annotation ( &seltasks[loop]->monthly_annotations,
            anns[loop2], 0, 0, indentation, newline );
        else if ( type == REPORT_TYPE_YEARLY )
          concat_annotation ( &seltasks[loop]->yearly_annotations,
            anns[loop2], 0, 0, indentation, newline );
        else if ( type == REPORT_TYPE_TOTAL )
          concat_annotation ( &seltasks[loop]->total_annotations,
            anns[loop2], 0, 0, indentation, newline );
      }
      if ( anns )
        free ( anns );
    }
    rounded = 0;
    if ( ( entry && entry->seconds ) || seltasks[loop]->todays_annotations ) {
      if ( type == REPORT_TYPE_DAILY ) {
        if ( ! found ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "\n%02d/%02d/%d %s\n", mon, mday, year,
                weekdays[wday] );
              fprintf ( fp, "-------------------------------------\n" );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<tr><th colspan=\"%d\">",
                ncols + 1 );
              fprintf ( fp, "%02d/%02d/%d %s\n", mon, mday, year,
                weekdays[wday] );
              fprintf ( fp, "</th></tr>\n" );
              break;
          }
          found = 1;
        }
        if ( entry && entry->seconds ) {
          rounded = do_round ( entry->seconds, round_incr );
          h = rounded / 3600;
          m = ( rounded - h * 3600 ) / 60;
          s = rounded % 60;
        } else {
          h = m = s = 0;
        }
        if ( format == REPORT_OUTPUT_HTML )
          fprintf ( fp, "<tr>" );
        if ( include_hours ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%3d:%02d:%02d - [%s] %s\n", h, m, s,
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp,
                "<td align=\"right\" valign=\"top\">%d:%02d:%02d</td>",
                h, m, s );
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
        } else {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "[%s] %s\n",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
        }
      }
      if ( rounded ) {
        ret += rounded;
        /* update totals */
        seltasks[loop]->weekly_total += rounded;
        seltasks[loop]->monthly_total += rounded;
        seltasks[loop]->yearly_total += rounded;
        seltasks[loop]->total += rounded;
      }
      if ( seltasks[loop]->todays_annotations ) {
        switch ( format ) {
          case REPORT_OUTPUT_TEXT:
            fprintf ( fp, "%s", seltasks[loop]->todays_annotations );
            break;
          case REPORT_OUTPUT_HTML:
            fprintf ( fp, "<td>%s</td>", seltasks[loop]->todays_annotations );
            break;
        }
        free ( seltasks[loop]->todays_annotations );
        seltasks[loop]->todays_annotations = NULL;
      }
      if ( type == REPORT_TYPE_DAILY && format == REPORT_OUTPUT_HTML )
        fprintf ( fp, "</tr>\n" );
    }
  }

  if ( type == REPORT_TYPE_DAILY && ret ) {
    h = ret / 3600;
    m = ( ret - h * 3600 ) / 60;
    s = ret % 60;
    switch ( format ) {
      case REPORT_OUTPUT_TEXT:
        fprintf ( fp, "---------\n" );
        fprintf ( fp, "%3d:%02d:%02d - %s\n", h, m, s, gettext("Total") );
        break;
      case REPORT_OUTPUT_HTML:
        fprintf ( fp, "<tr><td align=\"right\"><b>%d:%02d:%02d</b></td>",
          h, m, s );
        fprintf ( fp, "<td><b>%s</b></td></tr>\n", gettext("Total") );
        break;
    }
  }

  if ( ( type == REPORT_TYPE_WEEKLY ) && 
    ( wday == 6 || is_last ) ) {
    found = 0;
    for ( loop = 0; loop < num_seltasks; loop++ ) {
      if ( seltasks[loop]->weekly_total ||
        seltasks[loop]->weekly_annotations ) {
        if ( ! found ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "\n%s %s %s %02d/%02d/%d %s\n", gettext("Week"),
                seltasks[loop]->week_start, gettext("to"), mon, mday, year,
                weekdays[wday] );
              fprintf ( fp, "-------------------------------------\n" );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<tr><th colspan=\"%d\">", ncols + 1 );
              fprintf ( fp, "%s %s %s %02d/%02d/%d %s", gettext("Week"),
                seltasks[loop]->week_start, gettext("to"), mon, mday, year,
                weekdays[wday] );
              fprintf ( fp, "</th></tr>\n" );
              break;
          }
          found = 1;
        }
        if ( include_hours ) {
          h = seltasks[loop]->weekly_total / 3600;
          m = ( seltasks[loop]->weekly_total - h * 3600 ) / 60;
          s = seltasks[loop]->weekly_total % 60;
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%3d:%02d:%02d - [%s] %s\n", h, m, s,
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp,
                "<tr><td valign=\"top\" align=\"right\">%d:%02d:%02d</td>",
                h, m, s );
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
          seltasks[loop]->weekly_total = 0;
        } else {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "[%s] %s\n",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
        }
        if ( seltasks[loop]->weekly_annotations ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%s", seltasks[loop]->weekly_annotations );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td>%s</td>", seltasks[loop]->weekly_annotations );
              break;
          }
          free ( seltasks[loop]->weekly_annotations );
          seltasks[loop]->weekly_annotations = NULL;
        }
        if ( format == REPORT_OUTPUT_HTML )
          fprintf ( fp, "</tr>\n" );
      }
    }
  }

  if ( ( type == REPORT_TYPE_MONTHLY ) &&
    ( is_last ||
    ( ( ( tm->tm_year % 4 == 0 ) &&
      ( lmonth_days[tm->tm_mon] == tm->tm_mday ) ) ||
    ( ( tm->tm_year % 4 != 0 ) &&
      ( month_days[tm->tm_mon] == tm->tm_mday ) ) ) ) ) {
    found = 0;
    for ( loop = 0; loop < num_seltasks; loop++ ) {
      if ( seltasks[loop]->monthly_total ||
        seltasks[loop]->monthly_annotations ) {
        if ( ! found ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "\n%s %02d/%d\n", gettext("Month"), mon, year );
              fprintf ( fp, "-------------------------------------\n" );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<tr><th colspan=\"%d\">%s %02d/%d</th></tr>",
                ncols + 1, gettext("Month"), mon, year );
              break;
          }
          found = 1;
        }
        if ( include_hours ) {
          h = seltasks[loop]->monthly_total / 3600;
          m = ( seltasks[loop]->monthly_total - h * 3600 ) / 60;
          s = seltasks[loop]->monthly_total % 60;
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%3d:%02d:%02d - [%s] %s\n", h, m, s,
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">%d:%02d:%02d</td>",
                h, m, s );
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
          seltasks[loop]->monthly_total = 0;
        }
        else {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "[%s] %s\n",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
        }
        if ( seltasks[loop]->monthly_annotations ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%s", seltasks[loop]->monthly_annotations );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td>%s</td>",
                seltasks[loop]->monthly_annotations );
              break;
          }
          free ( seltasks[loop]->monthly_annotations );
          seltasks[loop]->monthly_annotations = NULL;
        }
        if ( format == REPORT_OUTPUT_HTML )
          fprintf ( fp, "</tr>\n" );
      }
    }
  }

  if ( ( type == REPORT_TYPE_YEARLY ) &&
    ( is_last || ( tm->tm_mon == 12 && tm->tm_mday == 31 ) ) ) {
    found = 0;
    for ( loop = 0; loop < num_seltasks; loop++ ) {
      if ( seltasks[loop]->yearly_total ||
        seltasks[loop]->yearly_annotations ) {
        if ( ! found ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "\n%s %d\n", gettext("Year"), year );
              fprintf ( fp, "-------------------------------------\n" );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<tr><th colspan=\"%d\">%s %d</th></tr>\n",
                ncols + 1, gettext("Year"), year );
              break;
          }
          found = 1;
        }
        if ( include_hours ) {
          h = seltasks[loop]->yearly_total / 3600;
          m = ( seltasks[loop]->yearly_total - h * 3600 ) / 60;
          s = seltasks[loop]->yearly_total % 60;
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%3d:%02d:%02d - [%s] %s\n", h, m, s,
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">%d:%02d:%02d</td>",
                h, m, s );
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
          seltasks[loop]->yearly_total = 0;
        }
        else {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "[%s] %s\n",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
        }
        if ( seltasks[loop]->yearly_annotations ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%s", seltasks[loop]->yearly_annotations );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td>%s</td>",
                seltasks[loop]->yearly_annotations );
              break;
          }
          free ( seltasks[loop]->yearly_annotations );
          seltasks[loop]->yearly_annotations = NULL;
        }
        if ( format == REPORT_OUTPUT_HTML )
          fprintf ( fp, "</tr>\n" );
      }
    }
  }

  if ( ( type == REPORT_TYPE_TOTAL ) && is_last ) {
    found = 0;
    for ( loop = 0; loop < num_seltasks; loop++ ) {
      if ( seltasks[loop]->yearly_total ) {
        if ( ! found ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "\n%s\n", gettext("Totals") );
              fprintf ( fp, "-------------------------------------\n" );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<tr><th COLSPAN=\"%d\">%s</th></tr>\n",
                ncols + 1, gettext("Totals") );
              break;
          }
          found = 1;
        }
        if ( include_hours ) {
          h = seltasks[loop]->total / 3600;
          m = ( seltasks[loop]->total - h * 3600 ) / 60;
          s = seltasks[loop]->total % 60;
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%3d:%02d:%02d - [%s] %s\n", h, m, s,
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">%d:%02d:%02d</td>", h, m, s );
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
          seltasks[loop]->total = 0;
        }
        else {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "[%s] %s\n",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td valign=\"top\">[%s] %s</td>",
                seltasks[loop]->taskdata->task->project_id < 0 ?
                "none" : seltasks[loop]->taskdata->project_name,
                seltasks[loop]->taskdata->task->name );
              break;
          }
        }
        if ( seltasks[loop]->total_annotations ) {
          switch ( format ) {
            case REPORT_OUTPUT_TEXT:
              fprintf ( fp, "%s", seltasks[loop]->total_annotations );
              break;
            case REPORT_OUTPUT_HTML:
              fprintf ( fp, "<td>%s</td>",
                seltasks[loop]->total_annotations );
              break;
          }
          free ( seltasks[loop]->total_annotations );
          seltasks[loop]->total_annotations = NULL;
        }
        if ( format == REPORT_OUTPUT_HTML )
          fprintf ( fp, "</tr>\n" );
      }
    }
  }

  if ( is_last ) {
    for ( loop = 0; loop < num_seltasks; loop++ ) {
      if ( seltasks[loop]->todays_annotations )
        free ( seltasks[loop]->todays_annotations );
      if ( seltasks[loop]->weekly_annotations )
        free ( seltasks[loop]->weekly_annotations );
      if ( seltasks[loop]->monthly_annotations )
        free ( seltasks[loop]->monthly_annotations );
      if ( seltasks[loop]->yearly_annotations )
        free ( seltasks[loop]->yearly_annotations );
      if ( seltasks[loop]->total_annotations )
        free ( seltasks[loop]->total_annotations );
      seltasks[loop]->todays_annotations = NULL;
      seltasks[loop]->weekly_annotations = NULL;
      seltasks[loop]->monthly_annotations = NULL;
      seltasks[loop]->yearly_annotations = NULL;
      seltasks[loop]->total_annotations = NULL;
    }
  }

  return ( ret );
}




static void display_html_results ( text )
char *text;
{
  char *path;
  int fd;
  char tempfile[50];
  char *command;

  path = get_client_path ( "netscape" );
  if ( path == NULL ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("Could not find \"netscape\"\nin your path."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
  }

  sprintf ( tempfile, "/tmp/gtimer-%d.html", (int) random () % 1000 );
  fd = open ( tempfile, O_WRONLY | O_CREAT | O_TRUNC, 0600 );
  write ( fd, text, strlen ( text ) );
  close ( fd );
  free ( text );

  command = (char *) malloc ( strlen ( path ) + 100 );
  sprintf ( command, "%s -raise -remote 'openURL(file:%s)'",
    path, tempfile );
  if ( system ( command ) != 0 ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"), gettext("Error communicating with Netscape."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL, NULL );
  }

  free ( path );
  free ( command );
}



static void ok_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ReportData *rd = (ReportData *) data;
  GList *selected, *item;
  int loop;
  ReportTaskData **seltasks;
  int num_selected = 0;
  int range = REPORT_RANGE_TODAY;
  int format = REPORT_OUTPUT_TEXT;
  int round_incr = REPORT_ROUND_NONE;
  FILE *fp;
  char *tempfile;
  time_t now, time_start, time_end, time_loop, total;
  struct tm *tm;
  struct stat buf;
  char *text = NULL;
  int fd, ret, ncols, h, m, s;

  selected = GTK_LIST ( rd->task_list ) ->selection;

  if ( ! selected ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You have not selected any tasks"),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
    return;
  }

  /* include hours / annotations*/
  for ( loop = 0; data_options[loop]; loop++ ) {
    if ( GTK_OPTION_MENU ( rd->data_menu ) -> menu_item ==
      rd->data_menu_items[loop] ) {
      switch ( loop ) {
        case REPORT_DATA_HOURS:
          rd->include_hours = 1;
          rd->include_annotations = 0;
          break;
        case REPORT_DATA_ANNOTATIONS:
          rd->include_hours = 0;
          rd->include_annotations = 1;
          break;
        case REPORT_DATA_BOTH:
          rd->include_hours = 1;
          rd->include_annotations = 1;
          break;
      }
    }
  }

  if ( ! rd->include_hours && ! rd->include_annotations ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("You must select either \"include hours\"\nor \"include annotations\"."),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
    return;
  }

  /* which tasks were selected... */
  seltasks = (ReportTaskData **) malloc
    ( sizeof ( ReportTaskData * ) * rd->num_tasks );
  for ( loop = 0; loop < rd->num_tasks; loop++ )
    seltasks[loop] = NULL;
  for ( item = selected; item != NULL; item = item->next ) {
    for ( loop = 0; loop < rd->num_tasks; loop++ ) {
      if ( item->data == rd->list_items[loop] ) {
        seltasks[num_selected] = (ReportTaskData *) malloc
          ( sizeof ( ReportTaskData ) );
        memset ( seltasks[num_selected], '\0', sizeof ( ReportTaskData ) );
        seltasks[num_selected++]->taskdata = rd->tasks[loop];
      }
    }
  }

  /* which time range ? */
  for ( loop = 0; time_options[loop]; loop++ ) {
    if ( GTK_OPTION_MENU ( rd->time_menu ) -> menu_item ==
      rd->time_menu_items[loop] ) {
      range = loop;
    }
  }

  /* which output format */
  for ( loop = 0; output_options[loop]; loop++ ) {
    if ( GTK_OPTION_MENU ( rd->output_menu ) -> menu_item ==
      rd->output_menu_items[loop] ) {
      format = loop;
    }
  }

  /* which rounding ? */
  for ( loop = 0; round_options[loop]; loop++ ) {
    if ( GTK_OPTION_MENU ( rd->round_menu ) -> menu_item ==
      rd->round_menu_items[loop] ) {
      round_incr = round_values[loop];
    }
  }

  gtk_grab_remove ( rd->window );
  gtk_widget_destroy ( rd->window );

  /* Generate the report...
  ** Write report to a file, then read it in...
  */
  tempfile = (char *) malloc ( 100 );
  sprintf ( tempfile, "/tmp/ttrpt-%d", (int) random () % 1000 );
  fp = fopen ( tempfile, "w" );
  if ( ! fp ) {
    create_confirm_window ( CONFIRM_ERROR,
      gettext("Error"),
      gettext("Error opening temporary file"),
      gettext("Ok"), NULL, NULL,
      NULL, NULL, NULL,
      NULL );
    free ( tempfile );
    for ( loop = 0; loop < num_visible_tasks; loop++ )
      free ( seltasks[loop] );
    free ( seltasks );
    free ( rd->tasks );
    free ( rd->list_items );
    free ( rd );
    return;
  }
  time ( &now );
  now -= config_midnight_offset;
  tm = localtime ( &now );
  switch ( range ) {
    case REPORT_RANGE_TODAY:
      time_start = time_end = now;
      break;
    case REPORT_RANGE_LAST_WEEK:
      time_start = now - ONE_DAY * tm->tm_wday - ONE_DAY * 7;
      time_end = now - ONE_DAY * tm->tm_wday - ONE_DAY;
      break;
    case REPORT_RANGE_THIS_AND_LAST_WEEK:
      time_start = now - ONE_DAY * tm->tm_wday - ONE_DAY * 7;
      time_end = time_start + ONE_DAY * 14;
      break;
    case REPORT_RANGE_LAST_TWO_WEEKS:
      time_start = now - ONE_DAY * tm->tm_wday - ONE_DAY * 14;
      time_end = now - ONE_DAY * tm->tm_wday - ONE_DAY;
      break;
    case REPORT_RANGE_THIS_MONTH:
      time_start = now - ( ONE_DAY * ( tm->tm_mday - 1 ) );
      time_end = now;
      break;
    case REPORT_RANGE_LAST_MONTH:
      if ( tm->tm_mon == 0 )
        time_start = now - ( ONE_DAY * ( tm->tm_mday - 1 ) ) - ONE_DAY * 31;
      else if ( tm->tm_year % 4 == 0 )
        time_start = now - ( ONE_DAY * ( tm->tm_mday - 1 ) ) -
          ONE_DAY * lmonth_days[tm->tm_mon-1];
      else
        time_start = now - ( ONE_DAY * ( tm->tm_mday - 1 ) ) -
          ONE_DAY * month_days[tm->tm_mon-1];
      time_end = now - ONE_DAY * tm->tm_mday;
      break;
    case REPORT_RANGE_THIS_YEAR:
      time_start = now - ONE_DAY * tm->tm_yday;
      time_end = now;
      break;
    default:
    case REPORT_RANGE_THIS_WEEK:
      time_start = now - ONE_DAY * tm->tm_wday;
      time_end = now;
      break;
  }

  if ( format == REPORT_OUTPUT_HTML ) {
    fprintf ( fp, "<html><head><title>GTimer %s</title></head>\n",
      gettext("Report") );
    fprintf ( fp, "%s", CSS_STYLE );
    fprintf ( fp, "<body>\n" );
    fprintf ( fp, "<table>\n" );
    ncols = 1;
    if ( rd->include_hours )
      ncols++;
    if ( rd->include_annotations )
      ncols++;
  }

  total = 0;
  for ( time_loop = time_start; time_loop <= time_end; time_loop += ONE_DAY ) {
    total += summarize_day ( fp, seltasks, num_selected, time_loop, rd->type,
      format, rd->include_hours, rd->include_annotations, round_incr,
      ( time_loop == time_end ) );
  }

  h = total / 3600;
  m = ( total - h * 3600 ) / 60;
  s = total % 60;

  switch ( format ) {
    case REPORT_OUTPUT_TEXT:
      fprintf ( fp, "\n%s\n-------------------------------------\n",
        gettext("Grand Total") );
      fprintf ( fp, "%3d:%02d:%02d\n", h, m, s );
      break;
    case REPORT_OUTPUT_HTML:
      fprintf ( fp, "<tr><th colspan=\"%d\">%s</th></tr>",
        1 + ncols, gettext("Grand Total") );
      fprintf ( fp, "<tr><td align=\"right\"><b>%d:%02d:%02d</b></td></tr>",
        h, m, s );
      fprintf ( fp, "</table><p><hr><font size=\"-1\">%s ",
        gettext("Generated by") );
      fprintf ( fp, "<a href=\"%s\">GTimer v%s (%s)</a>.\n",
        GTIMER_URL, GTIMER_VERSION, GTIMER_VERSION_DATE );
      fprintf ( fp, "</body></html>\n" );
      break;
  }

  fclose ( fp );
  if ( stat ( tempfile, &buf ) == 0 ) {
    text = (char *) malloc ( buf.st_size + 1 );
    fd = open ( tempfile, O_RDONLY );
    ret = read ( fd, text, buf.st_size );
    close ( fd );
    if ( ret >= 0 )
      text[ret] = '\0';
    else
      sprintf ( text, "%s %s\n", gettext("Error reading"), tempfile );
    switch ( format ) {
      case REPORT_OUTPUT_TEXT:
        display_text_results ( text );
        break;
      case REPORT_OUTPUT_HTML:
        display_html_results ( text );
        break;
    }
  }

  /* Free resources */
  unlink ( tempfile );
  free ( tempfile );
  for ( loop = 0; loop < num_selected; loop++ )
    free ( seltasks[loop] );
  free ( seltasks );
  free ( rd->tasks );
  free ( rd->list_items );
  free ( rd );
}


static void cancel_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ReportData *rd = (ReportData *) data;
  gtk_grab_remove ( rd->window );
  gtk_widget_destroy ( rd->window );
  free ( rd->tasks );
  free ( rd->list_items );
  free ( rd );
}



static void select_all_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ReportData *rd = (ReportData *) data;
#if GTK_VERSION < 10100
  int loop;

  for ( loop = 0; loop < rd->num_tasks; loop++ )
    gtk_list_select_item ( GTK_LIST ( rd->task_list ), loop );
#else
  gtk_list_select_all ( GTK_LIST ( rd->task_list ) );
#endif
}



static void select_none_callback ( widget, data )
GtkWidget *widget;
gpointer data;
{
  ReportData *rd = (ReportData *) data;
#if GTK_VERSION < 10100
  int loop;

  for ( loop = 0; loop < rd->num_tasks; loop++ )
    gtk_list_unselect_item ( GTK_LIST ( rd->task_list ), loop );
#else
  gtk_list_unselect_all ( GTK_LIST ( rd->task_list ) );
#endif
}




static GtkWidget *create_time_menu ( rd )
ReportData *rd;
{
  GtkWidget *menu;
  int loop;

  menu = gtk_menu_new ();

  for ( loop = 0; time_options[loop]; loop++ ) {
    rd->time_menu_items[loop] =
      gtk_menu_item_new_with_label ( time_options[loop] );
    gtk_menu_append ( GTK_MENU ( menu ), rd->time_menu_items[loop] );
    gtk_widget_show ( rd->time_menu_items[loop] );
  }
  return ( menu );
}




static GtkWidget *create_output_menu ( rd )
ReportData *rd;
{
  GtkWidget *menu;
  int loop;

  menu = gtk_menu_new ();

  for ( loop = 0; output_options[loop]; loop++ ) {
    rd->output_menu_items[loop] =
      gtk_menu_item_new_with_label ( output_options[loop] );
    gtk_menu_append ( GTK_MENU ( menu ), rd->output_menu_items[loop] );
    gtk_widget_show ( rd->output_menu_items[loop] );
  }
  return ( menu );
}




static GtkWidget *create_data_menu ( rd )
ReportData *rd;
{
  GtkWidget *menu;
  int loop;

  menu = gtk_menu_new ();

  for ( loop = 0; data_options[loop]; loop++ ) {
    rd->data_menu_items[loop] =
      gtk_menu_item_new_with_label ( data_options[loop] );
    gtk_menu_append ( GTK_MENU ( menu ), rd->data_menu_items[loop] );
    gtk_widget_show ( rd->data_menu_items[loop] );
  }
  return ( menu );
}


static GtkWidget *create_round_menu ( rd )
ReportData *rd;
{
  GtkWidget *menu;
  int loop;

  menu = gtk_menu_new ();

  for ( loop = 0; round_options[loop]; loop++ ) {
    rd->round_menu_items[loop] =
      gtk_menu_item_new_with_label ( round_options[loop] );
    gtk_menu_append ( GTK_MENU ( menu ), rd->round_menu_items[loop] );
    gtk_widget_show ( rd->round_menu_items[loop] );
  }
  return ( menu );
}



/*
** Create the report setup window.
** It's an add if taskdata is NULL.
*/
GtkWidget *create_report_window ( type )
report_type type;
{
  GtkWidget *report_window;
  GtkWidget *table;
  /*GtkTooltips *tooltips;*/
  GtkWidget *label, *button, *time_menu, *output_menu,
    *data_menu, *round_menu, *scrolled;
  ReportData *rd;
  GList *items = NULL;
  int loop;
  char msg[100], temp[512];

  rd = (ReportData *) malloc ( sizeof ( ReportData ) );
  memset ( rd, '\0', sizeof ( ReportData ) );
  rd->type = type;
  rd->window = report_window = gtk_dialog_new ();
  gtk_window_set_wmclass ( GTK_WINDOW ( rd->window ), "GTimer", "gtimer" );
  sprintf ( msg, "GTimer: %s", gettext("Report") );
  gtk_window_set_title (GTK_WINDOW (report_window), msg );
  gtk_window_position ( GTK_WINDOW(report_window), GTK_WIN_POS_MOUSE );
  gtk_grab_add ( report_window );
  gtk_widget_realize ( report_window );
  gdk_window_set_icon ( GTK_WIDGET ( rd->window )->window,
    NULL, appicon2, appicon2_mask );

  table = gtk_table_new ( 4, 2, FALSE );
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (report_window)->vbox ),
    table, TRUE, TRUE, 2 );

  label = gtk_label_new ( "Time Range: " );
  gtk_table_attach_defaults ( GTK_TABLE (table), label, 0, 1, 0, 1 );
  gtk_widget_show ( label );

  rd->time_menu = gtk_option_menu_new ();
  time_menu = create_time_menu ( rd );
  gtk_option_menu_set_menu ( GTK_OPTION_MENU ( rd->time_menu ), time_menu );
  gtk_option_menu_set_history ( GTK_OPTION_MENU ( rd->time_menu ),
    REPORT_RANGE_THIS_MONTH );
  gtk_table_attach_defaults ( GTK_TABLE (table), rd->time_menu, 1, 2, 0, 1 );
  gtk_widget_show ( rd->time_menu );


  label = gtk_label_new ( gettext("Format: ") );
  gtk_table_attach_defaults ( GTK_TABLE (table), label, 0, 1, 1, 2 );
  gtk_widget_show ( label );

  rd->output_menu = gtk_option_menu_new ();
  output_menu = create_output_menu ( rd );
  gtk_option_menu_set_menu ( GTK_OPTION_MENU ( rd->output_menu ), output_menu );
  gtk_option_menu_set_history ( GTK_OPTION_MENU ( rd->output_menu ),
    REPORT_OUTPUT_TEXT );
  gtk_table_attach_defaults ( GTK_TABLE (table), rd->output_menu, 1, 2, 1, 2 );
  gtk_widget_show ( rd->output_menu );

  label = gtk_label_new ( gettext("Data: ") );
  gtk_table_attach_defaults ( GTK_TABLE (table), label, 0, 1, 2, 3 );
  gtk_widget_show ( label );

  rd->data_menu = gtk_option_menu_new ();
  data_menu = create_data_menu ( rd );
  gtk_option_menu_set_menu ( GTK_OPTION_MENU ( rd->data_menu ), data_menu );
  gtk_option_menu_set_history ( GTK_OPTION_MENU ( rd->data_menu ),
    REPORT_OUTPUT_TEXT );
  gtk_table_attach_defaults ( GTK_TABLE (table), rd->data_menu, 1, 2, 2, 3 );
  gtk_widget_show ( rd->data_menu );

  label = gtk_label_new ( gettext("Rounding: ") );
  gtk_table_attach_defaults ( GTK_TABLE (table), label, 0, 1, 3, 4 );
  gtk_widget_show ( label );

  rd->round_menu = gtk_option_menu_new ();
  round_menu = create_round_menu ( rd );
  gtk_option_menu_set_menu ( GTK_OPTION_MENU ( rd->round_menu ), round_menu );
  gtk_option_menu_set_history ( GTK_OPTION_MENU ( rd->round_menu ),
    REPORT_OUTPUT_TEXT );
  gtk_table_attach_defaults ( GTK_TABLE (table), rd->round_menu, 1, 2, 3, 4 );
  gtk_widget_show ( rd->round_menu );

  gtk_widget_show ( table );

  label = gtk_label_new ( gettext("Tasks to include:") );
  gtk_label_set_justify ( GTK_LABEL ( label ), GTK_JUSTIFY_LEFT );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (report_window)->vbox ),
    label, FALSE, FALSE, 2 );
  gtk_widget_show ( label );

  /* list of tasks */
  scrolled = gtk_scrolled_window_new ( NULL, NULL );
  gtk_widget_set_usize ( scrolled, 300, 200 );
  gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW (scrolled),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS );
  gtk_box_pack_start ( GTK_BOX ( GTK_DIALOG (report_window)->vbox ),
    scrolled, TRUE, TRUE, 2 );
  gtk_widget_show ( scrolled );

  rd->task_list = gtk_list_new ();
  rd->tasks = (TaskData **) malloc ( sizeof ( TaskData * ) *
    num_visible_tasks );
  rd->list_items = (GtkWidget **) malloc ( sizeof ( GtkWidget * ) *
    num_visible_tasks );
  rd->num_tasks = num_visible_tasks;
  for ( loop = 0; loop < num_visible_tasks; loop++ ) {
    if ( visible_tasks[loop]->task->project_id < 0 )
      snprintf ( temp, sizeof ( temp ), "%s",
         visible_tasks[loop]->task->name );
    else
      snprintf ( temp, sizeof ( temp ),
        "[%s] %s", visible_tasks[loop]->project_name,
        visible_tasks[loop]->task->name );
    rd->list_items[loop] =
      gtk_list_item_new_with_label ( temp );
    items = g_list_append ( items, rd->list_items[loop] );
    rd->tasks[loop] = visible_tasks[loop];
    gtk_widget_show ( rd->list_items[loop] );
  }

#if GTK_VERSION < 10100
  gtk_container_add (GTK_CONTAINER (scrolled), rd->task_list );
#else
  gtk_scrolled_window_add_with_viewport ( GTK_SCROLLED_WINDOW ( scrolled ),
    rd->task_list );
#endif

  gtk_list_append_items ( GTK_LIST ( rd->task_list ), items );
  gtk_list_set_selection_mode ( GTK_LIST ( rd->task_list ),
    GTK_SELECTION_MULTIPLE );
  for ( loop = 0; loop < num_visible_tasks; loop++ ) {
    gtk_list_select_item ( GTK_LIST ( rd->task_list ), loop );
  }
  gtk_widget_show ( rd->task_list );
  
  /* add command buttons */
  /*tooltips = gtk_tooltips_new ();*/

  gtk_box_set_homogeneous ( GTK_BOX ( GTK_DIALOG (report_window)->action_area ),
    1 );
  button = gtk_button_new_with_label ( gettext("Ok") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (report_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_box_set_spacing ( GTK_BOX (GTK_DIALOG (report_window)->action_area),
    2 );
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (ok_callback), rd);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button, "Generate a report" );*/

  button = gtk_button_new_with_label ( gettext("Cancel") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (report_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (cancel_callback), rd);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button,
    "Cancel without generating a report" );*/

  button = gtk_button_new_with_label ( gettext("Select all") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (report_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (select_all_callback), rd);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button, "Select all tasks" );*/

  button = gtk_button_new_with_label ( gettext("Select none") );
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (report_window)->action_area),
    button, TRUE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
    GTK_SIGNAL_FUNC (select_none_callback), rd);
  gtk_widget_show (button);
  /*gtk_tooltips_set_tips (tooltips, button, "Unselect all tasks" );*/

  gtk_widget_show (report_window);

  return ( report_window );
}



/*
** Determine the path to an executable using the $PATH environment
** variable.
** Return value should be freed (unless it's NULL!)
*/
static char *get_client_path ( file )
char *file;
{
  char *path;
  static char *path2 = "/usr/bin/X11:/usr/local/bin:/usr/bin";
  char *env;
  struct stat buf;
  char *ret, *ptr;

  env = getenv ( "PATH" );
  if ( env ) {
    path = (char *) malloc ( strlen ( env ) + strlen ( path2 ) + 2 );
    sprintf ( path, "%s:%s", env, path2 );
  } else {
    path = (char *) malloc ( strlen ( path2 ) + 1 );
    strcpy ( path, path2 );
  }

  ptr = strtok ( path, ":" );
  while ( ptr ) {
    ret = (char *) malloc ( strlen ( ptr ) + strlen ( file ) + 2 );
    sprintf ( ret, "%s/%s", ptr, file );
    if ( stat ( ret, &buf ) == 0 ) {
      /* found it */
      free ( path );
      return ( ret );
    }
    ptr = strtok ( NULL, ":" );
  }

  /* not found */
  free ( path );
  return ( NULL );
}

