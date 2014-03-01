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
#     0123456789abcdef
    '|*       .       |',
    '| *      .       |',
    '|  *     .       |',
    '|   *    .       |',
    '|    *   .       |',
    '|     *  .       |',
    '|      * .       |',
    '|       *.       |',
    '|        *       |',
    '|        .*      |',
    '|        . *     |',
    '|        .  *    |',
    '|        .   *   |',
    '|        .    *  |',
    '|        .     * |',
    '|        .      *|'
]

def int2bin(n, count=32):
    """returns the binary of integer n, using count number of digits"""
    chars = [' ', '1']
    return "".join([chars[(n >> y) & 1] for y in range(count-1, -1, -1)])

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
    glitch = " "
    
    diffs = []
    for _ in range(4):
        diffs.append(0xa04)
    
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

        jitter = value & 0xf
        print jitterString0x0f[jitter],

        if abs(oldValue - value) > 4:
            #print  abs(oldValue - value),
            if bigDiff or abs(oldValue - value) > 7:
                oldValue = value
                bigDiff = False
                print "B ",
            else:
                bigDiff = True
                print "G ",
        else:
            print "  ",

            if value > 0x880:            
                diffs.pop(0)
                diffs.append(value)
                avg = 0
                for d in diffs:
                    avg += d
                avg = (avg) / len(diffs)
                print "avg=%3x " % (avg, ),
            else:
                print "        ",
            
                        
            data = value >> 4
            print " 0x%03x 0x%02x" % (value, data),
            if oldData != data:
                now = time.time()
                print "%3dms  %s" % (int((now - startTime) * 1000), int2bin(data, 8)),
                print "  0x%02x" % (data, ),
                startTime = now
                glitch = " "


            oldValue = value
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
