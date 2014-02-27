#include <pic16f1825.h>
#include <stdint.h>

static __code uint16_t __at (_CONFIG1) configword1 = _FOSC_INTOSC & _WDTE_OFF & _PWRTE_ON & _MCLRE_OFF & _CP_OFF & _CPD_OFF & _BOREN_OFF & _CLKOUTEN_OFF & _IESO_OFF & _FCMEN_OFF;
static __code uint16_t __at (_CONFIG2) configword2 = _WRT_OFF & _PLLEN_OFF & _STVREN_OFF & _LVP_OFF; 

extern void Init_input(void);
extern void Read_input(void);
extern void Init_output(void);
extern void Output_result(void);


/*****************************************************************************
 Init_hardware()
 
 Initializes all used peripherals of the PIC chip.
 ****************************************************************************/
static void Init_hardware(void) {
    //-----------------------------
    // Clock initialization
    // FIXME: do 32 MHz HW
    OSCCON = 0b01111010;    // 4x PLL disabled, 16 MHz HF, Internal oscillator

    //-----------------------------
    // IO Port initialization
    // FIXME: do 16F1825 ports
    PORTA = 0;
    LATA = 0;
    ANSELA = 0;
    TRISA = 0b11101000;     // Make all ports that are not used for the motor input
}


/*****************************************************************************
 main()
 
 No introduction needed ... 
 ****************************************************************************/
void main(void) {
    Init_hardware();
    Init_output();
    Init_input();

    while (1) {
        Read_input();
        Output_result();
    }
}

