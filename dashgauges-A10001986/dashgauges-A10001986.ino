/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 * This code was written without the use of artificial intelligence.
 *
 * License: Modified MIT NON-AI
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Build instructions (for Arduino IDE)
 * 
 * - Install the Arduino IDE
 *   https://www.arduino.cc/en/software
 *    
 * - This firmware requires the "ESP32-Arduino" framework. To install this framework, 
 *   in the Arduino IDE, go to "File" > "Preferences" and add the URL   
 *   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *   to "Additional Boards Manager URLs". The list is comma-separated.
 *   
 * - Go to "Tools" > "Board" > "Boards Manager", then search for "esp32", and install 
 *   the latest 2.x version by Espressif Systems. Versions >=3.x are not supported.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model (the
 *   CircuitSetup original boards are "NodeMCU-32S")
 *   
 * - Connect your ESP32 board using a suitable USB cable.
 *   Note that NodeMCU ESP32 boards come in two flavors that differ in which serial 
 *   communications chip is used: Either SLAB CP210x USB-to-UART or CH340. Installing
 *   a driver might be required.
 *   Mac: 
 *   For the SLAB CP210x (which is on NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   The port ("Tools -> "Port") is named /dev/cu.SLAB_USBtoUART, and the maximum
 *   upload speed ("Tools" -> "Upload Speed") can be used.
 *   The CH340 is supported out-of-the-box since Mojave. The port is named 
 *   /dev/cu.usbserial-XXXX (XXXX being some random number), and the maximum upload 
 *   speed is 460800.
 *   Windows:
 *   For the SLAB CP210x (which is on the NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   After installing this driver, connect your ESP32, start the Device Manager, 
 *   expand the "Ports (COM & LPT)" list and look for the port with the ESP32 name.
 *   Choose this port under "Tools" -> "Port" in Arduino IDE.
 *   For the CH340, another driver is needed. Try connecting the ESP32 and have
 *   Windows install a driver automatically; otherwise search google for a suitable
 *   driver. Note that the maximum upload speed is either 115200, or perhaps 460800.
 *
 * - Install required libraries. In the Arduino IDE, go to "Tools" -> "Manage Libraries" 
 *   and install the following libraries:
 *   - ArduinoJSON (>= 6.19): https://arduinojson.org/v6/doc/installation/
 *
 * - Download the complete firmware source code:
 *   https://github.com/realA10001986/Dash-Gauges/archive/refs/heads/main.zip
 *   Extract this file somewhere. Enter the "dashgauges-A10001986" folder and 
 *   double-click on "dashgauges-A10001986.ino". This opens the firmware in the
 *   Arduino IDE.
 *
 * - Go to "Sketch" -> "Upload" to compile and upload the firmware to your ESP32 board.
 *
 * - Install the sound-pack: 
 *   - Go to Config Portal, click "Update & Upload" and upload the sound-pack (DGA.bin,
 *     extracted from install/sound-pack-dgXX.zip) through the bottom file selector.
 *     An SD card must be present in the slot during this operation.
 *   Alternatively:
 *   - Copy DGA.bin to the top folder of a FAT32 (not ExFAT!) formatted SD card (max 
 *     32GB) and put this card into the slot while the Gauges are powered down. 
 *   - Now power-up. The sound-pack will now be installed. When finished, the Gauges 
 *     will reboot.
 */

/*  Changelog
 *
 *  2026/10/20 (A10001986) [2.00]
 *    - Re-arrange WiFi Configuration page for a better user experience
 *    - BTTFN: "Hostname of TCD" now defaults to "timecircuits" instead of being empty.
 *      As a result, factory-fresh Dash Gauges now connect to a TCD in the network without
 *      any user configuration changes.
 *    - Holding Button 1 during (real)power-on and, while still holding Button 1, flipping
 *      the side-switch exactly six times within 10 seconds resets all settings, except 
 *      the Gauge Hardware settings, to factory defaults.
 *    - New sound-pack (DG07)
 *    - Future firmware updates may only consist of one single file instead of "main 
 *      firmware" plus "sound-pack". Firmware updates will require an SD card.
 *      Future sound-pack uploads, if any, are done through the top file selector, and
 *      no longer through the bottom one. The bottom one is for custom mp3 file upload
 *      only.
 *    * If updating from <1.30, some settings will be reset to defaults.
 *  2026/09/20 (A10001986) [1.36]
 *    **********************************************************************************
 *    ** If updating from below 1.30, please see boxed note at version 1.31 below     **
 *    **********************************************************************************
 *    - Car Mode: Automatically pair Dash Gauges to currently connected TCD by means of
 *      the TCD's BSSID upon first successful connection in Car Mode. This avoids 
 *      connecting to other people's TCDs if in close range (eg on meet-ups).
 *    - Fix minor issues with ID3v1 tags
 *    - Speed up booting with folders containing many files
 *    - Music Player: The "TCD_DONE.TXT" file is now obsolete. The firmware instead uses
 *      a cache file in the top-most folder of the SD card ("musicXc") which needs to be
 *      deleted when files are added to the respective folder.
 *    - Protect the Dash Gauges from uploading a wrong firmware by accident. The filename
 *      of the firmware binary now must contain the word "gauges". The check is case-
 *      insensitive.
 *    - Bonjour/mDNS: Send good-bye packet on controlled reboots
 *  2026/08/25 (A10001986) [1.35]
 *    **********************************************************************************
 *    ** If updating from below 1.30, please see boxed note at version 1.31 below     **
 *    **********************************************************************************
 *    - New soundpack (DG06)
 *    - Some logic changes with regard to sequences interrupting each other
 *    - Unmute audio if muted while speaking IP address
 *    - More robust error handling in case of malformed config files
 *    - SD: More compatibility fixes
 *    - Code optimizations and minor fixes
 *  2026/07/17 (A10001986) [1.34]
 *    **********************************************************************************
 *    ** If updating from below 1.30, please see boxed note at version 1.31 below     **
 *    **********************************************************************************
 *    - Bugfix: Fix SD initialization in ESP32-Arduino to conform to specs.
 *      Sandisk Ultra 32GB cards (and possibly others that previously were
 *      not usable) are now recognized.
 *  2026/06/25 (A10001986) [1.33]
 *    **********************************************************************************
 *    ** If updating from below 1.30, please see boxed note at version 1.31 below     **
 *    **********************************************************************************
 *    - MQTT: Add "VOLUME_UP"/"VOLUME_DOWN"/"VOLUME_SET_xxx" commands
 *    - MQTT: Add publishing Music Player status to bttf/dg/mpstatus
 *    - MQTT: Don't queue and instead ignore (most) commands while fake-off or busy
 *    - Music Player: Pre-scan folders and display status in Config Portal
 *    - Music Player: Use new sorting algorithm as the old one caused a crash on a 
 *      large number of already sorted files (due to excessive recursion).
 *  2026/04/27 (A10001986) [1.32]
 *    **********************************************************************************
 *    ** If updating from below 1.30, please see boxed note at version 1.31 below     **
 *    **********************************************************************************
 *    - Add Car Mode: In addition to existing WiFi network connection settings, you 
 *      can now pre-configure TCD-AP SSID and password in WiFi Settings and quickly
 *      switch between normal (home, iPhone...) WiFi connection and Car WiFi connection 
 *      (through TCD-AP) by holding "Button 1" for 6 seconds (triple-beep) or through 
 *      the Config Portal.
 *    - Behavior of Button 1 changed: Press is still a brief press. Holding the button
 *      to read out the IP address now means holding it until a double-beep sounds, then
 *      the button must be released. (Previously, the IP read-out started automatically
 *      after 2 seconds, without releasing the button, and no sound was played.)
 *    - Fix race condition in Fake Power logic
 *    - New sound-pack (DG05)
 *  2026/03/25 (A10001986) [1.31]
 *    **********************************************************************************
 *    ** If updating from below 1.30, please install 1.30 first to have your          **
 *    ** settings converted. If 1.30 is skipped, some of your settings (static IP,    **
 *    ** volume, music folder number) will be restored to default values. It suffices **
 *    ** to install 1.30 and boot once; you can then immediately update to a later    **
 *    ** version.                                                                     **
 *    ** https://github.com/realA10001986/Dash-Gauges/releases/tag/V1.30              **
 *    **********************************************************************************
 *    - WiFi: Allow defining a BSSID (AP MAC address) to connect to a specific AP
 *      if multiple APs with identical SSID are available.
 *    - MQTT: Disable if server can't be resolved
 *    - WiFi: Do not power down AP long as a client is connected
 *    - Brush up Config Portal a bit
 *    - Code optimizations and fixes.
 *  2026/02/16 (A10001986) [1.30]
 *    - New file format for secondary and IP settings. This version of the firmware
 *      converts old to new.
 *    - "Shuffle" setting changes through 9222/9555 are saved (SD required).
 *    - Display MAC address (STA) on WiFi Configuration Page
 *    - Avoid falling back to stand-alone mode too early when TCD stops sending
 *      packets
 *    - Config files are now only written if actually changed which prolongs
 *      Flash life-span.
 *    - Code optimizations and fixes.
 *  [...]
 *  2023/09/21 (A10001986)
 *    - Initial version
 */

#include "dg_global.h"

#include <Arduino.h>
#include <Wire.h>

#include "dg_audio.h"
#include "dg_settings.h"
#include "dg_wifi.h"
#include "dg_main.h"

void setup()
{
    powerupMillis = millis();
    
    Serial.begin(115200);
    Serial.println();

    Wire.begin(-1, -1, 100000);

    main_boot();
    settings_setup();
    main_boot2();
    wifi_setup();
    audio_setup();
    main_setup();
    bttfn_loop();
}

void loop()
{    
    main_loop();
    audio_loop();
    wifi_loop();
    audio_loop();
    bttfn_loop();
}

#if defined(DG_DBG) || defined(DG_DBG_NET) || defined(DG_PROFILER)
#warning "Debug output is enabled. Binary not suitable for release."
#endif
