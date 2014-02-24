Open questions:
- Is the TX sending data at all?
- What is the timing of stick data to the NRF module?
- What is the timing with which stick data propagates to the receiver?
- How do the stick values translate into pulse width on the receiver?
- What resolution can we reliably achieve?
- what do the remaining bits of the startup sequence mean? Compare them with 
  other transmitters!


Note: the RF module firmware differs between HK300 and HK310
Also the X3S seems to have different firmware as the EEPROM ports are different!
I have not examined the serial protocol of the X3S yet to see if it is different
as well



******************************************************************************
******************************************************************************
*** SERIAL PORT
******************************************************************************
******************************************************************************

The serial port runs at 19200,N,8,1
The RX pin (input to the NRF) is pin 1 of the module 


#################
Stick data:

             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
            -------------------------------------------- 
CH3 pos 0   ff 55 aa aa a4 42 94 7a 8a 34 15 e6 08 03 71
CH3 pos 1   ff 55 aa aa a4 46 94 7a 72 90 f3 e6 08 04 97
TH  fwd     ff 55 aa aa a4 22 94 71 8a b1 3d e6 08 03 ed
TH  back    ff 55 aa aa a4 52 94 dc 8a 98 6a e6 08 04 9c
ST  left    ff 55 aa aa a3 42 53 7a 8a f0 a6 87 7c 04 7c
ST  right   ff 55 aa aa a5 42 dc 7a 8a 2a 48 22 7c 03 e3
--------------------------------------------------------
                         s t3 ss tt 33 cc cc XX XX kk kk
                     yy yy yy yy yy yy                   checksum bytes: CRC16 = cc cc
                                   checksum bytes: sum = kk kk



#################
Startup sequence:

Also appears every time a model is changed.
Is repeated 3 times, and often disrupts an ongoing transmission 
(causing CRC errors!)

             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
            -------------------------------------------- 
            ff aa 55 00 02 07 00 00 21 10 42 20 63 30 84 
            ff aa 55 00 02 07 00 00 21 10 42 20 63 30 84 
            ff aa 55 00 02 07 00 00 21 10 42 20 63 30 84 
                        mm

mm:     model code. mod0 = 0x02, mod15 = 0x11
rest:   possibly the random transmitter code?! Note: 00, 10, 20, 30

Wraith: model code 07      Receiver EEPROM: 9F5742107211
Cup Racer: model code 08   Receiver EEPROM: DABE8F96692F



#################
Failsafe:

Sent only if failsafe is on, and it is repeated after every 12 stick data 
transmissions.

             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
            -------------------------------------------- 
            ff 55 aa aa bb cc 78 78 06 c9 e2 a5 09 04 d2
--------------------------------------------------------
                              ss tt mm cc cc XX XX kk kk
                     yy yy yy yy yy yy                   checksum bytes: CRC16 = cc cc
                     xx xx xx xx xx xx xx xx             checksum bytes: sum = kk kk

ss, tt: Steering an throttle in percent. 
        0x78 means 0%, 0x00 means -120%, 0xf0 means +120%

mm:     bit mask whether which channel is enabled for failsafe:
        bit 0: steering
        bit 1: throttle
        bit 2: always 1 (CH3?)
            
  

#################
XX: 

Seems to be garbage data. 

For stick data it is influenced by steering wheel only (not trim, etc!)
Left:   87 7c
Centre: e6 08
Right:  22 7c

For Failsafe the value changes depending on which menus or buttons you
press.


          
#################
kk: 

16 bit result of summing the bytes 3 to 10     



#################
cc:

A checksum of bytes 3 .. 8 
It is using the CRC-CCITT (XModem) algorithm, 0x1021 polynominal
This checksum is most likely sent to the receiver to validate, while
the checksum at the end protects the serial communication.


UINT16 = unsigned int
UINT8 = unsigned char

UINT16 crcByte(UINT16 crc, UINT8 b)
{
    UINT8 t = 8;

    crc = crc ^ b << 8;
    do {
        if (crc & 0x8000)
            crc = crc << 1 ^ 0x1021;
        else
            crc = crc << 1;
    } while (--t);

    return crc;
}



******************************************************************************
******************************************************************************
*** EEPROM CONTENTS
******************************************************************************
******************************************************************************

The EEPROM connected to P0.7 and P1.0 contains binding data.
There is a 25 byte code for each model, starting at address 0.
These 25 bytes correspond with the data we read from the EEPROM in the
receiver.

Note that 6 bytes contain actual code, the rest are padding, incrementing
from the last code byte value onwards. Note sure if only the first 6 
bytes are transmitted over the air, or all 25 bytes.

Furthermore, address 0x19a and 0x19b contain the value 0xaa. It is not known 
whether this information is used, but it is present in both HK310 and X3S.


