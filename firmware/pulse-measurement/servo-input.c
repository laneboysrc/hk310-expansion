#include <pic16f1825.h>


/*****************************************************************************
 Init_input()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_input(void) {
    // FIXME: Timer 1 at max speed; Single pulse gate mode

    TMR1H = 0;
    TMR1L = 0;
}


/*****************************************************************************
 Read_input

 Called once per mainloop to read the input signal.
 This function shall set the global variable winch_mode depending on the  
 state transition logic desired.
 ****************************************************************************/
void Read_input(void) {
    // FIXME: use gate mode

    ++TMR1H;    
    
#if 0    
    CCP1CON = 0b00000000;   // Compare mode off

    // Wait until servo signal is LOW 
    // This ensures that we do not start in the middle of a pulse
    while (RA5 != 0) ;

    // Wait until servo signal is high; start of pulse
    while (RA5 != 1) ;

    // Start the time measurement
    TMR1ON = 1;         

    // Wait until servo signal is LOW again; end of pulse
    while (RA5 != 0) ;

    // Start the time measurement
    TMR1ON = 0;

    // Check if the measured pulse time is between 600 and 2500 us. If it is
    // outside we consider it an invalid servo pulse and stop further execution.
    //(TMR1H << 8) | TMR1L;
#endif
}

