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

TIMING_OFFSET = 0x20

jitterString0x0f = [
#     0123456789abcdef0123456789abcdef
    '|*       .                       |',
    '| *      .                       |',
    '|  *     .                       |',
    '|   *    .                       |',
    '|    *   .                       |',
    '|     *  .                       |',
    '|      * .                       |',
    '|       *.                       |',
    '|        *                       |',
    '|        .*                      |',
    '|        . *                     |',
    '|        .  *                    |',
    '|        .   *                   |',
    '|        .    *                  |',
    '|        .     *                 |',
    '|        .      *                |'
    '|        .       *               |',
    '|        .        *              |',
    '|        .         *             |',
    '|        .          *            |',
    '|        .           *           |',
    '|        .            *          |',
    '|        .             *         |',
    '|        .              *        |',
    '|        .               *       |',
    '|        .                *      |',
    '|        .                 *     |',
    '|        .                  *    |',
    '|        .                   *   |',
    '|        .                    *  |',
    '|        .                     * |',
    '|        .                      *|'
]


def int2bin(n, count=32):
    """Returns the binary of integer n, using count number of digits"""
    chars = ['.', '1']
    return "".join([chars[(n >> y) & 1] for y in range(count-1, -1, -1)])


oldData = 0
valueCount = 0
receivedValues = []
expectedValues = [0x0f, 0x00, 0x02]
startTime = time.time()

def processValue(value):
    global oldData
    global valueCount
    global receivedValues
    global startTime
    
    data = value >> 5
    
    print "  0x%03x 0x%02x" % (value, data),
    if oldData != data:
        print "  %s" % (int2bin(data, 8), ),
        if value > 0x880:
            now = time.time()
            print "%3dms " % (int((now - startTime) * 1000), ),
            startTime = now
            if valueCount != 3:
                print "  VALUECOUNT not 3: %d" % valueCount,
            else:
                error = False
                #for i, _ in enumerate(receivedValues):
                #    if i == 0:
                #        if expectedValues[i] != (receivedValues[i] & 0x3f):
                #            error = True
                #    else:
                #        if expectedValues[i] != (receivedValues[i] & 0x1f):
                #            error = True
                if error:                    
                    print "  DATA ERROR!",
                    sys.exit(1)
            valueCount = 0
            receivedValues = []
        else:
            receivedValues.append(data)
            valueCount = valueCount + 1
            print "       ",
    oldData = data


largeChange = False
def dump(port):
    global largeChange
    
    try:
        s = serial.Serial(port, 38400)
    except serial.SerialException, e:
        print("Unable to open port %s.\nError message: %s" % (port, e))
        sys.exit(0)

    oldValue = 0
    
    diffs = []
    for _ in range(4):
        diffs.append(0xa40)
    
    while True:
        c = s.read(1)
        numString = ""
        while c != "\n":
            numString = numString + c
            c = s.read(1)

        try:        
            value = int(numString, 10)
        except ValueError:
            continue
    
        if value < 0x880:            
            # Show the jitter diagram (lower 4 bits)
            jitter = value & 0x1f
            print jitterString0x0f[jitter],
        else:
            print '|        .                       |',

        print "0x%03x" % (value, ),

        # Remove the offset we added in the transmitter to ensure the minimum
        # pulse does not go down to zero.
        if value < 0x880:            
            value -= TIMING_OFFSET

        
        # Calculate a rolling average of the last n sync pulses
        if value > 0x880:            
            diffs.pop(0)
            diffs.append(value)
            avg = 0
            for d in diffs:
                avg += d
            avg = (avg) / len(diffs)
            print "avg=%3x" % (avg, ),
        else:
            print "       ",
                    
        processValue(value)

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
