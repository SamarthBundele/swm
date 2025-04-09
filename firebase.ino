

#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SPI.h>
#include <LoRa.h>
#include <addons/TokenHelper.h>

#define SS    D8
#define RST   D0
#define DIO0  D1

// Wi-Fi credentials
#define WIFI_SSID "Sky Airlines"
#define WIFI_PASSWORD "12345678"

// Firebase credentials
#define API_KEY "AIzaSyB-lrZ14Nl2BvONKv2VIEYtLzqfM3UyNCQ"
#define FIREBASE_PROJECT_ID "smart-water-meter-bt-iot"
#define USER_EMAIL "btiotsmartwatermeter@gmail.com"
#define USER_PASSWORD "Swm@iot7"
// Add these lines BEFORE setup()
FirebaseData fbdo; 
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(9600); // Updated baud rate
  delay(2000); // Allow time for Serial Monitor connection

  // Initialize LoRa with retries
  Serial.println("Initializing LoRa...");
  LoRa.setPins(SS, RST, DIO0);
  int loraRetries = 0;
  while (!LoRa.begin(433E6) && loraRetries < 10) {
    Serial.print(".");
    delay(500);
    loraRetries++;
  }
  if (loraRetries >= 10) {
    Serial.println(" LoRa init failed! Check wiring/frequency");
    while(1);
  }
  Serial.println(" LoRa initialized!");

  // Enhanced WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
    Serial.print(".");
    delay(300);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" WiFi connection failed!");
    return;
  }
  Serial.println("\nConnected to WiFi");

  // Firebase initialization check
  Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
  
  // Wait for authentication
  Serial.print("Authenticating with Firebase");
  while (Firebase.ready() == false && millis() - wifiStart < 15000) {
    Serial.print(".");
    delay(300);
  }
  if (!Firebase.ready()) {
    Serial.println(" Firebase auth failed!");
    return;
  }
  Serial.println(" Firebase authenticated");
}

void loop() {

  // Try to parse a packet from LoRa
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read(); // Read each byte as a character
    }
    
    Serial.printf("[LORA] Received: '%s' (RSSI: %d)\n", 
                  receivedData.c_str(), LoRa.packetRssi());

    // Prepare to send data to Firebase
    FirebaseJson content;
    content.set("fields/Message/stringValue", receivedData); // Set the message field

    // Create a unique document ID using the current timestamp or any unique identifier
    String documentId = String(millis()); // Using millis() as a unique ID

    // Send data to Firebase Firestore using createDocument with all required parameters
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", "EspData", documentId.c_str(), content.raw(), "")) {
      Serial.printf("[FIREBASE] Document created successfully: %s\n", fbdo.payload().c_str());
    } else {
      Serial.printf("[FIREBASE] Error: %s\n", fbdo.errorReason().c_str());
    }
  } else {
    Serial.println("No packet received.");
  }

}
