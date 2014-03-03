#******************************************************************************
#
#   hk310-filter.py
#
#   Modifies CH3 data in the HobbyKing HK310 or Turnigy X-3S transmitter
#   on the fly.
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

values = [
    0xa0,
    0x0f,
    0x00,
    0x02
]


new_ch3 = values[0]
disableLoop = 0

cur_value = 0



def prepareValues():
    # Sanitize values    
    for i, _ in enumerate(values):
        if i == 1:
            values[i] = values[i] & 0x7f
        elif i>= 2: 
            values[i] = values[i] & 0x3f
            new = values[i]
            old = values[i - 1]
            # Maximize the difference between adjacent values
            # This results in a minimum difference of 0x20
            if abs(old - new) < abs(old - (new | 0x40)):
                new = new | 0x40
            values[i] = new

    for i, _ in enumerate(values):
        print nextValue(i)



def nextValue(idx):
    global values

    rx_desired = values[idx] << 4
    rx_desired += 0x20;     # Add 0x20 to avoid tiny pulse durations
    if rx_desired >= 0x700:
        rx_desired += 2     # Above 0x700 add 2 for better stability! 
    rx = 2720 - 1 - rx_desired
    return rx  * 16 / 17

prepareValues()

def crc16_ccitt(crc, byte):
    """ Add a byte to the CRC16-CCITT checksum.
    """
    crc = crc ^ byte << 8

    for _ in range(8):
        if crc & 0x8000:
            crc = crc << 1 ^ 0x1021
        else:
            crc = crc << 1

    return crc & 0xffff


def hk310_filter(port):
    global new_ch3
    global cur_value
    global values
    
    try:
        s = serial.Serial(port, 19200)
    except serial.SerialException, e:
        print("Unable to open port %s.\nError message: %s" % (port, e))
        sys.exit(0)

    STATE_WAIT_FOR_SYNC = 0
    STATE_SYNC2 = 1
    STATE_SYNC3 = 2
    STATE_PAYLOAD3 = 3
    STATE_PAYLOAD4 = 4
    STATE_PAYLOAD5 = 5
    STATE_PAYLOAD6 = 6
    STATE_PAYLOAD7 = 7
    STATE_PAYLOAD8 = 8
    STATE_CRC_H = 9
    STATE_CRC_L = 10
    STATE_GARBAGE1 = 11
    STATE_GARBAGE2 = 12
    STATE_CHECKSUM_H = 13    
    STATE_CHECKSUM_L = 14    
    STATE_SKIP = 99

    count = 0

    state = STATE_WAIT_FOR_SYNC
    while True:
        b = s.read(1)
        if len(b) == 0:
            sys.exit(0)
        b = ord(b)
        
        if state == STATE_WAIT_FOR_SYNC:
            if b == 0xff:
                state = STATE_SYNC2
        
        elif state == STATE_SYNC2:
            if b == 0x55:
                state = STATE_SYNC3
            elif b != 0xff:
                state = STATE_WAIT_FOR_SYNC
        
        elif state == STATE_SYNC3:
            if b == 0xaa:
                state = STATE_PAYLOAD3
            elif b == 0xff:
                state = STATE_SYNC2
            
        elif state == STATE_PAYLOAD3:
            crc16 = crc16_ccitt(0, b)
            checksum = b
            if b == 0xaa:
                state = STATE_PAYLOAD4
            else:
                state = STATE_SKIP
                skip = 11
            new_crc16 = crc16_ccitt(0, b)
            new_checksum = b
            
        elif state == STATE_PAYLOAD4:
            checksum = checksum + b
            crc16 = crc16_ccitt(crc16, b)
            if (b >> 4) == 0xa:

                # High nibble of steering

                state = STATE_PAYLOAD5
            else:
                state = STATE_SKIP
                skip = 10
            new_checksum = new_checksum + b
            new_crc16 = crc16_ccitt(new_crc16, b)
            
        elif state == STATE_PAYLOAD5:
            checksum = checksum + b
            crc16 = crc16_ccitt(crc16, b)
            t, ch3 = (b >> 4), (b & 0x0f);

            # High nibble of throttle and CH3
            ch3 = new_ch3 >> 8
            
            t, ch3 = (t & 0x0f), (ch3 & 0x0f)
            b = (t << 4) + ch3
            state = STATE_PAYLOAD6
            new_checksum = new_checksum + b
            new_crc16 = crc16_ccitt(new_crc16, b)

        elif state == STATE_PAYLOAD6:
            checksum = checksum + b
            crc16 = crc16_ccitt(crc16, b)

            # Low byte of steering

            state = STATE_PAYLOAD7
            new_checksum = new_checksum + b
            new_crc16 = crc16_ccitt(new_crc16, b)

        elif state == STATE_PAYLOAD7:
            checksum = checksum + b
            crc16 = crc16_ccitt(crc16, b)

            # Low byte of throttle

            state = STATE_PAYLOAD8
            new_checksum = new_checksum + b
            new_crc16 = crc16_ccitt(new_crc16, b)

        elif state == STATE_PAYLOAD8:
            checksum = checksum + b
            crc16 = crc16_ccitt(crc16, b)
            
            # Low byte of CH3
            b = new_ch3 & 0xff
            
            state = STATE_CRC_H
            new_checksum = new_checksum + b
            new_crc16 = crc16_ccitt(new_crc16, b)
            
        elif state == STATE_CRC_H:
            checksum = checksum + b
            received_crc16 = b << 8
            state = STATE_CRC_L
            b = new_crc16 >> 8
            new_checksum = new_checksum + b

        elif state == STATE_CRC_L:
            checksum = checksum + b
            received_crc16 = received_crc16 + b

            if crc16 != received_crc16:
                print "WARNING: incoming CRC16 mismatch!"

            b = new_crc16 & 0xff
            state = STATE_GARBAGE1
            new_checksum = new_checksum + b

        elif state == STATE_GARBAGE1:
            state = STATE_GARBAGE2

        elif state == STATE_GARBAGE2:
            state = STATE_CHECKSUM_H

        elif state == STATE_CHECKSUM_H:
            received_checksum = b << 8
            b = checksum >> 8
            state = STATE_CHECKSUM_L

        elif state == STATE_CHECKSUM_L:
            received_checksum = received_checksum + b

            if checksum != received_checksum:
                print "WARNING: incoming checksum mismatch!"

            b = checksum & 0xff
            state = STATE_WAIT_FOR_SYNC
            #print "Packet successfully filtered"
            if count < 2:
                count += 1
            else:
                count = 0;
                if not disableLoop:
                    cur_value = cur_value + 1
                    if cur_value >= len(values):
                        cur_value = 0
                    new_ch3 = nextValue(cur_value)
                
        elif state == STATE_SKIP:
            skip = skip - 1
            if skip == 0:
                state = STATE_WAIT_FOR_SYNC
        
        s.write(chr(b))
        


if __name__ == '__main__':
    try:
        port = sys.argv[1]
    except IndexError:
        port = '/dev/ttyUSB0'

    try:
        hk310_filter(port)
    except KeyboardInterrupt:
        print ""
        #print "Average time between commands: %d ms" % (avgValue / avgCount)
        sys.exit(0)
