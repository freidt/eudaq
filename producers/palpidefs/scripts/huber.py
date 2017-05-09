#!/usr/bin/env python

#
# use crossed cable for RS232!!!
# TODO problems with this code occured when other devices try to acces port, not solved yet
# 

import serial
import array
import time
import os
import sys


class HuberConnection:
    def __init__(self, port_name):
        self.link=serial.Serial(port=port_name, baudrate=9600, bytesize=8, parity='N', stopbits=1, timeout=2, xonxoff=0, rtscts=0)

    def send(self, cmd):
        self.link.flushInput()
        self.link.write(cmd)
        time.sleep(0.5)
        res=self.link.read(100)
        return res


class Huber:
    def __init__(self,connection):
        self.connection = connection

    #def flushIO(self):
    #    self.connection.link.flushInput()
    #    self.connection.link.flushOutput()

    def get_device_name(self):
        cmd =  "[M01V07C6\r"
        res = self.connection.send(cmd)
        device_name = res[7:len(res)-3] 
        print device_name
        #return device_name

    def read_temperature(self):
        cmd = "[M01G0D**" + "****"
        checksum = str(hex((sum(bytearray(cmd)) & 0xff)))[2:4].upper()
        cmd = cmd + checksum + "\r" 

        res = self.connection.send(cmd)
        temp_set = int(res[9:13], 16)
        if temp_set>0x7FFF:
            temp_set = -(((int(temp_set,16)-1)^0xffff)&0xffff)/100.
        else:
            temp_set = temp_set/100.
    
        temp_act = int(res[13:17], 16)
        if temp_set>0x7FFF:
            temp_act = -(((int(temp_act,16)-1)^0xffff)&0xffff)/100.
        else:
            temp_act = temp_act/100.

        print "%2.2f\t%2.1f" % ( temp_set, temp_act )
        #print "%2.1f degC\t%2.1f degC" % ( temp_set, temp_act )
        return [str(temp_set), str(temp_act), res]


    def set_temperature(self, temp):
        temp = int(temp*100)
        temp_str = 0
        if temp >= 0:
            temp_str = str(hex(temp)).split('x')[1].zfill(4).upper() 
        else: 
            temp_str = str(hex(((abs(temp)^0xffff)+1)&0xffff)).split('x')[1].zfill(4).upper()

        cmd = "[M01G0DI*" + temp_str
        #print hex((sum(bytearray(cmd)) & 0xff))
        checksum = str(hex((sum(bytearray(cmd)) & 0xff)))[2:4].zfill(2).upper()
        cmd = cmd + checksum + "\r" 
        res = self.connection.send(cmd)

        temp_set = int(res[9:13], 16)
        if temp_set>0x7FFF:
            temp_set = -(((temp_set-1)^0xffff)&0xffff)/100.
        else:
            temp_set = temp_set/100.
    
        temp_act = int(res[13:17], 16)
        if temp_set>0x7FFF:
            temp_act = -(((temp_act-1)^0xffff)&0xffff)/100.
        else:
            temp_act = temp_act/100.

        print "%2.2f\t%2.1f" % ( temp_set, temp_act )
        #print "%2.1f degC\t%2.1f degC" % ( temp_set, temp_act )
        #return [str(temp_set), str(temp_act), res]

    
    def temperature_actual_is_set(self):
        temp = self.read_temperature()     
        if float(temp[0])-0.5 <= float(temp[1]) <=float(temp[0])+0.5:
            return True
        else:
            return False

    def set_standby(self):
        cmd = "[M01G0DO*" + "****"
        checksum = str(hex((sum(bytearray(cmd)) & 0xff)))[2:4].upper()
        cmd = cmd + checksum + "\r" 

        res = self.connection.send(cmd)
        #return res


def search_port_name():
#    dmesg=os.popen('dmesg | grep ttyUSB | grep pl2303 | tail -1').read()
#    port_name = "/dev/" + [y for y in dmesg.split() if 'tty' in y][0] 
    port_name="/dev/ttyUSB0"
    
    #print port_name
    return port_name

#con=HuberConnection(search_port_name())
#chiller=Huber(con)
#chiller.set_temperature(19.0)
#while not chiller.temperature_actual_is_set():
#    time.sleep(5)
#
#chiller.set_standby()

def main():
    # set up chiller conncetion
    #dev=sys.argv[1]
    #if dev!=search_port_name():
    #    print "port not correctly set!"
    #    sys.exit(1)
    dev = search_port_name()

    mode=int(sys.argv[1]) if len(sys.argv)>=2 else -1;
    con=HuberConnection(dev)
    chiller=Huber(con)
 
    # switch mode
    if mode==0:
        # read device name
        chiller.get_device_name()
    elif mode==1:
        # set temperature
        chiller.set_temperature(float(sys.argv[2]))
        count=0
        while chiller.temperature_actual_is_set() == False :
            time.sleep(5)
#            print chiller.temperature_actual_is_set()
            count = count+1
        if count != 0 :
            time.sleep(120)
    elif mode==2:
        # read temperature
        chiller.read_temperature()
    elif mode==3:
        # check if desired temperature value is reached
        return chiller.temperature_actual_is_set()
    elif mode==4:
        # read temperature
        chiller.set_standby()

## execute the main
if __name__ == "__main__":
    sys.exit(main())



