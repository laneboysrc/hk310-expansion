#******************************************************************************
#
#   hk310-reader.py
#
#   This utility reads the RS232 input to the NRF24LE1G chip in the 
#   HobbyKing HK310 transmitter.
#
#******************************************************************************
#
#   Author:         Werner Lane
#   E-mail:         laneboysrc@gmail.com
#
#******************************************************************************

import serial
import sys


def stickCommand(data):
    h = data[4] & 0x0f
    l = data[6]
    st = (h << 8) + l
    
    h = data[5] >> 4
    l = data[7]
    th = (h << 8) + l
    
    h = data[5] & 0x0f
    l = data[8]
    ch3 = (h << 8) + l

    error = ""
    if crc16_ccitt(data[3:9]) != ((data[9] << 8) + data[10]):
        error = "ERROR: CRC16 does not match"
    
    print "st=%04d  th=%04d  ch3=%04d  %s" % (st, th, ch3, error) 


def failsafeCommand(data):
    error = ""
    if crc16_ccitt(data[3:9]) != ((data[9] << 8) + data[10]):
        error = "ERROR: CRC16 does not match"
        
    st = data[6] - 120
    th = data[7] - 120
    mode = data[8]
    thEnabled = "off"
    if mode & 0x02: 
        thEnabled = "on "

    stEnabled = "off"
    if mode & 0x01: 
        stEnabled = "on "

    print "FAILSAFE: st=%4d%%, %s  th=%4d%%, %s  %s" % (st, stEnabled, th, thEnabled, error) 


def modelCode(data);
    print "Model code: ", ' '.join(["%02x" %  data[i] for i in range(15)])


def preprocessor_reader(port):
    try:
        s = serial.Serial(port, 19200)
    except serial.SerialException, e:
        print("Unable to open port %s.\nError message: %s" % (port, e))
        sys.exit(0)

    data = []
    while True:
        data.append(ord(s.read(1)))
        if len(data) == 15:
            if data[0] != 0xff:
                data = data[1:]
            else:
                if data[1] == 0x55  and  data[2] == 0xaa:
                    checksum = 0;
                    for i in (3, 4, 5, 6, 7, 8, 9, 10):
                        checksum += data[i]

                    if checksum != ((data[13] << 8) + data[14]):
                        print "SERIAL CHECKSUM ERROR"                    

                    elif data[3] == 0xaa:
                        stickCommand(data)
                    
                    elif data[3] == 0xbb:
                        failsafeCommand(data)

                    else:
                        print "Unknown command 0x%1x" % data[4] >> 4

                elif data[1] == 0xaa  and  data[2] == 0x55:
                    modelCode(data)
                else:
                    print "Unknown command header: ",
                    print ' '.join(["%02x" %  data[i] for i in range(15)])

                data = []


def crc16_ccitt(data):
    crc = 0
    for i in data:
        b = i
        crc = crc ^ b << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = crc << 1 ^ 0x1021
            else:
                crc = crc << 1

    return crc & 0xffff;


if __name__ == '__main__':
    try:
        port = sys.argv[1]
    except IndexError:
        port = '/dev/ttyUSB0'

    try:
        preprocessor_reader(port)
    except KeyboardInterrupt:
        print("")
        sys.exit(0)
