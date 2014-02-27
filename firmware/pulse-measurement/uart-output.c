#include <pic16f1825.h>

static unsigned char tx_value;

static unsigned int val;

#define FOSC 32                 // Osc frequency in MHz
#define BAUDRATE 38400          // Desired baudrate
#define BRGH_VALUE 1            // Either 0 or 1
#define SPBRG_VALUE (((10*FOSC*1000000/((64-(48*BRGH_VALUE))*BAUDRATE))+5)/10)-1

static void UART_send(void);

/*****************************************************************************
 Init_output()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_output(void) {
	SPBRG = SPBRG_VALUE;	// Baud Rate register, calculated by macro
	BRGH = BRGH_VALUE;

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
    
    tx_value = val / 10000;
    UART_send();
    val = val % 10000;
    tx_value = val  / 1000;
    UART_send();
    val = val % 1000;
    tx_value = val  / 100;
    UART_send();
    val = val % 100;
    tx_value = val  / 10;
    UART_send();
    tx_value = val % 10;
    UART_send();

    tx_value = '\n';
    UART_send();
    
    for (val = 0; val < 1000; val++) {
        ;
    }
    
}


/*****************************************************************************
 Send tx_value out via the UART
 ****************************************************************************/
void UART_send(void) {
    /* Wait for TSR register being empty */
    while (!TRMT);
 
    TXREG = tx_value;  
}

