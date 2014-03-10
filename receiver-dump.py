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

# Number of averages of the sync value. Used to check how good the clock
# calibration in the PIC works
AVERAGES = 4                

TIMING_OFFSET = 0x20
SYNC_VALUE_LIMIT = 0x880
SYNC_VALUE_NOMINAL = 0xa40
VALUE_COUNT = 3


# This string allows us to visually inspect the lower 5 bits of the measured
# pulse length, so we can see jitter and glitches.
# The dot represents the desired target value we are aiming for.
jitterString = [
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
    '|        .                      *|',
    '|        .                       |'    # Extra element containing just the boundaries.
]


def int2bin(n, count=32):
    """Returns the binary of integer n, using count number of digits"""
    chars = ['.', '1']
    return "".join([chars[(n >> y) & 1] for y in range(count-1, -1, -1)])


def readInt(port):
    """Read a decimal number, terminated by <LF>, from the given 
       serial port. This function returns only when a valid decimal number
       has been read.
    """
    while True:
        c = port.read(1)
        numString = ""
        while c != "\n":
            numString = numString + c
            c = port.read(1)

        try:        
            return int(numString, 10)
        except ValueError:
            pass


# "Static" variables for processValue()
oldData = 0
receivedValues = []
startTime = time.time()

def processValue(value):
    global oldData
    global receivedValues
    global startTime
    global minTime, maxTime

    # Remove the offset we added in the transmitter to ensure the minimum
    # pulse does not go down to zero.
    if value < SYNC_VALUE_LIMIT:
        value -= TIMING_OFFSET

    # If we are dealing with a sync value we clamp it to the nominal value
    # so that our check whether the value has changed does not trigger 
    # wrongly when jitter moves between e.g. 0xa40 and 0xa3f
    if value >= SYNC_VALUE_LIMIT:
        value = SYNC_VALUE_NOMINAL
    
    # The actual usable data are only the upper 7 bits (bits 11..5) because of
    # the jitter and glitches introduced by the receiver firmware    
    data = value >> 5
    
    print "  0x%03x 0x%02x" % (value, data),

    if oldData != data:
        if value < SYNC_VALUE_LIMIT:
            receivedValues.append(data)
        else:        
            now = time.time()
            diff = int((now - startTime) * 1000)
            print "  %3dms" % (diff, ),
            startTime = now
            
            if len(receivedValues) != VALUE_COUNT:
                print "VALUECOUNT IS %d, SHOULD BE %d" % (len(receivedValues), VALUE_COUNT),
            else:
                # Remove bit 5 which we use to force large boundaries between
                # values
                for i in range(1, len(receivedValues)):
                    receivedValues[i] = receivedValues[i] & 0x1f

                for d in receivedValues:           
                    print "0x%02x" % d,
                
                print "   ",    

                numBits = 6
                for d in receivedValues:           
                    print int2bin(d, numBits), " ",
                    numBits = 5
                    
            receivedValues = []
    oldData = data


def dump(port):
    try:
        s = serial.Serial(port, 38400)
    except serial.SerialException, e:
        print("Unable to open port %s.\nError message: %s" % (port, e))
        sys.exit(0)

    diffs = [SYNC_VALUE_NOMINAL] * AVERAGES
    
    while True:
        value = readInt(s)
    
        # Show the jitter diagram
        if value < SYNC_VALUE_LIMIT:            
            print jitterString[value & 0x1f],
        else:
            print jitterString[-1],

        # Print the raw value we read from the receiver
        print "0x%03x" % (value, ),
        
        # Calculate a rolling average of the last n sync pulses
        if value >= SYNC_VALUE_LIMIT:            
            diffs.pop(0)
            diffs.append(value)
            avg = 0
            for d in diffs:
                avg += d
            avg = (avg) / len(diffs)
            print "avg=0x%3x" % (avg, ),
        else:
            print "         ",
                    
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
        print
        sys.exit(0)
        
