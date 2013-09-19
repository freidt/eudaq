#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code

m = SlowControl.SlowControl(0) # HLVDS FEC (master)
s = SlowControl.SlowControl(1) # HLVDS ADC (slave)

readout_start = 0x675
readout_end   = readout_start + (60*60+90*90)*2*4 # two memories per pixel, every fourth clock cycle is readout
clk_high = 0x1
clk_low  = 0x3

addresses = [    0,   1,    2,    3,     4,     5,     6,     7,           8,   9,  10,     11,            12,          13,  14,  15,  16,       17,      18,    19,  20 ]
values    = [ 0x46, 0x0, 0x66, 0x80, 0x100, 0x580, 0x600, 0x666, readout_end, 0x0, 0x0,  0x646, readout_start, readout_end, 0x1, 0x1, 0x1, clk_high, clk_low, 0x667, 0x0 ]

SlowControl.write_burst(m, 6039, 0x0, values, False)

SlowControl.write_burst(s, 6039, 0x0, [0x4], False)

quit()
