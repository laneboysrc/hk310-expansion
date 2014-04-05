#******************************************************************************
#
#   hk310-reader.py
#
#   This utility reads the UART input to the NRF24LE1G chip in the
#   HobbyKing HK310 or Turnigy X-3S transmitter.
#
#   Use a USB-to-serial dongle to connect the HK310 to a PC.
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


lastTime = time.time()
avgValue = 0
avgCount = 0


def timeStamp():
    """ Helper function to print timestamps between commands, and to calculate
        the average command repetition time.
    """
    global lastTime, avgCount, avgValue
    now = time.time()
    diff_us = int((now - lastTime) * 1000000)
    lastTime = now
    avgValue += diff_us
    avgCount += 1
    #print "%08d -> " % (diff_us, ),


def crc16_ccitt(data):
    """ Calculate the CRC16-CCITT checksum of the given byte array.
    """
    crc = 0
    for b in data:
        crc = crc ^ b << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = crc << 1 ^ 0x1021
            else:
                crc = crc << 1

    return crc & 0xffff


def stickCommand(data):
    """ Handle the Stick data command, which holds steering, throttle and CH3
        data
    """
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

    timeStamp()
    print "st=%04d  th=%04d  ch3=%04d  %s" % (st, th, ch3, error)


def failsafeCommand(data):
    """ Handle the Failsafe command
    """
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

    timeStamp()
    print "FAILSAFE: st=%4d%%, %s  th=%4d%%, %s  %s" % (st, stEnabled,
        th, thEnabled, error)


def modelCode(data):
    """ Handle the Model code command
    """
    modelCode = data[4]
    timeStamp()
    print "Model code: %d (mod%02d)" % (modelCode, modelCode - 2)


def hk310_reader(port):
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
                if data[1] == 0x55 and data[2] == 0xaa:
                    checksum = 0
                    for i in range(3, 11):
                        checksum += data[i]

                    if checksum != ((data[13] << 8) + data[14]):
                        print "SERIAL CHECKSUM ERROR"

                    elif data[4] >> 4 == 0xa:
                        stickCommand(data)

                    elif data[4] >> 4 == 0xb:
                        failsafeCommand(data)

                    else:
                        print "Unknown command 0x%1x" % data[4] >> 4

                elif data[1] == 0xaa and data[2] == 0x55:
                    modelCode(data)
                else:
                    print "Unknown command header: ",
                    print ' '.join(["%02x" % data[i] for i in range(15)])

                data = []


if __name__ == '__main__':
    try:
        port = sys.argv[1]
    except IndexError:
        port = '/dev/ttyUSB0'

    try:
        hk310_reader(port)
    except KeyboardInterrupt:
        print ""
        print "Average time between commands: %d ms" % (avgValue / avgCount)
        sys.exit(0)
