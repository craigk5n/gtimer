/*
 * Routines for dealing with tasks
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
 * History:
 *	17-Apr-2005	Add support for subtracting a particular offset
 *			off of timers.  (Russ Allbery)
 *	09-Mar-2000	Added functions to allow for restoring to
 *			a previous state: taskMark(), taskMarkAll(),
 *			taskRestore(), taskRestoreAll()
 *	25-Mar-1999	Lots of WIN32 patches made by
 *			Thomas Epperly <Thomas.Epperly@aspentech.com>
 *	18-Mar-1999	Internationalization
 *	11-Nov-1998	Added support for options
 *	23-Apr-1998	Added new function: taskClearAll
 *	22-Mar-1998	Added an extra argument to TaskGetAnnotationEntries
 *			for time offset.
 *	15-Mar-1998	Debugged all malloc/free and found one place
 *			where free() was not called.
 *	15-Mar-1998	Fixed bug that caused SIGSEGV.  Only appeared
 *			if one more tasks (but not the last one) were
 *			deleted.
 *	15-Mar-1998	Added new function: taskAddAnnotation
 *	11-Mar-1998	Changed functions to K&R style
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32
#include <io.h>
#else
#include <dirent.h>
#endif
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "task.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

// PV: Internationalization
#include "gtimeri18n.h"

static int num_tasks = 0;
static Task **tasks = NULL;
static int max_task = -1;
static int last_number = -1;


#ifdef WIN32
static int valid_name ( filename )
char *filename;
{
  while ( *filename && isdigit ( *filename ) )
    filename++;
  return ( ! strcmp ( filename, ".task" ) );
}
#endif


/*
** Add a task.
*/
void taskAdd ( task )
Task *task;
{
  int loop;
  int new_max_task = 0;

  new_max_task = max_task;

  if ( task->number == -1 ) {
    for ( loop = 0; loop < max_task && task->number < 0; loop++ ) {
      if ( tasks[loop] == NULL )
        task->number = loop;
    }
    if ( task->number < 0 )
      task->number = ++max_task;
    new_max_task = max_task;
  } else if ( task->number > max_task ) {
    new_max_task = task->number;
  }

  if ( tasks == NULL ) {
    tasks = (Task **) malloc ( sizeof ( Task * ) * ( new_max_task + 1 ) );
    for ( loop = 0; loop < new_max_task; loop++ )
      tasks[loop] = NULL;
  } else
    tasks = (Task **) realloc ( tasks,
      sizeof ( Task * ) * ( new_max_task + 1 ) );

  for ( loop = max_task + 1; loop <= new_max_task; loop++ )
    tasks[loop] = NULL;

  max_task = new_max_task;

  tasks[task->number] = task;
  num_tasks++;
}



/*
** Create a new task
*/
Task *taskCreate ( name )
char *name;
{
  Task *task;

  task = (Task *) malloc ( sizeof ( Task ) );
  memset ( task, '\0', sizeof ( Task ) );
  task->name = (char *) malloc ( strlen ( name ) + 1 );
  strcpy ( task->name, name );
  time ( &task->created );
  task->number = -1; /* not yet assigned */

  return ( task );
}


/*
** Mark the current time in a task.  We can then use taskRestore()
** to restore the state of the task to what it was when this function
** was called.
*/
void taskMark ( task, offset )
Task *task;
int offset;
{
  int i;
  int seconds;

  for ( i = 0; i < task->num_entries; i++ ) {
    task->entries[i]->marked_seconds = task->entries[i]->seconds;
  }
  if ( offset > 0 && task->num_entries > 0 ) {
    seconds = task->entries[task->num_entries - 1]->marked_seconds;
    seconds -= offset;
    if ( seconds < 0 )
      seconds = 0;
    task->entries[task->num_entries - 1]->marked_seconds = seconds;
  }
}


/*
** Mark the current time in all tasks so that we can use taskRestoreAll()
** to restore the state of all tasks to where they are right now.
** NOTE: this is intended to be used only with the timing elements.
** Other stuff like annotations doesn't apply here, but annotations shouldn't
** be added during idle time anyhow...
*/
void taskMarkAll ( offset )
int offset;
{
  int loop;

  for ( loop = 0; loop <= max_task; loop++ ) {
    if ( tasks[loop] ) {
      taskMark ( tasks[loop], offset );
    }
  }
}


/*
** Restore a task to its previous state when taskMark() was called.
** This is intended to allow the application to bookmark the state
** of a task and then return to it (as far as timing data goes).
*/
void taskRestore ( task )
Task *task;
{
  int i;

  for ( i = 0; i < task->num_entries; i++ ) {
    task->entries[i]->seconds = task->entries[i]->marked_seconds;
  }
}



/*
** Restore all tasks to where they were when taskRestoreAll()
** was called.
*/
void taskRestoreAll ()
{
  int loop;

  for ( loop = 0; loop <= max_task; loop++ ) {
    if ( tasks[loop] ) {
      taskRestore ( tasks[loop] );
    }
  }
}



/*
** Delete a task.
*/
int taskDelete ( task, taskdir )
Task *task;
char *taskdir;
{
  char *path;

  path = (char *) malloc ( strlen ( taskdir ) + 10 );
  sprintf ( path, "%s/%d.task", taskdir, task->number );
  unlink ( path );
  sprintf ( path, "%s/%d.ann", taskdir, task->number );
  unlink ( path );
  free ( path );

  tasks[task->number] = NULL;
  num_tasks--;
  taskFree ( task );

  return ( 0 );
}



/*
** Return the number of tasks currently loaded.
*/
int taskCount () {
  return ( num_tasks );
}



/*
** Get a task by number.
*/
Task *taskGet ( number )
int number;
{
  return ( tasks[number] );
}


/*
** Get first task.
*/
Task *taskGetFirst () {
  last_number = -1;
  return ( taskGetNext() );
}


/*
** Get next task.
*/
Task *taskGetNext () {
  int loop;

  if ( ! num_tasks )
    return ( NULL );

  for ( loop = last_number + 1; loop <= max_task; loop++ ) {
    if ( tasks[loop] ) {
      last_number = loop;
      return ( tasks[loop] );
    }
  }
 
  return ( NULL );
}



/*
** Clear all tasks out of memory.
*/
void taskClearAll ()
{
  int loop;

  for ( loop = 0; loop <= max_task; loop++ ) {
    if ( tasks[loop] ) {
      taskFree ( tasks[loop] );
      tasks[loop] = 0;
    }
  }
  num_tasks = 0;
  last_number = max_task = -1;
}


/*
** Save a task to it's task file.
*/
int taskSave ( task, taskdir )
Task *task;
char *taskdir;
{
  char *path;
  FILE *fp;
  int loop;

  path = (char *) malloc ( strlen ( taskdir ) + 10 );
  sprintf ( path, "%s/%d.task", taskdir, task->number );

  fp = fopen ( path, "w" );
  if ( !fp ) {
    free ( path );
    return ( TASK_ERROR_SYSTEM_ERROR );
  }

  fprintf ( fp, "Format: 1.2\n" );
  fprintf ( fp, "Name: %s\n", task->name );
  fprintf ( fp, "Created: %u\n", (unsigned int)task->created );
  fprintf ( fp, "Options: %u\n", task->options );
  fprintf ( fp, "Project: %d\n", task->project_id );
  fprintf ( fp, "Data:\n" );

  for ( loop = 0; loop < task->num_entries; loop++ ) {
    if ( task->entries[loop]->seconds )
      fprintf ( fp, "%04d%02d%02d %d\n",
        task->entries[loop]->year, task->entries[loop]->mon,
        task->entries[loop]->mday, task->entries[loop]->seconds );
  }

  fclose ( fp );
  free ( path );

  return ( 0 );
}


/*
** Save all tasks
*/
int taskSaveAll ( taskdir )
char *taskdir;
{
  int loop;
  int ret;

  for ( loop = 0; loop <= max_task; loop++ ) {
    if ( tasks[loop] ) {
      ret = taskSave ( tasks[loop], taskdir );
      if ( ret )
        return ( ret );
    }
  }
  return ( 0 );
}



/*
** Free all resources of a task.
*/
void taskFree ( task )
Task *task;
{
  int loop;
  free ( task->name );
  for ( loop = 0; loop < task->num_entries; loop++ )
    free ( task->entries[loop] );
  if ( task->entries )
    free ( task->entries );
  for ( loop = 0; loop < task->num_annotations; loop++ ) {
    free ( task->annotations[loop]->text );
    free ( task->annotations[loop] );
  }
  if ( task->annotations )
    free ( task->annotations );
  free ( task );
}



/*
** Load a task from file.
*/
int taskLoad ( path, task )
char *path;
Task **task;
{
  FILE *fp;
  int fd;
  Task *newtask;
  char line[512], *ptr, *ptr2, temp[10], *annfile, *anntext;
  int len, created, number, options, project_id = -1;
  TaskTimeEntry *entry;
  TaskAnnotation *a;
  struct stat buf;

  fp = fopen ( path, "r" );
  if ( !fp )
    return ( TASK_ERROR_SYSTEM_ERROR );

  for ( ptr = path + strlen ( path ) - 1; *ptr != '/' && ptr != path; ptr-- );
  if ( *ptr == '/' )
    ptr++;
  sscanf ( ptr, "%d.task", &number );

  fgets ( line, 512, fp );
  len = strlen ( line );
  if ( line[len-1] == '\n' )
    line[len-1] = '\0';
  if ( strcmp ( line, "Format: 1.0" ) && strcmp ( line, "Format: 1.1" ) &&
    strcmp ( line, "Format: 1.2" ) ) {
    fclose ( fp );
    return ( TASK_ERROR_BAD_FILE );
  }

  newtask = (Task *) malloc ( sizeof ( Task ) );
  memset ( newtask, '\0', sizeof ( Task ) );
  newtask->number = number;
  newtask->project_id = -1; /* no project */

  while ( fgets ( line, 512, fp ) ) {
    len = strlen ( line );
    if ( line[len-1] == '\n' )
      line[len-1] = '\0';
    if ( strncmp ( line, "Name:", 5 ) == 0 ) {
      ptr = line + 5;
      if ( *ptr == ' ' )
        ptr++;
      newtask->name = (char *) malloc ( strlen ( ptr ) + 1 );
      strcpy ( newtask->name, ptr );
    } else if ( strncmp ( line, "Created:", 8 ) == 0 ) {
      sscanf ( line + 8, "%d", &created );
      newtask->created = (time_t) created;
    } else if ( strncmp ( line, "Project:", 8 ) == 0 ) {
      sscanf ( line + 8, "%d", &project_id );
      newtask->project_id = project_id;
    } else if ( strncmp ( line, "Options:", 8 ) == 0 ) {
      sscanf ( line + 8, "%d", &options );
      newtask->options = (unsigned int) options;
    } else if ( strcmp ( line, "Data:" ) == 0 ) {
      while ( fgets ( line, 512, fp ) ) {
        entry = (TaskTimeEntry *) malloc ( sizeof ( TaskTimeEntry ) );
        memset ( entry, '\0', sizeof ( TaskTimeEntry ) );
        strncpy ( temp, line, 4 );
        temp[4] = '\0';
        entry->year = atoi ( temp );
        strncpy ( temp, line + 4, 2 );
        temp[2] = '\0';
        entry->mon = atoi ( temp );
        strncpy ( temp, line + 6, 2 );
        temp[2] = '\0';
        entry->mday = atoi ( temp );
        entry->seconds = atoi ( line + 9 );
        entry->marked_seconds = entry->seconds;
        if ( ! newtask->entries )
          newtask->entries = (TaskTimeEntry **) malloc ( 
            sizeof ( TaskTimeEntry * ) );
        else
          newtask->entries = (TaskTimeEntry **) realloc ( newtask->entries,
            sizeof ( TaskTimeEntry * ) * ( newtask->num_entries + 1 ) );
        newtask->entries[newtask->num_entries] = entry;
        newtask->num_entries++;
      }
      break;
    } else {
      fclose ( fp );
      taskFree ( newtask );
      return ( TASK_ERROR_BAD_FILE );
    }
  }
  fclose ( fp );

  /* now load annotations */
  annfile = (char *) malloc ( strlen ( path ) + 1 );
  strcpy ( annfile, path );
  ptr = annfile + strlen ( annfile ) - 5;
  if ( strcmp ( ptr, ".task" ) == 0 ) {
    strcpy ( ptr, ".ann" );
    if ( stat ( annfile, &buf ) == 0 ) {
      fd = open ( annfile, O_RDONLY );
      anntext = (char *) malloc ( buf.st_size + 1 );
      read ( fd, anntext, buf.st_size );
      anntext[buf.st_size] = '\0';
      close ( fd );
      ptr = strtok ( anntext, "\n" );
      while ( ptr ) {
        ptr2 = ptr;
        while ( isdigit ( *ptr2 ) )
          ptr2++;
        if ( *ptr2 == ' ' ) {
          *ptr2 = '\0';
          a = (TaskAnnotation *) malloc ( sizeof ( TaskAnnotation ) );
          memset ( a, '\0', sizeof ( TaskAnnotation ) );
          a->text_time = atoi ( ptr );
          ptr2++;
          a->text = (char *) malloc ( strlen ( ptr2 ) + 1 );
          strcpy ( a->text, ptr2 );
          for ( ptr2 = a->text; *ptr2 != '\0'; ptr2++ )
            if ( *ptr2 == '\r' )
              *ptr2 = '\n';
          if ( newtask->annotations == NULL ) {
            newtask->annotations = (TaskAnnotation **) malloc (
              sizeof ( TaskAnnotation * ) );
          } else {
            newtask->annotations = (TaskAnnotation **) realloc (
              newtask->annotations,
              ( newtask->num_annotations + 1 ) * sizeof ( TaskAnnotation * ) );
          }
          newtask->annotations[newtask->num_annotations] = a;
          newtask->num_annotations++;
        }
        ptr = strtok ( NULL, "\n" );
      }
      free ( anntext );
    }
  }
  free ( annfile );

  taskAdd ( newtask );

  *task = newtask;

  return ( 0 );
}



int taskLoadAll ( taskdir )
char *taskdir;
{
#ifdef WIN32
   char *pattern;
   Task *task;
   long handle;
   struct _finddata_t fdata;
   pattern = malloc ( strlen ( taskdir ) + 8 );
   (void) strcat ( strcpy ( pattern, taskdir ), "/*.task" );
   if ( ( handle = _findfirst ( pattern, &fdata ) ) != -1 ) {
     char *path = malloc ( strlen ( taskdir ) + _MAX_FNAME + 2 );
     char *start = path + strlen ( taskdir ) + 1;
     (void) strcat ( strcpy ( path, taskdir ), "/" );
     do {
       (void) strcpy ( start, fdata.name );
       if ( valid_name ( fdata.name ) && !_access ( path, 4 ) ){
         taskLoad ( path, &task );
       }
     } while ( !_findnext ( handle, &fdata ) );
     _findclose ( handle );
     free ( path );
   }
   else {
     free ( pattern );
     return ( TASK_ERROR_SYSTEM_ERROR );
   }
   free ( pattern );
#else
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char *path, *ptr;
  Task *task;

  dir = opendir ( taskdir );
  if ( ! dir )
    return ( TASK_ERROR_SYSTEM_ERROR );
  while ( ( entry = readdir ( dir ) ) ) {
    path = (char *) malloc ( strlen ( taskdir ) + strlen ( entry->d_name )
      + 2 );
    sprintf ( path, "%s/%s", taskdir, entry->d_name );
    if ( stat ( path, &buf ) == 0 ) {
      for ( ptr = entry->d_name; isdigit ( *ptr ); ptr++ ) ;
      if ( strcmp ( ptr, ".task" ) == 0 && S_ISREG ( buf.st_mode ) ) {
        /* NOTE: add catching of errors here... */
        taskLoad ( path, &task );
      }
    }
    free ( path );
  }

#endif
  return ( 0 );
}


char *taskErrorString ( task_error )
int task_error;
{
  static char msg[256];

  switch ( task_error ) {
    case TASK_ERROR_SYSTEM_ERROR:
      sprintf ( msg, gettext("System Error (%d)"), errno );
      return ( msg );
    case TASK_ERROR_BAD_FILE:
      return ( gettext("Invalid task data file format") );
    default:
      sprintf ( msg,  gettext("Unknown error (%d)"), task_error );
      return ( msg );
  }
}




TaskTimeEntry *taskGetTimeEntry ( task, year, month, day )
Task *task;
int year, month, day;
{
  int loop;
  struct tm *tm;
  time_t now;

  if ( year < 100 ) {
    time ( &now );
    tm = localtime ( &now );
    year += 1900 + ( tm->tm_year % 100 );
  }
  
  for ( loop = 0; loop < task->num_entries; loop++ ) {
    if ( task->entries[loop]->year == year &&
      task->entries[loop]->mon == month &&
      task->entries[loop]->mday == day )
      return ( task->entries[loop] );
  }

  return ( NULL );
}




TaskTimeEntry *taskNewTimeEntry ( task, year, month, day )
Task *task;
int year, month, day;
{
  struct tm *tm;
  time_t now;
  TaskTimeEntry *ret;

  if ( year < 100 ) {
    time ( &now );
    tm = localtime ( &now );
    year += 1900 + ( tm->tm_year % 100 );
  }

  ret = (TaskTimeEntry *) malloc ( sizeof ( TaskTimeEntry ) );

  ret->year = year;
  ret->mon = month;
  ret->mday = day;
  ret->seconds = 0;
  ret->marked_seconds = 0;

  if ( task->entries )
    task->entries = (TaskTimeEntry **) realloc ( task->entries,
      ( task->num_entries + 1 ) * ( sizeof ( TaskTimeEntry * ) ) );
  else
    task->entries = (TaskTimeEntry **) malloc ( ( task->num_entries + 1 )
      * ( sizeof ( TaskTimeEntry * ) ) );
  task->entries[task->num_entries] = ret;
  task->num_entries++;

  return ( ret );
}


/*
** Get the options for the specified task.
*/
unsigned int taskGetOptions ( task )
Task *task;
{
  return task->options;
}


/*
** Determine if the specified option is enabled.
*/
unsigned int taskOptionEnabled ( task, option )
Task *task;
unsigned int option;
{
  if ( option & task->options )
    return 1;
  else
    return 0;
}


void taskSetOption ( task, option )
Task *task;
unsigned int option;
{
  task->options |= option;
}

void taskUnsetOption ( task, option )
Task *task;
unsigned int option;
{
  if ( taskOptionEnabled ( task, option ) )
    task->options -= option;
}

/*
** Add a task annotation to a task.  Can be more than one line of
** text.  We will convert '\n' into '\r' before saving and then
** back when loading.
** Note: It gets saved immediately (not when taskSave is called)
*/
void taskAddAnnotation ( task, taskdir, text )
Task *task;
char *taskdir;
char *text;
{
  char *ptr, *path, *newtext;
  TaskAnnotation *a;
  FILE *fp;

  a = (TaskAnnotation *) malloc ( sizeof ( TaskAnnotation ) );
  memset ( a, '\0', sizeof ( TaskAnnotation ) );
  time ( &a->text_time );
  a->text = (char *) malloc ( strlen ( text ) + 1 );
  strcpy ( a->text, text );

  if ( task->annotations == NULL ) {
    task->annotations = (TaskAnnotation **) malloc (
      sizeof ( TaskAnnotation * ) );
  } else {
    task->annotations = (TaskAnnotation **) realloc ( task->annotations,
      ( task->num_annotations + 1 ) * sizeof ( TaskAnnotation * ) );
  }
  task->annotations[task->num_annotations] = a;
  task->num_annotations++;

  /* now save to file */
  path = (char *) malloc ( strlen ( taskdir ) + 10 );
  sprintf ( path, "%s/%d.ann", taskdir, task->number );
  fp = fopen ( path, "a+" );
  if ( fp ) {
    newtext = (char *) malloc ( strlen ( text ) + 1 );
    strcpy ( newtext, text );
    for ( ptr = newtext; *ptr != '\0'; ptr++ )
      if ( *ptr == '\n' )
        *ptr = '\r';
    fprintf ( fp, "%d %s\n", (int)a->text_time, newtext );
    free ( newtext );
    fclose ( fp );
  }
  free ( path );
}



/*
** Get all annotations for the specified task on the specified day.
** NOTE: Caller must free return value.
** ??? Maybe we should take the midnight offset into consideration here ???
*/
TaskAnnotation **TaskGetAnnotationEntries ( task, year, month, day,
  time_offset, num_ret )
Task *task;
int year, month, day;
int time_offset;
int *num_ret;
{
  TaskAnnotation **ret = NULL;
  int loop = 0;
  struct tm *tm;
  int num = 0;
  time_t then;

  for ( loop = 0; loop < task->num_annotations; loop++ ) {
    then = task->annotations[loop]->text_time - time_offset;
    tm = localtime ( &then );
    if ( tm->tm_year + 1900 == year &&
      tm->tm_mon + 1 == month &&
      tm->tm_mday == day ) {
      if ( num ) {
        ret = (TaskAnnotation **) realloc ( ret,
          ( num + 1 ) * sizeof ( TaskAnnotation * ) );
      } else {
        ret = (TaskAnnotation **) malloc ( sizeof ( TaskAnnotation * ) );
      }
      ret[num] = task->annotations[loop];
      num++;
    }
  }

  *num_ret = num;
  return ( ret );
}



