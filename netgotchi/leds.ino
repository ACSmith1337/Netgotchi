// ============================================================================
// NETGOTCHI BI-COLOR LED STATUS INDICATOR
// ============================================================================
// Bi-color LED (Red/Green with shared common leg)
// Shows: Red, Green, Amber (both), or Off
//
// Wiring: Connect 2 color legs to digital pins, common leg to GND
// The test sketch (/tmp/led_test/led_test.ino) tells you which leg is which.
//
// After testing, update the pins below to match YOUR LED:
//   - LED_RED_PIN   = the pin that makes it RED when HIGH
//   - LED_GREEN_PIN = the pin that makes it GREEN when HIGH
//   - Common leg → GND (ground)
//
// ============================================================================

// USE_LEDS is defined in netgotchi.ino (main sketch) for Arduino IDE tab compilation.
// Do NOT move the #define here — Arduino IDE only propagates preprocessor directives
// from the main .ino to other tabs, not the reverse.

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
// BI-COLOR LED - common cathode wired to GND:
//   Leg 1 → D5 (Red control) - HIGH = on
//   Leg 2 → GND (Common cathode - connect directly to ground)
//   Leg 3 → D6 (Green control) - HIGH = on
//
// Common cathode: connect the shared leg to GND, not a GPIO pin.
// GPIO pins have current-sinking limits. Direct ground is reliable.

#define LED_RED_PIN    D5    // Leg 1 - Red
#define LED_GREEN_PIN  D6    // Leg 3 - Green
// Common cathode connected directly to GND - no GPIO needed

// Common cathode LED: color pins HIGH = on, GND = off.
// Simple wiring - no polarity configuration needed.

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

// PWM channels (ESP8266 analogWrite auto-maps pins to channels):
// D5(GPIO14) → channel 6, D6(GPIO12) → channel 4
const int RED_PWM_CHANNEL = 6;
const int GREEN_PWM_CHANNEL = 4;

// ============================================================================
// LED INITIALIZATION
// ============================================================================

void ledsInit() {
  Serial.println("[LED] Initializing...");
  // Configure color pins - common cathode is wired to GND, no GPIO needed
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  // Start with both off
  setAllOff();
  
  // Boot sequence
  bootAnimation();
  
  Serial.println("[LED] Boot animation done, LED should be GREEN (idle)");
  SerialPrintLn("Bi-color LED initialized on D5 (red) / D6 (green)");
}

void bootAnimation() {
  // Flash red, green, amber, off
  Serial.println("[LED] Boot: RED");
  setRedOn();
  delay(300);
  Serial.println("[LED] Boot: GREEN");
  setGreenOn();
  delay(300);
  Serial.println("[LED] Boot: AMBER");
  setBothOn();  // Amber
  delay(300);
  Serial.println("[LED] Boot: OFF");
  setAllOff();
  delay(200);
  
  // Start in idle state (green)
  Serial.println("[LED] Boot: setting idle GREEN");
  setLedColor();
}

// ============================================================================
// LED COLOR STATES
// ============================================================================

void updateLedState() {
  if (manualMode) return;  // Don't override manual control
  
  int newState = 0;
  
  // Priority order: most critical first
  // Each state has a DISTINCT visual pattern:
  //   0 = green solid          (normal/idle)
  //   1 = green breathing      (scanning in progress)
  //   2 = red fast flash       (intrusion - new host detected)
  //   3 = amber pulse          (vulnerabilities found)
  //   4 = red solid            (honeypot breached)
  //   5 = amber flash on/off   (evil twin AP detected)
  //   6 = red slow breathing   (WiFi disconnected)
  if (honeypotTriggered) {
    newState = 4;   // Red solid - honeypot breach
  } else if (newHostDetected && alertWindowActive) {
    newState = 2;   // Red flash - intrusion (new host on network)
  } else if (evilTwinDetected) {
    newState = 5;   // Amber flash - evil twin AP
  } else if (vulnerabilitiesFound > 0 && alertWindowActive) {
    newState = 3;   // Amber pulse - vulnerabilities active
  } else if (scanState == SCAN_SCANNING) {
    newState = 1;   // Green breathing - scanning in progress
  } else if (WiFi.status() != WL_CONNECTED) {
    newState = 6;   // Red slow breathe - disconnected
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
    case 4: // Honeypot breach - solid red
      setRedOn();
      break;
    case 5: // Evil twin - amber flash on/off (handled in animation)
      setBothOn();  // Amber = both on
      break;
    case 6: // Disconnected - red slow breathing (handled in animation)
      setRedOn();
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
      if (brightness <= LED_BRIGHTNESS) breathUp = true;
    }
    setLedRaw(0, brightness);  // Green breathing
  }
}

void ledsRedBreathing() {
  static unsigned long breathStart = 0;
  static bool breathUp = true;
  static int brightness = LED_BRIGHTNESS;
  
  unsigned long now = millis();
  if (now - breathStart >= 80) {  // Slower than green breathing
    breathStart = now;
    if (breathUp) {
      brightness += 2;
      if (brightness >= 1023) breathUp = false;
    } else {
      brightness -= 2;
      if (brightness <= LED_BRIGHTNESS) breathUp = true;
    }
    setLedRaw(brightness, 0);  // Red breathing
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
      if (intensity <= LED_BRIGHTNESS) pulseUp = true;
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

void ledsAmberFlash() {
  static bool flashOn = true;
  static unsigned long flashStart = 0;
  
  unsigned long now = millis();
  if (now - flashStart >= 250) {  // Medium-speed amber flash
    flashStart = now;
    flashOn = !flashOn;
    if (flashOn) {
      setBothOn();  // Amber
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
    case 2: // Intrusion - flash red on/off
      ledsFlash();
      break;
    case 3: // Vulnerability - pulse amber
      ledsPulse();
      break;
    case 5: // Evil twin - amber flash on/off
      ledsAmberFlash();
      break;
    case 6: // Disconnected - red slow breathing
      ledsRedBreathing();
      break;
    // Cases 0 (idle), 4 (honeypot): solid colors, no animation
  }
}

// ============================================================================
// WEB DASHBOARD API SUPPORT
// ============================================================================

String getLedColorState() {
  switch (ledColorState) {
    case 0: return "\\\"#00ff00\\\""; // Green - idle
    case 1: return "\\\"#00ff00\\\""; // Green - scanning
    case 2: return "\\\"#ff0000\\\""; // Red - intrusion
    case 3: return "\\\"#ffa500\\\""; // Amber - vulnerability
    case 4: return "\\\"#ff0000\\\""; // Red - honeypot
    case 5: return "\\\"#ffa500\\\""; // Amber - evil twin
    case 6: return "\\\"#ff0000\\\""; // Red - disconnected
    default: return "\\\"#00ff00\\\"";
  }
}

String getLedColorName() {
  switch (ledColorState) {
    case 0: return "\\\"Idle\\\"";
    case 1: return "\\\"Scanning\\\"";
    case 2: return "\\\"Intrusion\\\"";
    case 3: return "\\\"Vulnerability\\\"";
    case 4: return "\\\"Honeypot Breach\\\"";
    case 5: return "\\\"Evil Twin AP\\\"";
    case 6: return "\\\"WiFi Disconnected\\\"";
    default: return "\\\"Idle\\\"";
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

// setLedRaw: common cathode wired to GND. Drive color pins HIGH (via analogWrite) to light.
// Uses analogWrite exclusively - mixing digitalWrite and analogWrite on ESP8266 causes
// the pin to get stuck in PWM mode (hardware channel conflict).
// analogWrite(1023) = full 3.3V on. analogWrite(0) = off.
void setLedRaw(int redValue, int greenValue) {
  analogWrite(LED_RED_PIN, redValue);
  analogWrite(LED_GREEN_PIN, greenValue);
  if (redValue || greenValue) {
    Serial.printf("[LED] setRaw R=%d G=%d\n", redValue, greenValue);
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
