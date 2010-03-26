#!/usr/local/bin/perl
#
# Perl-based report generator for GTimer.
# Author: Craig Knudsen <cknudsen@radix.net>
#
# Comments:
#	The data files should always be up-to-date since the application
#	automatically saves data every 5 minutes (unless the app thinks
#	you're idle).
#
# Usage:
#	gtimer_report.pl
#	  [-task XX]
#	  [-today|-thisweek|-lastweek|-thismonth|-lastmonth|-thisyear]
#	  [-daily|-weekly|-monthly]
#	options:
#	  -today	include data for today
#	  -thisweek	include data for this week
#	  -lastweek	include data for last week
#	  -thismonth	include data for this month
#	  -thisyear	include data for this year
#	  -lastmonth	include data for last month
#	  -daily	generate a report by day (default)
#	  -weekly	generate a report by week
#	  -monthly	generate a report by month
#	  -task name	include the specified task.  the task name must
#			match exactly except for case.  the default is
#			to include all tasks if this option is not
#			specified.  this option can be used more than once
#			on the command line.
#	  -dir dirname	specifiy an alternate GTimer data directory
#	  -annotations	include annotations (will disable hours unless
#			the -hours option is also specified)
#	  -hours	include hours (default)
#
# Examples:
#	To generate a weekly status report for last week:
#	  gtimer_report.pl -annotations -weekly -lastweek
#	To generate a report of hours worked for each day this month:
#	  gtimer_report.pl -thismonth
#	To generate a report of all hours spent this year on task "GTimer"
#	on both a daily and monthly basis:
#	  gtimer_report.pl -thisyear -daily -monthly -task "GTimer"
#
# Not yet implemented:
#	HTML output
#	Yearly reports
#
# History:
#	08-Dec-1999	Fixed bug that was not including last task in
#			all reports.  (Thanks Richard Kilgore)
#	05-May-1999	Created
#
############################################################################

$RANGE_TODAY = 1;
$RANGE_THIS_WEEK = 2;
$RANGE_LAST_WEEK = 3;
$RANGE_THIS_MONTH = 4;
$RANGE_LAST_MONTH = 5;
$RANGE_THIS_YEAR = 6;
$RANGE_LAST_YEAR = 7;
@weekdays = ( "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" );
@month_days = ( 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 );
@lmonth_days = ( 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 );

$data_dir = "$ENV{HOME}/.gtimer";
$all_tasks = 1;
$annotations = 0;
$hours = 1;
$range = $RANGE_THIS_WEEK;
$midnight_offset = 0;


#
# Calculate a weekday from a given date
#
sub calc_weekday {
  my ( $m, $d, $y ) = @_; # month is 1-12
  my ( $wday, @d, @w );
  @w = ( "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" );

  $y += 1900 if ( $y < 1900 );
  @d = (0,3,2,5,0,3,5,1,4,6,2,4);
  $y-- if $m < 3;
  $wday = ($y+int($y/4)-int($y/100)+int($y/400)+$d[$m-1]+$d) % 7;

  $w[$wday];
}


sub load_tasks {
  my ( $line, $i, $name, $date, $time, $day, $mon, $year );

  $i = 0;
  while ( -f "$data_dir/$i.task" ) {
    undef ( $name );
    # read time data
    open ( TASK, "$data_dir/$i.task" ) ||
      die "Error opening $data_dir/$i.task: $!";
    while ( <TASK> ) {
      chop;
      if ( /Name:\s*/ ) {
        $name = $';
        if ( defined ( %named_tasks ) && !
          defined ( $named_tasks{"\U$name"} ) ) {
          undef ( $name );
          last;
        }
      } elsif ( /^(\d\d\d\d\d\d\d\d) (\d+)$/ ) {
        $date = $1;
        $time = $2;
        $hours{$i . ":" . $date} = $time;
      }
    }
    close ( TASK );
    if ( defined ( $name ) ) {
      $tasks[$i] = $name;
      $task_names{$name} = $i;
    }
    # read annotations
    if ( defined ( $name ) && $annotations && -f "$data_dir/$i.ann" ) {
      open ( ANN, "$data_dir/$i.ann" ||
        die "Error opening $data_dir/$i.ann: $!" );
      while ( <ANN> ) {
        chop;
        ( $date, $line ) = split ( / /, $_, 2 );
        $date -= $midnight_offset;
        ( $day, $mon, $year ) = ( localtime ( $date ) )[3,4,5];
        $date = sprintf "%04d%02d%02d", 1900 + $year, $mon + 1, $day;
        $annotations{$i . ":" . $date} = $line;
      }
      close ( ANN );
    }
    $last_task = $i;
    $i++;
  }
}



# handle options
for ( $i = 0; $i < @ARGV; $i++ ) {
  if ( $ARGV[$i] eq "-daily" || $ARGV[$i] eq "-d" ) {
    $daily = 2;
  } elsif ( $ARGV[$i] eq "-weekly" || $ARGV[$i] eq "-w" ) {
    $weekly = 1;
    $daily = 0 if ( $daily == 1 );
  } elsif ( $ARGV[$i] eq "-monthly" || $ARGV[$i] eq "-m" ) {
    $monthly = 1;
    $daily = 0 if ( $daily == 1 );
  } elsif ( $ARGV[$i] eq "-yearly" || $ARGV[$i] eq "-y" ) {
    $yearly = 1;
    $daily = 0 if ( $daily == 1 );
  } elsif ( $ARGV[$i] eq "-today" ) {
    $range = $RANGE_TODAY;
  } elsif ( $ARGV[$i] eq "-thisweek" ) {
    $range = $RANGE_THIS_WEEK;
  } elsif ( $ARGV[$i] eq "-lastweek" ) {
    $range = $RANGE_LAST_WEEK;
  } elsif ( $ARGV[$i] eq "-thismonth" ) {
    $range = $RANGE_THIS_MONTH;
  } elsif ( $ARGV[$i] eq "-lastmonth" ) {
    $range = $RANGE_LAST_MONTH;
  } elsif ( $ARGV[$i] eq "-thisyear" ) {
    $range = $RANGE_THIS_YEAR;
  } elsif ( $ARGV[$i] eq "-lastyear" ) {
    $range = $RANGE_LAST_YEAR;
  } elsif ( $ARGV[$i] eq "-hours" ) {
    $hours = 2;
  } elsif ( $ARGV[$i] eq "-annotations" || $ARGV[$i] eq "-a" ) {
    $annotations = 1;
    $hours = 0 if ( $hours == 1 );
  } elsif ( $ARGV[$i] eq "-dir" || $ARGV[$i] eq "-d" ) {
    $data_dir = $ARGV[++$i];
    die "No such data directory \"$data_dir\"\n" if ( ! -d $data_dir );
  } elsif ( $ARGV[$i] eq "-task" || $ARGV[$i] eq "-t" ) {
    $t = "\U$ARGV[++$i]";
    $named_tasks{$t} = 1;
  }
}


# get midnight offset
if ( open ( F, "$data_dir/.gtimerrc" ) ) {
  while ( <F> ) {
    if ( /^midnight-offset/ ) {
      chop ( $midnight_offset = <F> );
      last;
    }
  }
  close ( F );
}

  
&load_tasks;


# generate report
if ( $range == $RANGE_THIS_WEEK ) {
  ( $wday ) = ( localtime ( time ) )[6];
  $sunday = time - ( 3600 * 24 * $wday );
  $startdate = $sunday;
  $enddate = time;
} elsif ( $range == $RANGE_LAST_WEEK ) {
  ( $wday ) = ( localtime ( time ) )[6];
  $sunday = time - ( 3600 * 24 * ( $wday + 7 ) );
  $startdate = $sunday;
  $enddate = $sunday + ( 3600 * 24 * 6 );
} elsif ( $range == $RANGE_THIS_MONTH ) {
  ( $day ) = ( localtime ( time ) )[3];
  $startdate = time - ( 3600 * 24 * ( $day - 1 ) );
  $enddate = time;
} elsif ( $range == $RANGE_LAST_MONTH ) {
  ( $day, $mon, $year ) = ( localtime ( time ) )[3,4,5];
  $startdate = time - ( 3600 * 24 * ( $day - 1 ) );
  $mon--;
  if ( $mon == -1 ) {
    $mon = 11;
    $year--;
  }
  if ( $year % 4 == 0 ) {
    $days_last_month = $month_days[$mon];
  } else {
    $days_last_month = $lmonth_days[$mon];
  }
  $enddate = $startdate - ( 3600 * 24 );
  $startdate -= ( $days_last_month * 24 * 3600 );
} elsif ( $range == $RANGE_THIS_YEAR ) {
  ( $yday ) = ( localtime ( time ) )[7];
  $startdate = time - ( 3600 * 24 * ( $yday - 1 ) );
  $enddate = time;
} elsif ( $range == $RANGE_LAST_YEAR ) {
  ( $yday ) = ( localtime ( time ) )[7];
  $enddate = time - ( 3600 * 24 * ( $yday + 1 ) );
  ( $yday ) = ( localtime ( $enddate ) )[7];
  $startdate = $enddate - ( 3600 * 24 * ( $yday ) );
}

# -- debug stuff --
( $day, $mon, $year, $wday ) = ( localtime ( $startdate ) )[3,4,5,6];
printf "Date Range: %d/%d/%d %s to ", $mon + 1, $day, $year + 1900,
  $weekdays[$wday];
( $day, $mon, $year, $wday ) = ( localtime ( $enddate ) )[3,4,5,6];
printf "%d/%d/%d %s\n", $mon + 1, $day, $year + 1900,
  $weekdays[$wday];

for ( $date = $startdate; $date <= $enddate; $date += ( 24 * 3600 ) ) {
  ( $day, $mon, $year, $wday ) = ( localtime ( $date ) )[3,4,5,6];
  $datestr = sprintf "%04d%02d%02d", $year + 1900, $mon + 1, $day;
  $day = sprintf "\n%02d/%02d/%04d %s\n", $mon + 1, $day, $year + 1900,
    $weekdays[$wday];
  $daily_total = 0;
  $first_day_of_week = $date if ( ! defined ( $first_day_of_week ) );
  $last_day_of_week = $date;
  $first_day_of_month = $date if ( ! defined ( $first_day_of_month ) );
  $last_day_of_month = $date;
  for ( $i = 0; $i <= $last_task; $i++ ) {
    if ( defined ( $hours{$i . ":" . $datestr} ) ||
      defined ( $annotations{$i . ":" . $datestr} ) ) {
      if ( $hours && defined ( $hours{$i . ":" . $datestr} ) ) {
        $time = $hours{$i . ":" . $datestr};
        $hrs = $time / 3600;
        $mins = ( $time % 3600 ) / 60;
        $secs = $time % 60;
        $daily_out .= sprintf "%3d:%02d:%02d - %s\n",
          $hrs, $mins, $secs, $tasks[$i];
      }
      if ( defined ( $annotations{$i . ":" . $datestr} ) ) {
        if ( ! $hours ) {
          $daily_out .= "$tasks[$i]\n";
        }
        @lines = split ( /\r/, $annotations{$i . ":" . $datestr} );
        foreach $l ( @lines ) {
          if ( $hours ) {
            $daily_out .= sprintf "            %s\n", $l;
            $weekly_annotations[$i] .= sprintf "            %s\n", $l;
            $monthly_annotations[$i] .= sprintf "            %s\n", $l;
          } else {
            $daily_out .= sprintf "  %s\n", $l;
            $weekly_annotations[$i] .= sprintf "  %s\n", $l;
            $monthly_annotations[$i] .= sprintf "  %s\n", $l;
          }
        }
      }
      $daily_total += $time;
      $weekly_hours[$i] += $time;
      $monthly_hours[$i] += $time;
    }
  }
  if ( $daily_out ne "" && $daily ) {
    print "$day\n-------------------------------------\n";
    print $daily_out;
    if ( $hours ) {
      print "---------\n";
      $hrs = $daily_total / 3600;
      $mins = ( $daily_total % 3600 ) / 60;
      $secs = $daily_total % 60;
      printf "%3d:%02d:%02d - %s\n",
        $hrs, $mins, $secs, "Total";
    }
    $daily_out = "";
  }
  if ( defined ( @weekly_hours ) && $weekly && 
    ( $wday == 6 || $date == $enddate ) ) {
    ( $day, $mon, $year, $wday ) =
      ( localtime ( $first_day_of_week ) )[3,4,5,6];
    printf "\nWeek %02d/%02d/%04d %s to ",
      $mon + 1, $day, $year + 1900, $weekdays[$wday];
    ( $day, $mon, $year, $wday ) =
      ( localtime ( $last_day_of_week ) )[3,4,5,6];
    printf "%02d/%02d/%04d %s\n",
      $mon + 1, $day, $year + 1900, $weekdays[$wday];
    print "-------------------------------------\n";
    $total = 0;
    for ( $i = 0; $i <= $last_task; $i++ ) {
      if ( $weekly_hours[$i] != 0 || $weekly_annotations[$i] ne "" ) {
        if ( $hours ) {
          $total += $weekly_hours[$i];
          $hrs = $weekly_hours[$i] / 3600;
          $mins = ( $weekly_hours[$i] % 3600 ) / 60;
          $secs = $weekly_hours[$i] % 60;
          printf "%3d:%02d:%02d - %s\n",
            $hrs, $mins, $secs, $tasks[$i];
        } elsif ( $weekly_annotations[$i] ne "" ) {
          print "$tasks[$i]\n";
        }
        if ( $weekly_annotations[$i] ne "" ) {
          print $weekly_annotations[$i];
        }
      }
    }
    if ( $total && $hours ) {
      $hrs = $total / 3600;
      $mins = ( $total % 3600 ) / 60;
      $secs = $total % 60;
      print "---------\n";
      printf "%3d:%02d:%02d - %s\n", $hrs, $mins, $secs, "Total";
    }
    undef ( @weekly_hours );
    undef ( @weekly_annotations );
    undef ( $first_day_of_week );
  }
  ( $day, $mon, $year ) = ( localtime ( $date ) )[3,4,5];
  if ( $year % 4 == 0 ) {
    $ldom = $lmonth_days[$mon];
  } else {
    $ldom = $month_days[$mon];
  }
  if ( defined ( @monthly_hours ) && $monthly && 
    ( $day == $ldom || $date == $enddate ) ) {
    ( $day, $mon, $year, $wday ) =
      ( localtime ( $first_day_of_month ) )[3,4,5,6];
    printf "\nMonth %02d/%04d\n", $mon + 1, $year + 1900;
    print "-------------------------------------\n";
    $total = 0;
    for ( $i = 0; $i <= $last_task; $i++ ) {
      if ( $monthly_hours[$i] != 0 || $monthly_annotations[$i] ne "" ) {
        if ( $hours ) {
          $total += $monthly_hours[$i];
          $hrs = $monthly_hours[$i] / 3600;
          $mins = ( $monthly_hours[$i] % 3600 ) / 60;
          $secs = $monthly_hours[$i] % 60;
          printf "%3d:%02d:%02d - %s\n",
            $hrs, $mins, $secs, $tasks[$i];
        } elsif ( $monthly_annotations[$i] ne "" ) {
          print "$tasks[$i]\n";
        }
        if ( $monthly_annotations[$i] ne "" ) {
          print $monthly_annotations[$i];
        }
      }
    }
    if ( $total && $hours ) {
      $hrs = $total / 3600;
      $mins = ( $total % 3600 ) / 60;
      $secs = $total % 60;
      print "---------\n";
      printf "%3d:%02d:%02d - %s\n", $hrs, $mins, $secs, "Total";
    }
    undef ( @monthly_hours );
    undef ( @monthly_annotations );
    undef ( $first_day_of_month );
  }
}


# bye.
exit 0;

