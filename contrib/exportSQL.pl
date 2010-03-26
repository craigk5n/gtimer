#!/usr/bin/perl
#
# Export GTimer data to SQL.
#
# Usage:
#	To generate the SQL for creating the tables:
#	./exportSQL.pl -create
#
#	To create SQL to add all data
#	./exportSQL.pl
#
#	To create SQL to add all data and replace existing data
#	in the database.
#	./exportSQL.pl
#
# It is assumed you have the following table structure.
# project table with fields:
#   - project id (INT)
#   - project name (VARCHAR)
# task table with fields:
#   - project id (INT)
#   - task id (INT)
#   - task (VARCHAR)
# task_time with fields:
#   - task id (INT)
#   - date in YYYYMMDD format (INT)
#   - seconds (INT)
# task_note with fields:
#   - task id
#   - date in YYYYMMDD format (INT)
#   - time in HHMMSS format (INT)
#   - text of note (VARCHAR)
#
# GTimer does not put limits on the length of the text strings, so
# make the VARCHAR sizes big enough.
#
# History:
#	14-Mar-2003	cknudsen@cknudsen.com
#			Created
#
#############################################################################

# definition of project table
@project_table = (
  "PROJECTS", # Table name
  "PROJECT_ID", # fields...
  "PROJECT_NAME", 
);
# definition of task table
@task_table = (
  "TASKS", # Table name
  "PROJECT_ID", # fields...
  "TASK_ID", 
  "TASK_NAME", 
);
# definition of task time table
@task_time_table = (
  "TASK_TIME", # Table name
  "TASK_ID", # fields...
  "TASK_DATE", 
  "NUM_SECONDS", 
);
# definition of task notes table
@task_notes_table = (
  "TASK_NOTES", # Table name
  "TASK_ID", # fields...
  "NOTE_DATE", 
  "NOTE_TIME", 
  "NOTE_TEXT", 
);

$gtimer_dir = $ENV{'HOME'} . "/.gtimer";

for ( $i = 0; $i < @ARGV; $i++ ) {
  if ( $ARGV[$i] eq "-dir" ) {
    if ( -d $ARGV[++$i] ) {
      $gtimer_dir = $ARGV[$i];
    } else {
      die "Must supply valid directory with -dir option\n";
    }
  } elsif ( $ARGV[$i] eq "-o" || $ARGV[$i] eq "-output" ) {
    $output_file = $ARGV[++$i];
  } elsif ( $ARGV[$i] eq "-create" ) {
    $do_create++;
  }
}

# Just remove all single quotes since that is the safest/easiest
sub sqlQuote {
  my ( $instr ) = @_;

  $instr =~ s/'//g;
  return ( "'" . $instr . "'"  );
}

if ( defined ( $output_file ) ) {
  open ( F, ">$output_file" ) || die "Error writing $output_file";
  select ( F );
}

if ( defined ( $do_create ) ) {
print<<EOF;
CREATE TABLE $project_table[0] (
  $project_table[1] INT,
  $project_table[2] VARCHAR(128),
  PRIMARY KEY ( $project_table[1] )
);

CREATE TABLE $task_table[0] (
  $task_table[1] INT,
  $task_table[2] INT,
  $task_table[3] VARCHAR(128),
  PRIMARY KEY ( $task_table[2] )
);

CREATE TABLE $task_time_table[0] (
  $task_time_table[1] INT,
  $task_time_table[2] INT,
  $task_time_table[3] INT,
  PRIMARY KEY ( $task_time_table[1], $task_time_table[2] )
);

CREATE TABLE $task_notes_table[0] (
  $task_notes_table[1] INT,
  $task_notes_table[2] INT,
  $task_notes_table[3] INT,
  $task_notes_table[4] VARCHAR(128),
  PRIMARY KEY ( $task_notes_table[1], $task_notes_table[2], $task_notes_table[3] )
);
EOF
  exit ( 0 );
}

# Read in all GTimer data
print STDOUT "Reading GTimer data file...\n" if ( $verbose );
opendir ( DIR, $gtimer_dir ) || die "Error opening $gtimer_dir";
@files = readdir ( DIR );
closedir ( DIR );
@taskfiles = grep ( /^\d+\.task$/, @files );
@projectfiles = grep ( /^\d+\.project$/, @files );
@notefiles = grep ( /^\d+\.ann$/, @files );

$NO_PROJECT = '999999';

%project_names = ();
foreach $f ( @projectfiles ) {
  print STDOUT "Reading $gtimer_dir/$f\n" if ( $verbose );
  open ( F, "$gtimer_dir/$f" ) || die "Error reading $gtimer_dir/$f";
  if ( $f =~ /^(\d+).project$/ ) {
    $projectId = $1;
    $name = "Unnamed Project $projectId";
    while ( <F> ) {
      chop;
      if ( /^Name:\s+/ ) {
        $name = $';
      }
    }
    close ( F );
    $key = sprintf ( "%06d", $projectId );
    $project_names{$key} = $name;
  }
}
$key = sprintf ( "%06d", $NO_PROJECT );
$project_names{$key} = "Misc.";
printf STDOUT "Done reading projects.\n" if ( $verbose );

%tasks = ();
foreach $f ( @taskfiles ) {
  print STDOUT "Reading $gtimer_dir/$f\n" if ( $verbose );
  open ( F, "$gtimer_dir/$f" ) || die "Error reading $gtimer_dir/$f";
  if ( $f =~ /^(\d+).task$/ ) {
    $taskId = $1;
    $name = "Unnamed Task $taskId";
    $total_time = 0;
    undef ( $projectId );
    while ( <F> ) {
      chop;
      if ( /^Name:\s+/ ) {
        $name = $';
      } elsif ( /^Project:\s+(\d+)/ ) {
        $projectId = $1;
      } elsif ( /^(\d+)\s(\d+)$/ ) {
        $timestamp = $1;
        $seconds = $2;
        ( $mday, $mon, $year ) = ( localtime ( $timestamp ) )[3,4,5];
        $date = sprintf ( "%04d%02d%02d", $year + 1900, $mon + 1, $mday );
        $entry = sprintf ( "%d %d %d", $taskId, $timestamp, $seconds );
        push ( @time_entries, $entry );
      }
    }
    close ( F );
    $task_names{$taskId} = $name;
    $task_totals{$taskId} = $total_time;
    $task_projects{$taskId} = $projectId;
    $key = sprintf ( "%06d", $projectId );
    $tasks{$taskId} = 1;
    if ( defined ( $project_tasks{$key} ) ) {
      $project_tasks{$key} .= "," . $taskId;
    } else {
      $project_tasks{$key} = $taskId;
    }
  }
}
printf STDOUT "Done reading tasks.\n" if ( $verbose );

@notes = ();
foreach $f ( @notefiles ) {
  print STDOUT "Reading $gtimer_dir/$f\n" if ( $verbose );
  open ( F, "$gtimer_dir/$f" ) || die "Error reading $gtimer_dir/$f";
  if ( $f =~ /^(\d+).ann$/ ) {
    $taskId = $1;
    while ( <F> ) {
      chop;
      if ( /^(\d+)\s+/ ) {
        $timestamp = $1;
        $note = $';
        ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
           localtime($timestamp);
        $date = sprintf ( "%04d%02d%02d", $year + 1900, $mon + 1, $mday );
        $time = sprintf ( "%02d%02d%02d", $hour, $min, $sec );
        $entry = sprintf ( "%d %d %d %s", $taskId, $date, $time, $note );
        push ( @notes, $entry );
      }
    }
    close ( F );
  }
}
$key = sprintf ( "%06d", $NO_PROJECT );

# First, save a copy of the existing karm file to a backup
( $mday, $mon, $year ) = ( localtime(time) )[3,4,5];
if ( -f $output_file ) {
  $backup = sprintf ( "%s.%02d-%02d-%04d", $output_file,
    $mon + 1, $mday, $year + 1900 );
  if ( -f $backup ) {
    for ( $i = 1; -f "$backup.$i"; $i++ ) { }
    $backup = $backup . "." . $i;
  }
  printf "Backup: %s\n", $backup;
  printf STDOUT ( "Moving existing file: %s -> %s\n",
    $output_file, $backup ) if ( $verbose );
  system ( "mv $output_file $backup" );
}

# Write output file
print STDOUT "Writing output file: $output_file\n" if ( $verbose && defined ( $output_file ) );
if ( defined ( $output_file ) ) {
  open ( F, ">$output_file" ) ||
    die "Error writing $output_file";
  select ( F );
}
foreach $projectId ( sort keys ( %project_tasks ) ) {
  if ( ! defined ( $project_names{$projectId} ) ) {
    print STDERR "Data error: could not find project name for $projectId\n";
    $project_names{$projectId} = "Unnamed Project";
  }
  print "INSERT INTO $project_table[0] " .
        "( $project_table[1], $project_table[2] ) VALUES " .
        "( " . int($projectId) . ", " .
        &sqlQuote ( $project_names{$projectId} ) . " );\n\n";
}

foreach $taskId ( sort keys ( %tasks ) ) {
  if ( ! defined ( $task_names{$taskId} ) ) {
    print STDERR "Data error: could not find task name for $taskId\n";
    $task_names{$taskId} = "Unnamed Task";
  }
  print "INSERT INTO $task_table[0] " .
        "( $task_table[1], $task_table[2], $task_table[3] ) VALUES " .
        "( $task_projects{$taskId}, " .
        int($taskId) . ", " .
        &sqlQuote ( $task_names{$taskId} ) . " );\n\n";
}

foreach $entry ( sort ( @time_entries ) ) {
  ( $id, $date, $secs ) = split ( / /, $entry );
  print "INSERT INTO $task_time_table[0] " .
        "( $task_time_table[1], $task_time_table[2], $task_time_table[3] ) VALUES " .
        "( $id, $date, $secs );\n\n";
}

foreach $entry ( sort ( @notes ) ) {
  ( $id, $date, $time, $note ) = split ( / /, $entry, 4 );
  print "INSERT INTO $task_notes_table[0] " .
        "( $task_notes_table[1], $task_notes_table[2], $task_notes_table[3], $task_notes_table[4] ) VALUES " .
        "( $id, $date, $time, " . &sqlQuote ( $note ) . " );\n\n";
}

if ( defined ( $output_file ) ) {
  close ( F );
}

exit ( 0 );
