#include <Wire.h>
#include <Arduino.h>
#include <rgb_lcd.h>
#include <Adafruit_NeoTrellis.h>
#include <U8x8lib.h>


U8X8_SSD1306_128X64_NONAME_HW_I2C display(-1);
rgb_lcd lcd;
Adafruit_NeoTrellis trellis;


double I[17] = { -1,-1,-1,-1,
             -1,-1,-1,-1,
             -1,-1,-1,-1,
             -1,-1,-1,-1, 1 };
double W[17];
double mu = 0.002;
double sig = 0;
double err = 0;
double des = 0;

void scanI2C();
void setLed(uint16_t i);
TrellisCallback btnPress(keyEvent evt);
void update(double out);
double output();
void printLcd(double out);


void setup() {
  Serial.begin(9600);
  scanI2C();
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  if (!display.begin()) {
    Serial.println("SSD1306 fialed to initialize");
    for (;;);
  } else {
    Serial.println("SSD1306 initialized");
  }
  lcd.begin(16, 2);
  lcd.setRGB(255, 0, 0);
  Serial.println("LCD initialized");
  if (!trellis.begin(0x2E)) {
    Serial.println("Could not start trellis, check wiring?");
    for (;;);
  } else {
    Serial.println("NeoPixel Trellis started");
  }
  display.setFont(u8x8_font_chroma48medium8_r);
  display.display();

  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.registerCallback(i, btnPress);
    setLed(i);
    delay(50);
  }
  randomSeed(analogRead(0));
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    W[i] = random(-100, 100) / 100.0;
  }
}

void loop() {
  trellis.read();
  trellis.pixels.show();
  double out = output();

  if (digitalRead(3) == false) {
    des = 1;
    update(out);
  }
  if (digitalRead(2) == false) {
    des = -1;
    update(out);
  }
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      display.drawString(x * 4, y * 2, ([](int x, int y) { static char b[6]; sprintf(b, "%4d", (int)(W[x * 4 + y] * 100)); return b; })(x, y));
    }
  }
  printLcd(out);
  delay(50);
}

void setLed(uint16_t i) {
  trellis.pixels.setPixelColor(i, I[i] == 1 ? 0x000015 : 0x150000);
  trellis.pixels.show();
}

TrellisCallback btnPress(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    I[evt.bit.NUM] *= -1;
  }
  setLed(evt.bit.NUM);
  delay(5);
  return 0;
}

double output() {
  double out = 0;
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    out = out + W[i] * I[i];
  }
  return out;
}

void update(double out) {
  err = des - out;
  for (int i = 0; i < NEO_TRELLIS_NUM_KEYS + 1; i++) {
    W[i] = W[i] + 2 * mu * err * I[i];
  }
}

void scanI2C() {
  Wire.begin();
  for (uint8_t i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("0x");
      Serial.print(i, HEX);
      Serial.println(" ");
    }
  }
}

void printLcd(double out) {
  double sig = tanh(2 * out);
  lcd.setCursor(0, 0);
  lcd.print("Sig = ");
  lcd.print(sig);
  lcd.print("  ");
  if (sig < 0) {
    lcd.setRGB(int(pow(-sig, 4) * 245) + 10, 0, 0);
  } else if (sig > 0) {
    lcd.setRGB(0, 0, int(pow(sig, 4) * 245) + 10);
  } else {
    lcd.setRGB(0, 255, 0);
  }
}