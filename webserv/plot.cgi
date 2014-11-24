#! /usr/bin/perl -w

open (GNUPLOT, "|gnuplot -persist");
print GNUPLOT <<gnuplot_commands;

set title "CS410 Webserver"
set autoscale xy
set style data histogram
set nokey
set yrange [0:]
set boxwidth 1.5 relative
set label center
set style data histograms
set style fill solid 1.0 border -1
set terminal jpeg color blacktext "Helvetica" 16 
set output 'my_graph.jpeg'
set ylabel 'frequency'
set xlabel 'patterns'
plot 'output.dat' using 2: xtic(1) with histogram
set output
gnuplot_commands

close GNUPLOT;

