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
// Green diode has a ~3V forward voltage threshold — ESP8266 PWM at 1kHz
// needs high duty cycle to reliably drive it. Floor at 980 (~96%) keeps
// green bright during breathing animation while still showing pulse effect.
// Red diode is lower threshold so it stays visible even at lower values.
#define LED_BRIGHTNESS 980

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

// PWM frequency — ESP8266 default is 1kHz.
// WiFi SDK operations can silently corrupt the PWM hardware registers.
// For solid states: use digitalWrite(HIGH) — true constant 3.3V, immune to PWM corruption.
// For animations: use analogWrite — PWM breathing/flashing.
// NEVER mix on the same pin without clearing PWM first.
#define LED_PWM_FREQ 1000

// ============================================================================
// LED INITIALIZATION
// ============================================================================

void ledsInit() {
  // Configure color pins - common cathode is wired to GND, no GPIO needed
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  // Set PWM frequency for animation channels
  analogWriteFreq(LED_PWM_FREQ);
  
  // Start with both off
  setAllOff();
  
  // Boot sequence
  bootAnimation();
  
  SerialPrintLn("Bi-color LED initialized on D5 (red) / D6 (green)");
}

void bootAnimation() {
  // Green breathing in
  for (int b = 0; b <= 1023; b += 64) {
    setLedRaw(0, b);
    delay(25);
  }
  // Green breathing out
  for (int b = 1023; b >= 0; b -= 64) {
    setLedRaw(0, b);
    delay(25);
  }
  delay(100);

  // Red flash x3
  for (int f = 0; f < 3; f++) {
    setRedOn();
    delay(150);
    setAllOff();
    delay(150);
  }

  // Amber pulse up
  for (int p = 0; p <= 1023; p += 64) {
    setLedRaw(p, p);
    delay(25);
  }
  // Amber pulse down
  for (int p = 1023; p >= 0; p -= 64) {
    setLedRaw(p, p);
    delay(25);
  }

  delay(200);
  // Settle to idle state (green solid)
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
  //   1 = green flash          (scanning in progress)
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
    SerialPrintLn(String("LED state -> ") + getLedColorName());
    setLedColor();
  }
}

void setLedColor() {
  switch (ledColorState) {
    case 0: // Idle - solid green (digitalWrite)
      setGreenSolid();
      break;
    case 1: // Scanning - green flashing (handled in animation)
      setGreenOn();
      break;
    case 2: // Intrusion - red flashing (handled in animation)
      setRedOn();
      break;
    case 3: // Vulnerability - amber pulse (handled in animation)
      setBothOn();  // Amber = both on
      break;
    case 4: // Honeypot breach - solid red (digitalWrite)
      setRedSolid();
      break;
    case 5: // Evil twin - amber flash on/off (handled in animation)
      setBothOn();  // Amber = both on
      break;
    case 6: // Disconnected - red slow breathing (handled in animation)
      setRedOn();
      break;
    default:
      setGreenSolid();
      break;
  }
}

// ============================================================================
// LED ANIMATIONS
// ============================================================================

void ledsBreathing() {
  static unsigned long breathStart = 0;
  static bool breathUp = true;
  static int brightness = 0;
  
  unsigned long now = millis();
  if (now - breathStart >= 50) {
    breathStart = now;
    if (breathUp) {
      brightness += 3;
      if (brightness >= 1023) breathUp = false;
    } else {
      brightness -= 3;
      if (brightness <= 0) breathUp = true;
    }
    setLedRaw(0, brightness);  // Green breathing
  }
}

void ledsRedBreathing() {
  static unsigned long breathStart = 0;
  static bool breathUp = true;
  static int brightness = 0;
  
  unsigned long now = millis();
  if (now - breathStart >= 80) {  // Slower than green breathing
    breathStart = now;
    if (breathUp) {
      brightness += 2;
      if (brightness >= 1023) breathUp = false;
    } else {
      brightness -= 2;
      if (brightness <= 0) breathUp = true;
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
  static int intensity = 0;
  
  unsigned long now = millis();
  if (now - pulseStart >= 30) {
    pulseStart = now;
    if (pulseUp) {
      intensity += 5;
      if (intensity >= 1023) pulseUp = false;
    } else {
      intensity -= 5;
      if (intensity <= 0) pulseUp = true;
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

void ledsGreenFlash() {
  static bool flashOn = true;
  static unsigned long flashStart = 0;
  
  unsigned long now = millis();
  if (now - flashStart >= 300) {  // Medium-speed green flash
    flashStart = now;
    flashOn = !flashOn;
    if (flashOn) {
      setGreenOn();
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
  
  // First run: force LED color after all init is done.
  // Between boot animation and first ledsLoop(), display.begin() may have
  // touched I2C pins. Ensure the LED PWM duty cycle is set.
  // NOTE: D5/D6 are LED pins only — I2C uses D1/D2. Never share these pins.
  static bool firstRun = true;
  if (firstRun) {
    firstRun = false;
    setGreenSolid();  // Idle = solid green, immune to PWM corruption
  }
  
  if (currentMillis - previousLedMillis >= LED_UPDATE_INTERVAL) {
    previousLedMillis = currentMillis;
    updateLedState();
  }
  
  // Skip animations when manual mode is active - let the manual color hold steady
  if (manualMode) return;
  
  // Run animation based on state
  // SOLID states use digitalWrite — immune to WiFi SDK PWM corruption.
  // ANIMATED states use analogWrite — PWM breathing/flashing.
  switch (ledColorState) {
    case 0: // Idle - solid green (digitalWrite, no PWM)
      // digitalWrite is persistent — no need to re-apply every cycle
      break;
    case 1: // Scanning - flash green on/off (PWM)
      ledsGreenFlash();
      break;
    case 2: // Intrusion - flash red on/off (PWM)
      ledsFlash();
      break;
    case 3: // Vulnerability - pulse amber (PWM)
      ledsPulse();
      break;
    case 4: // Honeypot breach - solid red (digitalWrite, no PWM)
      // digitalWrite is persistent — no need to re-apply every cycle
      break;
    case 5: // Evil twin - amber flash on/off (PWM)
      ledsAmberFlash();
      break;
    case 6: // Disconnected - red slow breathing (PWM)
      ledsRedBreathing();
      break;
  }
}

// ============================================================================
// WEB DASHBOARD API SUPPORT
// ============================================================================

// JSON helpers — return plain values, the endpoint template adds JSON quotes.
// Do NOT add escaped quotes here; the browser JSON parser would include literal
// quote chars in the parsed string, breaking CSS color values.
String getLedColorState() {
  switch (ledColorState) {
    case 0: return "#00ff00"; // Green - idle
    case 1: return "#00ff00"; // Green - scanning
    case 2: return "#ff0000"; // Red - intrusion
    case 3: return "#ffa500"; // Amber - vulnerability
    case 4: return "#ff0000"; // Red - honeypot
    case 5: return "#ffa500"; // Amber - evil twin
    case 6: return "#ff0000"; // Red - disconnected
    default: return "#00ff00";
  }
}

String getLedColorName() {
  switch (ledColorState) {
    case 0: return "Idle";
    case 1: return "Scanning";
    case 2: return "Intrusion";
    case 3: return "Vulnerability";
    case 4: return "Honeypot Breach";
    case 5: return "Evil Twin AP";
    case 6: return "WiFi Disconnected";
    default: return "Idle";
  }
}

void handleLedCommand(String command) {
  if (command == "on") {
    setGreenSolid();
    manualMode = true;
    SerialPrintLn("LED: green ON");
  } else if (command == "off") {
    setAllSolidOff();
    manualMode = true;
    SerialPrintLn("LED: OFF");
  } else if (command == "red") {
    setRedSolid();
    manualMode = true;
    SerialPrintLn("LED: RED");
  } else if (command == "green") {
    setGreenSolid();
    manualMode = true;
    SerialPrintLn("LED: GREEN");
  } else if (command == "blue") {
    setBothSolid();
    manualMode = true;
    SerialPrintLn("LED: AMBER (no blue on bi-color)");
  } else if (command == "amber") {
    setBothSolid();
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

// setLedRaw: common cathode wired to GND. Drive color pins via analogWrite (PWM).
// analogWrite(1023) = 99.9% PWM duty (NOT constant — still PWM). analogWrite(0) = off via digitalWrite.
void setLedRaw(int redValue, int greenValue) {
    analogWrite(LED_RED_PIN, redValue);
    analogWrite(LED_GREEN_PIN, greenValue);
}

// ESP8266 v2.x: after analogWrite sets PWM on a pin, the GPIO mux routes to PWM.
// analogWrite(0) writes the GPIO register but does NOT re-route the mux back to GPIO.
// So digitalWrite after analogWrite(0) has no effect on the actual pin output.
// pinMode(OUTPUT) explicitly re-routes the mux. Must call before digitalWrite
// when transitioning from PWM animation to solid digital state.
void ledStopPwm(uint8_t pin) {
    analogWrite(pin, 0);
    pinMode(pin, OUTPUT);  // Re-route GPIO mux from PWM → digital
}

// Solid-state functions: stop PWM, re-route mux, then digitalWrite for TRUE constant 3.3V.
// Immune to WiFi SDK PWM corruption. Works on ESP8266 core v2.x and v3.x.
void setRedSolid() {
    ledStopPwm(LED_RED_PIN);
    digitalWrite(LED_RED_PIN, HIGH);
    ledStopPwm(LED_GREEN_PIN);
    digitalWrite(LED_GREEN_PIN, LOW);
}

void setGreenSolid() {
    ledStopPwm(LED_RED_PIN);
    digitalWrite(LED_RED_PIN, LOW);
    ledStopPwm(LED_GREEN_PIN);
    digitalWrite(LED_GREEN_PIN, HIGH);
}

void setBothSolid() {
    ledStopPwm(LED_RED_PIN);
    digitalWrite(LED_RED_PIN, HIGH);
    ledStopPwm(LED_GREEN_PIN);
    digitalWrite(LED_GREEN_PIN, HIGH);
}

void setAllSolidOff() {
    ledStopPwm(LED_RED_PIN);
    digitalWrite(LED_RED_PIN, LOW);
    ledStopPwm(LED_GREEN_PIN);
    digitalWrite(LED_GREEN_PIN, LOW);
}

// Helper: set red/green/both to full PWM brightness, or off
#define LED_FULL 1023
#define LED_OFF  0

void setRedOn()   { setLedRaw(LED_FULL, LED_OFF); }
void setGreenOn() { setLedRaw(LED_OFF, LED_FULL); }
void setBothOn()  { setLedRaw(LED_FULL, LED_FULL); }
void setAllOff()  { setLedRaw(LED_OFF, LED_OFF); }

#endif // USE_LEDS
