#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin Definitions
#define SS D8
#define RST D0
#define DIO0 D1
#define SENSOR D9

// OLED Pins
#define OLED_MOSI D7
#define OLED_CLK D5
#define OLED_DC D3
#define OLED_CS D2
#define OLED_RESET D4

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// Flow Sensor Variables
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
const int interval = 10000; // 10 seconds
float calibrationFactor = 7.5;
volatile byte pulseCount;
float flowRate;
float intervalVolume;
float totalVolume = 0;
unsigned long intervalMilliLitres = 0;

String message = "Flow Data: ";

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);

  pulseCount = 0;
  flowRate = 0.0;
  intervalMilliLitres = 0;

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("LoRa Failed!");
    display.display();
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("LoRa Initialized");
  display.display();
  delay(1000);
}

void loop() {
  currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Calculate flow rate and volume
    byte pulseInterval = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 / interval) * pulseInterval) / calibrationFactor;
    intervalMilliLitres = (flowRate / 60.0) * 1000.0 * (interval / 1000.0);
    intervalVolume = intervalMilliLitres / 1000.0;

    totalVolume += intervalVolume;

    // Display on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Flow Rate: ");
    display.print(flowRate, 2);
    display.print(" L/min");

    display.setCursor(0, 20);
    display.print("Interval Vol: ");
    display.print(intervalVolume, 2);
    display.print(" L");

    display.setCursor(0, 40);
    display.print("Total Volume: ");
    display.print(totalVolume, 2);
    display.print(" L");

    display.display();

    // Send LoRa packet
    String dataToSend = message + " Interval Volume: " + String(intervalVolume, 2) +
                        " L, Total: " + String(totalVolume, 2) + " L";

    LoRa.beginPacket();
    LoRa.print(dataToSend);
    LoRa.endPacket();
  }
}
