/**
Using Neo-pixels to demonstrate the Widrow-Hoff Adaline (LMS) algorithm
16 Neo-pixels in a 4x4 array connected to a 4x4 keypad used to set up patterns
for the Arduino to learn.
*/

#include "Adafruit_NeoTrellis.h"
#include "Arduino.h"
#include <Wire.h>
#include "rgb_lcd.h"

rgb_lcd lcd;

Adafruit_NeoTrellis trellis;  // Initialise NeoTrellis

float I[17] = { -1,-1,-1,-1,  // Input array (augmented)
             -1,-1,-1,-1,
             -1,-1,-1,-1,
             -1,-1,-1,-1, 1 };
float W[17];      // Weight array (augmented)
float mu = 0.002; // Learning rate variable
float out = 0;    // Output variable
float sig = 0;    // Sigmoid output
float err = 0;    // Error value
float des = 0;    // Desired value +1 or -1


void setLed(uint16_t i) {
  trellis.pixels.setPixelColor(i, I[i] == 1 ? 0x000015 : 0x150000);
  trellis.pixels.show(); // Update the neopixels!
}
//define a callback for key presses
TrellisCallback toggle(keyEvent evt) {
  // Check is the pad pressed?
  I[evt.bit.NUM] = evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && I[evt.bit.NUM] == -1 ? 1 : -1;
  setLed(evt.bit.NUM);
  delay(5);
  return 0;
}


// compute output result from adaline
void output() {
  out = 0;    // reset output variable
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    out = out + W[i] * I[i];     // calculate output of adaline to include bias (augmented vector)
  }
}

// select/adjust weights manually
void update() {
  err = des - out;
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    W[i] = W[i] + 2 * mu * err * I[i];  // update the weight vector using LMS rule
  }
}

// Main loop of prgram

void setup() {
  lcd.begin(16, 2);  // set up the LCD's number of columns and rows:
  lcd.setRGB(255, 0, 0);  // Set green backlight off
  Serial.begin(9600);  // Setup the serial port
  pinMode(2, INPUT_PULLUP);  // Setup digital input as a pull-up type
  pinMode(3, INPUT_PULLUP);  // Setup digital input as a pull-up type
  if (!trellis.begin()) {
    Serial.println("Could not start trellis, check wiring?");
    while (1) delay(1);
  } else {
    Serial.println("NeoPixel Trellis started");
  }

  //activate all keys and set callbacks
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.registerCallback(i, toggle);
  }

  //show initial blank pattern
  for (uint16_t i = 0; i < trellis.pixels.numPixels(); i++) {
    setLed(i);
    delay(50);   // Slight delay in illuminating pattern
  }
  // initialise W with random values
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    W[i] = random(-100, 100) / 100;     // random values between -1 and +1
  }
}

void loop() {
  trellis.read();  // interrupt management does all the work! :)
  trellis.pixels.show();

  if (digitalRead(3) == false) { // Read input pin 'plus'
    des = 1;
    update();
  }
  if (digitalRead(2) == false) { // Read input pin 'minus'
    des = -1;
    update();
  }
  output();           // Compute output of neuron
  sig = tanh(2 * out);  // Sigmoid output function (2*) to get closer to +/-1
  lcd.setCursor(0, 0);  // Position cursor at 0,0 on LCD
  lcd.print("Sig = ");  // Print "Sig = " on LCD
  lcd.print(sig);     //  Print Sig value
  lcd.print("  ");  // Print some blank spaces to clear any old digits
  lcd.setRGB((sig < 0) ? int(pow(-sig,3) * 245) + 10 : 0, (sig == 0) ? 255 : 0, (sig > 0) ? int(pow(sig,3) * 245) + 10 : 0);
  delay(50); // Introduce small delay to slow the update cycle to be visible on LCD
}