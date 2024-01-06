/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.backtothefutu.re
 *
 * License: MIT
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
 *   the latest version by Espressif Systems.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model (the
 *   CircuitSetup original boards are "NodeMCU-32S")
 *   
 * - Connect your ESP32 board.
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
 *   - WifiManager (tablatronix, tzapu) https://github.com/tzapu/WiFiManager
 *     (Tested with 2.0.13beta, 2.0.15-rc1, 2.0.16-rc2)
 *     For versions 2.0.16-rc2 and below, in order to avoid a delay when powering up
 *     several BTTFN-connected props, change _preloadwifiscan to false in WiFiManager.h 
 *     before compiling:
 *     -boolean       _preloadwifiscan        = true;
 *     +boolean       _preloadwifiscan        = false;
 *   - ArduinoJSON >= 6.19: https://arduinojson.org/v6/doc/installation/
 *
 * - Download the complete firmware source code:
 *   https://github.com/realA10001986/Dash-Gauges/archive/refs/heads/main.zip
 *   Extract this file somewhere. Enter the "dashgauges-A10001986" folder and 
 *   double-click on "dashgauges-A10001986.ino". This opens the firmware in the
 *   Arduino IDE.
 *
 * - Go to "Sketch" -> "Upload" to compile and upload the firmware to your ESP32 board.
 *
 * - Install the audio files: 
 *   - Copy the contents of install/sound-pack-xxxxxxxx.zip in the top folder of a FAT32 
 *     (not ExFAT!) formatted SD card (max 32GB) and put this card into the slot while
 *     the device is powered down. Now power-up the device.
 *   - The audio files will be installed automatically, SD no longer needed afterwards.
 *     (but is recommended to be left in slot for saving settings and avoiding flash
 *     wear on the ESP32.)
 */

/*  Changelog
 *   
 *  TODO: Way to make device say IP address stand-alone
 *  
 *  2023/01/04 (A10001986)
 *    - Read millis() instead of using static variable in main loop (too many subs 
 *      to keep track)
 *  2023/12/21 (A10001986)
 *    - Correct voltage for Simpson Roentgens meter to 14mV. TODO: Find a suitable
 *      resistor value for driving it with a reasonable voltage. 140mV is ridiculous.
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
 *    - Correct VU meter max voltage to 1.66. VU meter is manually adjusted to "0"
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
