vsim -L unisim -L work work.cordic_tb

view -undock -height 950 -width 1900 wave

configure wave -namecolwidth 200
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -rowmargin 10
configure wave -childrowmargin 2

onerror {resume}

add wave -noupdate -divider "TOP"
add wave -noupdate -radix hexadecimal /cordic_tb/*

add wave -noupdate -divider "MULT"
add wave -noupdate -radix hexadecimal /cordic_tb/uut1/*

add wave -noupdate -divider "DIVIDE"
add wave -noupdate -radix hexadecimal /cordic_tb/uut2/*

add wave -noupdate -divider "POLAR-RECT"
add wave -noupdate -radix hexadecimal /cordic_tb/uut3/*

add wave -noupdate -divider "RECT-POLAR"
add wave -noupdate -radix hexadecimal /cordic_tb/uut4/*

run 220 us

