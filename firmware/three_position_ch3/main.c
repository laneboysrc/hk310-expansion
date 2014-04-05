/*****************************************************************************

    three-position-ch3
    
    This program converts CH3 of the HK310 transmitter from a two-position
    switch to a three-position switch.
    
    It is designed for the PIC12F1840 8-pin microcontroller.
    Use SDCC (http://sdcc.sourceforge.net/) to compile it.
    
    Port usage:
    
    RA0:  IN  (7)  [ICSPDAT]
    RA1:  IN  (6)  [ICSPCLK]
    RA2:  AN  (5)  Switch in
    RA3:  IN  (4)  [VPP]
    RA4:  OUT (3)  UART TX (to HK310 NRF module
    RA5:  OUT (2)  UART RX (from HK310 MCU)
    
    The switch is connected in such a way that the outside contacts are 
    GND and Vdd, and the middle contact (which is now either GND, Vdd or open)
    to RA2. In addition two resistors of ~100k value need to be placed
    over the switch terminals to form a voltage divider, with the tap
    on RA2. This enabled us to detect whether the swithc is open.


                     ^ Vdd              ^ Vdd
                     |                  |
                    +-+                 |
                100k| |         ___     |    1 pole 3 position switch
        -----+      | |          |      |   
        PIC  |      +-+          |_/ o--+  
             |       |          _/       
         RA2 +-------o-------o-/   
             |       |    
             |      +-+              o--+    
        -----+      | |                 |
                100k| |                 |
                    +-+                 |
                     |                  |
                    --- Vss            --- Vss (GND)


*****************************************************************************/


#include <pic12f1840.h>
#include <stdint.h>

#include "hk310-filter.h"
#include "uart.h"


static __code uint16_t __at (_CONFIG1) configword1 = _FOSC_INTOSC & _WDTE_OFF & _PWRTE_ON & _MCLRE_OFF & _CP_OFF & _CPD_OFF & _BOREN_OFF & _CLKOUTEN_OFF & _IESO_OFF & _FCMEN_OFF;
static __code uint16_t __at (_CONFIG2) configword2 = _WRT_OFF & _PLLEN_OFF & _STVREN_OFF & _LVP_OFF; 


/*****************************************************************************
 Init_hardware()
 
 Initializes all used peripherals of the PIC chip.
 ****************************************************************************/
static void Init_hardware(void) {
    //-----------------------------
    // Clock initialization
    OSCCON = 0b01111000;    // 16MHz: 4x PLL disabled, 8 MHz HF, Clock determined by FOSC<2:0>
    
    //-----------------------------
    // IO Port initialization
    TRISA = 0b11111111;     // Make all ports A input
    ANSELA = 0b00000100;    // RA2 is analog in, all others are digital I/O
    APFCON = 0b10000100;    // Use RA4/RA5 for UART TX/RX
    
    ADCON0 = 0b00001001;    // ADC on, measure RA2
    ADCON1 = 0b01110000;    // Left justified, use FRC, VREF is VDD
    
    INTCON = 0;
}


/*****************************************************************************
 Returns the next 12-bit value to transmit over CH3
 ****************************************************************************/
uint16_t next_CH3_value() 
{
    uint8_t adc;

    // Perform the ADC conversion. Takes 1..6us only
    GO = 1;
    while (GO);

    adc = ADRESH;       // We only use the most significant 8 bits
    
    // If we are close to Vdd / Vss send the endpoints accordingly, otherwise
    // send the neutral position as the switch must be open. 
    if (adc > 200) {
        return 650;    
    }
    if (adc < 55) {
        return 1650;    
    }
    return 1150;   

    // Exercise for the reader:
    // The endpoints could track the existing CH3 endpoints in the transmitter.
    // The user would have to set the end point in the transmitter, and our 
    // software would track that endpoint, storing it in the EEPROM persistently
    // Of course the user would have to manually move the CH3 switch to ensure
    // that our software can learn both endpoints.
    //
    // How could we adjust the center position without an additional 
    // user interface?
}


/*****************************************************************************
 main()
 
 No introduction needed ... 
 ****************************************************************************/
void main(void) {
    Init_hardware();
    Init_UART();
    
    while (1) {
        uint8_t b;
        b = UART_read_byte();
        b = filter(b);
        UART_send(b);
    }
}




