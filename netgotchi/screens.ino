// ============================================================================
// NETGOTCHI DISPLAY & SCREENS
// ============================================================================
// This file handles all display initialization, screen rendering, and UI

// ============================================================================
// DISPLAY INITIALIZATION
// ============================================================================

void displayInit() {
  if (!hasDisplay) return;
  
  // Initialize SSD1306 OLED display
  if (oled_type_ssd1306) {
    // Use D1/D2 for I2C (standard Wemos D1 Mini pins).
    // DO NOT use D5/D6 — those pins are reserved for the bi-color LED
    // and sharing them with I2C kills the LED PWM output.
    if (screenWireParams) {
      Wire.begin(D2, D1);  // SDA=D2(GPIO4), SCL=D1(GPIO5)
    }
    
    if (!display.begin(2, 0x3C)) {
      SerialPrintLn("SSD1306 allocation failed");
      for (;;); // Halt if display fails
    }
  } 
  // Initialize other display types (SH1106, SSD1305)
  else {
    if (!display.begin()) {
      SerialPrintLn("Display allocation failed");
      for (;;); // Halt if display fails
    }
  }
}

// ============================================================================
// MAIN SCREEN FUNCTIONS
// ============================================================================

void screenAnimations() {
  // Rotates between different animation screens
  switch (animation) {
    case 0:
      displayNetgotchi();
      break;
    case 1:
      drawSpace();
      break;
    default:
      animation = 0;
      break;
  }
  
  if (animation > max_anim) {
    animation = 0;
  }
}

void displayNetgotchi() {
  displayClearDisplay();
  updateAndDrawStars();
  
  // Show network info or offline status
  if (enableNetworkMode) {
    displayTimeAndDate();
    displayStats();
  } else {
    displayOfflineMode();
  }
  
  netgotchi_face();
  displayDisplay();
}

void drawSpace() {
  displayClearDisplay();
  updateAndDrawStars();
  drawUFO();
  
  // Show network info or offline status
  if (enableNetworkMode) {
    displayTimeAndDate();
    displayStats();
  } else {
    displayOfflineMode();
  }
  
  displayDisplay();
}

void displayRippleSpace() {
  drawRipple();
  netgotchi_face();
  
  // Show network info or offline status
  if (enableNetworkMode) {
    displayTimeAndDate();
    displayStats();
  } else {
    displayOfflineMode();
  }
  
  displayDisplay();
}

// ============================================================================
// ANIMATION FUNCTIONS
// ============================================================================

// Starfield Animation
void initStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-1000, 1000);
    stars[i].y = random(-1000, 1000);
    stars[i].z = random(1, 1000);
  }
}

void updateAndDrawStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    // Move star towards viewer
    stars[i].z -= 5;
    if (stars[i].z <= 0) {
      stars[i].z = 1000;
    }
    
    // 3D projection to 2D screen (fixed-point math, no float div)
    int x = (stars[i].x * 64 / stars[i].z) + SCREEN_WIDTH / 2;
    int y = (stars[i].y * 32 / stars[i].z) + SCREEN_HEIGHT / 2;
    
    // Draw star if within screen bounds
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
      displayDrawPixel(x, y, 1);
    }
  }
}

// UFO Animation
void drawUFO() {
  int ufoSize = 8;
  
  // Draw UFO body
  displayDrawLine(ufoX - ufoSize, ufoY, ufoX + ufoSize, ufoY, 1);
  displayDrawLine(ufoX, ufoY - ufoSize / 2, ufoX, ufoY + ufoSize / 2, 1);
  displayDrawCircle(ufoX, ufoY, ufoSize / 2, 1);
  
  // Smooth floating motion
  ufoX = SCREEN_WIDTH / 2 + sin(millis() / 1000.0) * 20;
  ufoY = SCREEN_HEIGHT / 2 + cos(millis() / 1500.0) * 10;
}

// Ripple Animation — non-blocking
int frame = 0;
int numCircles = 10;
int maxRadius = 70;

void drawRipple() {
  static unsigned long lastFrame = 0;
  if (millis() - lastFrame < 100) return;  // Cap at 10fps
  lastFrame = millis();
  
  displayClearDisplay();
  int radius = (frame * 10) % maxRadius;
  displayDrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, radius, 1);
  frame++;
}

// ============================================================================
// UI COMPONENTS
// ============================================================================

void displayTimeAndDate() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  
  // Get date components
  time_t epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t*)&epochTime);
  int currentDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;
  
  // Display time
  displaySetSize(1);
  displaySetTextColor(1);
  displaySetCursor(5, 0);
  displayPrint(formattedTime);
  
  // Display date
  displaySetCursor(0, 8);
  displayPrintDate("%02d/%02d/%d", currentDay, currentMonth, currentYear);
}

void displayStats() {
  // Bottom status bar
  displaySetCursor(0, 55);
  displayPrint("Hosts:" + String(ipnum) + " VU:" + String(vulnerabilitiesFound));
  
  // Honeypot status (top right)
  displaySetCursor(75, 0);
  displayPrint("Honeypot");
  
  if (honeypotTriggered) {
    // Blink warning
    if ((seconds % 2) == 0) {
      displaySetCursor(80, 8);
      displayPrint("Breached");
      displaySetCursor(40, 16);
      
      #if defined(ESP8266)
        displayPrint(ftpSrv.getHoneyPotBreachIPandTime());
      #endif
    }
  } else {
    displaySetCursor(80, 8);
    displayPrint("OK");
  }
  
  // Scan status indicator
  displaySetCursor(90, 55);
  if (scanState == SCAN_SCANNING) displayPrint("Scan");
  else if (alertWindowActive) displayPrint("Alert");
  else displayPrint("Idle");
}

void netgotchi_face() {
  displaySetSize(2);
  drawnetgotchiFace(animState);
  
  // Animate face every second
  if (seconds - old_seconds > 1) {
    // Random horizontal movement
    moveX = moveX + random(-5, 5);
    if (moveX > 20) moveX = 5;
    if (moveX < -20) moveX = -5;
    
    old_seconds = seconds;
    animState++;
    if (animState > 5) animState = 0;
  }
  
  displaySetSize(1);
}

void displayOfflineMode() {
  displaySetCursor(0, 55);
  displayPrint("Netgotchi is Offline");
}

// ============================================================================
// INFO SCREENS
// ============================================================================

void netgotchiIntro() {
  displayClearDisplay();
  displaySetSize(1);
  displaySetTextColor(1);
  displaySetCursor(0, 0);
  displayPrintln("Netgotchi v." + String(VERSION));
  displayPrintln("created by MXZZ ");
  delay(500);
}

void NetworkStats() {
  displayClearDisplay();
  displaySetCursor(0, 8);
  
  // Network connection status
  networkStatus = (WiFi.status() == WL_CONNECTED) ? "connected" : "disconnected";
  displayPrint("Network: " + networkStatus);
  displaySetCursor(0, 16);
  
  // Ping test (run once)
  if (scanOnce) {
    IPAddress cloudflare(1, 1, 1, 1);
    SerialPrintLn("pinging cloudflare");
    
    if (Ping.ping(cloudflare, 2)) {
      externalNetworkStatus = "Reachable";
      displayPrintln();
      scanOnce = false;
      
      stats = "\n min: " + String(Ping.minTime()) + "ms"
            + "\n avg: " + String(Ping.averageTime()) + "ms"
            + "\n max: " + String(Ping.maxTime()) + "ms";
      
      delay(500);
      SerialPrintLn("ping sent");
      SerialPrintLn(stats);
    } else {
      externalNetworkStatus = "Unreachable";
    }
  }
  
  displayPrintln("Network Speed: " + stats);
  displayPrintln("Internet: " + externalNetworkStatus);
  displayDisplay();
}

void displayNetgotchiStats() {
  displayClearDisplay();
  displaySetCursor(0, 0);
  displayPrintln("Netgotchi v." + String(VERSION));
  displaySetCursor(0, 10);
  
  displayPrintln("IP:  " + currentIP.toString());
  displayPrintln("Uptime:" + String(seconds) + "sec");
  
  if (WiFi.status() == WL_CONNECTED) {
    displayPrintln("SSID:" + WiFi.SSID());
  }
  
  displayDisplay();
}

void displayIPS() {
  static unsigned long lastUpdate = 0;
  static int scanIndex = 0;
  static int pageRows = 0;
  static bool scanning = true;
  static String cachedPrefix = "";
  
  displayClearDisplay();
  
  if (ipnum == 0) {
    // Reset state
    scanIndex = 0;
    pageRows = 0;
    scanning = true;
    nextScreen();
    return;
  }
  
  // Build prefix once
  if (cachedPrefix == "") {
    cachedPrefix = String(currentIP[0]) + "." + String(currentIP[1]) + "." + String(currentIP[2]) + ".";
  }
  
  // Non-blocking: show one IP every 500ms, or resume after summary pause
  if (millis() - lastUpdate >= 500) {
    if (pageRows == 5) {
      // Summary pause done — resume scanning
      displayClearDisplay();
      displaySetCursor(0, 0);
      displayPrintln("Found Hosts:");
      pageRows = 0;
      scanning = true;
      lastUpdate = millis();
      displayDisplay();
      return;
    }
    
    if (!scanning) return;
    
    // Find next host with a status
    bool found = false;
    while (scanIndex < 255) {
      scanIndex++;
      if (ips[scanIndex] == 1 || ips[scanIndex] == -1 || ips[scanIndex] == 2) {
        found = true;
        break;
      }
    }
    
    if (found) {
      // Screen full — show summary then continue
      if (pageRows >= 4) {
        displayClearDisplay();
        displaySetCursor(5, 0);
        displayPrintln("Hosts:" + String(ipnum));
        displayDisplay();
        // Non-blocking pause — next update triggers clear+continue
        lastUpdate = millis();  // Reset timer for 1s pause
        scanning = false;
        pageRows = 5;  // Signal: showing summary
        return;
      }
      
      displaySetCursor(0, 20 + pageRows * 10);
      
      if (ips[scanIndex] == 1) {
        displayPrintln(cachedPrefix + String(scanIndex) + " UP");
      } else if (ips[scanIndex] == 2) {
        displayPrintln(cachedPrefix + String(scanIndex) + " WRNG!");
      } else {
        displayPrintln(cachedPrefix + String(scanIndex) + " DOWN");
      }
      pageRows++;
      displayDisplay();
    } else {
      // Done scanning
      scanning = false;
      scanIndex = 0;
      pageRows = 0;
      cachedPrefix = "";
    }
  }
  
  // If not yet started, show header immediately
  if (pageRows == 0 && millis() - lastUpdate < 100) {
    displaySetCursor(0, 0);
    displayPrintln("Found Hosts:");
    displayDisplay();
  }
}

void displaySettings() {
  displayClearDisplay();
  displaySetCursor(0, 0);
  displayPrintln("Settings v." + String(VERSION));
  displaySetCursor(0, 10);
  
  // Display menu with selection indicator
  for (int i = 0; i < settingLength; i++) {
    String prefix = (selectedSetting == i) ? ">" : " ";
    displayPrintln(prefix + settings[i]);
  }
  
  displayDisplay();
}

void displayVulnerabilities() {
  static unsigned long lastVulnUpdate = 0;
  static int currentVulnIndex = 0;
  static int currentPortIndex = 0;
  
  displayClearDisplay();
  displaySetCursor(0, 0);
  
  if (vulnerableHostCount == 0) {
    displayPrintln("Vulnerabilities:");
    displaySetCursor(0, 15);
    displayPrintln("No vulnerable");
    displayPrintln("hosts found!");
    displaySetCursor(0, 45);
    
    // Check for honeypot breach - don't show "Secure!" if breached
    if (honeypotTriggered) {
      displayPrint("(O_O) BREACH!");
    } else {
      displayPrint("(^_^) Secure!");
    }
    
    displayDisplay();
    return;
  }
  
  // Header
  displayPrintln("Vulnerable Hosts:");
  displayPrint("Found: ");
  displayPrintln(String(vulnerableHostCount));
  displayPrintln(" ");
  
  // Non-blocking update every 2 seconds
  if (millis() - lastVulnUpdate >= 2000) {
    lastVulnUpdate = millis();
    
    // Find next active vulnerable host
    bool foundHost = false;
    for (int attempts = 0; attempts < MAX_VULNERABLE_HOSTS && !foundHost; attempts++) {
      currentVulnIndex = (currentVulnIndex + 1) % MAX_VULNERABLE_HOSTS;
      
      if (vulnerableHosts[currentVulnIndex].active && 
          vulnerableHosts[currentVulnIndex].portCount > 0) {
        foundHost = true;
        currentPortIndex = 0; // Reset port index for new host
      }
    }
  }
  
  // Display current vulnerable host
  if (vulnerableHosts[currentVulnIndex].active && 
      vulnerableHosts[currentVulnIndex].portCount > 0) {
    
    // Display IP address
    displaySetCursor(0, 25);
    displayPrint("IP: ");
    displayPrintln(vulnerableHosts[currentVulnIndex].ip.toString());
    
    // Display open ports (show up to 3 ports per screen)
    displaySetCursor(0, 35);
    displayPrintln("Open Ports:");
    
    int portsToShow = min(3, vulnerableHosts[currentVulnIndex].portCount);
    for (int p = 0; p < portsToShow; p++) {
      displaySetCursor(0, 45 + (p * 8));
      displayPrint("- ");
      displayPrintln(vulnerableHosts[currentVulnIndex].openPorts[p]);
    }
    
    // Show indicator if more ports exist
    if (vulnerableHosts[currentVulnIndex].portCount > 3) {
      displaySetCursor(90, 55);
      displayPrint("+" + String(vulnerableHosts[currentVulnIndex].portCount - 3));
    }
  }
  
  displayDisplay();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void nextScreen() {
  currentScreen++;
  
  if (currentScreen > maxScreens) {
    currentScreen = 0;
    
    // Change animation style
    animation++;
    if (animation > max_anim) {
      animation = 0;
    }
  }
}

int getPixelAt(int x, int y) {
  // Check if coordinates are within bounds
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
    return -1; // Out of bounds
  }
  
  // Calculate buffer position
  int byteIndex = (y / 8) * SCREEN_WIDTH + x;  // Which byte
  int bitIndex = y % 8;                         // Which bit in byte
  
  // Get pixel value from buffer
  uint8_t* buffer = displayGetBuffer();
  uint8_t byteValue = buffer[byteIndex];
  
  // Check if pixel is set (1 = white, 0 = black)
  bool isWhite = (byteValue & (1 << bitIndex)) != 0;
  return isWhite ? 1 : 0;
}

String getPixelMatrix() {
  // Send raw display buffer as hex — 1024 bytes → 2048 chars
  // Much smaller than JSON array (~16KB). JS decodes back to pixels.
  uint8_t* buffer = displayGetBuffer();
  if (!buffer) return "[]";
  
  String hex = "\"";
  const char hexChars[] = "0123456789abcdef";
  for (int i = 0; i < SCREEN_WIDTH * (SCREEN_HEIGHT / 8); i++) {
    hex += hexChars[buffer[i] >> 4];
    hex += hexChars[buffer[i] & 0x0F];
  }
  hex += "\"";
  return hex;
}
