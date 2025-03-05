#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>  // Added for WiFi.mode() and WIFI_STA

// Structure for received CAN message
typedef struct struct_message {
    unsigned long canId;
    uint8_t len;
    uint8_t data[8];
} struct_message;

struct_message receivedMessage;

// Callback function when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
    
    Serial.print("Received CAN ID: ");
    Serial.print(receivedMessage.canId);
    Serial.print("|Length:");
    Serial.print(receivedMessage.len);

    // Buffer to store formatted data
    char payload[256];
    String dataString = "";

    // Process data and create JSON string based on CAN ID
    if (receivedMessage.canId == 2281734144) {
        dataString = "{";
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            float decimalValue = (float)word / 1000.0;
            char dec[10];
            sprintf(dec, "%.3f", decimalValue);
            if (i == 0) dataString += "\"Vcell1\": " + String(dec);
            else if (i == 2) dataString += ", \"Vcell2\": " + String(dec);
            else if (i == 4) dataString += ", \"Vcell3\": " + String(dec);
            else if (i == 6) dataString += ", \"Vcell4\": " + String(dec);
        }
        dataString += "}";
    }
    else if (receivedMessage.canId == 2281799680) {
        dataString = "{";
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            float decimalValue = (float)word / 1000.0;
            char dec[10];
            sprintf(dec, "%.3f", decimalValue);
            if (i == 0) dataString += "\"Vcell5\": " + String(dec);
            else if (i == 2) dataString += ", \"Vcell6\": " + String(dec);
            else if (i == 4) dataString += ", \"Vcell7\": " + String(dec);
            else if (i == 6) dataString += ", \"Vcell8\": " + String(dec);
        }
        dataString += "}";
    }
    else if (receivedMessage.canId == 2281865216) {
        dataString = "{";
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            float decimalValue = (float)word / 1000.0;
            char dec[10];
            sprintf(dec, "%.3f", decimalValue);
            if (i == 0) dataString += "\"Vcell9\": " + String(dec);
            else if (i == 2) dataString += ", \"Vcell10\": " + String(dec);
            else if (i == 4) dataString += ", \"Vcell11\": " + String(dec);
            else if (i == 6) dataString += ", \"Vcell12\": " + String(dec);
        }
        dataString += "}";
    }
    else if (receivedMessage.canId == 2281930752) {
        dataString = "{";
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            float decimalValue = (float)word / 1000.0;
            char dec[10];
            sprintf(dec, "%.3f", decimalValue);
            if (i == 0) dataString += "\"Vcell13\": " + String(dec);
            else if (i == 2) dataString += ", \"Vcell14\": " + String(dec);
            else if (i == 4) dataString += ", \"Vcell15\": " + String(dec);
            else if (i == 6) dataString += ", \"Vcell16\": " + String(dec);
        }
        dataString += "}";
    }
    else if (receivedMessage.canId == 2214625280) {
        dataString = "{";
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            float decimalValue;
            if (i == 0 || i == 4) {
                decimalValue = (float)word / 10.0;
            } else {
                decimalValue = ((float)word - 30000.0) / 10.0;
            }
            char dec[10];
            sprintf(dec, "%.3f", decimalValue);
            if (i == 0) dataString += "\"VoltT\": " + String(dec);
            else if (i == 2) dataString += ", \"A\": " + String(dec);
            else if (i == 4) dataString += ", \"VoltO\": " + String(dec);
            else if (i == 6) dataString += ", \"A2\": " + String(dec);
        }
        dataString += "}";
    }
    else if (receivedMessage.canId == 2415951872) {
        dataString = "{";
        for (int i = 0; i < min((int)4, (int)receivedMessage.len); i++) {
            int decimalValue = receivedMessage.data[i] - 40;
            char dec[10];
            sprintf(dec, "%d", decimalValue);
            if (i == 0) dataString += "\"T1\": " + String(dec);
            else if (i == 1) dataString += ", \"T2\": " + String(dec);
            else if (i == 2) dataString += ", \"T3\": " + String(dec);
            else if (i == 3) dataString += ", \"T4\": " + String(dec);
        }
        dataString += "}";
    }
    else if (receivedMessage.canId == 2214756352) { // 84028000 in decimal
        dataString = "{";
        for (int i = 0; i < receivedMessage.len; i++) {
            float decimalValue;
            if (i == 5 && i + 1 < receivedMessage.len) {
                uint16_t byte56 = (receivedMessage.data[i] << 8) | receivedMessage.data[i + 1];
                decimalValue = (float)byte56 / 160.0;
                i++; // Skip the next byte
            } else {
                decimalValue = (float)receivedMessage.data[i] / 160.0;
            }
            char dec[10];
            sprintf(dec, "%.3f", decimalValue);
            if (i == 0) dataString += "\"S1\": " + String(dec);
            else if (i == 1) dataString += ", \"S2\": " + String(dec);
            else if (i == 2) dataString += ", \"S3\": " + String(dec);
            else if (i == 3) dataString += ", \"S4\": " + String(dec);
            else if (i == 4) dataString += ", \"S5\": " + String(dec);
            else if (i == 5) dataString += ", \"S6\": " + String(dec);
            else if (i == 6) dataString += ", \"S7\": " + String(dec);
        }
        dataString += "}";
    }
    else {
        dataString = "{}"; // Empty JSON object for unhandled CAN IDs
    }

    // Print to Serial (for debugging)
    Serial.println(dataString);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial to initialize
    }
    Serial.println("WT32-ETH01 ESP-NOW Receiver");

    // Initialize ESP-NOW
    WiFi.mode(WIFI_STA);  // Set to Station mode for ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        while (1) delay(100);
    }

    // Register callback for receiving data
    esp_now_register_recv_cb(OnDataRecv);

    Serial.println("Receiver Ready");
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
}

void loop() {
    delay(100);  // Small delay to prevent watchdog issues
}