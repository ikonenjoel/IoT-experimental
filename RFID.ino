#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Constants.h>
#include <MFRC522Debug.h>
#include <WiFi.h>
#include <HTTPClient.h>

// RFID Pins (ESP32 VSPI)
const uint8_t SS_PIN  = 5;
const uint8_t RST_PIN = 22;

// RGB LED Pins
const uint8_t RED_PIN   = 25;
const uint8_t GREEN_PIN = 26;
const uint8_t BLUE_PIN  = 27;

MFRC522DriverPinSimple ss_pin(SS_PIN);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// Replace with your development machine's local IP address and port
const char* serverName = "http://192.168.1.39:8000/log_messages.php";
const char* ssid = "Replace me";
const char* password = "Replace me";

// Helper function to set RGB LED colors
void setLED(bool red, bool green, bool blue) {
  digitalWrite(RED_PIN, red ? HIGH : LOW);
  digitalWrite(GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(BLUE_PIN, blue ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Yellow: Connecting
  setLED(true, true, false);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi. IP: ");
  Serial.println(WiFi.localIP());

  // Hardware reset pulse for MFRC522
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(50);
  digitalWrite(RST_PIN, HIGH);
  delay(50);

  SPI.begin();
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  mfrc522.PCD_SetAntennaGain(MFRC522Constants::RxGain_48dB);
  mfrc522.PCD_AntennaOn();

  // Solid Blue: Ready / Standby
  setLED(false, false, true);
  Serial.println("Ready. Scan a card...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Construct hexadecimal UID string
  String uidStr = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    char buff[4];
    sprintf(buff, "%02X", mfrc522.uid.uidByte[i]);
    uidStr += buff;
  }

  Serial.println("----------------------------------------");
  Serial.print("Scanned UID: ");
  Serial.println(uidStr);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String httpRequestData = "uid=" + uidStr;
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.printf("HTTP Code: %d | Server Response: %s\n", httpResponseCode, response.c_str());

      // Parse verdict returned by the server
      if (response.indexOf("\"status\":\"accepted\"") >= 0) {
        setLED(false, true, false); // Green
        Serial.println("Server Verdict: Accepted");
      } else {
        setLED(true, false, false); // Red
        Serial.println("Server Verdict: Denied");
      }
    } else {
      Serial.printf("HTTP Request failed: %s\n", http.errorToString(httpResponseCode).c_str());
      setLED(true, false, false); // Flash Red on network error
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(1500); // Display result for 1.5 seconds

  // Return to Blue (Standby)
  setLED(false, false, true);
}
