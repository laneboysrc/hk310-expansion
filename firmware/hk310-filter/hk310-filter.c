#include <pic16f1825.h>
#include <stdint.h>

#include "uart.h"
#include "keyboard-matrix.h"


static __code uint16_t __at (_CONFIG1) configword1 = _FOSC_INTOSC & _WDTE_OFF & _PWRTE_ON & _MCLRE_OFF & _CP_OFF & _CPD_OFF & _BOREN_OFF & _CLKOUTEN_OFF & _IESO_OFF & _FCMEN_OFF;
static __code uint16_t __at (_CONFIG2) configword2 = _WRT_OFF & _PLLEN_OFF & _STVREN_OFF & _LVP_OFF; 


#define SYNC_VALUE_NOMINAL 0xa40

#define STATE_WAIT_FOR_SYNC 0
#define STATE_SYNC2 1
#define STATE_SYNC3 2
#define STATE_PAYLOAD3 3
#define STATE_PAYLOAD4 4
#define STATE_PAYLOAD5 5
#define STATE_PAYLOAD6 6
#define STATE_PAYLOAD7 7
#define STATE_PAYLOAD8 8
#define STATE_CRC_H 9
#define STATE_CRC_L 10
#define STATE_GARBAGE1 11
#define STATE_GARBAGE2 12
#define STATE_CHECKSUM_H 13    
#define STATE_CHECKSUM_L 14    
#define STATE_SKIP 15

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
 Add byte 'b' to the CRC16-CCITT checksum.
 ****************************************************************************/
uint16_t crc16_ccitt(uint16_t crc16, uint8_t b) 
{
    uint8_t i;
    
    crc16 = crc16 ^ b << 8;

    for (i = 0; i < 8 ; i++) {
        if (crc16 & 0x8000) {
            crc16 = crc16 << 1 ^ 0x1021;
        }
        else {
            crc16 = crc16 << 1;
        }
    }
    return crc16;
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


static uint16_t c = 0;
/*****************************************************************************
 Returns the next 12-bit value to transmit over CH3
 ****************************************************************************/
uint16_t nextValue() 
{
    static uint8_t index = 0;
    uint16_t rx = SYNC_VALUE_NOMINAL;
    uint8_t values[4];

    //payload = c;

    values[0] = 0;
    values[1] = (payload >> 0) & 0x3f;
    values[2] = maximizeDifference(values[1], (payload >> 6) & 0x1f);
    values[3] = maximizeDifference(values[2], (payload >> 11) & 0x1f);
    
    // Clear the bits that we have sent accross
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
        ++c;
    }
    else {
        rx = values[index] << 5;
        rx += 0x20;         // Add 0x20 to avoid tiny pulse durations
        rx += (rx >> 8);    
    }

    ++index;
    if (index > 3) {
        index = 0;
    }

    return (2720 - 1 - rx) * 16 / 17;
}
    
    
static uint8_t state = STATE_WAIT_FOR_SYNC;
static uint16_t new_ch3 = (2720 - 1 - SYNC_VALUE_NOMINAL) * 16 / 17;
static uint8_t count = 0;
/*****************************************************************************
 filter()
 
 Takes a byte received from the MCU, processes it, and outputs a byte to be 
 sent to the NRF module.
 
 The process is aware of the protocol between the MCU and the NRF module so
 that it can alter data in a meaningful way. Currently it modifies the CH3
 value, and the checksums as needed.
 
 Input: b: byte to process
 ****************************************************************************/
uint8_t filter(uint8_t b)
{
    static uint8_t skip;
    static uint16_t new_checksum;
    static uint16_t new_crc16;



    switch (state) {
        case STATE_WAIT_FOR_SYNC:
            if (b == 0xff) {
                state = STATE_SYNC2;
            }
            break;
            
        case STATE_SYNC2:
            if (b == 0x55) {
                state = STATE_SYNC3;
            }
            else if (b != 0xff) {
                state = STATE_WAIT_FOR_SYNC;
            }
            break;
            
        case STATE_SYNC3:
            if (b == 0xaa) {
                state = STATE_PAYLOAD3;
            }
            else if (b == 0xff) {
                state = STATE_SYNC2;
            }
            break;
            
        case STATE_PAYLOAD3:
            if (b == 0xaa) {
                state = STATE_PAYLOAD4;
            }
            else {
                state = STATE_SKIP;
                skip = 11;
            }
            new_checksum = (int)b;
            new_crc16 = crc16_ccitt(0, b);
            break;
            
        case STATE_PAYLOAD4:
            if ((b & 0xf0) == 0xa0) {
                // High nibble of steering
                state = STATE_PAYLOAD5;
            }
            else {
                state = STATE_SKIP;
                skip = 10;
            }
            new_checksum += (int)b;
            new_crc16 = crc16_ccitt(new_crc16, b);
            break;
            
        case STATE_PAYLOAD5:
            {
                uint8_t t;
                uint8_t ch3;
                t = b >> 4; 
                ch3 = b & 0x0f;

                // High nibble of throttle and CH3
                ch3 = (new_ch3 >> 8) & 0x0f;
                
                b = (t << 4) + ch3;
            }
            state = STATE_PAYLOAD6;
            new_checksum += (int)b;
            new_crc16 = crc16_ccitt(new_crc16, b);
            break;
            
        case STATE_PAYLOAD6:
            // Low byte of steering

            state = STATE_PAYLOAD7;
            new_checksum += (int)b;
            new_crc16 = crc16_ccitt(new_crc16, b);
            break;
            
        case STATE_PAYLOAD7:
            // Low byte of throttle

            state = STATE_PAYLOAD8;
            new_checksum += (int)b;
            new_crc16 = crc16_ccitt(new_crc16, b);
            break;
            
        case STATE_PAYLOAD8:
            // Low byte of CH3
            b = new_ch3 & 0xff;
            
            state = STATE_CRC_H;
            new_checksum += (int)b;
            new_crc16 = crc16_ccitt(new_crc16, b);
            break;
            
        case STATE_CRC_H:
            state = STATE_CRC_L;
            b = new_crc16 >> 8;
            new_checksum += (int)b;
            break;
            
        case STATE_CRC_L:
            b = new_crc16 & 0xff;
            state = STATE_GARBAGE1;
            new_checksum += (int)b;
            break;
            
        case STATE_GARBAGE1:
            state = STATE_GARBAGE2;
            break;

        case STATE_GARBAGE2:
            state = STATE_CHECKSUM_H;
            break;
            
        case STATE_CHECKSUM_H:
            b = new_checksum >> 8;
            state = STATE_CHECKSUM_L;
            break;
            
        case STATE_CHECKSUM_L:  
            b = new_checksum & 0xff;
            state = STATE_WAIT_FOR_SYNC;
            ++count;
            if (count > 2) {
                count = 0;
                new_ch3 = nextValue();
            }

            // Scan the keyboard. Only set bits here, they are cleared whenever
            // we've sent the respective data to the receiver. This way
            // we can deal with short button pushes even though the reaction
            // time is ~175ms.    
            payload |= Scan_keyboard();
            break;

        case STATE_SKIP:
            --skip;
            if (skip == 0) {
                state = STATE_WAIT_FOR_SYNC;
            }
            break;
    }
    
    return b;    
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




