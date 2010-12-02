#!/usr/bin/perl

use strict;

my ($type, $file, $libdir, $bindir, $incdir) = @ARGV;

$libdir = "/usr/local/lib/"	unless ($libdir);
$bindir = "/usr/local/bin/"	unless ($bindir);
$incdir = "/usr/local/include/"	unless ($incdir);

my $matched = 0;
my $total   = 0;
open(FH, "<$file") or die "Can`t open file \"$file\"\n";
while ( <FH> )
{
	next if (/^@/);
	next if (/^share\/(examples|doc)/);
	chomp;
	$total++;
	$matched ++ if (/^lib\/(.*)/ && -e "$libdir/$1" );
	$matched ++ if (/^include\/(.*)/ && -e "$incdir/$1" );
#	print "$_\n";
}

close FH;
if (!$matched)
{
	print "0\n";
	exit 0;
}

print ($total * 100 / $matched);