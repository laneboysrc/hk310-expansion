#include <pic16f1825.h>
#include <stdint.h>

    
    
    
// TLC5940 LED driver serial communication ports, for both master and slave
#define PORT_SCK        PORTC, 0
#define PORT_SDI        PORTC, 2
#define PORT_SDO        PORTC, 1
#define PORT_XLAT       LATA, 4
#define PORT_BLANK      LATA, 5
#define PORT_GSCLK      LATC, 5


uint8_t temp;
uint8_t light_data[16];

#define temp _temp
#define light_data _light_data


/******************************************************************************
; Init_TLC5940
;
; Initializes the operation of the TLC5940 LED driver chip and sets all LED
; outputs to Off.
;
; We are not using the PWM capabilities of the chip but rather it's dot
; correction feature to adjust brightness to alter the constant current for
; each LED individually.
;*****************************************************************************/
void Init_TLC5940(void)
{
    //-----------------------------
    // MSSP1 initialization for the TLC5940
    SSP1CON1 = 0b00100000;
              // |||||||+ SSPM0  (b'0000' = SPI Master mode, clock = FOSC/4)
              // ||||||+- SSPM1
              // |||||+-- SSPM2
              // ||||+--- SSPM3
              // |||+---- CKP    (Idle state for clock is a low level)
              // ||+----- SSPEN  (Enables serial port and configures SCKx, SDOx, SDIx and SSx as the source of the serial port pins)
              // |+------ SSPOV  (Clear overflow)
              // +------- WCOL   (Clear collision)

    SSP1STAT = 0b01000000;
              // |||||||+ 
              // ||||||+- 
              // |||||+-- 
              // ||||+--- 
              // |||+---- 
              // ||+----- 
              // |+------ CKE   (Transmit occurs on transition from active to idle clock state)
              // +------- SMP   (Input data sampled at middle of data output time)

    __asm
    ; =============================
    ; Initialize the TLC5940 ports, blank the output
    BANKSEL LATA
    bsf     PORT_BLANK
    bcf     PORT_GSCLK
    bcf     PORT_XLAT

    ; =============================
    ; Clear the dot correction register by clocking out 96 bits (12 bytes) of
    ; zeros.
    BANKSEL temp
    movlw   12              ; Dot correction data is 96 bits (12 bytes)
    movwf   temp

    BANKSEL SSP1BUF
clear_dc_loop:
    BANKSEL SSP1BUF
    clrf    SSP1BUF
    btfss   SSP1STAT, 0     ; Wait for transmit done flag BF being set
    goto    $-1
    movf    SSP1BUF, w      ; Clears BF flag
    BANKSEL temp
    decfsz  temp, f
    goto    clear_dc_loop

    BANKSEL LATA
    bsf     PORT_XLAT       ; Create latch pulse to process the data
    nop
    bcf     PORT_XLAT

    ; =============================
    ; Enable the outputs
    ; Make one GSCLK clock pulse to shift the greyscale data into the
    ; greyscale register.
    BANKSEL LATA
    bcf     PORT_BLANK  
    bsf     PORT_GSCLK      
         
    return
    __endasm
    ;
}


/******************************************************************************
; TLC5940_send
;
; Sends the value in the 16 light_data registers to the TLC5940 LED driver.
; The data is sent as 6 bit "dot correction" value as we are using the TLC5940s
; capability to programmatically change the constant current source that drives
; each individual LED.
;
; Note: The 16 bytes of light_data are being destroyed!
;******************************************************************************/
void TLC5940_send(void)
{
    __asm
    ; =============================
    ; First we pack the data from the 16 bytes into 12 bytes (6 bit x 16) as 
    ; needed by the TLC5940 Dot Correction register.
    ;
    ; We do that by using indirect addressing. We perform a loop 4 times where
    ; each loop run packs 4 bytes into 3.
    ;
    ; A single loop reads from FSR0 and writes to FSR1. The pointers are 
    ; decremented when the respective byte is full.
    ; The packing loop transforms the following input into output:
    ;
    ;      Byte N-3                   Byte N-2                   Byte N-1                   Byte N
    ; IN:  xx xx 15 14 13 12 11 10    xx xx 25 24 23 22 21 20    xx xx 35 34 33 32 31 30    xx xx 45 44 43 42 41 40    
    ; OUT:                            21 20 15 14 13 12 11 10    33 32 31 30 25 24 23 22    45 44 43 42 41 40 35 34
    ;
    ; Note that the algorithm starts at the top (MSB), as the data that goes
    ; on the bus to the TLC5940 is MSB first.
    ; This way on input light_data corresponds to OUT0 and light_data+15 
    ; corresponds to OUT15 on the TLC5940.
    movlw   HIGH light_data+15
    movwf   FSR0H
    movwf   FSR1H
    movlw   LOW light_data+15
    movwf   FSR0L
    movwf   FSR1L

TLC5940_send_pack_loop:
    ; Bits 45..40 --> temp[7..2]
    moviw   FSR0--
    lslf    WREG, f
    lslf    WREG, f
    BANKSEL temp
    movwf   temp
    
    ; Bits 35..34 --> temp[1..0]; temp -> *FSR1--
    moviw   0[FSR0]
    swapf   WREG, f 
    andlw   b'00000011'
    iorwf   temp, w
    movwi   FSR1--

    ; Bits 33..30 --> temp[7..4]
    moviw   FSR0--
    swapf   WREG, f 
    andlw   b'11110000'
    movwf   temp

    ; Bits 25..22 --> temp[3..0]; temp -> *FSR1--
    moviw   0[FSR0]
    lsrf    WREG, f
    lsrf    WREG, f
    andlw   b'00001111'
    iorwf   temp, w
    movwi   FSR1--

    ; Bits 21..20 --> temp[7..6]
    moviw   FSR0--
    swapf   WREG, f 
    lslf    WREG, f
    lslf    WREG, f
    andlw   b'11000000'
    movwf   temp

    ; Bits 15..10 --> temp[5..0]; temp -> *FSR1--
    moviw   FSR0--
    andlw   b'00111111'
    iorwf   temp, w
    movwi   FSR1--

    ; Check if we reached light_data+3 in the output; If yes we are done!
    movfw   FSR1L
    sublw   LOW light_data+3
    bnz     TLC5940_send_pack_loop


    ; =============================
    ; Now we send 12 bytes in light_data+15..light_data+4 to the TLC5940
    movlw   12
    movwf   temp              
    movlw   HIGH light_data+15
    movwf   FSR0H
    movlw   LOW light_data+15
    movwf   FSR0L

TLC5940_send_loop:
    BANKSEL SSP1BUF        
    moviw   FSR0--
    movwf   SSP1BUF
    btfss   SSP1STAT, 0     ; Wait for transmit done flag BF being set
    goto    $-1
    movf    SSP1BUF, w      ; Clears BF flag
    BANKSEL temp
    decfsz  temp, f
    goto    TLC5940_send_loop

    BANKSEL LATA
    bsf     PORT_XLAT
    nop
    bcf     PORT_XLAT 

       
    return
    
    __endasm
    ;
}    

