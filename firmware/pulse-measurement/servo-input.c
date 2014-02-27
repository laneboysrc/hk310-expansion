#include <pic16f1825.h>


/*****************************************************************************
 Init_input()

 Called once during start-up of the firmware. 
 ****************************************************************************/
void Init_input(void) {
    // FIXME: Timer 1 at max speed; Single pulse gate mode

    TMR1H = 0;
    TMR1L = 0;
    
    T1GCON = 0b11010000;   // Single shot gate mode
    T1CON = 0b01000001;     // Timer1 runs on Fosc; Timer enabled
}


/*****************************************************************************
 Read_input

 Called once per mainloop to read the input signal.
 This function shall set the global variable winch_mode depending on the  
 state transition logic desired.
 ****************************************************************************/
void Read_input(void) {
    TMR1H = 0;              // Clear the timer
    TMR1L = 0;

    T1GGO = 1;              // Start measuring a pulse
    while (T1GGO);          // Wait for the measurement to be finished
}

