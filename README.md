# M5Stick-NEMO
Firmware for high-tech pranks and digital self-defense on the M5StickC Plus2

![M5-Nemo Matrix Logo](https://github.com/n0xa/m5stick-nemo/blob/main/NEMOMatrix.png)
Logo by @unagironin

## Okay so maybe we're back...

## Name and Background
NEMO started a personal project to help me learn more about ESP32 development with the Arduino IDE. I decided to replicate a few common, trending pranks that were getting a lot of attention in the tech community, as a challenge to myself, and to also better understand these attacks.  
NEMO is named after the small, clever and stubborn fish in Finding Nemo. This project stands in contrast to another high-tech gadget that's associated with certain sea-dwelling creatures. I did want to prove that there are a lot of things you can do with a small development kit and some curiosity. I have no delusions of superseding the capabilities of any similar device with this project. It's just for fun, and my own education.  

![M5-Nemo on M5StickC Plus2](https://github.com/n0xa/m5stick-nemo/blob/main/M5-Nemo.jpg)

## My ToDo list
* ideas?

This tree builds only for the M5StickC Plus2. StickC, StickC Plus, StickS3, and Cardputer are not supported here.

## Features
* Wireless Attack Detection: BLE Hunter, Deauth Hunter and PineAP Hunter modes provide situational awareness of ongoing attacks in your area.
* [TV B-Gone](http://www.righto.com/2010/11/improved-arduino-tv-b-gone.html) port (thanks to MrArm's [HAKRWATCH](https://github.com/MrARM/hakrwatch)) to shut off many infrared-controlled TVs, projectors and other devices
* [AppleJuice](https://github.com/ECTO-1A/AppleJuice) iOS Bluetooth device pairing spam
* Bluetooth device notification spamming for SwiftPair (Windows) and Android
* WiFi Spam - Funny SSIDs, WiFi Rickrolling, and a Random mode that creates hundreds of randomly-named SSIDs per minute
* WiFi NEMO Portal - A captive portal that tries to social engineer email credentials - saves usernames and passwords to SD Card (if inserted into a supported reader)
* WiFi SSID Scanner - Display 2.4 GHz SSIDs nearby, get information about them, and even clone the SSIDs in NEMO Portal
* User-adjustable 24 hour digital clock backed by the StickC Plus2 RTC
* EEPROM-backed settings for rotation, brightness, automatic dimming, auto-off, and NEMO Portal SSID
* Battery level and credits in the settings menu
* Optional Mini JoyC hat on G0/G26 for stick navigation

## User Interface
There are three buttons on the StickC Plus2:
* Home: tap the power button (closest to the USB port). Stops the current process and returns to the main menu.
* Next: tap the side button. Moves the cursor to the next menu option. In function modes this usually returns to the previous menu.
* Select: tap the M5 button on the front. Activates the selected option and wakes a dimmed screen.

A Mini JoyC hat can also move the cursor. Left goes back, right selects, and up/down step through the menu. JoyC options are in Settings.

## NEMO Portal
In NEMO Portal mode, NEMO activates an open WiFi Hotspot named "Nemo Free WiFi" (configurable in portal.h) with DNS, DHCP and Web servers activated. 
* NEMO Portal serves a fake login page that claims to provide internet access if you log in.
* This is a social engineering attack, and will log the username and passwords entered on the page. 
* From the Wifi Scan details, you can clone an existing SSID from the scan list. Exiting NEMO Portal will clear the Evil Twin SSID
* You can view captured credentials by connecting to the portal from your own device and browsing to http://172.0.0.1/creds
* You can set a custom SSID by connecting to the portal from your own device and browsing to http://172.0.0.1/ssid
* If your device supports EEPROM for settings, the custom SSID you enter will be saved as the default, even if powered off.
* SD card logging is off in this build. Uncomment `SDCARD` near the top of `src/m5stick-nemo/m5stick-nemo.ino` only if a reader is wired to the Plus2 pin header. Credentials are then appended to `nemo-portal-creds.txt`.
* NEMO Portal is only for use on professional engagements with a valid scope of work, educational or demonstration purposes. Storage, sale, or use of personal information without consent is against the law. 🤓

## BLE Hunter
Identifies presence of excessive BLE Device Advertisements
* Usage
  * Each session executes 10 consecutive scans and then the counter is reset
  * Buttons are briefly unresponsive during BLE scanning
  * If the number of BLE Advertisements exceeds the threshold during a, BLE Hunter will beep
  * Press the SELECT button to disable the alerts temporarily
  * The signal strength meter uses the RSSI of the most recently received device advertisement. When you are near a heavy BLE Spam attack, this is likely to be related to the offending signal.
  * Use signal strength meter to help you determine the source of the signal. Lower strength is further away. Stronger, you're closer.
  * Keep hands away from the USB Port of M5Stack devices. The BLE antenna is near the USB port and your body will block some of the signal.
  * Adjust the RSSI Sensitivity and Alert Packet count for your environment. Busy places may have a lot of BLE Advertisements that are not BLE Spam.
* Configuration
  * Adjust `BH Alert Pkts` in settings to modify the threshold for alerts.
  * Adjust "BH RSSI` in settings to adjust the sensitivity of the signal strength meter (smaller negative numbers are less sensitive, -20dBm is less sensitive than -50dBm) 

## Deauth Hunter
Identifies the presence of WiFi Deauthentication Attacks
* Usage
  * Each session executes for 10 seconds and then the counter is reset
  * Deauth Hunter will scan 2.4 GHz channels and sniff for WiFi Management Frames with the Deauthentication Flag set
  * Occasional Deauthentication frames are completely normal, but more than a few per second usually indicates a WiFi Denial Of Service or Handshake Capture attack
  * Press the SELECT button to disable the alerts temporarily and pause the channel scanning. This will allow you to freeze on the channel being used by the Deauthentication Attack.
  * The signal strength meter uses the RSSI of the most recently received deauthentication frame.
  * Use signal strength meter to help you determine the source of the signal. Lower strength is further away. Stronger, you're closer.
  * Keep hands away from the USB Port of M5Stack devices. The WiFi antenna is near the USB port and your body will block some of the signal.
  * Adjust the RSSI Sensitivity and Alert Packet count for your environment.
* Configuration
  * Adjust `DH Alert Pkts` in settings to modify the threshold for alerts.
  * Adjust "DH RSSI` in settings to adjust the sensitivity of the signal strength meter (smaller negative numbers are less sensitive, -20dBm is less sensitive than -50dBm) 

## PineAP Hunter
Identifies the presence of rogue access points that are broascasting many different SSIDs. This is a characteristic of the "PineAP" feature of the Hak5 WiFi Pineapple, and similar KARMA attacks.  
* Usage
  * PineAP Hunter performs WiFi Scans and builds a list of all identified BSSID/SSID pairs (up to a total of 50) in memory
  * Buttons are briefly unresponsive during WiFi Scans in PineAP Hunter mode
  * Any BSSID advertising a number of SSIDs above the alert threshold is added to the PineAP List and the alert is triggered.
  * Move the cursor to the desired PineAP in the list and press the SELECT button to view the SSID List. This also stops the alerts.
  * Keep hands away from the USB Port of M5Stack devices. The WiFi antenna is near the USB port and your body will block some of the signal.
  * Adjust the Alert SSID count for your environment if you receive false positives.
  * The SSID list will continue to update, with the most recently seen SSID at the top of the list, with RSSI to the left of each entry
  * Use the most recently-reported RSSI to help you determine the source of the signal.  (smaller negative numbers are stronger, -20dBm is closer to you than -50dBm)
* Configuration
  * Adjust `PH Alert SSIDs` in settings to modify the threshold for alerts. 
* Notes
  * Some professional access points may trigger the default value of 5. My home network, for example, broadcasts 3 different SSIDs on each access point.
  * Due to how ESP32 WiFi Scanning works, only one SSID per BSSID (MAC Address of an access point) can be identified at a time.
  * PineAP Hunter will have to perform no fewer than `PH Alert SSIDs` scans before it will positively identify a rogue access point. Consider keeping this value at or below 5 unless you receive false positives.
  * For very busy areas with a PineAP advertising dozens of SSIDs, it may take several minutes to build the entire list of SSIDs used.
  * These are all side-effects of the WiFi API for ESP32. Despite these limitations, it seems to work great, but it is not as quick to detect malicious activity as the other tools.

## Install from M5Burner
This is the absolute easiest way to get NEMO
* [M5Burner](https://burner.m5stack.com/device/stickc-plus2) is the official tool for StickC Plus2 firmware. Upstream NEMO builds are published there by 4x0nn. This fork is flashed from the local `build/` image.
* Launch M5Burner
* Select StickC Plus2
* Use the search at the top of the app to look for "NEMO". Official upstream builds are uploaded by "4x0nn". This fork is built locally, not published there.
* Click Download
* Click Burn

## Install .bin files manually with esptool.py
* Install ESP-IDF tools per the [Espressif Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
* Open the esp-idf CMD tool (on Windows) - on Mac or Linux, esp-idf.py and esptool.py should be in the system path.
```bash
esptool --chip esp32 --port /dev/cu.usbserial-XXXXXXXX write-flash -z 0x0 build/M5Nemo-M5StickCPlus2.bin
```

The port is a COM port such as `COM4` on Windows, or a `/dev/cu.usbserial-*` / `/dev/ttyUSB0` path on macOS and Linux. Flash the merged image at offset `0x0`. The chip is an ESP32, not an ESP32-S3.

## Building from Source
The supported build is `./scripts/build-stickc-plus2.sh`. It installs the M5Stack ESP32 core 3.2.5, pins **M5GFX 0.2.36** and **M5Unified 0.2.8**, compiles `src/m5stick-nemo`, and merges the flash image to `build/M5Nemo-M5StickCPlus2.bin`.

Leave M5GFX on 0.2.36. Version 0.3.x boots this Stick with a black screen. The script installs whatever the library index pulls, then replaces M5GFX and M5Unified with those two versions.

```bash
./scripts/build-stickc-plus2.sh
./scripts/flash.sh /dev/cu.usbserial-XXXXXXXX
```

Requirements: Arduino CLI (or the copy shipped inside the Arduino IDE) and esptool. The core index is `https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json`. The board is `m5stack:esp32:m5stack_stickc_plus2`, partition scheme Huge APP. Libraries land in `.arduino/`, which is gitignored.

The sketch is already the Plus2 firmware. There is no board `#define` to uncomment. Pass `-DLANGUAGE_PT_BR` (or `LANGUAGE_EN_GB`, `LANGUAGE_IT_IT`, `LANGUAGE_FR_FR`) instead of `-DLANGUAGE_EN_US` in the script if you want another menu language. The default is US English, which also limits Wi-Fi scans to channels 1–11.

### Arduino IDE
* Add the M5Stack board URL above under File → Preferences → Additional boards manager URLs.
* Install the M5Stack ESP32 core 3.2.5 and select M5StickC Plus2.
* Install IRremote, then install M5GFX 0.2.36 and M5Unified 0.2.8 without letting the library manager upgrade M5GFX to 0.3.x.
* Set the partition scheme to Huge APP (`No OTA (Large APP)`).
* Open `src/m5stick-nemo/m5stick-nemo.ino`.



## Troubleshooting
* Several features output debugging information to the serial monitor. Use the Serial Monitor feature in Arduino IDE or M5Burner to gather this information. It may have useful hints. When filing a bug report, it often helps to include serial monitor output.
* Reset the EEPROM from Settings → Clear Settings, or hold the side button while powering on.
* TV-B-Gone's IR LED can be observed through a smart phone camera, emitting a pale purple beam of light. If it seems to be on constantly, or if it never flashes at all during TV-B-Gone operations, something is wrong. Report a bug. There's a known issue with TVBG not working after using Bluetooth spam or random wifi spam.
* Try viewing wifi lists from several different devices if you suspect wifi spam isn't working. Sometimes, Linux network manager can see networks that smart phones cannot. Please include the results of this testing if reporting wifi spam problems.
* Apple has patched a lot of Bluetooth stuff since summer 2023. If testing AppleJuice, try some of the AppleTV device types, as they tend to be more reliable due to apple not filtering out weaker bluetooth signals for that platform.


## Reporting Bugs
Please report bugs via GitHub Issues. These are easier to track than comments on social media posts, M5Burner entries, etc. If something isn't working, please include:
* Firmware version
* How you installed it (M5Burner, compiled yourself, esptool.py)
* Hardware 
* Specifically what feature and options are not working
* How you determined that it is not working and what testing you did. Include device models and operating systems of what you tested with, and any errors or relevant output from Serial Monitor, if applicable.
* If you figure out how to fix a bug you identified, PRs are welcome!

## Contributing
Contributions are welcome. 
* Please look at the GitHub Issues for the project. There are feature suggestions and bugs reported there, and I'd appreciate PRs that address those.
* When submitting a Pull Request, please target the develop branch. The easiest way to do this is to fork ALL branches, or to simply create a "develop" branch in your own fork, then use GitHub to Sync your develop branch.
* This fork targets the M5StickC Plus2 only. Use `FGCOLOR`, `BGCOLOR`, `BIG_TEXT` / `MEDIUM_TEXT` / `SMALL_TEXT` / `TINY_TEXT`, and the `DISP` alias for the built-in display.
* Feel free to add your github to the contributors array as part of your pull request.

Things I'd like help on:
* Improved localization/translations of the menu, not just NEMO Portal HTML.
  * Probably needs a new .h file
  * Be configurable in settings and use an eeprom byte to save the setting
  * Make gratuitous use of `const` in the implementation so that localization strings are stored only in, and referenced directly from, the flash storage as opposed to using lots of SRAM.
* Infrared
  * A new "Region" of TV-B-Gone that's full of additional IR codes that can turn RGB LED strips, air conditioners, fans, sound bars and the like on and off
  * A way to convert or use flipper zero IR codes from within NEMO's code-base
Things I probably won't merge in:
* WiFi deauthentication
* Bluetooth spam that potentially disrupts health and fitness trackers, smart watches, etc.
