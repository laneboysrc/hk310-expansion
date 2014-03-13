/*****************************************************************************

    analog-ch3
    
    This program converts CH3 of the HK310 transmitter from a two-position
    switch into an analog channel.
    
    It is designed for the PIC12F1840 8-pin microcontroller.
    
    Port usage:
    
    RA0:  IN  (7)  [ICSPDAT]
    RA1:  IN  (6)  [ICSPCLK]
    RA2:  AN  (5)  Analog input for a potentiometer
    RA3:  IN  (4)  [VPP]
    RA4:  OUT (3)  UART TX (to HK310 NRF module
    RA5:  OUT (2)  UART RX (from HK310 MCU)

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
    APFCON0 = 0b11000000;   // Use RA4/RA5 for UART TX/RX
    
    ADCON0 = 0b00001001;    // ADC on, measure RA2
    ADCON1 = 0b11110000;    // Right justified, use FRC, VREF is VDD
    
    INTCON = 0;
}


/*****************************************************************************
 Returns the next 12-bit value to transmit over CH3
 ****************************************************************************/
uint16_t next_CH3_value() 
{
    uint16_t adc;

    // Perform the ADC conversion. Takes 1..6us only
    GO = 1;
    while (GO);

    adc = (ADRESH << 8) | ADRESL;

    // The range of 1000us to 2000us on a servo output corresponds to the range
    // 1650 .. 650, with 1150 being neutral. Since the range of 1000 is almost  
    // identical to 10 bits (1023) we simply use a 1:1 linear mapping with 
    // appropriate offset that 512 (max of 10 bit range divided by 2) gives
    // neutral.
    return (1150 - 512) + adc;
    
    // Exercise for the reader:
    // It could be desireable to be able to adjust the endpoints. This could 
    // be achieved by using the existing CH3 endpoints in the transmitter.
    // The user would have to set the end point in the transmitter, and our 
    // software would track that endpoint, storing it in the EEPROM persistently
    // Of course the user would have to manually move the CH3 switch to ensure
    // that our software can learn both endpoints.
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




