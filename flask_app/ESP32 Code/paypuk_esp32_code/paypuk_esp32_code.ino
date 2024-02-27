#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WebServer.h>
#include <EEPROM.h>

// Configuration
char ssid[50];
char password[50];
char serverUrl[100];
bool bar_paypuk = false;

bool config_mode = false;

// Custom ID for the ESP32
const String espId = "ESP32_CUSTOM_ID";

// Define the pins used for the SPI connection with the RFID module
#define RST_PIN     22          // Configurable, see typical pin layout above
#define SS_PIN      21          // Configurable, see typical pin layout above
#define RESET_BUTTON_PIN 27     // Button to reset EEPROM data

// Create MFRC522 instance
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

WebServer server(80);

void setup() {
    Serial.begin(115200);
    delay(100);

    // Setup button pin
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

    // Initialize RFID reader
    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522

    // Load data from EEPROM
    EEPROM.begin(512);
    EEPROM.get(0, ssid);
    EEPROM.get(50, password);
    EEPROM.get(100, serverUrl);
    EEPROM.get(200, bar_paypuk);

    Serial.print(ssid);
    Serial.print(password);
    Serial.print(serverUrl);

    // Start WiFi AP if credentials are not set
    if (strcmp(ssid, "None") == 0 && strcmp(password, "None") == 0) {
      config_mode = true;
      strcpy(serverUrl, "http://{Server-IP}:5000/data");
      startAccessPoint();
    } 
    else {
        connectToWiFi();
    }

    // Start server
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.begin();

    Serial.println("HTTP server started");

    // Setup button pin
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
    if(config_mode) server.handleClient();
    else{

    // Check if button is pressed to reset EEPROM data
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      Serial.println("Reset Start");
        delay(5000); //Press Button for 5 seconds
        if (digitalRead(RESET_BUTTON_PIN) == LOW) {
          resetEEPROMData();
          delay(500); // Debouncing delay
        }
        else Serial.println("No Reset");
    }

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


}

void startAccessPoint() {
    Serial.println("Starting access point...");
    WiFi.softAP(espId, "password");
    Serial.println("Access point started");
}

void connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");

    // Check if button is pressed to reset EEPROM data
    if (digitalRead(RESET_BUTTON_PIN) == LOW) {
      Serial.println("Reset Start");
        delay(5000); //Press Button for 5 seconds
        if (digitalRead(RESET_BUTTON_PIN) == LOW) {
          resetEEPROMData();
          delay(500); // Debouncing delay
        }
        else Serial.println("No Reset");
    }

    }
    Serial.println("Connected to WiFi");
}

void handleRoot() {
    String content = "<html><body>";
    content += "<h1>PayPuk Configuration</h1>";
    content += "<form action='/save' method='post'>";
    content += "Wifi-SSID: <input type='text' name='ssid' value='" + String(ssid) + "'><br>";
    content += "Wifi-Password: <input type='password' name='password' value='" + String(password) + "'><br>";
    content += "Paypuk Server URL: <input type='text' name='serverUrl' value='" + String(serverUrl) + "'><br>";
    content += "Use Bar Paypuk: <input type='checkbox' name='bar_paypuk' " + String(bar_paypuk ? "checked" : "") + "><br>"; // Checkbox for bar_paypuk
    content += "<input type='submit' value='Save'>";
    content += "</form></body></html>";
    server.send(200, "text/html", content);
}

void handleSave() {
    String ssidNew = server.arg("ssid");
    String passwordNew = server.arg("password");
    String serverUrlNew = server.arg("serverUrl");
    bar_paypuk = server.hasArg("bar_paypuk");

    ssidNew.toCharArray(ssid, 50);
    passwordNew.toCharArray(password, 50);
    serverUrlNew.toCharArray(serverUrl, 100);

    // Save to EEPROM
    EEPROM.put(0, ssid);
    EEPROM.put(50, password);
    EEPROM.put(100, serverUrl);
    EEPROM.put(200, bar_paypuk);
    EEPROM.commit();

    server.send(200, "text/plain", "Configuration saved. Restarting...");
    delay(500);
    ESP.restart();
}

void sendDataToServer(String tagId) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Prepare data
    String postData = "{\"rfid_id\":\"" + tagId + "\",\"esp_id\":\"" + espId + "\",\"bar_paypuk\":\"" + bar_paypuk + "\"}";

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

void resetEEPROMData() {
    strcpy(ssid, "None");
    strcpy(password, "None");
    strcpy(serverUrl, "None");
    bar_paypuk = false;
    

    EEPROM.put(0, ssid);
    EEPROM.put(50, password);
    EEPROM.put(100, serverUrl);
    EEPROM.put(200, bar_paypuk);
    EEPROM.commit();

    Serial.println("EEPROM data reset to 'None'");
    ESP.restart();
}
