// ============================================================================
// NETGOTCHI LED STATUS INDICATOR
// ============================================================================
// NeoPixel LED ring/strip for visual status feedback
// Wire: GPIO1 (TX) -> DIN, 5V -> VCC, GND -> GND
//
// Arduino IDE compiles all .ino tabs in the same folder together.
// USE_LEDS and NUM_LEDS defined here are visible across all files.
//
// TO ENABLE LEDs:
//   - Uncomment both #define lines below
//   - Set NUM_LEDS to match your strip/ring count (4-16 recommended)
//   - Install FastLED library (Library Manager -> "FastLED")
// TO DISABLE: comment out USE_LEDS line (saves ~6KB flash)
// ============================================================================

#define USE_LEDS
#define NUM_LEDS 8

#ifdef USE_LEDS

#include <FastLED.h>

#define LED_PIN 1          // GPIO1 - DMA capable, no serial needed in production
#define LED_BRIGHTNESS 50  // 0-255, keep low to save power
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

unsigned long previousLedMillis = 0;
const long ledUpdateInterval = 1000;  // Update LEDs every second

int ledColorState = 0;  // 0=idle, 1=scanning, 2=intrusion, 3=vulnerability, 4=honeypot, 5=evilTwin, 6=disconnected

// ============================================================================
// LED INITIALIZATION
// ============================================================================

void ledsInit() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  
  // Rainbow boot animation
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHS((i * 255 / NUM_LEDS) & 255, 255, 255);  // Hue cycle
    FastLED.show();
    delay(40);
  }
  FastLED.clear();
  FastLED.show();
  
  SerialPrintLn("LEDs initialized: " + String(NUM_LEDS) + " on GPIO1");
}

// ============================================================================
// LED COLOR STATES
// ============================================================================

// Get current LED color based on Netgotchi state
void updateLedState() {
  int newState = 0;
  
  // Priority order: most critical first
  if (honeypotTriggered) {
    newState = 4;  // Purple - honeypot breach
  } else if (evilTwinDetected) {
    newState = 5;  // Magenta - evil twin
  } else if (vulnerabilitiesFound > 0 && startScan) {
    newState = 3;  // Orange - vulnerability
  } else if (startScan) {
    newState = 1;  // Blue - scanning
  } else if (WiFi.status() != WL_CONNECTED) {
    newState = 6;  // Dim gray - disconnected
  } else {
    newState = 0;  // Green - idle/normal
  }
  
  if (newState != ledColorState) {
    ledColorState = newState;
    setLedColor();
  }
}

// Set LED colors based on state
void setLedColor() {
  switch (ledColorState) {
    case 0: // Idle - solid green
      setAllLeds(CRGB(0, 255, 0));
      break;
    case 1: // Scanning - blue breathing
      setAllLeds(CRGB(0, 100, 255));
      break;
    case 2: // Intrusion - red flashing
      setAllLeds(CRGB(255, 0, 0));
      break;
    case 3: // Vulnerability - orange pulse
      setAllLeds(CRGB(255, 165, 0));
      break;
    case 4: // Honeypot breach - purple
      setAllLeds(CRGB(128, 0, 255));
      break;
    case 5: // Evil twin - magenta strobe
      setAllLeds(CRGB(255, 0, 255));
      break;
    case 6: // Disconnected - dim gray
      setAllLeds(CRGB(50, 50, 50));
      break;
    default:
      setAllLeds(CRGB(0, 255, 0));
      break;
  }
  FastLED.show();
}

// Helper: set all LEDs to same color
void setAllLeds(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

// ============================================================================
// LED ANIMATIONS
// ============================================================================

// Breathing animation for scanning state
void ledsBreathing() {
  static unsigned long breathStart = 0;
  static bool breathUp = true;
  static int brightness = LED_BRIGHTNESS;
  
  unsigned long now = millis();
  if (now - breathStart >= 50) {
    breathStart = now;
    if (breathUp) {
      brightness += 3;
      if (brightness >= 255) breathUp = false;
    } else {
      brightness -= 3;
      if (brightness <= 20) breathUp = true;
    }
    FastLED.setBrightness(brightness);
    setAllLeds(CRGB(0, 100, 255));
  }
}

// Flashing animation for critical alerts
void ledsFlash() {
  static bool flashOn = true;
  static unsigned long flashStart = 0;
  
  unsigned long now = millis();
  if (now - flashStart >= 200) {
    flashStart = now;
    flashOn = !flashOn;
    if (flashOn) {
      setAllLeds(CRGB(255, 0, 0));
    } else {
      setAllLeds(CRGB(0, 0, 0));
    }
  }
}

// Strobe animation for evil twin
void ledsStrobe() {
  static bool strobeOn = true;
  static unsigned long strobeStart = 0;
  
  unsigned long now = millis();
  if (now - strobeStart >= 150) {
    strobeStart = now;
    strobeOn = !strobeOn;
    if (strobeOn) {
      setAllLeds(CRGB(255, 0, 255));
    } else {
      setAllLeds(CRGB(0, 0, 0));
    }
  }
}

// Pulse animation for vulnerability
void ledsPulse() {
  static unsigned long pulseStart = 0;
  static bool pulseUp = true;
  static int intensity = 100;
  
  unsigned long now = millis();
  if (now - pulseStart >= 30) {
    pulseStart = now;
    if (pulseUp) {
      intensity += 5;
      if (intensity >= 255) pulseUp = false;
    } else {
      intensity -= 5;
      if (intensity <= 50) pulseUp = true;
    }
    CRGB color = CRGB(intensity, intensity * 0.65, 0);
    setAllLeds(color);
  }
}

// ============================================================================
// LED MAIN LOOP - call from netgotchi_loop()
// ============================================================================

void ledsLoop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousLedMillis >= ledUpdateInterval) {
    previousLedMillis = currentMillis;
    updateLedState();
  }
  
  // Run appropriate animation based on state
  switch (ledColorState) {
    case 1: // Scanning - breathe
      ledsBreathing();
      break;
    case 2: // Intrusion - flash
      ledsFlash();
      break;
    case 3: // Vulnerability - pulse
      ledsPulse();
      break;
    case 5: // Evil twin - strobe
      ledsStrobe();
      break;
    // Cases 0, 4, 6: solid color, no animation needed
  }
}

// ============================================================================
// LED COLOR FOR WEB DASHBOARD (CSS color string)
// ============================================================================

String getLedColorState() {
  switch (ledColorState) {
    case 0: return "\"#00ff00\""; // Green
    case 1: return "\"#0066ff\""; // Blue
    case 2: return "\"#ff0000\""; // Red
    case 3: return "\"#ffa500\""; // Orange
    case 4: return "\"#8000ff\""; // Purple
    case 5: return "\"#ff00ff\""; // Magenta
    case 6: return "\"#333333\""; // Gray
    default: return "\"#00ff00\"";
  }
}

String getLedColorName() {
  switch (ledColorState) {
    case 0: return "\"Idle\"";
    case 1: return "\"Scanning\"";
    case 2: return "\"Intrusion\"";
    case 3: return "\"Vulnerability\"";
    case 4: return "\"Honeypot Breach\"";
    case 5: return "\"Evil Twin\"";
    case 6: return "\"Disconnected\"";
    default: return "\"Idle\"";
  }
}

// ============================================================================
// LED CONTROL COMMANDS (for web interface)
// ============================================================================

void handleLedCommand(String command) {
  if (command == "on") {
    setAllLeds(CRGB(0, 255, 0));
    SerialPrintLn("LEDs: ON (green)");
  } else if (command == "off") {
    setAllLeds(CRGB(0, 0, 0));
    SerialPrintLn("LEDs: OFF");
  } else if (command == "red") {
    setAllLeds(CRGB(255, 0, 0));
    SerialPrintLn("LEDs: red");
  } else if (command == "green") {
    setAllLeds(CRGB(0, 255, 0));
    SerialPrintLn("LEDs: green");
  } else if (command == "blue") {
    setAllLeds(CRGB(0, 0, 255));
    SerialPrintLn("LEDs: blue");
  } else if (command == "auto") {
    updateLedState();
    SerialPrintLn("LEDs: auto mode");
  }
}

#endif // USE_LEDS
