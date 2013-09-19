#! /usr/bin/env python
##
## code testing of class SlowControl
##
import SlowControl # slow control code
import time

m = SlowControl.SlowControl(0) # HLVDS FEC (master)
s = SlowControl.SlowControl(1) # ADC FEC (slave)

# enable digital I/Os on the master
SlowControl.write_list(m, 0x1977, [0x2, 0x1], [ 0x300, 0x1FF ], False)

# enable slave
SlowControl.write_burst(s, 6039, 0x3, [ 0x1 ], False)

time.sleep(1)

# enable sync on the master
# request a single event
# in order to make the trigger/timer unit running, 0x4 (3rd bit) in
# register 0x15 has to be set
SlowControl.write_list(m, 6039, [ 0x18, 0x15 ], [ 0x1, 0x6 ], False)
SlowControl.write_list(m, 6039, [ 0x15 ], [ 0x4 ], False)

quit()
