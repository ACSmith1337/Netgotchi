# Netgotchi

> Network security monitor for ESP8266. Ping scan, vulnerability detection, evil twin alerting, honeypot, and LED status indicator.

A network guardian built for the ESP8266. Periodically scans your WiFi network, detects intrusions, monitors for vulnerabilities, and uses a bi-color LED to show real-time security status at a glance.

Fork of [MXZZ/Netgotchi](https://github.com/MXZZ/Netgotchi). ESP8266 port by [itsOwen](https://github.com/itsOwen).

## Features

- Periodic network scanning (every 6 min, configurable)
- Intrusion detection -- alerts when a new host appears on the network
- Vulnerability scanning -- checks 8 common services (Telnet, FTP, SSH, VNC, RDP, SMB, HTTP, HTTPS)
- Evil twin detection -- monitors for APs spoofing your network SSID
- FTP honeypot -- triggers alarm when accessed
- Bi-color LED status indicator (red/green) with distinct patterns per state
- OLED display with animated screens (starfield, UFO, ripple)
- Headless serial output (for cyberdeck integration)
- Web dashboard -- real-time status, LED control, host list

## Hardware

- Wemos D1 Mini (ESP8266) or any ESP8266 board
- 0.96" OLED display (SSD1306, SH1106, or SSD1305)
- Bi-color LED (common cathode, red + green)
- Optional: buttons for navigation (D7, D6, D2, D3)

### Wiring

OLED: I2C on D1 (SCL) and D2 (SDA). Standard Wemos D1 Mini pins.

Bi-color LED (common cathode):
- Leg 1 -> D5 (red control, HIGH = on)
- Leg 2 -> GND (common cathode)
- Leg 3 -> D6 (green control, HIGH = on)

## LED Status Patterns

The bi-color LED shows seven distinct states using solid, flash, and pulse patterns:

| State | Color | Pattern | Meaning |
|-------|-------|---------|---------|
| Idle | Green | Solid | Normal operation |
| Scanning | Green | Flash | Network scan in progress |
| Intrusion | Red | Flash | New host detected on network |
| Vulnerability | Amber | Pulse | Vulnerable services found |
| Honeypot Breach | Red | Solid | Honeypot was accessed |
| Evil Twin AP | Amber | Flash | Rogue AP detected |
| Disconnected | Red | Slow breathe | WiFi disconnected |

The LED is fully automatic but can be manually overridden via the web dashboard (green, red, amber, off, or auto).

## Web Dashboard

Visit the device IP address on port 80 from any browser on the same network. The dashboard provides:

- **Display** -- live pixel-level mirror of the OLED screen
- **Controls** -- remote button presses (navigate screens, toggle pins, adjust time)
- **System Status** -- WiFi RSSI, uptime, IP, SSID, CPU load, free heap, relay pin state
- **Security** -- honeypot status, evil twin scan toggle, host count, vulnerability count
- **LED Status** -- real-time LED color and state, manual override buttons, full LED legend
- **Network Hosts** -- on-demand scan and display of all discovered hosts

All panels refresh automatically. Mobile-friendly responsive design.

## Setup

1. Open the `.ino` file in Arduino IDE. Make sure all tabs are open.
2. Select your OLED display type by setting the appropriate flag to 1 (default: `#define oled_type_ssd1306 1`)
3. Install required libraries (see below)
4. Select your board (ESP8266) in Arduino IDE
5. Flash the code
6. On first boot, Netgotchi creates an AutoConnectAP hotspot for WiFi setup
7. Once connected, it starts monitoring

## Libraries

- ESP8266 core
- ESP8266FtpServer (modified -- use the version in /libraries folder)
- Adafruit_GFX
- Adafruit_SSD1306 (or SH110X / SSD1305 for your display)
- ESPping
- NTPClient
- WiFiManager
- Button2

Install via Arduino Library Manager, except ESP8266FtpServer which must be the modified version included in this repo.

## Configuration

Key settings in `netgotchi.ino`:

| Setting | Default | Description |
|---------|---------|-------------|
| `intervalScan` | 6 min | How often to scan the network |
| `alertTimeout` | 3 min | How long alerts remain active after scan |
| `securityScanActive` | true | Enable/disable vulnerability scanning |
| `evilTwinScanEnabled` | true | Enable/disable evil twin detection |
| `webInterface` | true | Enable/disable web dashboard |
| `headless` | true | Enable/disable serial status output |
| `debug` | true | Enable/disable debug Serial output |

## Headless Mode

For cyberdeck integration, serial output provides real-time status:

```
^_^ Honeypot:OK EvilTwin:OK Host-Found:5 Vulnerabilities:2
```

Parse this from your host system to build custom displays or alerts.

## License

GNU General Public License v3.0

Original by [MXZZ](https://github.com/MXZZ) | ESP32 port by [itsOwen](https://github.com/itsOwen)
