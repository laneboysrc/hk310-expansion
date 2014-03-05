#include <pic16f1825.h>
#include <stdint.h>

#include "uart.h"
#include "keyboard-matrix.h"

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
    ANSELA = 0;
    ANSELC = 0;
    APFCON0 = 0b00000000;   // Use RC4/RC5 for UART TX/RX; RA4 for T1G
    APFCON1 = 0;    
    TRISA = 0b11111111;     // Make all ports A input
    TRISC = 0b11111111;     // Make all ports C input
    
    INTCON = 0;
}


/*****************************************************************************
 main()
 
 No introduction needed ... 
 ****************************************************************************/
void main(void) {
    static uint16_t oldKeys = 0xffff;
    uint16_t keys;

    Init_hardware();
    Init_UART();
    Init_keyboard();
    
    while (1) {
        uint8_t b;
        
        keys = Scan_keyboard();
        if (keys != oldKeys) {
            UART_send_binary_uint(keys);
            oldKeys = keys;
        }
        
        if (UART_receive_byte_pending()) {
            b = UART_read_byte();
            UART_send('E');
            UART_send('c');
            UART_send('h');
            UART_send('o');
            UART_send(':');
            UART_send(' ');
            UART_send(b);
            UART_send('\n');
        }
    }
}




