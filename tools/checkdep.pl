#!/usr/local/bin/perl

use strict;

my ($type, $file, $dir) = @ARGV;
#my ($type, $file, $libdir, $bindir, $incdir) = @ARGV;

$dir = "/usr/local/"	unless ($dir);
#$libdir = "/usr/local/lib/"	unless ($libdir);
#$bindir = "/usr/local/bin/"	unless ($bindir);
#$incdir = "/usr/local/include/"	unless ($incdir);

my $matched = 0;
my $total   = 0;
open(FH, "<$file") or die "Can`t open file \"$file\"\n";
while ( <FH> )
{
	next if (/^@/);
	next if (/^share\/(examples|doc)/);
	chomp;
	$total++;
	$matched ++ if (-e "$dir/$_");
#	print "$_\n";
}

close FH;
if (!$matched)
{
	print "0\n";
	exit 0;
}

print int($matched * 100 / $total)."\n";
