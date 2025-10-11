/*
  Heartbeat monitor with OLED graph (simplistic)

  - Reads analog input from A0
  - Simple rising-edge detection using an adaptive baseline
  - Computes BPM from RR interval
  - Accepts values only in 60..130 BPM range; ignores others (noise)
  - Displays BPM on SSD1306 128x64 OLED and a scrolling line graph

  Dependencies:
    - Adafruit GFX Library
    - Adafruit SSD1306

  Wiring (I2C):
    - OLED VCC -> 3.3V or 5V (per module)
    - OLED GND -> GND
    - OLED SCL -> Arduino SCL (e.g., A5 on UNO)
    - OLED SDA -> Arduino SDA (e.g., A4 on UNO)

  Sensor:
    - Heartbeat analog signal -> A0
    - Common GND
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Analog input pin
const int SENSOR_PIN = A0;

// Detection parameters
// Exponential moving average for baseline
float ema = 512.0f;            // start near half-scale
const float emaAlpha = 0.05f;  // 0.0..1.0, higher = faster tracking
int thresholdDelta = 30;       // delta above baseline for peak detection
bool prevAbove = false;        // track rising edge

// Timing
unsigned long lastBeatMs = 0;  // last detected beat time
const unsigned long refractoryMs = 250; // minimum gap to avoid double count

// BPM state
int bpm = 0;                   // last computed BPM (accepted)
int bpmDisplayed = 0;          // last displayed valid BPM

// Graph buffer (scrolling)
uint8_t graphBuf[SCREEN_WIDTH]; // y values for the last SCREEN_WIDTH points
const uint8_t graphTop = 20;     // top Y coordinate for graph area
const uint8_t graphBottom = SCREEN_HEIGHT - 1; // bottom of graph area

// Utility: map BPM to graph Y within 60..130 BPM
uint8_t mapBpmToY(int v) {
  // constrain to valid range for mapping
  if (v < 60) v = 60;
  if (v > 130) v = 130;
  // map: 60 BPM -> bottom, 130 BPM -> top
  long y = graphBottom - (long)(graphBottom - graphTop) * (v - 60) / (130 - 60);
  if (y < graphTop) y = graphTop;
  if (y > graphBottom) y = graphBottom;
  return (uint8_t)y;
}

void setup() {
  pinMode(SENSOR_PIN, INPUT);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // If OLED init fails, stay here
    for (;;) {
      // optional: blink onboard LED if available
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Heartbeat Monitor"));
  display.setCursor(0, 10);
  display.println(F("Range: 60-130 BPM"));
  display.display();

  // Initialize graph buffer to mid line (e.g., 95 BPM)
  uint8_t midY = mapBpmToY(95);
  for (int i = 0; i < SCREEN_WIDTH; i++) {
    graphBuf[i] = midY;
  }
}

void drawUI() {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Heartbeat (BPM)"));

  // BPM big text
  display.setTextSize(2);
  display.setCursor(0, 12);
  if (bpmDisplayed >= 60 && bpmDisplayed <= 130) {
    display.print(bpmDisplayed);
  } else {
    display.print(F("--"));
  }

  // Range label
  display.setTextSize(1);
  display.setCursor(80, 0);
  display.print(F("60"));
  display.setCursor(80, 8);
  display.print(F("130"));

  // Draw graph axes or grid lines
  // Horizontal lines for 60, 95, 130 BPM
  uint8_t y60 = mapBpmToY(60);
  uint8_t y95 = mapBpmToY(95);
  uint8_t y130 = mapBpmToY(130);
  display.drawLine(0, y60, SCREEN_WIDTH - 1, y60, SSD1306_WHITE);
  display.drawLine(0, y95, SCREEN_WIDTH - 1, y95, SSD1306_WHITE);
  display.drawLine(0, y130, SCREEN_WIDTH - 1, y130, SSD1306_WHITE);

  // Draw scrolling graph
  for (int x = 1; x < SCREEN_WIDTH; x++) {
    display.drawLine(x - 1, graphBuf[x - 1], x, graphBuf[x], SSD1306_WHITE);
  }

  display.display();
}

void pushGraph(int validBpm) {
  // shift left
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
    graphBuf[i] = graphBuf[i + 1];
  }
  // push new value
  graphBuf[SCREEN_WIDTH - 1] = mapBpmToY(validBpm);
}

void loop() {
  // Sample analog
  int raw = analogRead(SENSOR_PIN); // 0..1023

  // Update baseline (EMA)
  ema = (1.0f - emaAlpha) * ema + emaAlpha * raw;

  // Determine signal above baseline
  bool isAbove = (raw > (ema + thresholdDelta));

  unsigned long now = millis();

  // Rising-edge detection with refractory period
  if (isAbove && !prevAbove) {
    // potential beat start
    if (now - lastBeatMs > refractoryMs) {
      if (lastBeatMs != 0) {
        unsigned long rr = now - lastBeatMs; // ms between beats
        // Compute BPM
        int newBpm = (int)(60000UL / rr);
        // Accept only if in valid range
        if (newBpm >= 60 && newBpm <= 130) {
          bpm = newBpm;
          bpmDisplayed = bpm;
          pushGraph(bpm);
          drawUI();
        }
        // else: ignore as noise
      }
      lastBeatMs = now;
    }
  }

  prevAbove = isAbove;

  // Optionally refresh UI periodically to keep display alive
  // If no valid BPM yet, show initial graph and "--"
  static unsigned long lastRefresh = 0;
  if (now - lastRefresh > 500) {
    lastRefresh = now;
    drawUI();
  }

  // Small delay to limit sampling rate (~200 Hz)
  delay(5);
}
