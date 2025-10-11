# Heartbeat OLED Monitor (Simplistic)

This Arduino sketch reads an analog heartbeat signal from `A0`, detects beats with a simple baseline + threshold method, computes BPM, and displays only valid BPM in the 60–130 range on a 128x64 SSD1306 OLED along with a scrolling graph.

## Hardware

- Arduino (e.g., UNO/Nano)
- 128x64 I2C OLED (SSD1306, address 0x3C)
- Heartbeat sensor (analog output), common ground

### Wiring (I2C)

- OLED VCC -> 3.3V or 5V (per module)
- OLED GND -> GND
- OLED SCL -> Arduino SCL (UNO: A5)
- OLED SDA -> Arduino SDA (UNO: A4)
- Sensor signal -> A0
- Sensor GND -> GND

## Libraries

Install via Library Manager:

- Adafruit GFX Library
- Adafruit SSD1306

## Upload

1. Open `heartbeat_oled.ino` in the Arduino IDE.
2. Select your board and port.
3. Upload the sketch.

## Notes

- This is a simplistic approach intended to reject noise outside 60–130 BPM.
- Adjust `thresholdDelta` and `emaAlpha` in the sketch to fit your sensor signal amplitude and variability.
- Sampling is ~200 Hz (delay 5 ms). You can change `delay(5)` if needed.
