#******************************************************************************
#
#   receiver-dump.py
#
#******************************************************************************
#
#   Author:         Werner Lane
#   E-mail:         laneboysrc@gmail.com
#
#******************************************************************************

import serial
import sys
import time

jitterString0x07 = [
    '|*  ..   |',
    '| * ..   |',
    '|  *..   |',
    '|   *.   |',
    '|   .*   |',
    '|   ..*  |',
    '|   .. * |',
    '|   ..  *|'
]

jitterString0x0f = [
    '|*      ..       |',
    '| *     ..       |',
    '|  *    ..       |',
    '|   *   ..       |',
    '|    *  ..       |',
    '|     * ..       |',
    '|      *..       |',
    '|       *.       |',
    '|       .*       |',
    '|       ..*      |',
    '|       .. *     |',
    '|       ..  *    |',
    '|       ..   *   |',
    '|       ..    *  |',
    '|       ..     * |',
    '|       ..      *|'
]

def int2bin(n, count=32):
    """returns the binary of integer n, using count number of digits"""
    return "".join([str((n >> y) & 1) for y in range(count-1, -1, -1)])

def dump(port):
    try:
        s = serial.Serial(port, 38400)
    except serial.SerialException, e:
        print("Unable to open port %s.\nError message: %s" % (port, e))
        sys.exit(0)

    oldValue = 0
    oldData = 0
    startTime = time.time()
    bigDiff = False
    while True:
        c = s.read(1)
        numString = ""
        while c != "\n":
            numString = numString + c
            c = s.read(1)

        try:        
            value = int(numString, 10)
        except ValueError:
            value = 0
        if abs(oldValue - value) > 4:
            #print  abs(oldValue - value)
            if bigDiff or abs(oldValue - value) > 32:
                oldValue = value
                bigDiff = False
            else:
                bigDiff = True
                #print "GLITCH", abs(oldValue - value)
        else:
            oldValue = value
            #print "%s  %d" % (int2bin(value >> 3, 7), value & 0x7)
            
            jitter = value & 0x7
            print jitterString0x07[jitter],
            
            data = value >> 3
            if oldData != data:
                now = time.time()
                print "%3d %s" % (int((now - startTime) * 1000), int2bin(data, 6)),
                startTime = now
            oldData = data
            print


if __name__ == '__main__':
    try:
        port = sys.argv[1]
    except IndexError:
        port = '/dev/ttyUSB0'

    try:
        dump(port)
    except KeyboardInterrupt:
        print ""
        sys.exit(0)
