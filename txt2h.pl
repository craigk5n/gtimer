#!/usr/bin/perl
# Tool for converting a plain text file into a C string (char *) that
# can be used in a C program.
#
# This is a rewrite of this perl script.  i lost the original in the
# unfortunate hard disk crash of 2006 :-(
#
# Usage:
#	perl txt2h.pl < plaintext.txt > output
# History:
#	25 Mar 2010	Craig Knudsen <craig@k5n.us>
#			Created (again!)
#
############################################################################

$varname = 'changelog_text';

print 'static char *' . $varname . ' = "\\' . "\n";
while ( <> ) {
  chop ();
  s/\\/\\\\/g;
  s/"/\\"/g;
  print;
  print "\\n\\\n";
}
print "\";\n";

exit 0;

