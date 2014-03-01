#include <pic16f1825.h>

unsigned char wait;
unsigned int avg[4];

extern unsigned int tx_uint;
extern unsigned char tx_value;
extern void UART_send_uint(void);
extern void UART_send(void);


unsigned char locked;

/*****************************************************************************
 Init_input()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_input(void) {
    unsigned char i;

    TMR1H = 0;
    TMR1L = 0;
    
    // Set Timer1 to 1us resolution    
    T1GCON = 0b11010000;   // Single shot gate mode
    T1CON = 0b00100001;    // Timer1 runs on Fosc/4; 1:4 pre-scaler; Timer enabled

    for (i = 0; i < 4; i++) {
        avg[i] = 0xa04;
    }
    
    locked = 0;
}


/*****************************************************************************
 Read_input

 Called once per mainloop to read the input signal.
 This function shall set the global variable winch_mode depending on the  
 state transition logic desired.
 ****************************************************************************/
void Read_input(void) {
    unsigned char i;
    unsigned int a;
    
    TMR1H = 0;              // Clear the timer
    TMR1L = 0;

    T1GGO = 1;              // Start measuring a pulse
    while (T1GGO);         // Wait for the measurement to be finished
    
    a = (TMR1H << 8) + TMR1L;
    if (a <= 0x900) {
        return;
    }
        
    for (i = 0; i < 3; i++) {
        avg[i] = avg[i + 1];
    }
    avg[i] = (TMR1H << 8) + TMR1L;

    if (wait) {
        --wait;
        return;
    }
    
    a = 0;
    for (i = 0; i < 4; i++) {
        a += avg[i];
    }
    a = a >> 2;    
    
    // FIXME: do a proper binary search algorithm
    
    if (a < 0xa01  ||  a > 0xa07) {
        wait = 5;

        if (a < 0xa01) {
            if ((0xa04 - a) > 15) {
                OSCTUNE = (OSCTUNE + 10) & 0x3f;
                if (locked) {
                    tx_value = 'U';
                    UART_send();
                    tx_value = '\n';
                    UART_send();        
                }
                locked = 0;
            }
            else {
                OSCTUNE = (OSCTUNE + 1) & 0x3f;
            }
        }
        if (a > 0xa06) {
            if ((a - 0xa04) > 15) {
                OSCTUNE = (OSCTUNE - 10) & 0x3f;
                if (locked) {
                    tx_value = 'U';
                    UART_send();
                    tx_value = '\n';
                    UART_send();        
                }
                locked = 0;
            }
            else {
                OSCTUNE = (OSCTUNE - 1) & 0x3f;
            }
        }
        //tx_uint = 0xa04 - a;
        //UART_send_uint();
    }
    else {
        if (!locked) {
            tx_value = 'L';
            UART_send();
            tx_value = '\n';
            UART_send();        
        }
        locked = 1;
    }    
}

