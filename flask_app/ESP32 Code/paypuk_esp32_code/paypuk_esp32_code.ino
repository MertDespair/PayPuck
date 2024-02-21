#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// WiFi credentials
const char* ssid = "Kripo-Ueberwachungswagen";
const char* password = "Fu$kSiemens";

// Server URL
const char* serverUrl = "http://192.168.137.180:5000/data"; // Ersetzen Sie YOUR_PC_IP_ADDRESS durch die IP-Adresse Ihres PCs


// Custom ID für den ESP32
const String espId = "ESP32_CUSTOM_ID";

// Define the pins used for the SPI connection with the RFID module
#define RST_PIN     22          // Configurable, see typical pin layout above
#define SS_PIN      21          // Configurable, see typical pin layout above

// Create MFRC522 instance
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
    Serial.begin(115200);
    delay(100);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize RFID reader
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522

    Serial.println("RFID reader initialized");
}

void loop() {
    // Look for new cards
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        // Read card UID
        String tagId = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            tagId += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
            tagId += String(mfrc522.uid.uidByte[i], HEX);
        }
        tagId.toUpperCase();

        // Send data to server
        sendDataToServer(tagId);

        // Halt for a moment to avoid multiple readings
        delay(1000);
    }
}

void sendDataToServer(String tagId) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Prepare data
    String postData = "{\"rfid_id\":\"" + tagId + "\",\"esp_id\":\"" + espId + "\"}";

    // Send data to server
    int httpResponseCode = http.POST(postData);
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}
