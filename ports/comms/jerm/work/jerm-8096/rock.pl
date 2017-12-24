#!/usr/bin/perl -w
# $Id: rock.pl,v 1.1 2003/05/23 09:14:37 candy Exp $
use strict;
use Getopt::Std;

use vars qw($opt_V);


sub strtol($) {
	return ($_[0] =~ /^0/) ? oct($_[0]) : $_[0];
}

sub checksum(@) {
	my(@v) = @_;
	my($i, $sum);
	$sum = 0;
	for ($i = 0; $i <= $#v; $i++) {
		$sum += $v[$i];
	}
	return (0 - $sum) & 0xffff;
}

sub print_2c($) {
	my($x) = @_;
	printf("%c%c", $x & 255, ($x >> 8) & 255);
}


# av: 0=id 1=flag, 2=word7,,,
sub nain(@) {
	my(@av) = @_;
	my $w1 = 0x81ff;
	my $id = strtol($av[0]);
	my $flag = strtol($av[1]);
	shift(@av);
	shift(@av);
	my $ndata = $#av == -1 ? 0 : $#av + 2;
	my $hsum = checksum($w1, $id, $flag, $ndata);
	print_2c($w1);
	print_2c($id);
	print_2c($ndata);
	print_2c($flag);
	print_2c($hsum);
	my $i;
	if ($#av != -1) {
		unshift(@av, 0); # sequence number
		for ($i = 0; $i <= $#av; $i++) {
			print_2c($av[$i]);
		}
		my $dsum = checksum(@av);
		print_2c($dsum);
	}
}

my $usage =
	"usage: $0 [-V] message_id flag [data7 data8 ...]\n" .
	"flag: DCL0 QRAN 00XX XXXX\n" .
	"  stop=0x8000, start=0x4000, query=0x0800\n" .
	"message_id:\n" .
	" 1000: Geodetic Position Status  1100: Built-In Test Result\n" .
	" 1001: ECEF Position Status      1102: Measurement Time Mark\n" .
	" 1002: Channel Summary           1108: UTC Time Mark Pulse\n" .
	" 1003: Visible Satellites        1130: Serial Port Parameter\n" .
	" 1005: Differential GPS Status   1135: EEPROM Update\n" .
	" 1007: Channel Measurement       1136: EEPROM Status\n" .
	" 1011: Receiver ID\n" .
	" 1012: User-Settings\n" .
	" 1211 0 M (Map = M)\n" .
	" 1220 0 P (Platform = P:0=def 5=auto)\n" .
	" 1221 0 MPGH 0 0 0 0 0 0 0 (MFilter PosPinning GNDTr HAlt)\n" .
	" 1331 0 0 1 (To NMEA)\n" .
	""
	;

sub main() {
	my $ex = 1;
	getopts('V');
	if ($#ARGV < 1 || defined($opt_V)) {
		printf(STDERR $usage);
	}
	else {
		nain(@ARGV);
		$ex = 0;
	}
	return $ex;
}

exit(main());

