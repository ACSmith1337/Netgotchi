// ============================================================================
// NETGOTCHI BI-COLOR LED STATUS INDICATOR
// ============================================================================
// Bi-color LED (Red/Green with shared common leg)
// Shows: Red, Green, Amber (both), or Off
//
// Wiring: Connect 3 legs to any 3 digital pins on Wemos D1 Mini
// The test sketch (/tmp/led_test/led_test.ino) tells you which pin is which.
//
// After testing, update the pins below to match YOUR LED:
//   - LED_RED_PIN   = the pin that makes it RED when HIGH (or LOW if common anode)
//   - LED_GREEN_PIN = the pin that makes it GREEN when HIGH (or LOW if common anode)
//   - LED_COMMON_PIN = the shared leg
//
// ============================================================================

#define USE_LEDS

// --- CONFIGURE THESE AFTER RUNNING THE TEST SKETCH ---
// WEMOS D1 MINI PIN RESTRICTIONS:
//   D0=GPIO16  RELAY pin (security pin)
//   D1=GPIO5   I2C SCL (OLED display)
//   D2=GPIO4   I2C SDA (OLED display)
//   D3=GPIO0   Flash button / WiFiManager (BOOT MODE!)
//   D4=GPIO2   Built-in LED
//
// SAFE PINS: D5(GPIO14), D6(GPIO12), D7(GPIO13)
//
// BI-COLOR LED with Leg 2 as common (shared between both colors):
//   Leg 1 → D6 (Red control)
//   Leg 2 → D7 (Common - shared leg)
//   Leg 3 → D5 (Green control)
//
// Wiring confirmed: Leg1↔Leg2=Red, Leg3↔Leg2=Green
// Leg 3 is physically longest but Leg 2 is the electrical common

#define LED_RED_PIN    D6    // Leg 1 - Red anode/cathode
#define LED_GREEN_PIN  D5    // Leg 3 - Green anode/cathode
#define LED_COMMON_PIN D7    // Leg 2 - Common (shared leg)

// Polarity: set true if common ANODE (common = HIGH, colors = LOW)
//           set false if common CATHODE (common = LOW, colors = HIGH)
// Most bi-color LEDs are common anode. If one color works and the other doesn't,
// flip this value.
#define COMMON_ANODE   true

// Brightness via PWM (0-1023). Lower = dimmer, higher = brighter.
// Default 900 gives ~88% brightness — bright but not blinding.
#define LED_BRIGHTNESS 900

// Update interval in milliseconds
#define LED_UPDATE_INTERVAL 1000

// ============================================================================
// INTERNAL - DO NOT EDIT BELOW
// ============================================================================

#ifdef USE_LEDS

unsigned long previousLedMillis = 0;
int ledColorState = 0;  // 0=idle, 1=scanning, 2=intrusion, 3=vulnerability, 4=honeypot, 5=evilTwin, 6=disconnected
bool ledEnabled = true;
bool manualMode = false;

// PWM channels for dimming
const int RED_PWM_CHANNEL = 2;
const int GREEN_PWM_CHANNEL = 4;

// ============================================================================
// LED INITIALIZATION
// ============================================================================

void ledsInit() {
  // Configure pins
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_COMMON_PIN, OUTPUT);
  
  // Set common pin
  digitalWrite(LED_COMMON_PIN, COMMON_ANODE ? HIGH : LOW);
  
  // Start with both off
  setAllOff();
  
  // Boot sequence
  bootAnimation();
  
  SerialPrintLn("Bi-color LED initialized on D6/D7/D5");
}

void bootAnimation() {
  // Flash red, green, amber, off
  setRedOn();
  delay(300);
  setGreenOn();
  delay(300);
  setBothOn();  // Amber
  delay(300);
  setAllOff();
  delay(200);
  
  // Start in idle state (green)
  setLedColor();
}

// ============================================================================
// LED COLOR STATES
// ============================================================================

void updateLedState() {
  if (manualMode) return;  // Don't override manual control
  
  int newState = 0;
  
  // Priority order: most critical first
  if (honeypotTriggered) {
    newState = 4;   // Red - honeypot breach
  } else if (evilTwinDetected) {
    newState = 5;   // Red - evil twin  
  } else if (vulnerabilitiesFound > 0 && startScan) {
    newState = 3;   // Amber - vulnerability
  } else if (startScan) {
    newState = 1;   // Green pulsing - scanning
  } else if (WiFi.status() != WL_CONNECTED) {
    newState = 6;   // Off - disconnected
  } else {
    newState = 0;   // Green solid - idle/normal
  }
  
  if (newState != ledColorState) {
    ledColorState = newState;
    setLedColor();
  }
}

void setLedColor() {
  switch (ledColorState) {
    case 0: // Idle - solid green
      setGreenOn();
      break;
    case 1: // Scanning - green breathing (handled in animation)
      setGreenOn();
      break;
    case 2: // Intrusion - red flashing (handled in animation)
      setRedOn();
      break;
    case 3: // Vulnerability - amber pulse (handled in animation)
      setBothOn();  // Amber = both on
      break;
    case 4: // Honeypot breach - red
      setRedOn();
      break;
    case 5: // Evil twin - red strobe (handled in animation)
      setRedOn();
      break;
    case 6: // Disconnected - off
      setAllOff();
      break;
    default:
      setGreenOn();
      break;
  }
}

// ============================================================================
// LED ANIMATIONS
// ============================================================================

void ledsBreathing() {
  static unsigned long breathStart = 0;
  static bool breathUp = true;
  static int brightness = LED_BRIGHTNESS;
  
  unsigned long now = millis();
  if (now - breathStart >= 50) {
    breathStart = now;
    if (breathUp) {
      brightness += 3;
      if (brightness >= 1023) breathUp = false;
    } else {
      brightness -= 3;
      if (brightness <= 100) breathUp = true;
    }
    setLedRaw(0, brightness);
  }
}

void ledsFlash() {
  static bool flashOn = true;
  static unsigned long flashStart = 0;
  
  unsigned long now = millis();
  if (now - flashStart >= 200) {
    flashStart = now;
    flashOn = !flashOn;
    if (flashOn) {
      setRedOn();
    } else {
      setAllOff();
    }
  }
}

void ledsPulse() {
  static unsigned long pulseStart = 0;
  static bool pulseUp = true;
  static int intensity = LED_BRIGHTNESS;
  
  unsigned long now = millis();
  if (now - pulseStart >= 30) {
    pulseStart = now;
    if (pulseUp) {
      intensity += 5;
      if (intensity >= 1023) pulseUp = false;
    } else {
      intensity -= 5;
      if (intensity <= 200) pulseUp = true;
    }
    setLedRaw(intensity, intensity);  // Both on = amber
  }
}

void ledsStrobe() {
  static bool strobeOn = true;
  static unsigned long strobeStart = 0;
  
  unsigned long now = millis();
  if (now - strobeStart >= 150) {
    strobeStart = now;
    strobeOn = !strobeOn;
    if (strobeOn) {
      setRedOn();
    } else {
      setAllOff();
    }
  }
}

// ============================================================================
// LED MAIN LOOP
// ============================================================================

void ledsLoop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousLedMillis >= LED_UPDATE_INTERVAL) {
    previousLedMillis = currentMillis;
    updateLedState();
  }
  
  // Skip animations when manual mode is active - let the manual color hold steady
  if (manualMode) return;
  
  // Run animation based on state
  switch (ledColorState) {
    case 1: // Scanning - breathe green
      ledsBreathing();
      break;
    case 2: // Intrusion - flash red
      ledsFlash();
      break;
    case 3: // Vulnerability - pulse amber
      ledsPulse();
      break;
    case 5: // Evil twin - strobe red
      ledsStrobe();
      break;
    // Cases 0, 4, 6: solid colors, no animation
  }
}

// ============================================================================
// WEB DASHBOARD API SUPPORT
// ============================================================================

String getLedColorState() {
  switch (ledColorState) {
    case 0: return "\"#00ff00\""; // Green
    case 1: return "\"#00ff00\""; // Green
    case 2: return "\"#ff0000\""; // Red
    case 3: return "\"#ffa500\""; // Amber
    case 4: return "\"#ff0000\""; // Red
    case 5: return "\"#ff0000\""; // Red
    case 6: return "\"#333333\""; // Off
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

void handleLedCommand(String command) {
  if (command == "on") {
    setGreenOn();
    manualMode = true;
    SerialPrintLn("LED: green ON");
  } else if (command == "off") {
    setAllOff();
    manualMode = true;
    SerialPrintLn("LED: OFF");
  } else if (command == "red") {
    setRedOn();
    manualMode = true;
    SerialPrintLn("LED: RED");
  } else if (command == "green") {
    setGreenOn();
    manualMode = true;
    SerialPrintLn("LED: GREEN");
  } else if (command == "blue") {
    setBothOn();
    manualMode = true;
    SerialPrintLn("LED: AMBER (no blue on bi-color)");
  } else if (command == "amber") {
    setBothOn();
    manualMode = true;
    SerialPrintLn("LED: AMBER");
  } else if (command == "auto") {
    manualMode = false;
    updateLedState();
    SerialPrintLn("LED: auto mode");
  }
}

// ============================================================================
// LOW-LEVEL LED CONTROL
// ============================================================================

// setLedRaw receives brightness value (0=off, 1023=full on).
// COMMON_ANODE handles the inversion internally.
void setLedRaw(int redValue, int greenValue) {
  if (COMMON_ANODE) {
    // Common anode: LOW = ON. Invert PWM so that high brightness value = LED on more.
    //   brightness 1023 → analogWrite(0)   → pin 100% LOW = fully on
    //   brightness 0    → analogWrite(1023) → pin 100% HIGH = fully off
    analogWrite(LED_RED_PIN, 1023 - redValue);
    analogWrite(LED_GREEN_PIN, 1023 - greenValue);
  } else {
    // Common cathode: HIGH = ON. PWM value maps directly.
    //   brightness 1023 → analogWrite(1023) → pin 100% HIGH = fully on
    //   brightness 0    → analogWrite(0)    → pin 100% LOW = fully off
    analogWrite(LED_RED_PIN, redValue);
    analogWrite(LED_GREEN_PIN, greenValue);
  }
}

// Helper: set red/green/both to full brightness, or off
#define LED_FULL 1023
#define LED_OFF  0

void setRedOn()   { setLedRaw(LED_FULL, LED_OFF); }
void setGreenOn() { setLedRaw(LED_OFF, LED_FULL); }
void setBothOn()  { setLedRaw(LED_FULL, LED_FULL); }
void setAllOff()  { setLedRaw(LED_OFF, LED_OFF); }

#endif // USE_LEDS
