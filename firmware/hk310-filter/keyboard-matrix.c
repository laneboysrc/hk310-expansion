#include <pic16f1825.h>
#include <stdint.h>

#include "keyboard-matrix.h"

/*
    Routines for handling a 12 key matrix
    
    Rows:    RA2, RA4, RA5        
    Columns: RC0, RC1, RC2, RC3   
*/


void Init_keyboard(void)
{
    WPUA = 0b00110100;  // Weak pull-ups on RA2, RA4 and RA5
    WPUC = 0b00001111;  // Weak pull-ups on RC0, RC1, RC2 and RC3
    NOT_WPUEN = 0;
    
    TRISA &= ~0b00110100;   // Ports RA2, RA4 and RA5 outputs
    TRISC |= 0b00001111;    // Ports RC0, RC1, RC2 and RC3 inputs
    
    LATA2 = 0;
    LATA4 = 0;
    LATA5 = 0;
}


uint16_t Scan_keyboard(void)
{
    uint16_t val = 0;
    static uint8_t keyPressed = 0;
    
    if (!keyPressed) {
        LATA2 = 0;
        LATA4 = 0;
        LATA5 = 0;
    
        if ((PORTC & 0x0f) != 0x0f) {
            keyPressed = 1;
        }
    }
    
    if (keyPressed) {
        LATA4 = 1;
        LATA5 = 1;
        LATA2 = 0;
        val = (~PORTC & 0x0f) << 8; 

        LATA2 = 1;
        LATA4 = 0;
        val |= (~PORTC & 0x0f) << 4;

        LATA4 = 1;
        LATA5 = 0;
        val |= (~PORTC & 0x0f);
        
        if (val == 0) {
            keyPressed = 0;
        }
    }
        
    return val;
}

