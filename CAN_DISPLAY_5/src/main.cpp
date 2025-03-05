#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#define HEIGHT 240
#define WIDTH  240
#define NUM_DISPLAYS 5
const int CS_PINS[NUM_DISPLAYS] = { 13, 33, 32, 25, 21 };

TFT_eSPI tft = TFT_eSPI();

// Structure for received CAN message
typedef struct struct_message {
    unsigned long canId;
    uint8_t len;
    uint8_t data[8];
} struct_message;

// Structure for display data
typedef struct display_data {
    float vcell[16]; // Vcell1-16
    float voltT, a;  // From 2214625280
    float t[4];      // T1-T4
    float s6;        // Only S6 (labeled as SOC)
} display_data;

struct_message receivedMessage;
volatile display_data myData = {0};
display_data lastData = {0};
volatile bool newDataAvailable = false;

// Update a single value on a display at a specific position (centered)
void drawValue(int csPin, const char* label, float value, int x, int y, uint16_t color, bool largeFont = false) {
    digitalWrite(csPin, LOW);
    tft.startWrite();
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextDatum(MC_DATUM); // Middle-center for all
    char buffer[20];
    sprintf(buffer, "%s %.2f", label, value);
    
    if (largeFont) {
        tft.setTextSize(3); // Larger font for SOC, A, VoltT (24x32px)
    } else {
        tft.setTextSize(2); // Smaller font for grids (6x8px)
    }
    tft.drawString(buffer, x, y);
    tft.endWrite();
    digitalWrite(csPin, HIGH);
}

// Update Display 0 (Vcell1-6, custom X, Y coordinates)
void updateDisplay0() {
    digitalWrite(CS_PINS[0], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    // Define your custom X, Y coordinates here
    drawValue(CS_PINS[0], "VCell1", myData.vcell[0], 120, 20+10, TFT_WHITE);  // Example: top center
    drawValue(CS_PINS[0], "VCell2", myData.vcell[1], 120, 40+10+15, TFT_WHITE);
    drawValue(CS_PINS[0], "VCell3", myData.vcell[2], 120, 60+10+30, TFT_WHITE);
    drawValue(CS_PINS[0], "VCell4", myData.vcell[3], 120, 80+10+45, TFT_WHITE);
    drawValue(CS_PINS[0], "VCell5", myData.vcell[4], 120, 100+10+60, TFT_WHITE);
    drawValue(CS_PINS[0], "VCell6", myData.vcell[5], 120, 120+10+75, TFT_WHITE);
    tft.endWrite();
    digitalWrite(CS_PINS[0], HIGH);
}

// Update Display 1 (Vcell7-12, custom X, Y coordinates)
void updateDisplay1() {
    digitalWrite(CS_PINS[1], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    // Define your custom X, Y coordinates here
    drawValue(CS_PINS[1], "VCell7", myData.vcell[6], 120, 20+10, TFT_WHITE);  // Example: top center
    drawValue(CS_PINS[1], "VCell8", myData.vcell[7], 120, 40+10+15, TFT_WHITE);
    drawValue(CS_PINS[1], "VCell9", myData.vcell[8], 120, 60+10+30, TFT_WHITE);
    drawValue(CS_PINS[1], "VCell10", myData.vcell[9], 120, 80+10+45, TFT_WHITE);
    drawValue(CS_PINS[1], "VCell11", myData.vcell[10], 120, 100+10+60, TFT_WHITE);
    drawValue(CS_PINS[1], "VCell12", myData.vcell[11], 120, 120+10+75, TFT_WHITE);
    tft.endWrite();
    digitalWrite(CS_PINS[1], HIGH);
}

// Update Display 2 (Vcell13-16, T1-T4, custom X, Y coordinates)
void updateDisplay2() {
    digitalWrite(CS_PINS[2], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    // Define your custom X, Y coordinates here
    drawValue(CS_PINS[2], "VCell13", myData.vcell[12], 120, 20+10, TFT_WHITE);  // Example: top center
    drawValue(CS_PINS[2], "VCell14", myData.vcell[13], 120, 35+10+10, TFT_WHITE);
    drawValue(CS_PINS[2], "VCell15", myData.vcell[14], 120, 50+10+20, TFT_WHITE);
    drawValue(CS_PINS[2], "VCell16", myData.vcell[15], 120, 65+10+30, TFT_WHITE);
    drawValue(CS_PINS[2], "Temp1", myData.t[0], 120, 80+10+40, TFT_WHITE);
    drawValue(CS_PINS[2], "Temp2", myData.t[1], 120, 95+10+50, TFT_WHITE);
    drawValue(CS_PINS[2], "Temp3", myData.t[2], 120, 110+10+60, TFT_WHITE);
    drawValue(CS_PINS[2], "Temp4", myData.t[3], 120, 125+10+70, TFT_WHITE);
    tft.endWrite();
    digitalWrite(CS_PINS[2], HIGH);
}

// Update Display 4 (A, VoltT, vertical split)
void updateDisplay4() {
    digitalWrite(CS_PINS[4], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawValue(CS_PINS[4], "A", myData.a, WIDTH / 2, HEIGHT / 4, TFT_WHITE, true);
    drawValue(CS_PINS[4], "VoltT", myData.voltT, WIDTH / 2, HEIGHT *3 / 4, TFT_WHITE, true);
    tft.endWrite();
    digitalWrite(CS_PINS[4], HIGH);
}

// Update Display 5 (S6 only, labeled as SOC)
void updateDisplay5() {
    digitalWrite(CS_PINS[3], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawValue(CS_PINS[3], "SOC", myData.s6, WIDTH / 2, HEIGHT / 2, TFT_WHITE, true);
    tft.endWrite();
    digitalWrite(CS_PINS[3], HIGH);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));
    
    Serial.print("CAN ID: "); Serial.print(receivedMessage.canId);
    Serial.print(" | Len: "); Serial.println(receivedMessage.len);

    if (receivedMessage.canId == 2281734144) {
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            myData.vcell[i / 2] = (float)word / 1000.0;
        }
    }
    else if (receivedMessage.canId == 2281799680) {
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            myData.vcell[i / 2 + 4] = (float)word / 1000.0;
        }
    }
    else if (receivedMessage.canId == 2281865216) {
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            myData.vcell[i / 2 + 8] = (float)word / 1000.0;
        }
    }
    else if (receivedMessage.canId == 2281930752) {
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            myData.vcell[i / 2 + 12] = (float)word / 1000.0;
        }
    }
    else if (receivedMessage.canId == 2214625280) {
        for (int i = 0; i < receivedMessage.len; i += 2) {
            uint16_t word = (receivedMessage.data[i] << 8) | (i + 1 < receivedMessage.len ? receivedMessage.data[i + 1] : 0);
            if (i == 0) myData.voltT = (float)word / 10.0;
            else if (i == 2) myData.a = ((float)word - 30000.0) / 10.0;
        }
    }
    else if (receivedMessage.canId == 2415951872) {
        for (int i = 0; i < min((int)4, (int)receivedMessage.len); i++) {
            myData.t[i] = (float)(receivedMessage.data[i] - 40);
        }
    }
    else if (receivedMessage.canId == 2214756352) {
        for (int i = 0; i < receivedMessage.len; i++) {
            if (i == 5 && i + 1 < receivedMessage.len) {
                uint16_t byte56 = (receivedMessage.data[i] << 8) | receivedMessage.data[i + 1];
                myData.s6 = (float)byte56 / 160.0;
                Serial.print("S6 updated to: "); Serial.println(myData.s6);
                break;
            }
        }
    }
    newDataAvailable = true;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }

    tft.begin();
    tft.setRotation(0);
    for (int i = 0; i < NUM_DISPLAYS; i++) {
        pinMode(CS_PINS[i], OUTPUT);
        digitalWrite(CS_PINS[i], HIGH);
        digitalWrite(CS_PINS[i], LOW);
        tft.startWrite();
        tft.fillScreen(TFT_BLACK);
        tft.endWrite();
        digitalWrite(CS_PINS[i], HIGH);
    }

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        while (1) delay(100);
    }
    esp_now_register_recv_cb(OnDataRecv);

    Serial.println("WT32-ETH01 ESP-NOW Receiver with TFT");
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
}

void loop() {
    if (newDataAvailable) {
        noInterrupts();
        display_data localData;
        memcpy(&localData, (void*)&myData, sizeof(display_data));
        newDataAvailable = false;
        interrupts();

        if (memcmp(&localData.vcell[0], &lastData.vcell[0], 6 * sizeof(float)) != 0) {
            updateDisplay0();
            memcpy(&lastData.vcell[0], &localData.vcell[0], 6 * sizeof(float));
        }
        if (memcmp(&localData.vcell[6], &lastData.vcell[6], 6 * sizeof(float)) != 0) {
            updateDisplay1();
            memcpy(&lastData.vcell[6], &localData.vcell[6], 6 * sizeof(float));
        }
        if (memcmp(&localData.vcell[12], &lastData.vcell[12], 4 * sizeof(float)) != 0 || 
            memcmp(&localData.t, &lastData.t, 4 * sizeof(float)) != 0) {
            updateDisplay2();
            memcpy(&lastData.vcell[12], &localData.vcell[12], 4 * sizeof(float));
            memcpy(&lastData.t, &lastData.t, 4 * sizeof(float));
        }
        if (localData.a != lastData.a || localData.voltT != lastData.voltT) {
            updateDisplay4();
            lastData.a = localData.a;
            lastData.voltT = localData.voltT;
        }
        if (localData.s6 != lastData.s6) {
            updateDisplay5();
            lastData.s6 = localData.s6;
            Serial.println("Updating Display 5 with S6");
        }
    }
    delay(100);
}