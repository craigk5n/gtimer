/*
 * Copyright (C) 1999 Craig Knudsen
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * Description:
 *	Helps you keep track of time spent on different tasks.
 *
 * Author:
 *	Craig Knudsen, cknudsen@cknudsen.com, http://www.cknudsen.com/
 *
 * Home Page:
 *	http://www.cknudsen.com/gtimer/
 *
 * History:
 *	04-Apr-98	Created
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#if HAVE_STRING_H
#include <string.h>
#endif

#ifdef GTIMER_MEMDEBUG
#include "memdebug/memdebug.h"
#endif

#define CONFIG_DEFAULTS
#include "config.h"

#define MAX_ATTR	256
#define MAX_LEN		1024

/*
** Local variables.
*/
static char *attrnames[MAX_ATTR];	/* attribute names */
static char *attrvalues[MAX_ATTR];	/* attribute values */
static int num_attr = 0;		/* no. of attributes in above arrays */
static int modified = 0;		/* modified since read from file */

static char *my_strtok (
#ifndef _NO_PROTO
  char *ptr1, char *tok
#endif
);




/*
** Read all the attributes in the specified file.
*/
int configReadAttributes ( path )
char *path;
{
  int loop, old;
  char *text = NULL, *ptr;
  struct stat buf;
  int fd;

  modified = 0;

  for ( loop = 0; loop < num_attr; loop++ ) {
    free ( attrnames[loop] );
    free ( attrvalues[loop] );
  }
  num_attr = 0;

  for ( loop = 0; default_config[loop]; loop += 2 ) {
    attrnames[num_attr] = (char *) malloc
      ( strlen ( default_config[loop] ) + 1 );
    strcpy ( attrnames[num_attr], default_config[loop] );
    attrvalues[num_attr] = (char *) malloc
      ( strlen ( default_config[loop+1] ) + 1 );
    strcpy ( attrvalues[num_attr], default_config[loop+1] );
    num_attr++;
  }

  if ( stat ( path, &buf ) != 0 )
    return ( -1 );
  fd = open ( path, O_RDONLY );
  if ( fd >= 0 ) {
    text = (char *) malloc ( buf.st_size + 1 );
    read ( fd, text, buf.st_size );
    text[buf.st_size] = '\0';
    close ( fd );
    ptr = my_strtok ( text, "\n" );
    while ( ptr ) {
      for ( old = -1, loop = 0; loop < num_attr; loop++ ) {
        if ( strcmp ( attrnames[loop], ptr ) == 0 ) {
          old = loop;
          break;
        }
      }
      if ( old >= 0 ) {
        ptr = my_strtok ( NULL, "\n" );
        if ( ! ptr )
          break;
        free ( attrvalues[old] );
        attrvalues[old] = (char *) malloc ( strlen ( ptr ) + 1 );
        strcpy ( attrvalues[old], ptr );
      }
      else {
        attrnames[num_attr] = (char *) malloc ( strlen ( ptr ) + 1 );
        strcpy ( attrnames[num_attr], ptr );
        ptr = my_strtok ( NULL, "\n" );
        if ( ! ptr )
          break;
        attrvalues[num_attr] = (char *) malloc ( strlen ( ptr ) + 1 );
        strcpy ( attrvalues[num_attr], ptr );
        num_attr++;
      }
      ptr = my_strtok ( NULL, "\n" );
    }
  }
  else {
    return ( -1 );
  }

  if ( text )
    free ( text );
  return ( 0 );
  
}




/*
** Get the value for a specified attribute.
*/
int configGetAttribute ( attribute, value )
char *attribute;
char **value;
{
  int loop;

  for ( loop = 0; loop < num_attr; loop++ ) {
    if ( strcmp ( attrnames[loop], attribute ) == 0 ) {
      *value = attrvalues[loop];
      return ( 0 );
    }
  }

  return ( -1 );

}


/*
** Get a value in int form.
*/
int configGetAttributeInt ( attribute, value )
char *attribute;
int *value;
{
  char *ptr;
  int ret;

  ret = configGetAttribute ( attribute, &ptr );
  if ( ret == 0 ) {
    *value = atoi ( ptr );
    return 0;
  } else
    return -1;
}


/*
** Set the value for a specified attribute.
*/
int configSetAttribute ( attribute, value )
char *attribute;
char *value;
{
  int loop;

  modified = 1;

  for ( loop = 0; loop < num_attr; loop++ ) {
    if ( strcmp ( attrnames[loop], attribute ) == 0 ) {
      free ( attrvalues[loop] );
      attrvalues[loop] = (char *) malloc ( strlen ( value ) + 1 );
      strcpy ( attrvalues[loop], value );
      return ( 0 );
    }
  }

  /*
  ** Attribute does not exits.  Add it.
  */
  attrnames[num_attr] = (char *) malloc ( strlen ( attribute ) + 1 );
  strcpy ( attrnames[num_attr], attribute );
  attrvalues[num_attr] = (char *) malloc ( strlen ( value ) + 1 );
  strcpy ( attrvalues[num_attr], value );
  num_attr++;

  return ( 0 );

}


/*
** Set the value for a specified attribute.
*/
int configSetAttributeInt ( attribute, value )
char *attribute;
int value;
{
  char temp[50];
  sprintf ( temp, "%d", value );
  return ( configSetAttribute ( attribute, temp ) );
}




/*
** Save the attributes in memory to the specified file.
*/
int configSaveAttributes ( attrfile )
char *attrfile;
{
  FILE *fp;
  int loop;

  fp = fopen ( attrfile, "w" );
  if ( ! fp ) {
    return ( -1 );
  }

  for ( loop = 0; loop < num_attr; loop++ ) {
    fprintf ( fp, "%s\n%s\n", attrnames[loop], attrvalues[loop] );
  }
  fclose ( fp );

  modified = 0;

  return ( 0 );
}



/*
** Tell the caller if changes have been made since last read/save.
*/
int configModified ()
{
  return ( modified );
}



/*
** Clear out all values.
*/
void configClear ()
{
  int loop;

  for ( loop = 0; loop < num_attr; loop++ ) {
    free ( attrnames[loop] );
    free ( attrvalues[loop] );
  }
  num_attr = 0;
  modified = 1;
}


static char *my_strtok ( ptr1, tok )
char *ptr1;
char *tok;
{
  static char *last;
  char *p;

  if ( ! ptr1 )
    ptr1 = last;
  else
    last = ptr1;

  if ( ! ptr1 )
    return ( NULL );

  for ( p = ptr1; *p != '\0'; p++ ) {
    if ( strncmp ( p, tok, strlen ( tok ) ) == 0 ) {
      *p = '\0';
      last = p + strlen ( tok );
      return ( ptr1 );
    }
  }

  last = NULL;
  return ( ptr1 );
}

