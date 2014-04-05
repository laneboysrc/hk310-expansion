/*****************************************************************************

    analog_ch3_arduino
    
    This program converts CH3 of the HK310 transmitter from a two-position
    switch into an analog channel.
    
    It is designed for Arduino.
    
    Port usage:
    AN0:  Analog input for a potentiometer connected between +5V and GND
    RX:   UART input from the MCU
    TX:   UART output to the NRF module

    IMPORTANT:
    ==========
    It is best to use a Arduino Pro Mini 3.3V version as on Arduino's with
    built-in USB-to-serial converter the Tx/Rx pins could damage the HK310.

*****************************************************************************/

#define ANALOG_PIN 0



/*****************************************************************************
 Returns the next 12-bit value to transmit over CH3
 ****************************************************************************/
uint16_t next_CH3_value() 
{
  uint16_t adc;

  // Perform the ADC conversion
  adc = analogRead(ANALOG_PIN);

  // The range of 1000us to 2000us on a servo output corresponds to the range
  // 1650 .. 650, with 1150 being neutral. Since the range of 1000 is almost  
  // identical to 10 bits (1023) we simply use a 1:1 linear mapping with 
  // appropriate offset that 512 (max of 10 bit range divided by 2) gives
  // neutral.
  return (1150 - 512) + adc;
}


/*****************************************************************************
 Initialization code; runs once.
 ****************************************************************************/
void setup() 
{
  Serial.begin(19200);
  analogReference(DEFAULT);
}


/*****************************************************************************
 Main code; runs repeatedly
 ****************************************************************************/
void loop() 
{
  uint8_t b;

  if (Serial.available() > 0) {
    b = Serial.read();
    b = filter(b);
    Serial.write(b);
  }
}

