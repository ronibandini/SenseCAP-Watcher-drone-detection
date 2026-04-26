// Drone Watcher with SenseCAP Watcher and Wio Terminal
// Receives UART notifications from SenseCap Watcher via UART
// Pins: 40-pin header Pin 10 (RX) = BCM15/PB26, Pin 6/9 (GND)
// Roni Bandini 4/2026 MIT License
// https://www.instagram.com/ronibandini/

#include "TFT_eSPI.h"
#include "Free_Fonts.h"
#include <Seeed_Arduino_FS.h>
#include <ArduinoJson.h> 

TFT_eSPI tft;

// --- Settings ---
#define MAX_EVENTS  20
#define UART_BAUD   115200
#define LOG_FILE    "/drone_log.txt"
#define DEBOUNCE_MS 200

#define COL_BG       0x0841
#define COL_PANEL    0x10A3
#define COL_ACCENT   0xF800
#define COL_GREEN    0x07E0
#define COL_AMBER    0xFD20
#define COL_WHITE    0xFFFF
#define COL_GRAY     0x8410
#define COL_DARKGRAY 0x2945
#define COL_CYAN     0x07FF

struct DroneEvent {
  char timestamp[20]; // "HH:MM:SS" from millis
};

DroneEvent events[MAX_EVENTS];
int   eventCount   = 0;
int   scrollOffset = 0;
int   currentView  = 0;   // 0=dashboard, 1=log
bool  newAlert     = false;
unsigned long lastBlink   = 0;
bool  blinkState   = false;
unsigned long lastDebounce = 0;


void millisToTimeStr(unsigned long ms, char* buf) {
  unsigned long totalSec = ms / 1000;
  int h = totalSec / 3600;
  int m = (totalSec % 3600) / 60;
  int s = totalSec % 60;
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
}

// -----------------------------------------------
// Drone icon sort of
// -----------------------------------------------
void drawDroneIcon(int cx, int cy, int sz, uint16_t color) {
  int arm = sz;
  tft.drawLine(cx - arm, cy - arm, cx + arm, cy + arm, color);
  tft.drawLine(cx + arm, cy - arm, cx - arm, cy + arm, color);
  tft.drawLine(cx - arm - 1, cy - arm, cx + arm - 1, cy + arm, color);
  tft.drawLine(cx + arm + 1, cy - arm, cx - arm + 1, cy + arm, color);

  tft.fillCircle(cx, cy, sz / 3, color);
  tft.fillCircle(cx, cy, sz / 5, COL_BG);

  int motorR = sz / 4;
  tft.fillCircle(cx - arm, cy - arm, motorR, color);
  tft.fillCircle(cx + arm, cy - arm, motorR, color);
  tft.fillCircle(cx - arm, cy + arm, motorR, color);
  tft.fillCircle(cx + arm, cy + arm, motorR, color);

  int pr = sz / 3 + 2;
  tft.drawCircle(cx - arm, cy - arm, pr, COL_CYAN);
  tft.drawCircle(cx + arm, cy - arm, pr, COL_CYAN);
  tft.drawCircle(cx - arm, cy + arm, pr, COL_CYAN);
  tft.drawCircle(cx + arm, cy + arm, pr, COL_CYAN);

  tft.drawLine(cx - sz/3, cy + sz/3, cx - sz/2, cy + sz/2 + sz/4, color);
  tft.drawLine(cx + sz/3, cy + sz/3, cx + sz/2, cy + sz/2 + sz/4, color);
  tft.drawLine(cx - sz/2 - 4, cy + sz/2 + sz/4,
               cx + sz/2 + 4, cy + sz/2 + sz/4, color);
}


void drawHeader() {
  tft.fillRect(0, 0, 320, 28, COL_PANEL);
  tft.drawFastHLine(0, 28, 320, COL_ACCENT);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COL_ACCENT);
  tft.drawString("DRONE WATCH", 8, 7);

  // Uptime 
  char timeBuf[12];
  millisToTimeStr(millis(), timeBuf);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COL_GRAY);
  tft.drawString(timeBuf, 210, 7);

}
// -----------------------------------------------
// Dashboard 
// -----------------------------------------------

void drawDashboard() {
  tft.fillScreen(COL_BG);
  drawHeader();

  tft.fillRoundRect(4, 34, 312, 50, 4, COL_PANEL);
  tft.drawRoundRect(4, 34, 312, 50, 4, COL_DARKGRAY);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.setTextColor(COL_GRAY);
  tft.drawString("STATUS", 12, 42);
  tft.drawString("DETECTIONS", 170, 42);

  tft.setFreeFont(&FreeSansBold12pt7b);
  if (newAlert) {
    tft.setTextColor(COL_ACCENT);
    tft.drawString("ALERT", 12, 60);
  } else {
    tft.setTextColor(COL_GREEN);
    tft.drawString("OK", 12, 60);
  }

  tft.setTextColor(COL_WHITE);
  char countStr[6];
  sprintf(countStr, "%d", eventCount);
  tft.drawString(countStr, 185, 60);

  // Drone icon, chico
  int iconCX = 160;
  int iconCY = 140;    
  uint16_t iconColor = newAlert ? COL_ACCENT : COL_CYAN;
  drawDroneIcon(iconCX, iconCY, 32, iconColor);

  if (newAlert) {
    tft.drawCircle(iconCX, iconCY, 50, COL_ACCENT);
    tft.drawCircle(iconCX, iconCY, 64, blinkState ? COL_AMBER : COL_BG);
    tft.drawCircle(iconCX, iconCY, 78, blinkState ? COL_ACCENT : COL_BG);
  } else {
    tft.drawCircle(iconCX, iconCY, 50, COL_DARKGRAY);
    tft.drawCircle(iconCX, iconCY, 64, COL_DARKGRAY);
  }


  tft.fillRect(0, 196, 320, 26, COL_PANEL);   
  tft.setFreeFont(&FreeSans9pt7b);
  if (eventCount > 0) {
    tft.setTextColor(COL_GRAY);
    tft.drawString("LAST:", 6, 202);
    tft.setTextColor(COL_AMBER);
    tft.drawString(events[eventCount - 1].timestamp, 60, 202);
  } else {
    tft.setTextColor(COL_DARKGRAY);
    tft.drawString("NO EVENTS YET", 90, 202);
  }

  // Footer 
  tft.fillRect(0, 222, 320, 18, COL_DARKGRAY);
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(COL_GRAY);
  tft.drawString("[RIGHT]=Log  [PRESS]=Clear Roni Bandini 4/2026", 10, 226);
}


void drawLogView() {
  tft.fillScreen(COL_BG);
  drawHeader();

  tft.fillRect(0, 29, 320, 14, COL_DARKGRAY);
  tft.setFreeFont(NULL);
  tft.setTextSize(1);
  tft.setTextColor(COL_GRAY);
  tft.drawString("  # UPTIME AT DETECTION", 4, 31);

  int visibleRows = 8;
  int rowH        = 22;
  int startY      = 46;

  for (int i = 0; i < visibleRows; i++) {
    int idx = scrollOffset + i;
    if (idx >= eventCount) break;

    int y      = startY + i * rowH;
    bool isLast = (idx == eventCount - 1);

    uint16_t rowBg = (i % 2 == 0) ? COL_BG : COL_PANEL;
    tft.fillRect(0, y, 320, rowH - 2, rowBg);

    if (isLast) {
      tft.drawFastHLine(0, y, 320, COL_ACCENT);
      tft.drawFastHLine(0, y + rowH - 2, 320, COL_ACCENT);
    }

    tft.setFreeFont(NULL);
    tft.setTextSize(1);
    tft.setTextColor(COL_GRAY);
    char idxStr[4];
    sprintf(idxStr, "%2d", idx + 1);
    tft.drawString(idxStr, 4, y + 6);

    // drone icon
    tft.fillCircle(28, y + rowH / 2, 3, isLast ? COL_ACCENT : COL_CYAN);
    tft.drawLine(23, y + rowH/2 - 4, 33, y + rowH/2 + 4, isLast ? COL_ACCENT : COL_CYAN);
    tft.drawLine(33, y + rowH/2 - 4, 23, y + rowH/2 + 4, isLast ? COL_ACCENT : COL_CYAN);

    tft.setTextColor(isLast ? COL_AMBER : COL_WHITE);
    tft.drawString(events[idx].timestamp, 42, y + 6);
  }

  if (eventCount > visibleRows) {
    int barH   = 220 - startY;
    int thumbH = max(10, barH * visibleRows / eventCount);
    int thumbY = startY + (barH - thumbH) * scrollOffset / (eventCount - visibleRows);
    tft.fillRect(314, startY, 6, barH, COL_DARKGRAY);
    tft.fillRect(314, thumbY, 6, thumbH, COL_ACCENT);
  }

  tft.drawFastHLine(0, 222, 320, COL_DARKGRAY);
  tft.setTextColor(COL_DARKGRAY);
  tft.drawString("[LEFT] Back  [UP/DOWN] Scroll", 8, 225);
}


void refreshView() {
  if (currentView == 0) drawDashboard();
  else drawLogView();
}


void addEvent() {
  char ts[20];
  millisToTimeStr(millis(), ts);

  if (eventCount < MAX_EVENTS) {
    strncpy(events[eventCount].timestamp, ts, 19);
    events[eventCount].timestamp[19] = '\0';
    eventCount++;
  } else {
    for (int i = 0; i < MAX_EVENTS - 1; i++) events[i] = events[i + 1];
    strncpy(events[MAX_EVENTS - 1].timestamp, ts, 19);
    events[MAX_EVENTS - 1].timestamp[19] = '\0';
  }

  File f = SD.open(LOG_FILE, FILE_WRITE);
  if (f) { f.println(ts); f.close(); }

  newAlert   = true;
  blinkState = true;
  lastBlink  = millis();
}

// -----------------------------------------------
// UART decoding :)
// -----------------------------------------------
void parseSerial() {
  static String buffer = "";
  static int braceDepth = 0;
  static bool inString = false;
  static bool escape = false;

  while (Serial1.available()) {
    char c = Serial1.read();
    Serial.print(c);   

 
    if (buffer.length() == 0 && c != '{') {
      continue;   
    }

    buffer += c;

    if (escape) {
      escape = false;
    } else if (c == '\\' && inString) {
      escape = true;
    } else if (c == '"') {
      inString = !inString;
    } else if (!inString) {
      if (c == '{') braceDepth++;
      else if (c == '}') braceDepth--;
    }

    // all keys closed
    if (braceDepth == 0 && buffer.length() > 1) {
      Serial.println();
      Serial.print(">> JSON completo (");
      Serial.print(buffer.length());
      Serial.println(" bytes)");

      DynamicJsonDocument doc(1024 * 100 + 512);
      DeserializationError err = deserializeJson(doc, buffer);

      if (err) {
        Serial.print(">> Error JSON: ");
        Serial.println(err.c_str());
      } else {
        if (doc.containsKey("prompt")) {
          String prompt = doc["prompt"].as<String>();
          Serial.print(">> prompt: ");
          Serial.println(prompt);
          prompt.toUpperCase();
          if (prompt.indexOf("DRONE") >= 0) {
            Serial.println(">> DRONE DETECTADO");
            addEvent();
            refreshView();
          }
        }
      }

      buffer = "";
      braceDepth = 0;
      inString = false;
      escape = false;
    }

    // Check overflow
    if (buffer.length() > 110000) {
      Serial.println(">> Buffer overflow — descartando");
      buffer = "";
      braceDepth = 0;
      inString = false;
      escape = false;
    }
  }
}

// -----------------------------------------------
// Setup
// -----------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial1.begin(UART_BAUD);

  tft.begin();
  tft.setRotation(3);

  if (!SD.begin(SDCARD_SS_PIN, SDCARD_SPI)) {
    Serial.println("SD init failed — logging disabled");
  }

  pinMode(WIO_5S_UP,    INPUT_PULLUP);
  pinMode(WIO_5S_DOWN,  INPUT_PULLUP);
  pinMode(WIO_5S_LEFT,  INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  pinMode(WIO_BUZZER,   OUTPUT);

  // Load persisted log from SD  
  File f = SD.open(LOG_FILE, FILE_READ);
  if (f) {
    while (f.available() && eventCount < MAX_EVENTS) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        strncpy(events[eventCount].timestamp, line.c_str(), 19);
        events[eventCount].timestamp[19] = '\0';
        eventCount++;
      }
    }
    f.close();
  }

  tft.fillScreen(COL_BG);
  tft.setFreeFont(&FreeSansBold12pt7b);
  tft.setTextColor(COL_ACCENT);
  tft.drawString("DRONE WATCH", 80, 90);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(COL_GRAY);
  tft.drawString("SenseCAP Watcher Integration", 40, 120);
  tft.setTextColor(COL_DARKGRAY);
  tft.drawString("Serial1  Pin10=RX  Pin6=GND", 45, 145);
  drawDroneIcon(160, 185, 28, COL_CYAN);
  delay(2500);
  Serial.println("Drone detection started...");
  refreshView();
}

// -----------------------------------------------
// Loop
// -----------------------------------------------
void loop() {
  parseSerial();

  // Blink alert rings every 500ms
  if (newAlert && millis() - lastBlink > 500) {
    blinkState = !blinkState;
    lastBlink  = millis();
    if (currentView == 0) drawDashboard();
  }

  if (millis() - lastDebounce < DEBOUNCE_MS) goto skip_buttons;

  if (digitalRead(WIO_5S_RIGHT) == LOW && currentView == 0) {
    currentView  = 1;
    scrollOffset = max(0, eventCount - 8);
    refreshView();
    lastDebounce = millis();
  } else if (digitalRead(WIO_5S_LEFT) == LOW && currentView == 1) {
    currentView  = 0;
    refreshView();
    lastDebounce = millis();
  } else if (digitalRead(WIO_5S_UP) == LOW && currentView == 1) {
    if (scrollOffset > 0) { scrollOffset--; refreshView(); }
    lastDebounce = millis();
  } else if (digitalRead(WIO_5S_DOWN) == LOW && currentView == 1) {
    if (scrollOffset < eventCount - 1) { scrollOffset++; refreshView(); }
    lastDebounce = millis();
  } else if (digitalRead(WIO_5S_PRESS) == LOW) {
    newAlert     = false;
    refreshView();
    lastDebounce = millis();
  }

  skip_buttons:
  delay(50);
}