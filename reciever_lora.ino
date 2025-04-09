#include <SPI.h>
#include <LoRa.h>

#define SS    D8
#define RST   D0
#define DIO0  D1

void setup() {
  Serial.begin(9600);
  unsigned long startTime = millis();
  while (!Serial && millis() - startTime < 5000) {
    // Wait for Serial to initialize, but not forever (max 5 seconds)
    yield();
  }
  Serial.println("LoRa Receiver");

  LoRa.setPins(SS, RST, DIO0); // Set the pins for LoRa module

  if (!LoRa.begin(433E6)) { // Initialize LoRa on frequency 433 MHz
    Serial.println("Starting LoRa failed!");
    while (true) {
      delay(100);
      yield(); // Avoid watchdog timer reset
    }
  }
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  // Try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    Serial.print("Received packet: '");

    // Read packet
    while (LoRa.available()) {
      char c = (char)LoRa.read(); // Read each byte as a character
      Serial.print(c); // Print each character as it's received
    }

    // Print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}
