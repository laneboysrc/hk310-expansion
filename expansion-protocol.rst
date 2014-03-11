Protocol for channel expansion on the HobbyKing HK310 and Turnigy X-3S
###############################################################################



Introduction
===============================================================================

This document describes the design utilized to get more "channels" out of the 
3-channel HobbyKing HK310 and Turnigy X-3S RC radios.

The goal is to extend the system with buttons and switches for special
functions like switching lights, horns, controlling a sound module, pump
for a firetruck, etc.

It is also desireable to control additional servos, for example for the spray
nozzle on a firetruck.



Getting custom data transmitted
===============================================================================

Referring to hk310-info, the MCU in the transmitter connects to the NRF module
via a serial port running at 19200,N,8,1.

We can simply hook an additional microcontroller between the MCU and the NRF
module and change/inject the data. This way we can take control of channel 3, 
which we can use to transmit any data we like.

::


                                 +---------------+ 
       New buttons       /       |               | 
       and switches  ---/  ------|      MCU      | 
                         /    +--|               | 
                     ---/  ---+  +---------------+ 
                                     Rx|   |Tx
    +---------------+                  ^   v             +---------------+
    |               |                  |   |             |               |
    |      MCU      |---->----->----->-- X -->----->-----|   NRF module  |
    |               |                    \               |               |
    +---------------+                     cut            +---------------+



Note that in the HK310 the MCU runs at 5V, while the NRF module runs at 3.3V.
In the X-3S both MCU and NRF module run at 3.3V.

Since we can send out a byte immediately after we received it (instead of
waiting for all 15 bytes that are in a packet), the delay introduced is 
only 520us, which is negligable considering packets are repeated only every 
13.28ms.



Analyzing the transmission path
===============================================================================

As outlined in hk310-info, the pulse that we read from the receiver is jittery
and has an occasional glitch. These errors are certainly caused by the 
receiver generating the pulse as we can assume that the digital path
over the air is protected by an error detection scheme.

In the long run it may be worth investigating whether it is possible to alter
the receiver (firmware) to generate more precise pulses, or even output
raw data (for example via SPI, or a UART). For now we have to live with 
the unreliable pulses.

Sending stick data value in the range of 0x000 to 0x9ff causes unique pulse
durations in the output of the receiver. Since the lower five bits are 
unreliable we are able to extract slightly more than 6 bits (0x00 to 0x9e, 
with 0x00 to 0x7e being exactly 7 bits).

Another issue of the transmission path is that the various modules involved
in sending and receiving data run asynchronously at different frequencies:

- The MCU sends data to the NRF module every **13.28ms**
- The NRF module sends information to the receiver at an unknown interval (every 5ms?)
- The receiver generates servo pulses every **16ms**

Since transmitter and receiver run asynchronously we need to design our
protocol to include a form of synchronization if we want to transmit more than
6 bits. The jitter and glitches will also require filtering, which will delay 
the output.



Other issues 
===============================================================================

The PIC microcontroller we intend to use as decoder has a built-in RC oscillator
that is factory tuned to +/-1%. While this is quite precise, it is still not good
enough for our application (1% of 12 bits in our data channel causes an error of 
40, more than 5 bits!).

There are different solutions that can be applied to resolve this issue:

- Tap into the crystal oscillator present in the receiver
- Use a separate crystal oscillator
- Calibrate the RC oscillator

The 16 MHz of the crystal oscillator in the receiver would work perfectly for
our application. However, the signal available on the X2 output of the NRF 24LE1
chip is not accepted by the PIC, which would require a square wave input clock.

A separate crystal oscillator would work fine as well, but would add cost and
make bread-boarding difficult too.

Calibrating the RC oscillator was the method finally chosen. Since we wanted
to transmit more than 6 bits anyway, we needed to implement a sync mechanism.
The chosen sync value is a pulse duration of 0xa40 (2624us). This value is 
above the data range of 6 bits payload and 5 bit reserved for glitches and 
jitter, so payload will never have such value. It is also a very large pulse, 
making it useful as reference.

Since the oscillator is factory calibtrated to 1%, the inital pulse measurement
should be 0xa25 .. 0xa5a. Even worst case a valid payload would be only 0x813,
so the firmware can reliably detect the sync value. The firmware will then
adjust the OSCTUNE register in steps until the sync value is 0xa40 (+/-3).
The firmware can constantly monitor and tweak OSCTUNE so that potential drifts
at run-time are adjusted for.
This method also has the advantage that any inaccuracy of the NRF24LE1 clock
is also compensated.



Protocol
===============================================================================

The data sent over the CH3 data stream is as follows:

    SYNC PAYLOAD1_6 PAYLOAD2_5 ... PAYLOADn_5 SYNC PAYLOAD1_6 ...

The protocol is a repeating series of values. The range of the values is
0x00..0x3f (6 bits), plus the special value 0xa40 used as SYNC value.

On the transmitter side the payload values correspond to the number sent to 
the NRF module as follows::

    v = value << 5
    v = v + 0x20
    v = v + (v >> 8)
    tx_value = (2720 - v - 1) * 16 / 17

Note that we add an offset of 0x20 as to prevent that we generate extremely
short pulses on the CH3 output of the receiver.

On the receiver side it is simply a matter of subtracting the offset 0x20 and 
shifting the pulse duration, measured with 1 microsecond resolution, to the 
right by 5. 

Since there is no synchronization between transmitter and receiver, the
receiver has to determine which pulse belongs to which position in the 
protocol. This is done by ensuring that no **consecutive** values are the same.

- Since SYNC is a number outside of the payload range, this condition is
  guaranteed in all cases.

- PAYLOAD1_6 has bits 0..5 dedicated to the payload, hence can have
  any value between 0x00 and 0x3f. 

- PAYLOAD2_5 .. PAYLOADn_5 have bits 0..4 dedicated to the  
  payload. Bit 5 is chosen as that the difference with the previous 6-bit 
  value is as large as possible. With this algorithm we have a difference of
  at least 512us (0x200) between neighboring values, which helps with 
  detecting and recovering from glitches.
  
Because we have to deal with glitches and asynchronicity, the transmitter is
repeating every value 2 times. This means the response time of the 
received values is as follows:

SYNC + 1 payload (6 bits)
        ~79-96ms

SYNC + 2 payload (11 bits)
        ~95-144ms

SYNC + 3 payload (16 bits)
        ~159-193ms

(approx 60ms per value)



Driving servos
===============================================================================

The payload can transmit any kind of data, so it is possible to use a number
of bits in the payload and use them to generate a servo pulse. 

One has to consider resolution and response time. As described in the previous
section, the response time is as low as 200ms for a 16-bit total payload. 
This means that the servo will follow the input with a very significant delay,
and large jumps -- certainly not useful for steering or throttle, but possibly
suitable for auxillary functions like the nozzle on a firetruck.

One may get away with 5 or 6 bit resolution as end-points and neutral could 
be programmed in the decoder. 

Servos may also be controlled with up/down or left/right buttons, moving
the servo a step at a time.

If the additional servos are mutually exclusive with steering and throttle (i.e.
you don't need to drive the vehicle when the additional servos are in use), then
the decoder can also be used to multiplex them. For example, when a switch
is in position A on the transmitter then the decoder would route throttle and
steering to the actual throttle and steering output, but when the switch is
in position B the decoder will route it to servo output 3 and 4. 
Note that the steering/throttle input signal may come from the original MCU in 
the transmitter, or from sticks etc connected directly to the encoder we 
added between the MCU and the NRF module.

Another potential use-case could be a servo output that follows the original 
steering servo, but only when a switch is in a certain position. This could 
be useful for 4-wheel steered vehicles, where we have a switch that lets us
choose 4-wheel steering, 2-wheel steering, and crab mode.

In a similar manner a dig can be implemented for rock crawlers with two motors
and speed controllers.

