#!/usr/bin/perl
#
# Export GTimer data to Karm format.
#
# The Karm file is typically:
#   ~/.kde/share/apps/karm/karmdata.txt
#
# History:
#	13-Mar-2003	cknudsen@cknudsen.com
#			Created
#
#############################################################################


$gtimer_dir = $ENV{'HOME'} . "/.gtimer";
$verbose = 0;
$output_file = $ENV{'HOME'} . "/.kde/share/apps/karm/karmdata.txt";

for ( $i = 0; $i < @ARGV; $i++ ) {
  if ( $ARGV[$i] eq "-dir" ) {
    if ( -d $ARGV[++$i] ) {
      $gtimer_dir = $ARGV[$i];
    } else {
      die "Must supply valid directory with -dir option\n";
    }
  } elsif ( $ARGV[$i] eq "-o" || $ARGV[$i] eq "-output" ) {
    $output_file = $ARGV[++$i];
  } elsif ( $ARGV[$i] eq "-verbose" || $ARGV[$i] eq "-v" ) {
    $verbose++;
  }
}

# Read in all GTimer data
print "Reading GTimer data file...\n" if ( $verbose );
opendir ( DIR, $gtimer_dir ) || die "Error opening $gtimer_dir";
@files = readdir ( DIR );
closedir ( DIR );
@taskfiles = grep ( /^\d+\.task$/, @files );
@projectfiles = grep ( /^\d+\.project$/, @files );

$NO_PROJECT = '999999';

%project_names = ();
foreach $f ( @projectfiles ) {
  print "Reading $gtimer_dir/$f\n" if ( $verbose );
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
printf "Done reading projects.\n" if ( $verbose );

%tasks = ();
foreach $f ( @taskfiles ) {
  print "Reading $gtimer_dir/$f\n" if ( $verbose );
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
        $date = $1; # ignore since karm doesn't use dates
        $seconds = $2;
        $total_time += $seconds;
      }
    }
    close ( F );
    $task_names{$taskId} = $name;
    $task_totals{$taskId} = $total_time;
    $task_projects{$taskId} = $projectId;
    $key = sprintf ( "%06d", $projectId );
    if ( defined ( $project_tasks{$key} ) ) {
      $project_tasks{$key} .= "," . $taskId;
    } else {
      $project_tasks{$key} = $taskId;
    }
  }
}
printf "Done reading tasks.\n" if ( $verbose );

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
  printf ( "Moving existing file: %s -> %s\n",
    $output_file, $backup ) if ( $verbose );
  system ( "mv $output_file $backup" );
}

# Write output file
print "Writing output file: $output_file\n" if ( $verbose );
open ( F, ">$output_file" ) ||
  die "Error writing $output_file";
print F "# Karm save data\n";
foreach $projectId ( sort keys ( %project_tasks ) ) {
  if ( ! defined ( $project_names{$projectId} ) ) {
    print STDERR "Data error: could not find project name for $projectId\n";
    $project_names{$projectId} = "Unnamed Project";
  }
  @tasks = split ( /,/, $project_tasks{$projectId} );
  $task_out = "";
  $time_total = 0;
  foreach $t ( @tasks ) {
    $time_total += $task_totals{$t};
    $minutes = $task_totals{$t} / 60;
    $task_out .= sprintf "2\t%d\t%s\n", $minutes, $task_names{$t};
  }
  $minutes = $time_total / 60;
  printf F "1\t%d\t%s\n", $minutes, $project_names{$projectId};
  printf "Writing project: %s\n", $project_names{$projectId} if ( $verbose );
  print F $task_out;
}
close ( F );


exit ( 0 );
