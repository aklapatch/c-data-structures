set terminal gif size 1500,1000
set output "mem_test.gif"
set xlabel 'size'
set logscale x 
set ylabel 'alignment'
set zlabel 'clocks' rotate by 90
set surface
set hidden3d
set view 70,45
splot for [col=3:6] 'mem_test.txt' using 1:2:col with lines title columnheader
