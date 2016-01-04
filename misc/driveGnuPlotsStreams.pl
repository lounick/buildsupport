#!/usr/bin/perl -w
use strict;

sub usage {
    print "Usage: $0 <options>\n";
    print <<OEF;
where options are (in order):

  NumberOfStreams                        How many streams to plot (windows)

  Stream1_WindowSampleSize               this many samples per window for Stream1
  <Stream2_WindowSampleSize>             ...for stream2
...
  <StreamN_WindowSampleSize>             ...for streamN

  Stream1_YRangeMin Stream1_YRangeMax    Min and Max values for stream 1
  <Stream2_YRangeMin Stream1_YRangeMax>  ...for stream 2
...
  <StreamN_YRangeMin StreamN_YRangeMax>  ...for stream N

  Stream1_Title                          Title used for stream 1
  <Stream1_Title>                        ...for stream 2
...
  <StreamN_Title>                        ...for stream N

  Stream1_geometry			 X and Y position in pixels from the top left
  <Stream2_geometry>                     ...for stream 2
...
  <StreamN_geometry>                     ...for stream N


The last parameters (geometries) are of the form: WIDTHxHEIGHT+XOFF+YOFF
OEF
    exit(1);
}

sub Arg {
    if ($#ARGV < $_[0]) {
	print "Expected parameter missing...\n\n";
	usage;
    }
    $ARGV[int($_[0])];
}

sub main {
    my $argIdx = 0;
    my $numberOfStreams = Arg($argIdx++);
    print "Will display $numberOfStreams Streams (in $numberOfStreams windows)...\n";
    my @sampleSizes;
    for(my $i=0; $i<$numberOfStreams; $i++) {
	my $samples = Arg($argIdx++);
	push @sampleSizes, $samples;
	print "Stream ".($i+1)." will use a window of $samples samples\n";
    }
    my @ranges;
    for(my $i=0; $i<$numberOfStreams; $i++) {
	my $miny = Arg($argIdx++);
	my $maxy = Arg($argIdx++);
	push @ranges, [ $miny, $maxy ];
	print "Stream ".($i+1)." will use a range of [$miny, $maxy]\n";
    }
    my @titles;
    for(my $i=0; $i<$numberOfStreams; $i++) {
	my $title = Arg($argIdx++);
	push @titles, $title;
	print "Stream ".($i+1)." will use a title of '$title'\n";
    }
    my @geometries;
    for(my $i=0; $i<$numberOfStreams; $i++) {
	my $geometry = Arg($argIdx++);
	push @geometries, $geometry;
	print "Stream ".($i+1)." will use a geometry of '$geometry'\n";
    }
    my @gnuplots;
    my @buffers;
    my @xcounters;
    shift @ARGV; # number of streams
    for(my $i=0; $i<$numberOfStreams; $i++) {
	shift @ARGV; # sample size
	shift @ARGV; # miny
	shift @ARGV; # maxy
	shift @ARGV; # title
	shift @ARGV; # geometry
	local *PIPE;
	my $geometry = $geometries[$i];
	open PIPE, "|gnuplot -geometry $geometry" || die "Can't initialize gnuplot number ".($i+1)."\n";
	select((select(PIPE), $| = 1)[0]);
	push @gnuplots, *PIPE;
	print PIPE "set xtics\n";
	print PIPE "set ytics\n";
	print PIPE "set yrange [".($ranges[$i]->[0]).":".($ranges[$i]->[1])."]\n";
	print PIPE "set style data linespoints\n";
	print PIPE "set grid\n";
	my @data = [];
	push @buffers, @data;
	push @xcounters, 0;
    }
    my $streamIdx = 0;
    select((select(STDOUT), $| = 1)[0]);
    while(<>) {
	chomp;
	my @parts = split /:/;
	$streamIdx = $parts[0];
	my $buf = $buffers[$streamIdx];
	my $pip = $gnuplots[$streamIdx];
	my $xcounter = $xcounters[$streamIdx];
	my $title = @titles[$streamIdx];

	# data buffering (up to stream sample size)
	push @{$buf}, $parts[1];
	#print "stream $streamIdx: ";
	print $pip "set xrange [".($xcounter-$sampleSizes[$streamIdx]).":".($xcounter+1)."]\n";
	print $pip "plot \"-\" title '$title'\n";
	my $cnt = 0;
	for my $elem (reverse @{$buf}) {
	    #print " ".$elem;
	    print $pip ($xcounter-$cnt)." ".$elem."\n";
	    $cnt += 1;
	}
	#print "\n";
	print $pip "e\n";
	if ($cnt>=$sampleSizes[$streamIdx]) {
	    shift @{$buf};
	}
	$xcounters[$streamIdx]++;
    }
    for(my $i=0; $i<$numberOfStreams; $i++) {
	my $pip = $gnuplots[$i];
	print $pip "exit;\n";
	close $pip;
    }
}

main;
