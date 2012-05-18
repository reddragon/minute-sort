set xrange [2.5 : 6.5]
set xlabel 'Size of the Input File (in GiB)'
set ylabel 'Ratio of Running Time to n log n'
set terminal png
set output 'usertime.png'
plot 'usertime.plt' with linespoints
set output 'wallclocktime.png'
plot 'wallclocktime.plt' with linespoints

