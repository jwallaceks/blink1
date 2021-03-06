#!/usr/bin/env perl
#
# url-to-blink1.pl -- given a URL (and optionally a path to blink1-tool)
#                     fetch a hex color code (like "#CC00FF") and 
#                     turn blink(1) that color.
# Example:
#  ./url-to-blink1.pl http://todbot.com/tst/color.txt ../command-line/blink1-tool
#

use strict;
use warnings;

die "usage: url-to-blink1.pl <url> [blink1-tool-path]\n" if( @ARGV<1 );

my $url = $ARGV[0]; 
my $blink1toolpath = $ARGV[1] if( $ARGV[1] );

if( !defined($blink1toolpath) || ! -e $blink1toolpath ) {  # no user-specified path
    chomp( $blink1toolpath = `which blink1-tool` );        # try to find it
    if( $blink1toolpath =~ /not found/ ) {                 # if not found
        $blink1toolpath = './blink1-tool';                 # try path of last resort
    }
}
die "no blink1-tool '$blink1toolpath'\n" if( ! -e $blink1toolpath );

print "fetching '$url'...\n";
my $txt = `curl -s $url`;  # fetch contents of URL

# if match color code like "#9900FF", run blink1-tool cmd
if( $txt =~ /\#(\w\w)(\w\w)(\w\w)/ ) { 
    my ($r,$g,$b) = ($1,$2,$3);
    print "got color: #$r$g$b\n";
    print `$blink1toolpath --rgb 0x$r,0x$g,0x$b`;
}

