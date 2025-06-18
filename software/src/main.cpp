//include the libraries
#include <Wire.h>
#include <Arduino.h>
#include <rgb_lcd.h>
#include <Adafruit_NeoTrellis.h>
#include <U8x8lib.h>

#define BTNP 2
#define BTNN 3

//define the class instances
U8X8_SSD1306_128X64_NONAME_HW_I2C display(-1);
rgb_lcd lcd;
Adafruit_NeoTrellis trellis;


double I[17] = { -1,-1,-1,-1,
             -1,-1,-1,-1,
             -1,-1,-1,-1,
             -1,-1,-1,-1, 1 };
double weights[17];
double mu = 0.002;

//define the functions used in the program
TrellisCallback btnPress(keyEvent evt); //trellis callback function
double output();                        //calculate the output of the algorithm   
void scanI2C();                         //scan the i2c bus for devices
void setLed(uint16_t i);                //set the color of a button
void update(double out, double des);    //calculate the error and update the weights
void printLcd(double out);              //update the lcd display
void printOled();                       //update the oled display
void reset();                           //reset the weights back to 0
void menu();                            //open up the menu
int getDecimalDigit(double num,int d);   //get the third decimal digit of a number
void setup() {
  Serial.begin(9600);
  scanI2C();
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  //initialize the oled display
  if (!display.begin()) {
    Serial.println("SSD1306 fialed to initialize");
    for (;;);
  } else {
    Serial.println("SSD1306 initialized");
  }
  display.setFont(u8x8_font_chroma48medium8_r); //select a font
  display.clear();                              //clear the display
  display.display();                            //display the cleared display

  //initialize the lcd display
  lcd.begin(16, 2);
  Serial.println("LCD initialized");

  //initialize the trellis
  if (!trellis.begin(0x2E)) {
    Serial.println("Could not start trellis, check wiring?");
    for (;;);
  } else {
    Serial.println("NeoPixel Trellis started");
  }

  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING); //set the callback function to rising edges
    trellis.registerCallback(i, btnPress);             //register the callback function
    setLed(i);                                         //set the initial color
  }
  delay(1000);
}


void loop() {
  // read the trellis and let it handle the callback
  trellis.read();

  double out = output();// calculate the output of the algorithm

  // handle the button inputs
  
  printLcd(out); // update the lcd
  if (!digitalRead(BTNP)&&!digitalRead(BTNN)) {
    menu();
  }
  if (digitalRead(BTNP) == false) {

    update(out, 1); //calculate the error and update the weights
  }
  if (digitalRead(BTNN) == false) {
    update(out, -1); //calculate the error and update the weights
  }
  printOled();   // update the oled display
  delay(50);
}

void setLed(uint16_t i) {
  // set the pixel color to red or blue depending on the state of the button
  trellis.pixels.setPixelColor(i, I[i] == 1 ? 0x000015 : 0x150000);
  trellis.pixels.show(); //push the changes to the trellis
}

//callback function for the trellis that is called when a button state changes
TrellisCallback btnPress(keyEvent evt) {
  if (evt.bit.EDGE != SEESAW_KEYPAD_EDGE_RISING) return 0; // ignore key releases 
  I[evt.bit.NUM] *= -1; // invert the value of the key
  setLed(evt.bit.NUM); //update the button color
  delay(5); //and wait a bit to debounce
  return 0;
}

double output() {
  double out = 0;
  // calculate the output
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    out = out + weights[i] * I[i];
  }
  return out;
}

void update(double out, double des) {
  // calculate the error
  double err = des - out;
  // update the weights
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    weights[i] = weights[i] + 2 * mu * err * I[i];
  }
}

// scan the i2c bus for devices
void scanI2C() {
  Wire.begin();
  //loop trough all the i2c addresses
  for (uint8_t i = 1; i < 127; i++) {
    // send a ping to the device
    Wire.beginTransmission(i);
    // if the device responds, print its address
    if (Wire.endTransmission() == 0) {
      Serial.print("0x");
      Serial.print(i, HEX);
      Serial.println(" ");
    }
  }
}

int getDecimalDigit(double num,int d) {
  num = fabs(num); // make sure it's positive
  num *= pow(10,d+1);     // move d digits after decimal to before decimal
  int temp = static_cast<int>(num); // truncate to int
  return temp % 10; // get the last digit
}
void printLcd(double out) {
  double sig = tanh(2 * out);
  lcd.setCursor(0, 0); //set the cursor to the top left corner
  //print the output to the lcd
  lcd.print("Sig = ");
  lcd.print(sig);
  lcd.print("  ");

  //set the color of the lcd display depending on the output
  int brightness = pow(abs(sig), 4) * 245 + 10;
  if (sig < 0) {
    lcd.setRGB(brightness, 0, 0); //red if sig is negative
  } else if (sig > 0) {
    lcd.setRGB(0, 0, brightness);  //blue if sig is positive
  } else {
    lcd.setRGB(0, 255, 0);        //green otherwise
  }
}
void printOled() {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      char buffer[6]; // create a buffer for the string we want to print to the display
      sprintf(buffer, "%4d", (int)(weights[x * 4 + y] * 100)); // format the number to a string
      display.drawString(x * 4, y * 2, buffer); // print the string to the display
    }
  }
}
void menu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Adaline Menu");
  int i = 0;
  const char* menuItems[] = { "Reset", "Change mu", "Exit" };
  while (!digitalRead(BTNN) || !digitalRead(BTNP)); //wait for buttons to be released
  while (true) {
    lcd.setCursor(0, 1);
    // add 16 spaces after the string and remove everything after the 16th character to make sure that the line is cleared of previous text
    lcd.print((String(menuItems[i]) + "                ").substring(0, 16));
    while (digitalRead(BTNN) && digitalRead(BTNP)); //wait for button press
    delay(50); //wait for a possible second button press
    Serial.println(i);
    if (!digitalRead(BTNN) && !digitalRead(BTNP)) {
      if (i == 0) {
        reset();
      }
      if (i == 1) {
        // mu = getNumber();
        double newMu = mu;
        double d = 0;
        int l = 1;
        
        while (true) {
          lcd.print((String(newMu) + "                ").substring(0, 16));
          if (!digitalRead(BTNP)) {
            d++;
            if (d > 9) { d = 0; }
            delay(50); //debounce
            while (digitalRead(BTNP)); //wait for button release
            delay(50); //debounce
          } else if (!digitalRead(BTNN)) {
            d--;
            if (d < 0) { d = 9; }
            delay(50); //debounce
            while (digitalRead(BTNN)); //wait for button release
            delay(50); //debounce
          }
        }
      }
      if (i == 2) {
      }
      break;
    } else if (!digitalRead(BTNP)) {
      i++;
      if (i >= sizeof(menuItems) / sizeof(menuItems[0])) { i = 0; } //wrap around
      delay(50); //debounce
      while (digitalRead(BTNP)); //wait for button release
      delay(50); //debounce
    } else if (!digitalRead(BTNN)) {
      i--;
      if (i < 0) { i = sizeof(menuItems) / sizeof(menuItems[0])-1; } //wrap around
      delay(50); //debounce
      while (digitalRead(BTNN)); //wait for button release
      delay(50); //debounce
    }
  }
  lcd.clear();
}
void reset() {
  for (int i = 0; i < 17; i++) {
    weights[i] = 0;
    I[i] = 0;
  }
}
