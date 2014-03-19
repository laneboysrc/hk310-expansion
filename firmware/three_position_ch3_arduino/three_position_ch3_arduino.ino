/*****************************************************************************

    three_position_ch3_arduino
    
    This program converts CH3 of the HK310 transmitter from a two-position
    switch to a three-position switch.
    
    It is designed for Arduino.
    
    Port usage:
    
    AN0:  Switch in
    TX:   UART TX (to HK310 NRF module
    RX:   UART RX (from HK310 MCU)


    IMPORTANT:
    ==========
    Many Arduino boards are running on 5V, while the NRF module runs
    at 3.3V. Therefore level shifting is required on the TX pin of the 
    Arduino to ensure the NRF module does not get damaged.

    
    The switch is connected in such a way that the outside contacts are 
    GND and Vdd, and the middle contact (which is now either GND, Vdd or open)
    to RA2. In addition two resistors of ~100k value need to be placed
    over the switch terminals to form a voltage divider, with the tap
    on RA2. This enabled us to detect whether the swithc is open.


                     ^ Vdd              ^ Vdd
                     |                  |
                    +-+                 |
                100k| |         ___     |    1 pole 3 position switch
        -----+      | |          |      |   
    Arduino  |      +-+          |_/ o--+  
             |       |          _/       
         AN0 +-------o-------o-/   
             |       |    
             |      +-+              o--+    
        -----+      | |                 |
                100k| |                 |
                    +-+                 |
                     |                  |
                    --- Vss            --- Vss (GND)
   
    
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
  
  // If we are close to Vdd / Vss send the endpoints accordingly, otherwise
  // send the neutral position as the switch must be open. 
  if (adc > 800) {
      return 650;    
  }
  if (adc < 200) {
      return 1650;    
  }
  return 1150;   
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

