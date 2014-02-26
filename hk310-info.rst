HobbyKing HK310 and Turnigy X-3S deep-dive
###############################################################################



Open questions
===============================================================================

- What is the timing with which stick data propagates to the receiver?
- What resolution can we reliably achieve?



Introduction
===============================================================================

This document describes the internal functionality of the **HobbyKing HK310**
transmitter. Most findings also apply to the **Turnigy X-3S** transmitter.

The HobbyKing HK300 uses the same hardware RF module as the HK310 and X-3S, but
the module contains a different firmware and uses the pins of the module
in an entirely different way.

The HK310 comprises of a microcontroller (MCU) of unknown origin, and a RF module
using Nordic Semiconductor NRF24LE1 (refered to as NRF module in this document). 

The MCU reads the sticks and switches, drives the display; it basically handles
all user-facing functions of the transmitter.

The MCU connects to the NRF module using a serial port (UART). See 
`Serial port protocol`_ for details on the communication protocol.
Because the MCU is operating at 5V and the NRF module at 3.3V, a simple 
resistor divider of 1k/2.4k is placed in the Tx (MCU) / Rx (NRF module) path. 
Note that the X-3S transmitter has a different MCU operating at 3.3V, so no
level shifting hardware is present.

A 24C16A I2C EEPROM is connected to the NRF module. This EEPROM resides on the 
transmitter PCB, not the NRF module. The EEPROM contains random bytes used
to differentiate between different transmitters and models. 
See `EEPROM contents`_ for more info.

Note that the transmitter PCB contains a second EEPROM that connects to the 
MCU. This EEPROM is likely to hold the model data etc. but has not been
investigated yet.



NRF module pin-out
===============================================================================

::

  +--------------------------------------------------------------------------+
  |   GND            Tx   GND                      O (Hole)                  |
  |   o    o    o    o    o    o                                             |
  |   GND  P0.2 PROG P0.3 NC   NC                                            |
  |                                                                          |
  |   3V3                                                                    |
  |   o    o    o    o    o    o                                             |
  |   VDD  P0.1 P0.0 P1.6 P1.5 NC                                            |
  |  +--------------------------------------------------------------------+  |
  |  |                                                                    |  |
  |  |                                                                    |  |
  |  |                                                                    |  |
  |  |                                                                    |  |
  |  |                                                                    |  |
  |  +--------------------------------------------------------------------+  |
  |   GND  3V3                                                               |
  |   o    o    o    o    o    o                                             |
  |   GND  VDD  P0.6 P1.1 P1.3 RESET                                         |
  |                                                                          |
  |   Rx             SCL  SDA  Tx-Off?                                       |
  |   o    o    o    o    o    o                                             |
  |   P0.4 P0.5 P0.7 P1.0 P1.2 P1.4                                          |
  | O (Hole)                                                                 |
  +--------------------------------------------------------------------------+

P1.4 (Tx-Off?) goes to the amplifier chip in the NRF module. It has short pulses
that repeat with about 200Hz. Speculation: this may be a kind of enable 
signal to the amplifier, turning it on only when the NRF chip actually sends
packets to preserve power. 

Programming pins:

- P0.7: FMOSI
- P1.0: FMISO
- P1.0: FCSN
- PROG

The X-3S uses different pins to connect to the EEPROM:

- P1.1 = SDA
- P1.3 = SCL



Serial port protocol
===============================================================================

The serial port runs at **19200,N,8,1**.

Data is only sent **to** the NRF module. The Tx line of the NRF module
is connected to the MCU in the transmitter, but there is no data being sent at 
all.

Data is sent to the NRF module in **packets of 15 bytes**. The first three bytes
indicate the packet type. There are two packet types in use:

:``ff 55 aa``: Stick data and Failsafe 
:``ff aa 55``: Model number

The Stick data and Failsafe packet structure is as follows:

:Bytes 0..2:    Header (``ff 55 aa``)
:Bytes 3..8:    Payload
:Bytes 9, 10:   CRC16-CCITT of Payload
:Bytes 11, 12:  Not used, seems to contain junk data. For stick data it is 
                influenced by steering wheel only (not trim, etc!)
                
                - Left:   ``87 7c``
                - Centre: ``e6 08``
                - Right:  ``22 7c``

                For Failsafe the value changes depending on which buttons are 
                pressed.
:Bytes 13, 14:  16-bit sum of bytes 3 to 10, MSB first

The first payload byte is always ``aa``. The high nibble of byte 4 is ``a`` for
Stick data or ``b`` for Failsafe.

Packets that are of type Stick data and Failsafe are repeated every **13.28ms**
(75.3Hz).
Note that this does not match the output rate of the receiver, which is 16ms 
(62.5Hz).



Checksum algorithms 
---------------------------------------

Data is protected with two checksums.

The whole packet travelling over the serial port is protected with a
simple 16-bit sum of the bytes 3 to 10. MSB is stored in byte 13, LSB in byte
14. Note that bytes 11 and 12 are not protected, they don't seem to be in
use and seem to leak internal information of the MCU.

The Stick data and Failsafe packets include a checksum using the CRC16-CCITT 
algorithm, ``0x1021`` polynominal. This checksum is most likely sent to the 
receiver. 

The checksum is calculated over the payload bytes 3..8, and is stored in
bytes 9 (MSB) and 10 (LSB).



Stick data
---------------------------------------

Each channel is a 12 bit number. The highest nibbles are packed in bytes
4 and 5, the low bytes are in bytes 6..8. 

The value being transmitted is offset by 350us in the receiver.
For example, a value of 650 translates to a 1000us pulse being output, a value
of 1650 outputs a 2000us pulse. 

This implies that the full range ``0..fff`` translates into pulses between
350 and 4445us. Worst case (three channels times 4.445ms) would be 13.34ms.


::

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
                         yy yy yy yy yy yy                   
                                       checksum bytes: CRC16 = cc cc
                                       checksum bytes: sum = kk kk


Failsafe
---------------------------------------

Failsafe packets are only sent if the failsafe function is enabled for the
current model. 

Failsafe packets are transmitted after every 14 stick data transmissions.

::

             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
            -------------------------------------------- 
            ff 55 aa aa bb cc 78 78 06 c9 e2 a5 09 04 d2
            --------------------------------------------
                              ss tt mm cc cc XX XX kk kk
                     yy yy yy yy yy yy                   checksum bytes: CRC16 = cc cc
                     xx xx xx xx xx xx xx xx             checksum bytes: sum = kk kk


:ss, tt:    Steering, throttle in percent. 
            0x78 means 0%, 0x00 means -120%, 0xf0 means +120%

:mm:        bit mask whether which channel is enabled for failsafe:
            bit 0: steering
            bit 1: throttle
            bit 2: always 1 (CH3?)


The percentage value translates into the following pulse timings on the
respective servo output::
            
    +120%   +100%       0%    -100%    -120%
     784us   904us   1540us   2120us   2240us



Model number
---------------------------------------

This packet is sent after power on and every time a model is changed.

It is repeated three times every 46.4ms, and often disrupts an ongoing 
transmission, causing CRC errors -- which is most likely the reason for
repeating it three times.

Changing a model takes 197.3ms, then 3 model number commands are sent,
and then the first Stick data (or Failsafe) packet after 168.1ms.

::

             0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
            -------------------------------------------- 
            ff aa 55 00 02 07 00 00 21 10 42 20 63 30 84 
            ff aa 55 00 02 07 00 00 21 10 42 20 63 30 84 
            ff aa 55 00 02 07 00 00 21 10 42 20 63 30 84 
                        mm


:mm:     model code. mod0 = 0x02, mod15 = 0x11
:rest:   unknown, but constant data independent of the model number

The model code serves as index into the code data stored in the EEPROM
that is connected to the NRF module.
          



EEPROM contents
===============================================================================

The EEPROM connected to P0.7 (SCL) and P1.0 (SDA) contains binding data.
**Note**: the EEPROM is connected to different pins on the X-3S: P0.6 = SDA, 
P1.1 = SCL

There is a 25 byte code for each model, starting at address 0.
These 25 bytes correspond with the data we read from the EEPROM in the
receiver.

Note that only the first 6 bytes contain random values, the rest are padding 
(value increments from the last random byte value onwards. Note sure if only the 
first 6 bytes are transmitted over the air, or all 25 bytes.
The data found in the receiver EEPROM matches all 25 bytes, but that could
just be done as a kind of checksum.

Furthermore, address ``0x19a`` and ``0x19b`` contain the value ``0xaa``. 
It is not known whether this information is used, but it is present in both 
HK310 and X-3S.


