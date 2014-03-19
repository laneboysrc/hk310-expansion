#include <stdint.h>

uint16_t next_CH3_value(void);


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

static uint8_t state = STATE_WAIT_FOR_SYNC;


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
    static uint16_t new_ch3;


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

                new_ch3 = next_CH3_value();
                
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


