/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * License: MIT NON-AI
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
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   The port ("Tools -> "Port") is named /dev/cu.SLAB_USBtoUART, and the maximum
 *   upload speed ("Tools" -> "Upload Speed") can be used.
 *   The CH340 is supported out-of-the-box since Mojave. The port is named 
 *   /dev/cu.usbserial-XXXX (XXXX being some random number), and the maximum upload 
 *   speed is 460800.
 *   Windows:
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
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
 * - Install the audio data: 
 *   Method 1:
 *   - Go to Config Portal, click "Update" and upload the audio data (DGA.bin, extracted
 *     from install/sound-pack-xxxxxxxx.zip) through the bottom file selector.
 *     An SD card must be present in the slot during this operation.
 *   Method 2:
 *   - Copy DGA.bin to the top folder of a FAT32 (not ExFAT!) formatted SD card (max 
 *     32GB) and put this card into the slot while the Gauges are powered down. 
 *   - Now power-up. The audio files will now be installed. When finished, the Gauges 
 *     will reboot.
 */

/*  Changelog
 *  
 *  2024/10/26 (A10001986) [1.10]
 *    - Add support for TCD multicast notifications: This brings more immediate speed 
 *      updates (no more polling; TCD sends out speed info when appropriate), and 
 *      less network traffic in time travel sequences.
 *      The TCD supports this since Oct 26, 2024.
 *  2024/09/11 (A10001986)
 *    - Fix C99-compliance
 *  2024/09/09 (A10001986)
 *    - Tune BTTFN poll interval
 *  2024/08/29 (A10001986)
 *    - Fix gauge type selection "0-2.048V" for Roentgens
 *  2024/08/28 (A10001986)
 *    - Treat TCD-speed from Remote like speed from RotEnc
 *  2024/08/11 (A10001986)
 *    - In addition to the custom "key3" and "key6" sounds, now also "key1", 
 *      "key4", "key7" and "key9" are available/supported; command codes are 
 *      9001, 9004, 9007, 9009.
 *  2024/07/16 (A10001986)
 *    - Remove Gauge Type "Simpson Roentgens Meter"; the Original Simpson meter needs
 *      10k+8k2 resistors and is run with type "Generic 0-2.048V".
 *  2024/07/15 (A10001986)
 *    - Add "PLAY_DOOR_OPEN"/"_CLOSE" MQTT commands; honored only when "Play door sounds" is 
 *      disabled in CP, as those would interfere with the door switch logic.
 *  2024/07/13 (A10001986)
 *    - Make slow drain during Time Travel optional for each meter (analog gauges only)
 *  2024/07/07 (A10001986)
 *    - Fix filename sorting
 *  2024/07/07 (A10001986) [1.0]
 *    - Fix MQTT commands
 *    - MQTT: Add MP_FOLDER_x commands to set music folder number
 *    - Fine tune pointer movement during Time Travel
 *  2024/06/05 (A10001986)
 *    - Minor fixes for WiFiManager
 *    * Switched to esp32-arduino 2.0.17 for pre-compiled binary.
 *  2024/05/17 (A10001986)
 *    - Some optimizations for digital gauges
 *  2024/05/05 (A10001986)
 *    - Remove i2c addresses from array entries; reset MCP4728 EEPROM if not used.
 *  2024/05/04 (A10001986)
 *    - Add "EMPTY" and "REFILL" MQTT commands
 *  2024/05/01 (A10001986)
 *    - CB1.04: Swap pins for Backlight and Door 1 switch
 *  2024/04/30 (A10001986)
 *    - Add "beep" sound when TT button is held to unlock Gauge hardware type selection
 *      in CP
 *  2024/04/28 (A10001986)
 *    - CB 1.04: Changed Side Switch from active high to active low
 *  2024/04/27 (A10001986)
 *    - Support second door switch on new Control Board v1.03
 *  2024/04/25 (A10001986)
 *    - Add mechanism to avoid switching on/off digital gauges too quickly. Gauges
 *      with motors might get confused and stuck at arbitrary positions on very short
 *      periods of power (if the cap hasn't fully been charged to bring them back to 0).
 *  2024/04/24 (A10001986)
 *    - Added generic analog gauge types (0-5V, 0-4.095V, 0-2.048V)
 *  2024/04/22 (A10001986)
 *    - Gauge types can now be selected separately, not only in "groups" any more. 
 *      For each gauge (Primary, Percent Power, Roentgens), the hardware access method 
 *      and type (voltage range) can be defined.
 *      The complete definition table is now in dgdisplay.cpp.
 *    - For binary gauges, the threshold is now configurable in the CP
 *    - CB 1.01: Add support for "Button 1"; HOLDing it makes the DG say the IP
 *      address out loud
 *  2024/04/10 (A10001986)
 *    - Add support for SwitchBoard v2 (unreleased)
 *      The firmware auto-detects the hardware version.
 *    - Add Music Folder to CP; Music-folder-change optimization
 *    - Make hardware support for volume knob a compile time option
 *      (knob support disabled by default, see global.h) 
 *  2024/04/07 (A10001986)
 *    - Re-locate volume setting in CP
 *  2024/04/06 (A10001986)
 *    - Rewrite settings upon clearing AP-PW only if AP-PW was actually set.
 *    - Re-phrase "shuffle" Config option
 *    - Re-phrase "Fixed volume" settings in Config Portal
 *  2024/03/26 (A10001986)
 *    - BTTFN device type update
 *  2024/02/08 (A10001986)
 *    - CP: Add header to "Saved" page so one can return to main menu
 *  2024/02/06 (A10001986)
 *    - Fix reading and parsing of JSON document
 *    - Fixes for using ArduinoJSON 7; not used in bin yet, too immature IMHO.
 *  2024/02/05 (A10001986)
 *    - Tweaks for audio upload
 *  2024/02/04 (A10001986)
 *    - Include fork of WiFiManager (2.0.16rc2 with minor patches) in order to cut 
 *      down bin size
 *  2024/02/03 (A10001986)
 *    - Audio data (DGA.bin) can now be uploaded through Config Portal ("UPDATE" page). 
 *      Requires an SD card present.
 *    - Check for audio data present also in FlashROMode; better container validity
 *      check; display hint in CP if current audio not installed
 *  2024/01/26 (A10001986)
 *    - Reformat FlashFS only if audio file installation fails due to a write error
 *    - Add sound-pack versioning; re-installation required with this FW update
 *    - Allow sound installation in Flash-RO-mode
 *  2024/01/22 (A10001986)
 *    - Fix for BTTFN-wide TT vs. TCD connected by wire
 *  2024/01/21 (A10001986)
 *    - Major cleanup (no functional changes)
 *  2024/01/18 (A10001986)
 *    - Fix WiFi menu size
 *  2024/01/15 (A10001986)
 *    - Flush outstanding delayed saves before rebooting and on fake-power-off
 *    - Remove "Restart" menu item from CP, can't let WifiManager reboot behind
 *      our back.
 *  2023/01/04 (A10001986)
 *    - Read millis() instead of using static variable in main loop (too many subs 
 *      to keep track)
 *  2023/12/21 (A10001986)
 *    - Correct voltage for Simpson Roentgens meter to 14mV. TODO: Find a suitable
 *      resistor value for driving it with a reasonable voltage. 14mV is ridiculous.
 *  2023/12/21 (A10001986)
 *    - Add hardware config for original Simpson Roentgen meter (0-0.14V)
 *  2023/12/16 (A10001986)
 *    - Make DoorSwitch support a compile time option in order to allow using GPIO
 *      for other purposes
 *  2023/12/08 (A10001986)
 *    - Add option to trigger a BTTFN-wide TT when pressing the TT button (instead
 *      of a stand-alone TT).
 *    - Fix wakeup vs. SS logic
 *  2023/11/30 (A10001986)
 *   - Some better sounds
 *   - Fix end of "special" LED signal
 *  2023/11/22 (A10001986)
 *    - Wake up only if "speed" is from RotEnc, not when from GPS
 *  2023/11/20 (A10001986)
 *    - Wake up on GPS speed/RotEnc changes
 *  2023/11/06 (A10001986)
 *    - Abort audio file installer on first error
 *  2023/11/05 (A10001986)
 *    - Settings: (De)serialize JSON from/to buffer instead of file
 *    - Fix corrupted Shuffle setting
 *  2023/11/04 (A10001986)
 *    - Unmount filesystems before reboot
 *  2023/11/02 (A10001986)
 *    - Start CP earlier to reduce startup delay caused by that darn WiFi scan upon
 *      CP start.
 *    * WiFiManager: Disable pre-scanning of WiFi networks when starting the CP.
 *      Scanning is now only done when accessing the "Configure WiFi" page.
 *      To do that in your own installation, set _preloadwifiscan to false
 *      in WiFiManager.h
 *  2023/11/01 (A10001986)
 *    - Change default volume to fixed, level 6; fix saving from CP.
 *    - Stop audio earlier before rebooting when saving CP settings
 *  2023/10/31 (A10001986)
 *    - BTTFN: User can now enter TCD's hostname instead of IP address. If hostname
 *      is given, TCD must be on same local network. Uses multicast, not DNS.
 *    - Add "binary" gauges types; "binary" means the gauges only know "empty" and "full".
 *      Two types added: One for when gauges have separate pins, one when gauges are 
 *      interconnected. This is meant for gauge types with higher voltages that are 
 *      driven through relays.
 *  2023/10/29 (A10001986)
 *    - Fixes for saving volume (cp value not copied; copy sec.settings)
 *  2023/10/26 (A10001986)
 *    - New startup sound, played with delay
 *  2023/10/17 (A10001986)
 *    - Add Plutonium-unrelated "door switch" facility. Connect a door switch to 
 *      IO27 (active low; wires to door switch need to be GND and IO27), and the Panel 
 *      will play a "door" sound whenever the switch activates (NC/NO selectable in CP), 
 *      with optional delay.
 *    - Early-init side & door switches and do not trigger event until next change
 *  2023/10/14 (A10001986)
 *    - Disable s-s upon entering new "full" percentages
 *    - Correct max voltage for VU meter to 1.6V
 *  2023/10/05-10 (A10001986)
 *    - Add support for "wakeup" command (BTTFN/MQTT)
 *    - Make "empty" level configurable
 *    - Correct VU meter max voltage to 1.66. Meter is manually adjusted to "0"
 *      on the green "Roentgens" scale, which is a bit into the original VU scale.
 *    - Some screen-saver logic fixes
 *  2023/10/04 (A10001986)
 *    - Fix audio playback from flash FS
 *  2023/09/30 (A10001986)
 *    - Extend remote commands to 32 bit
 *    - Fix volume control (CP vs TCD-remote commands)
 *  2023/09/29 (A10001986)
 *    - Add emercengy procedure for deleting static IP and AP password (hold TT while
 *      powering up, switch side switch twice (empty LED lits), then release TT button.
 *    - Add volume control in CP
 *  2023/09/28 (A10001986)
 *    - New sounds; complete installer; add volume control
 *  2023/09/27 (A10001986)
 *    - First version of TT
 *  2023/09/26 (A10001986)
 *    - Make TCD-alarm sound optional
 *    - Make refill more robust; better LED handling; many other changes.
 *    - Refactor gauge handling for easily adding other gauge types
 *  2023/09/25 (A10001986)
 *    - Add option to handle TT triggers from wired TCD without 5s lead. Respective
 *      options on TCD and external props must be set identically.
 *  2023/09/23 (A10001986)
 *    - Add remote control facility through TCD keypad (requires BTTFN connection 
 *      with TCD). Commands for Dash Gauges are 9000-9999.
 *  2023/09/22 (A10001986)
 *    - Read DAC EEPROM data and reset if not properly set
 *    - lots of other additions
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
