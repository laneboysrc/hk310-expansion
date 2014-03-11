#include <pic16f1825.h>
#include <stdint.h>

#define NUM_AVERAGES 4


struct {
    unsigned locked : 1;
    unsigned dataChanged : 1;
} flags;

static uint8_t wait;
static uint16_t avg[NUM_AVERAGES];

extern uint8_t tx_value;
extern uint16_t tx_uint;
extern void UART_send(void);
extern void UART_send_uchar(void);


/*****************************************************************************
 Init_input()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_input(void) {
    TMR1H = 0;
    TMR1L = 0;
    
    // Set Timer1 to 1us resolution    
    T1GCON = 0b11010000;   // Single shot gate mode
    T1CON = 0b00100001;    // Timer1 runs on Fosc/4; 1:4 pre-scaler; Timer enabled

    flags.locked = 0;
    flags.dataChanged = 0;
    
    wait = NUM_AVERAGES;
}


/*****************************************************************************
 ****************************************************************************/
void tuneOscillator(uint16_t value) 
{
    char i;

    for (i = 0; i < NUM_AVERAGES - 1; i++) {
        avg[i] = avg[i + 1];
    }
    avg[i] = value;

    if (wait) {
        --wait;
        return;
    }

    // Calculate the average of the last four values, to remove jitter
    value = 0;
    for (i = 0; i < NUM_AVERAGES; i++) {
        value += avg[i];
    }
    value = value / NUM_AVERAGES;     

    wait = NUM_AVERAGES;
    i = OSCTUNE & 0x3f;
    // Convert 6-bit signed into 6-bit unsigned
    i = i ^ 0x20;
    
    if (value < 0xa31) {
        flags.locked = 0;
        i += 10;
    }
    else if (value > 0xa57) {
        flags.locked = 0;
        i -= 10;
    }
    else if (value < 0xa3e) {
        ++i;
    }
    else if (value > 0xa42) {
        --i;
    }
    else {
        wait = 0;
        flags.locked = 1;
    }    

    if (i < 0) {
        i = 0;
    }
    else if (i > 0x3f) {
        i = 0x3f;
    } 

    // Convert 6-bit unsigned back into 6-bit signed
    i = i ^ 0x20;
    OSCTUNE = i;
}


/*****************************************************************************
 Read_input

 Called once per mainloop to read the input signal.
 This function shall set the global variable winch_mode depending on the  
 state transition logic desired.
 ****************************************************************************/
void Read_input(void) {
    uint16_t value;

    TMR1H = 0;              // Clear the timer
    TMR1L = 0;

    T1GGO = 1;              // Start measuring a pulse
    while (T1GGO);         // Wait for the measurement to be finished
    value = (TMR1H << 8) + TMR1L;

    /* If we receive a value > 0x880 then it can only be the special value
       0xa40, which is used for syncing as well as calibrating the oscillator.  
       
       0x880 has been chosen because we must ensure that it triggers regardless
       of how mis-tuned our local oscillator is.
     */
    if (value > 0x880) {
        tuneOscillator(value);
    }
}

