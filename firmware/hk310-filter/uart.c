#include <pic16f1825.h>
#include <stdint.h>

#include "uart.h"

#define SPBRG_VALUE 208L     // 19200 @ 16 MHz


/*****************************************************************************
 Init_UART()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_UART(void) {
	BRG16 = 1;
	BRGH = 1;
	SPBRGH = SPBRG_VALUE >> 8;
	SPBRGL = SPBRG_VALUE & 0xff;

	SYNC=0;			// Disable Synchronous/Enable Asynchronous
	SPEN=1;			// Enable serial port
	TXEN=1;			// Enable transmission mode
	CREN=1;			// Enable reception mode

	TXREG = '\n';
}


/*****************************************************************************
 Send tx_value out via the UART
 ****************************************************************************/
void UART_send(uint8_t c) {
    // Wait for TSR register being empty, then send the character
    while (!TRMT);
    TXREG = c;  
}


/*****************************************************************************
 Send tx_uint as decimal number with leading zeros out via the UART
 ****************************************************************************/
void UART_send_uint(uint16_t tx_uint) {
    uint8_t tx_value;

    tx_value = 0;
    while (tx_uint >= 9999) {
        tx_uint -= 10000;
        ++tx_value;
    }
    tx_value += '0';
    UART_send(tx_value);

    tx_value = 0;
    while (tx_uint >= 1000) {
        tx_uint -= 1000;
        ++tx_value;
    }
    tx_value += '0';
    UART_send(tx_value);
    
    tx_value = 0;
    while (tx_uint >= 100) {
        tx_uint -= 100;
        ++tx_value;
    }
    tx_value += '0';
    UART_send(tx_value);

    tx_value = 0;
    while (tx_uint >= 10) {
        tx_uint -= 10;
        ++tx_value;
    }
    tx_value += '0';
    UART_send(tx_value);

    tx_value = tx_uint + '0';
    UART_send(tx_value);

    UART_send('\n');
}


/*****************************************************************************
 Send (unsigned char)tx_uint as decimal number with leading zeros out via 
 the UART
 ****************************************************************************/
void UART_send_uchar(uint8_t tx_uint) {
    uint8_t tx_value;

    tx_value = 0;
    while (tx_uint >= 100) {
        tx_uint -= 100;
        ++tx_value;
    }
    tx_value += '0';
    UART_send(tx_value);

    tx_value = 0;
    while (tx_uint >= 10) {
        tx_uint -= 10;
        ++tx_value;
    }
    tx_value += '0';
    UART_send(tx_value);

    tx_value = tx_uint + '0';
    UART_send(tx_value);

    UART_send('\n');
}


/******************************************************************************
; UART_read_byte
;
; Recieve one byte from the UART in W.
;
; To enable reception of a byte, CREN must be 1. 
;
; On any error, recover by pulsing CREN low then back to high. 
;
; When a byte has been received the RCIF flag will be set. RCIF is 
; automatically cleared when RCREG is read and empty. RCREG is double buffered, 
; so it is a two byte deep FIFO. If a third byte comes in, then OERR is set. 
; You can still recover the two bytes in the FIFO, but the third (newest) is 
; lost. CREN must be pulsed negative to clear the OERR flag. 
;
; On a framing error FERR is set. FERR is automatically reset when RCREG is 
; read, so errors must be tested for *before* RCREG is read. It is *NOT* 
; recommended that you ignore the error flags. Eventually an error will cause 
; the receiver to hang up if you don't clear the error condition.
;*****************************************************************************/
uint8_t UART_read_byte(void)
{
    do {
#if 0    
        if (OERR) {
            CREN = 0;
            WREG = RCREG;
            WREG = RCREG;
            WREG = RCREG;
            CREN = 1;
        }
        
        if (FERR) {
            WREG = RCREG;
        }
#endif        
    } while (!RCIF);
    
    return RCREG;
}
