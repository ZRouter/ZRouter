#!/usr/bin/perl
use strict; 


while (<STDIN>)
{
	my @in = split(/\s+/); 
	print "in=\"".join(" ", @in)."\"\n";

	for my $i (reverse 0 .. $#in)
	{
		for my $o (split(/\s+/, "one two")) 
		{
			print "$i - $in[$i] eq $o\n"; 
			if ($in[$i] eq $o)
			{
				print "$i - delete $in[$i] eq $o\n"; 
				delete $in[$i]; 
				last;
			} 
		} 
	} 

	print "in=\"".join(" ", @in)."\"\n";

}
