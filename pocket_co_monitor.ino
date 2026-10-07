#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int mq7AnalogPin = 36;
const int buzzerPin    = 4;

const int SAMPLES = 30;
const unsigned long WARMUP_TIME_MS = 60000;

long analogTotal = 0;

const int CO_SAFE   = 10;
const int CO_WARN   = 30;

unsigned long lastBeep = 0;
int beepInterval = 1000;

bool isWarmingUp = true;
unsigned long warmupStart = 0;

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED not found. Check I2C address"));
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("POCKET CO MONITOR"));
  display.setCursor(0, 16);
  display.println(F("Warming Up..."));
  display.display();

  warmupStart = millis();
  isWarmingUp = true;
}

void loop() {
  analogTotal = 0;
  for (int i = 0; i < SAMPLES; i++) {
    analogTotal += analogRead(mq7AnalogPin);
    delay(5);
  }
  int rawADC = analogTotal / SAMPLES;
  if (rawADC > 4095) rawADC = 4095;
  if (rawADC < 0)   rawADC = 0;

  int coPercent = map(rawADC, 0, 4095, 0, 100);

  unsigned long elapsedWarmup = millis() - warmupStart;
  if (isWarmingUp && elapsedWarmup < WARMUP_TIME_MS) {
    int secLeft = (WARMUP_TIME_MS - elapsedWarmup) / 1000;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("POCKET CO MONITOR"));
    display.setCursor(0, 16);
    display.println(F("Initializing Heater"));
    display.setCursor(0, 32);
    display.print(F("Ready in: "));
    display.print(secLeft + 1);
    display.print(F("s"));
    display.setCursor(0, 48);
    display.print(F("CO: ~"));
    display.print(coPercent);
    display.print(F("%"));
    display.display();

    noTone(buzzerPin);
    digitalWrite(buzzerPin, LOW);
    delay(250);
    return;
  } else if (isWarmingUp) {
    isWarmingUp = false;
    display.clearDisplay();
    display.display();
    delay(250);
  }

  String statusText = "SAFE";
  bool alertWarn   = false;
  bool alertDanger = false;

  if (coPercent <= CO_SAFE) {
    statusText  = "SAFE";
    alertWarn   = false;
    alertDanger = false;
    beepInterval = 1500;
  } else if (coPercent <= CO_WARN) {
    statusText  = "WARNING";
    alertWarn   = true;
    alertDanger = false;
    beepInterval = 800;
  } else {
    statusText  = "DANGER";
    alertWarn   = false;
    alertDanger = true;
    beepInterval = 250;
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(F("CO: "));
  display.print(coPercent);
  display.print(F("%"));

  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print(statusText);

  int barWidth = map(coPercent, 0, 100, 0, 124);
  display.drawRect(2, 50, 124, 10, SSD1306_WHITE);
  if (barWidth > 0) {
    display.fillRect(2, 50, barWidth, 10, SSD1306_WHITE);
  }

  display.display();

  unsigned long now = millis();
  if ((alertWarn || alertDanger) && (now - lastBeep >= beepInterval)) {
    lastBeep = now;
    tone(buzzerPin, 1800, 120);
  } else if (!alertWarn && !alertDanger) {
    noTone(buzzerPin);
  }

  Serial.print(F("Raw ADC: "));
  Serial.print(rawADC);
  Serial.print(F(" | CO %: "));
  Serial.print(coPercent);
  Serial.print(F(" | Status: "));
  Serial.println(statusText);

  delay(250);
}
