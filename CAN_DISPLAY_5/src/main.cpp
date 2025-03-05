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
    float s6;        // Only S6
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
        tft.setTextSize(3); // Larger font for S6, A, VoltT (18x24px)
    } else {
        tft.setTextSize(1); // Default font for grids (12x16px)
    }
    tft.drawString(buffer, x, y);
    tft.endWrite();
    digitalWrite(csPin, HIGH);
}

// Update Display 0 (Vcell1-6, 2x3 grid)
void updateDisplay0() {
    digitalWrite(CS_PINS[0], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    for (int i = 0; i < 6; i++) {
        int x = (i % 2) * 120 + 60;
        int y = (i / 2) * 80 + 40;
        char label[8];
        sprintf(label, "VCell%d", i + 1);
        drawValue(CS_PINS[0], label, myData.vcell[i], x, y, TFT_WHITE);
    }
    tft.endWrite();
    digitalWrite(CS_PINS[0], HIGH);
}

// Update Display 1 (Vcell7-12, 2x3 grid)
void updateDisplay1() {
    digitalWrite(CS_PINS[1], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    for (int i = 0; i < 6; i++) {
        int x = (i % 2) * 120 + 60;
        int y = (i / 2) * 80 + 40;
        char label[8];
        sprintf(label, "VCell%d", i + 7);
        drawValue(CS_PINS[1], label, myData.vcell[i + 6], x, y, TFT_WHITE);
    }
    tft.endWrite();
    digitalWrite(CS_PINS[1], HIGH);
}

// Update Display 2 (Vcell13-16, T1-T4, 2x4 grid)
void updateDisplay2() {
    digitalWrite(CS_PINS[2], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    for (int i = 0; i < 4; i++) {
        int x = (i % 2) * 120 + 60;
        int y = (i / 2) * 60 + 30;
        char label[8];
        sprintf(label, "VCell%d", i + 13);
        drawValue(CS_PINS[2], label, myData.vcell[i + 12], x, y, TFT_WHITE);
    }
    for (int i = 0; i < 4; i++) {
        int x = (i % 2) * 120 + 60;
        int y = (i / 2 + 2) * 60 + 30;
        char label[4];
        sprintf(label, "Temp%d", i + 1);
        drawValue(CS_PINS[2], label, myData.t[i], x, y, TFT_WHITE);
    }
    tft.endWrite();
    digitalWrite(CS_PINS[2], HIGH);
}

// Update Display 4 (A, VoltT, vertical split)
void updateDisplay4() {
    digitalWrite(CS_PINS[4], LOW);
    tft.startWrite();
    tft.fillScreen(TFT_BLACK);
    drawValue(CS_PINS[4], "A", myData.a, WIDTH / 2, HEIGHT / 4, TFT_WHITE, true);
    drawValue(CS_PINS[4], "VoltT", myData.voltT, WIDTH / 2, HEIGHT * 3 / 4, TFT_WHITE, true);
    tft.endWrite();
    digitalWrite(CS_PINS[4], HIGH);
}

// Update Display 5 (S6 only)
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