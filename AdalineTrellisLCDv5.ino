/**
Using Neo-pixels to demonstrate the Widrow-Hoff Adaline (LMS) algorithm
16 Neo-pixels in a 4x4 array connected to a 4x4 keypad used to set up patterns
for the Arduino to learn.  Inoutting manual synapse weights using the COM port and command line
*/

#include "Adafruit_NeoTrellis.h"
#include "Arduino.h"
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

Adafruit_NeoTrellis trellis;  // Initialise NeoTrellis

int bright = 80;  // set brightness level for Neopixels
float I[17]={-1,-1,-1,-1,  // Input array (augmented)
             -1,-1,-1,-1,
             -1,-1,-1,-1,
             -1,-1,-1,-1, 1};
float W[17];   // Weight array (augmented)
float mu = 0.002;  // Learning rate variable
float out = 0;    // Output variable
float sig = 0;    // Sigmoid output
float err = 0;    // Error value
float des = 0;    // Desired value +1 or -1
bool plus, minus; // Inputs to drive the desired output

#define RED 0x1500000   // Define red with low brightness
#define GREEN 0x0015000  // Define green with low brightness
#define BLUE 0x000015  // Define blue with low brightness 
#define OFF 0x000000  // Define off with low brightness

void setup() {
  lcd.begin(16, 2);  // set up the LCD's number of columns and rows:
  lcd.setPWM(REG_GREEN, 0);  // Set green backlight off
  lcd.setPWM(REG_BLUE, 0);  // Set blue backlight off
  lcd.setPWM(REG_RED, bright);  // Set red backlight on
  Serial.begin(9600);  // Setup the serial port
  pinMode(2, INPUT_PULLUP);  // Setup digital input as a pull-up type
  pinMode(3, INPUT_PULLUP);  // Setup digital input as a pull-up type
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while(1) delay(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }
   
  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.registerCallback(i, toggle);
  }

  //show initial blank pattern
  for (uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    if (I[i]==1){
      trellis.pixels.setPixelColor(i, BLUE);  //  Set button blue if '+1' in pattern
    } else {
      trellis.pixels.setPixelColor(i, RED);  //  Set button red if '-1' in pattern
    }
    trellis.pixels.show();  //  Show button colours
    delay(50);   // Slight delay in illuminating pattern
  }
  // initialise W with random values
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS+1; i++) {
      W[i] = random(-100,100)/100;     // random values between -1 and +1
  }
}

//define a callback for key presses
TrellisCallback toggle(keyEvent evt){
  // Check is the pad pressed?
   if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && I[evt.bit.NUM]==-1) {
    trellis.pixels.setPixelColor(evt.bit.NUM, BLUE); // toggle Blue = +1
    I[evt.bit.NUM]=1;
    delay(5);
    } else {
    trellis.pixels.setPixelColor(evt.bit.NUM, RED); // toggle Off = -1
    I[evt.bit.NUM]=-1;
    delay(5);
    }
    trellis.pixels.show(); // Update the neopixels!
    return 0;
}


// compute output result from adaline
void output() {
      out = 0;    // reset output variable
      for (int i = 0; i < NEO_TRELLIS_NUM_KEYS+1; i++) {
          out = out + W[i]*I[i];     // calculate output of adaline to include bias (augmented vector)
        }
      }

// select/adjust weights manually
void update() {
      err = des - out;
      for (int i = 0; i < NEO_TRELLIS_NUM_KEYS+1; i++) {
          W[i] = W[i] + 2*mu*err*I[i];  // update the weight vector using LMS rule
        }
}

// Main loop of prgram
void loop() {
  trellis.read();  // interrupt management does all the work! :)
  trellis.pixels.show();

  if (digitalRead(3)==false){ // Read input pin 'plus'
    des = 1;
    update();
  }
  if (digitalRead(2)==false){ // Read input pin 'minus'
    des = -1;
    update();
  }
  output();           // Compute output of neuron
  sig = tanh(2*out);  // Sigmoid output function (2*) to get closer to +/-1
  lcd.setCursor(0, 0);  // Position cursor at 0,0 on LCD
  lcd.print("Sig = ");  // Print "Sig = " on LCD
  lcd.print(sig);     //  Print Sig value
  lcd.print("  ");  // Print some blank spaces to clear any old digits
  if(sig>0){
    lcd.setPWM(REG_BLUE, abs(int(sig*125))+125);  // Set LCD background to Blue if output is positive
    lcd.setPWM(REG_RED, 0);
    } else {
    lcd.setPWM(REG_BLUE, 0);
    lcd.setPWM(REG_RED, abs(int(sig*125))+125);  // Set LCD background to Red if output is negative
  }
  delay(50); // Introduce small delay to slow the update cycle to be visible on LCD
}
