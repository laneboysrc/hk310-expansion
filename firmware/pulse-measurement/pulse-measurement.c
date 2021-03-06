/******************************************************************************

   Test program to measure the CH3 pulse width and output its value as
   decimal number on the UART output (38400/n/8/1)

   This program runs on the DIY RC Light Controller hardware using a 
   PIC16F1825 and a TLC5940.
   Refer to https://github.com/laneboysrc/rc-light-controller
   for a desciption of the DIY RC Light Controller project.

   Use SDCC (http://sdcc.sourceforge.net/) to compile it.
       
   Port usage:
   ===========                                             
   RA1:  IN  (12) (UART Rx), ICSPCLK
   RA0:  IN  (13) ICSPDAT
   RA3:  IN  ( 4) CH3 input using T1G; Vpp
   RC4:  OUT ( 6) UART Tx

   RC0:  OUT (10) SCLK TLC5940
   RC2   OUT ( 8) SIN TLC5940 (SDO on 16F1825)
   RC1:  IN  ( 9) Reserved for SPI EEPROM (SDI on 16F1825)
   RA4:  OUT ( 3) XLAT TLC5940
   RA5:  OUT ( 2) BLANK TLC5940
   RC5:  OUT ( 5) GSCLK TLC5940

   RA2:  OUT (11) Reserved for audio out (PWM using CCP3) 
   RC3:  OUT ( 7) Reserved for SPI EEPROM (CS)

*/
#include <pic16f1825.h>
#include <stdint.h>


static __code uint16_t __at (_CONFIG1) configword1 = _FOSC_INTOSC & _WDTE_OFF & _PWRTE_ON & _MCLRE_OFF & _CP_OFF & _CPD_OFF & _BOREN_OFF & _CLKOUTEN_OFF & _IESO_OFF & _FCMEN_OFF;
static __code uint16_t __at (_CONFIG2) configword2 = _WRT_OFF & _PLLEN_OFF & _STVREN_OFF & _LVP_OFF; 

extern void Init_input(void);
extern void Read_input(void);

extern void Init_output(void);
extern void Output_result(void);

extern void Init_TLC5940(void);
extern void TLC5940_send(void);
extern uint8_t light_data[16];

extern void Process_value(void);

extern struct {
    unsigned locked : 1;
    unsigned dataChanged : 1;
} flags;

extern uint8_t receivedValues[3];

#define LED_LOCKED 12
#define LED_DATA_CHANGED 13
#define LED_BLINK 14
#define LED_SYNC 15

#define LED_VALUE 0x0f  // Medium brightness

/*****************************************************************************
 Init_hardware()
 
 Initializes all used peripherals of the PIC chip.
 ****************************************************************************/
static void Init_hardware(void) {
    //-----------------------------
    // Clock initialization
    OSCCON = 0b01111000;    // 16MHz: 4x PLL disabled, 8 MHz HF, Clock determined by FOSC<2:0>

    //TODO: store OSCTUNE value in EEPROM!
    //OSCTUNE = 0b001011;

    //OSCTUNE = 0b011111;     // max freq  a3c   changes 6.5% full scale?
    //OSCTUNE = 0b000000;     // nominal:  9e5 
    //OSCTUNE = 0b100000;     // min freq: 990
    
    
    //-----------------------------
    // IO Port initialization
    LATA = 0;
    LATC = 0;
    ANSELA = 0;
    ANSELC = 0;
    TRISA = 0b00001011;     // Make all ports A except RA0, RA1 and RA3 output
    TRISC = 0b00000010;     // Make all ports C except RC1 output
    APFCON0 = 0b10001000;   // Use RC4/RA1 for UART TX/RX; RA3 for T1G
    APFCON1 = 0;    
    
    INTCON = 0;
}


/*****************************************************************************
 ****************************************************************************/
void Output_LEDs(void) 
{
    uint8_t i;
    uint8_t mask;
    static uint8_t blink;
    static uint8_t cachedValues[3];
    
    if (flags.dataChanged) {
        for (i = 0; i < 3; i++) {
            cachedValues[i] = receivedValues[i];
        }
    }
    
    for (i = 0; i < 16; i++) {
        light_data[i] = 0x00;
    }

    mask = 1;
    for (i = 0; i < 5; i++) {
        light_data[i] = cachedValues[2] & mask ? LED_VALUE : 0;
        mask = mask << 1;
    }

    mask = 1;
    for (i = 6; i < 10; i++) {
        light_data[i] = cachedValues[1] & mask ? LED_VALUE : 0;
        mask = mask << 1;
    }

    mask = 1;
    for (i = 11; i < 12; i++) { // we only have 12 LEDs available...
        light_data[i] = cachedValues[2] & mask ? LED_VALUE : 0;
        mask = mask << 1;
    }
    
    if (flags.dataChanged) {
        light_data[LED_DATA_CHANGED] = LED_VALUE;
    }

    if (flags.locked) {
        light_data[LED_LOCKED] = LED_VALUE;
    }

    if (((TMR1H << 8) | TMR1L) > 0x880) {
        light_data[LED_SYNC] = LED_VALUE;
    }

    if (++blink & 0x20) {
        light_data[LED_BLINK] = LED_VALUE;
    }
    
    TLC5940_send();    
}


/*****************************************************************************
 main()
 
 No introduction needed ... 
 ****************************************************************************/
void main(void) {
    Init_hardware();
    Init_output();
    Init_input();
    Init_TLC5940();
    
    while (1) {
        Read_input();
        Process_value();
        Output_result();
        Output_LEDs();
    }
}




