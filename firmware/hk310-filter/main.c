#include <pic16f1825.h>
#include <stdint.h>

#include "hk310-filter.h"
#include "uart.h"
#include "keyboard-matrix.h"


static __code uint16_t __at (_CONFIG1) configword1 = _FOSC_INTOSC & _WDTE_OFF & _PWRTE_ON & _MCLRE_OFF & _CP_OFF & _CPD_OFF & _BOREN_OFF & _CLKOUTEN_OFF & _IESO_OFF & _FCMEN_OFF;
static __code uint16_t __at (_CONFIG2) configword2 = _WRT_OFF & _PLLEN_OFF & _STVREN_OFF & _LVP_OFF; 


#define SYNC_VALUE_NOMINAL 0xa40

uint16_t payload;


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
 We use bit 5 of the 2nd and 3rd payload byte to maximize the difference
 between adjacent payload values. This is also needed because two neighboring
 payload values could be the same value, which means the receiving and
 can't distinguish between them. 
 
 We calculate the difference between the previous value and the current on,
 and compare it with the difference between the previous value and the 
 current one with bit 5 set. Whichever gives a larger difference is used.
 ****************************************************************************/
uint8_t maximizeDifference(uint8_t old, uint8_t new)
{
    uint8_t diff1;   
    uint8_t diff2;   
    uint8_t new2;   
    
    new2 = new | (1 << 5);
    
    diff1 = (old >= new) ? old - new : new - old;
    diff2 = (old >= new2) ? old - new2 : new2 - old;
    
    if (diff2 > diff1) {
        return new2;
    }
    return new;
    
}


//static uint16_t binary_counter = 0;
static uint8_t count = 0;
static uint8_t index = 0;
/*****************************************************************************
 Returns the next 12-bit value to transmit over CH3
 ****************************************************************************/
uint16_t next_CH3_value() 
{
    static uint16_t new_ch3;
    uint16_t rx;
    uint8_t values[4];


    // Scan the keyboard. Only set bits here, they are cleared whenever
    // we've sent the respective data to the receiver. This way
    // we can deal with short button pushes even though the reaction
    // time is ~175ms.    
    payload |= Scan_keyboard();
    //payload = binary_counter;


    // Each of the 4 value numbers is sent for 3 times so we can assure
    // that the receiver has seen it at least once. This is necessary as the
    // MCU, NRF module and receiver run asynchronously.
    if (count == 0) {

        values[0] = 0;
        values[1] = (payload >> 0) & 0x3f;
        values[2] = maximizeDifference(values[1], (payload >> 6) & 0x1f);
        values[3] = maximizeDifference(values[2], (payload >> 11) & 0x1f);
        
        // Clear the bits that we have sent accross. See above why we do this.
        if (index == 1) {
            payload &= 0xffc0;
        }
        if (index == 2) {
            payload &= 0xf83f;
        }
        if (index == 3) {
            payload &= 0x07ff;
        }


        if (index == 0) {
            rx = SYNC_VALUE_NOMINAL;
            //++binary_counter;
        }
        else {
            rx = values[index] << 5;
            rx += 0x20;         // Add 0x20 to avoid tiny pulse durations
            rx += (rx >> 8);    
        }

        new_ch3 = (2720 - 1 - rx) * 16 / 17;

        ++index;
        if (index > 3) {
            index = 0;
        }
    }

    ++count;
    if (count >= 3) {
        count = 0;
    }

    return new_ch3;
}


/*****************************************************************************
 main()
 
 No introduction needed ... 
 ****************************************************************************/
void main(void) {
    Init_hardware();
    Init_UART();
    Init_keyboard();
    
    while (1) {
        uint8_t b;
        b = UART_read_byte();
        b = filter(b);
        UART_send(b);
    }
}




