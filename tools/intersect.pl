#!/usr/bin/perl

use strict;


my @array1 = @ARGV;
my @array2 = ();

while (<STDIN>)
{
	chomp;
	push @array2, split(/\s+/);
}


my @union = my @intersection = my @difference = ();
my %count = ();
foreach my $element (@array1, @array2) { $count{$element}++ }
foreach my $element (keys %count) 
{
	push @union, $element;
        push @{ $count{$element} > 1 ? \@intersection : \@difference }, $element;
}

print "INTESECTION: ".join(" ", @intersection)."\n";
print "DIFFERENCE: ".join(" ", @difference)."\n";

