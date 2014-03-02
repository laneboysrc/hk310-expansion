#include <pic16f1825.h>

#define NUM_AVERAGES 4

unsigned char data;
struct {
    unsigned locked : 1;
    unsigned dataChanged : 1;
} flags;

static unsigned char wait;
static unsigned int avg[NUM_AVERAGES];


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

    for (i = 0; i < NUM_AVERAGES; i++) {
        avg[i] = 0xa04;
    }
    
    flags.locked = 0;
    flags.dataChanged = 0;
}


/*****************************************************************************
 Read_input

 Called once per mainloop to read the input signal.
 This function shall set the global variable winch_mode depending on the  
 state transition logic desired.
 ****************************************************************************/
void Read_input(void) {
    unsigned char i;
    unsigned int value;
    unsigned int absDiff;

    static char bigDiff = 0;
    static unsigned int oldValue = 0xa04;
    static unsigned char oldData = 0xff;

    flags.dataChanged = 0;
    
    TMR1H = 0;              // Clear the timer
    TMR1L = 0;


    T1GGO = 1;              // Start measuring a pulse
    while (T1GGO);         // Wait for the measurement to be finished

    
    value = (TMR1H << 8) + TMR1L;


    /* Glitch eliminator
       Despite being a fully digital system, every now and then there are
       glitches on the servo output. The sympthom of the glitch is that
       it makes the pulse a bit longer.
      
       So we only use values that are within 4us of each-other. Everything
       else is either a glitch, or a value change. If the difference is
       really big we assume a value change, otherwise a glitch.
       
       The values have been determined through trial and error.
      */      
    absDiff = oldValue > value ? oldValue - value : value - oldValue;
    if (absDiff > 4) {
        if (bigDiff || absDiff > 7) {
            oldValue = value;
            bigDiff = 0;
        }
        else {
            bigDiff = 1;
        }
        return;
    }
    oldValue = value;
    
    
    /* Extract bits 11:4, which contain the payload. If we are locked and it
       is a new data value, then send it further. 
     */
    data = value >> 4;
    if (oldData != data  &&  flags.locked) {
        flags.dataChanged = 1;
    }
    oldData = data;
    
    
    /* If we receive a value > 0x880 then it can only be the special value
       0xa04, which is used for syncing as well as calibrating the oscillator.  
       
       0x880 has been chosen because we must ensure that it triggers regardless
       of how mis-tuned our local oscillator is.
     */
    if (value <= 0x880) {
        return;
    }
        
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

    
    // FIXME: do a proper binary search algorithm
    // FIXME: do proper OSCTUNE 6-bit signed math!
    
    if (value < 0xa01  ||  value > 0xa07) {
        wait = 5;

        if (value < 0xa01) {
            if ((0xa04 - value) > 15) {
                OSCTUNE = (OSCTUNE + 10) & 0x3f;
                flags.locked = 0;
            }
            else {
                OSCTUNE = (OSCTUNE + 1) & 0x3f;
            }
        }
        else {
            if ((value - 0xa04) > 15) {
                OSCTUNE = (OSCTUNE - 10) & 0x3f;
                flags.locked = 0;
            }
            else {
                OSCTUNE = (OSCTUNE - 1) & 0x3f;
            }
        }
    }
    else {
        flags.locked = 1;
    }    
}

