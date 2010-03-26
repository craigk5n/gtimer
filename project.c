/*
 * Routines for dealing with projects
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
 * History:
 *	18-Apr-2005	Fix memory clobber when saving projects.  Based on
 *			debugging work by Ove Kaaven.
 *	20-Feb-2003	Created
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

#include "project.h"

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

// PV: Internationalization
#include "gtimeri18n.h"

static int num_projects = 0;
static Project **projects = NULL;
static int max_project = -1;
static int last_number = -1;


#ifdef WIN32
static int valid_name ( filename )
char *filename;
{
  while ( *filename && isdigit ( *filename ) )
    filename++;
  return ( ! strcmp ( filename, ".project" ) );
}
#endif


/*
** Add a project.
*/
void projectAdd ( project )
Project *project;
{
  int loop;
  int new_max_project = 0;

  new_max_project = max_project;

  if ( project->number == -1 ) {
    for ( loop = 0; loop < max_project && project->number < 0; loop++ ) {
      if ( projects[loop] == NULL )
        project->number = loop;
    }
    if ( project->number < 0 )
      project->number = ++max_project;
    new_max_project = max_project;
  } else if ( project->number > max_project ) {
    new_max_project = project->number;
  }

  if ( projects == NULL ) {
    projects = (Project **) malloc ( sizeof ( Project * ) * ( new_max_project + 1 ) );
    for ( loop = 0; loop < new_max_project; loop++ )
      projects[loop] = NULL;
  } else
    projects = (Project **) realloc ( projects,
      sizeof ( Project * ) * ( new_max_project + 1 ) );

  for ( loop = max_project + 1; loop <= new_max_project; loop++ )
    projects[loop] = NULL;

  max_project = new_max_project;

  projects[project->number] = project;
  num_projects++;
}



/*
** Create a new project
*/
Project *projectCreate ( name )
char *name;
{
  Project *project;

  project = (Project *) malloc ( sizeof ( Project ) );
  memset ( project, '\0', sizeof ( Project ) );
  project->name = (char *) malloc ( strlen ( name ) + 1 );
  strcpy ( project->name, name );
  time ( &project->created );
  project->number = -1; /* not yet assigned */

  return ( project );
}


/*
** Delete a project.
** Must have no tasks associated with it.
*/
int projectDelete ( project, projectdir )
Project *project;
char *projectdir;
{
  char *path;

  /* TODO: make sure project has no tasks */

  path = (char *) malloc ( strlen ( projectdir ) + 10 );
  sprintf ( path, "%s/%d.project", projectdir, project->number );
  unlink ( path );
  sprintf ( path, "%s/%d.ann", projectdir, project->number );
  unlink ( path );
  free ( path );

  projects[project->number] = NULL;
  num_projects--;
  projectFree ( project );

  return ( 0 );
}



/*
** Return the number of projects currently loaded.
*/
int projectCount () {
  return ( num_projects );
}



/*
** Get a project by number.
*/
Project *projectGet ( number )
int number;
{
  return ( projects[number] );
}


/*
** Get first project.
*/
Project *projectGetFirst () {
  last_number = -1;
  return ( projectGetNext() );
}


/*
** Get next project.
*/
Project *projectGetNext () {
  int loop;

  if ( ! num_projects )
    return ( NULL );

  for ( loop = last_number + 1; loop <= max_project; loop++ ) {
    if ( projects[loop] ) {
      last_number = loop;
      return ( projects[loop] );
    }
  }
 
  return ( NULL );
}



/*
** Clear all projects out of memory.
*/
void projectClearAll ()
{
  int loop;

  for ( loop = 0; loop <= max_project; loop++ ) {
    if ( projects[loop] ) {
      projectFree ( projects[loop] );
      projects[loop] = 0;
    }
  }
  num_projects = 0;
  last_number = max_project = -1;
}


/*
** Save a project to it's project file.
*/
int projectSave ( project, projectdir )
Project *project;
char *projectdir;
{
  char *path;
  FILE *fp;
  int loop;

  path = (char *) malloc ( strlen ( projectdir ) + 20 );
  sprintf ( path, "%s/%d.project", projectdir, project->number );

  fp = fopen ( path, "w" );
  if ( !fp ) {
    free ( path );
    return ( PROJECT_ERROR_SYSTEM_ERROR );
  }

  fprintf ( fp, "Format: 1.2\n" );
  fprintf ( fp, "Name: %s\n", project->name );
  fprintf ( fp, "Created: %u\n", (unsigned int)project->created );
  fprintf ( fp, "Options: %u\n", project->options );

  fclose ( fp );
  free ( path );

  return ( 0 );
}


/*
** Save all projects
*/
int projectSaveAll ( projectdir )
char *projectdir;
{
  int loop;
  int ret;

  for ( loop = 0; loop <= max_project; loop++ ) {
    if ( projects[loop] ) {
      ret = projectSave ( projects[loop], projectdir );
      if ( ret )
        return ( ret );
    }
  }
  return ( 0 );
}



/*
** Free all resources of a project.
*/
void projectFree ( project )
Project *project;
{
  free ( project->name );
  free ( project );
}



/*
** Load a project from file.
*/
int projectLoad ( path, project )
char *path;
Project **project;
{
  FILE *fp;
  int fd;
  Project *newproject;
  char line[512], *ptr, *ptr2, temp[10], *annfile, *anntext;
  int len, created, number, options;
  struct stat buf;

  fp = fopen ( path, "r" );
  if ( !fp )
    return ( PROJECT_ERROR_SYSTEM_ERROR );

  for ( ptr = path + strlen ( path ) - 1; *ptr != '/' && ptr != path; ptr-- );
  if ( *ptr == '/' )
    ptr++;
  sscanf ( ptr, "%d.project", &number );

  fgets ( line, 512, fp );
  len = strlen ( line );
  if ( line[len-1] == '\n' )
    line[len-1] = '\0';
  if ( strcmp ( line, "Format: 1.0" ) && strcmp ( line, "Format: 1.1" ) &&
    strcmp ( line, "Format: 1.2" ) ) {
    fclose ( fp );
    return ( PROJECT_ERROR_BAD_FILE );
  }

  newproject = (Project *) malloc ( sizeof ( Project ) );
  memset ( newproject, '\0', sizeof ( Project ) );
  newproject->number = number;

  while ( fgets ( line, 512, fp ) ) {
    len = strlen ( line );
    if ( line[len-1] == '\n' )
      line[len-1] = '\0';
    if ( strncmp ( line, "Name:", 5 ) == 0 ) {
      ptr = line + 5;
      if ( *ptr == ' ' )
        ptr++;
      newproject->name = (char *) malloc ( strlen ( ptr ) + 1 );
      strcpy ( newproject->name, ptr );
    } else if ( strncmp ( line, "Created:", 8 ) == 0 ) {
      sscanf ( line + 8, "%d", &created );
      newproject->created = (time_t) created;
    } else if ( strncmp ( line, "Options:", 8 ) == 0 ) {
      sscanf ( line + 8, "%d", &options );
      newproject->options = (unsigned int) options;
    } else {
      fclose ( fp );
      projectFree ( newproject );
      return ( PROJECT_ERROR_BAD_FILE );
    }
  }
  fclose ( fp );

  projectAdd ( newproject );

  *project = newproject;

  return ( 0 );
}



int projectLoadAll ( projectdir )
char *projectdir;
{
#ifdef WIN32
   char *pattern;
   Project *project;
   long handle;
   struct _finddata_t fdata;
   pattern = malloc ( strlen ( projectdir ) + 8 );
   (void) strcat ( strcpy ( pattern, projectdir ), "/*.project" );
   if ( ( handle = _findfirst ( pattern, &fdata ) ) != -1 ) {
     char *path = malloc ( strlen ( projectdir ) + _MAX_FNAME + 2 );
     char *start = path + strlen ( projectdir ) + 1;
     (void) strcat ( strcpy ( path, projectdir ), "/" );
     do {
       (void) strcpy ( start, fdata.name );
       if ( valid_name ( fdata.name ) && !_access ( path, 4 ) ){
         projectLoad ( path, &project );
       }
     } while ( !_findnext ( handle, &fdata ) );
     _findclose ( handle );
     free ( path );
   }
   else {
     free ( pattern );
     return ( PROJECT_ERROR_SYSTEM_ERROR );
   }
   free ( pattern );
#else
  DIR *dir;
  struct dirent *entry;
  struct stat buf;
  char *path, *ptr;
  Project *project;

  dir = opendir ( projectdir );
  if ( ! dir )
    return ( PROJECT_ERROR_SYSTEM_ERROR );
  while ( ( entry = readdir ( dir ) ) ) {
    path = (char *) malloc ( strlen ( projectdir ) + strlen ( entry->d_name )
      + 2 );
    sprintf ( path, "%s/%s", projectdir, entry->d_name );
    if ( stat ( path, &buf ) == 0 ) {
      for ( ptr = entry->d_name; isdigit ( *ptr ); ptr++ ) ;
      if ( strcmp ( ptr, ".project" ) == 0 && S_ISREG ( buf.st_mode ) ) {
        /* NOTE: add catching of errors here... */
        projectLoad ( path, &project );
      }
    }
    free ( path );
  }

#endif
  return ( 0 );
}


char *projectErrorString ( project_error )
int project_error;
{
  static char msg[256];

  switch ( project_error ) {
    case PROJECT_ERROR_SYSTEM_ERROR:
      sprintf ( msg, "%s (%d)", gettext("System Error"), errno );
      return ( msg );
    case PROJECT_ERROR_BAD_FILE:
      return ( gettext("Invalid project data file format") );
    default:
      sprintf ( msg, "%s(%d)", gettext("Unknown error"),
        project_error );
      return ( msg );
  }
}





/*
** Get the options for the specified project.
*/
unsigned int projectGetOptions ( project )
Project *project;
{
  return project->options;
}


/*
** Determine if the specified option is enabled.
*/
unsigned int projectOptionEnabled ( project, option )
Project *project;
unsigned int option;
{
  if ( option & project->options )
    return 1;
  else
    return 0;
}


void projectSetOption ( project, option )
Project *project;
unsigned int option;
{
  project->options |= option;
}

void projectUnsetOption ( project, option )
Project *project;
unsigned int option;
{
  if ( projectOptionEnabled ( project, option ) )
    project->options -= option;
}



#if 0
/*
** Get all annotations for the specified project on the specified day.
** NOTE: Caller must free return value.
** ??? Maybe we should take the midnight offset into consideration here ???
*/
Task **ProjectGetTasks ( project, num_ret )
Project *project;
int *num_ret;
{
  Task **ret = NULL;
  int loop = 0;
  struct tm *tm;
  int num = 0;
  time_t then;

  ret = (Task **) malloc ( sizeof ( Task * ) * project->num_tasks );
  memcpy ( ret, project->tasks, sizeof ( Task * ) * project->num_tasks );

  *num_ret = project->num_tasks;
  return ( ret );
}

#endif


