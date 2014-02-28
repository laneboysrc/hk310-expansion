#include <pic16f1825.h>

static unsigned char tx_value;

static unsigned int val;

#define SPBRG_VALUE 104

static void UART_send(void);

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

 FIXME: implement and document
 ****************************************************************************/
void Output_result(void) {
    val = (TMR1H << 8) + TMR1L;

#if 0
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


    tx_value = 0;
    while (val >= 9999) {
        val -= 10000;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = 0;
    while (val >= 1000) {
        val -= 1000;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = 0;
    while (val >= 100) {
        val -= 100;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = 0;
    while (val >= 10) {
        val -= 10;
        ++tx_value;
    }
    tx_value += '0';
    UART_send();

    tx_value = val + '0';
    UART_send();

    tx_value = '\n';
    UART_send();

#if 0    
    for (val = 0; val < 65500; val++) {
        val = val - 33;
        val = val + 42;
        val = val - 145;
        val = val + 33;
        val = val - 42;
        val = val + 145;
        val = val - 33;
        val = val + 42;
        val = val - 145;
        val = val + 33;
        val = val - 42;
        val = val + 145;
    }
#endif    
}


/*****************************************************************************
 Send tx_value out via the UART
 ****************************************************************************/
void UART_send(void) {
    /* Wait for TSR register being empty */
    while (!TRMT);
 
    TXREG = tx_value;  
}

