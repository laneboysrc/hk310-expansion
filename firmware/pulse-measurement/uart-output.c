#include <pic16f1825.h>

#define SPBRG_VALUE 104     // 38400 @ 16 MHz

void UART_send(void);
void UART_send_uint(void);

unsigned char tx_value;
unsigned int tx_uint;

extern unsigned char locked;

/*****************************************************************************
 Init_output()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_output(void) {
	BRG16 = 1;
	BRGH = 1;
	SPBRGH = SPBRG_VALUE >> 8;
	SPBRGL = SPBRG_VALUE & 0xff;

	SYNC=0;			// Disable Synchronous/Enable Asynchronous
	SPEN=1;			// Enable serial port
	TXEN=1;			// Enable transmission mode
	CREN=0;			// Disable reception mode

	TXREG = '\n';
}


/*****************************************************************************
 Output_result()

 Outputs the timer 1 value as decimal number, including leading zeros
 ****************************************************************************/
void Output_result(void) {
    if (! locked) {
        return;
    }

    tx_uint = (TMR1H << 8) + TMR1L;
    UART_send_uint();
    
#if 0
    val = (TMR1H << 8) + TMR1L;

    tx_value = val & 0x0800 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0400 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0200 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0100 ? '1' : '0';
    UART_send();

    tx_value = ' ';
    UART_send();

    tx_value = val & 0x0080 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0040 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0020 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0010 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0008 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0004 ? '1' : '0';
    UART_send();

    tx_value = val & 0x00012 ? '1' : '0';
    UART_send();

    tx_value = val & 0x0001 ? '1' : '0';
    UART_send();
#endif
}


/*****************************************************************************
 Send tx_uint as decimal number with leading zeros out via the UART
 ****************************************************************************/
void UART_send_uint(void) {
    tx_value = 0;
    while (tx_uint >= 9999) {
        tx_uint -= 10000;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = 0;
    while (tx_uint >= 1000) {
        tx_uint -= 1000;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = 0;
    while (tx_uint >= 100) {
        tx_uint -= 100;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = 0;
    while (tx_uint >= 10) {
        tx_uint -= 10;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = tx_uint + '0';
    UART_send();

    tx_value = '\n';
    UART_send();
}


/*****************************************************************************
 Send tx_value out via the UART
 ****************************************************************************/
void UART_send(void) {
    // Wait for TSR register being empty, then send the character in tx_value
    while (!TRMT);
    TXREG = tx_value;  
}

