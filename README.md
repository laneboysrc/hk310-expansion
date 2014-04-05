Hacking the HobbyKing HK310 to modify the channels
===========

With the information found in this project you can modify a
**HobbyKing HK310**  or a **Turnigy X-3S** transmitter to send arbitrary
information on the three channels to the receiver. This allows for example to:

- Change the **AUX channel** from a two positin switch to a **three position switch**

- Change the **AUX channel** to an **analog channel**

- Perform **custom mixing** on any of the channels

- Transmit **arbitrary data over the AUX channel** to a custom decoder as to
  expand the system to a large number of on/off channels, e.g. to control
  lights and accessories on a scale RC truck

- **Build your own custom transmitter** by just reusing the wireless
  module

Sample programs for the **Arduino pro mini** are provided.

Start by reading [hk310-info.md](hk310-info.md), which describes how 
the HK310 works and how we can modify it.

The `./logs` directory contains dumps of the serial communication within the
HK310, which was used to decode and understand the system.

The document [expansion-protocol.md](expansion-protocol.md) proposes
a way of how to add 16 on/off channels to the system.

The `./firmware` directory contains source code for various sample programs, 
such as changing the AUX channel to a three position switch or an analog
channel. Code is available for **Arduino** as well as for **Microchip PIC**.

`./transmitter-eeprom-contents` and `./receiver-eeprom-contents` contain dumps 
of the EEPROM that is connected to the NRF module and in the receiver 
respectively, which contains the unique binding data.

Several Python programs are available that are useful to work directly 
on a PC without having to use a microcontroller.






