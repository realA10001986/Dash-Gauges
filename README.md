# Dash Gauges

This repository holds some DIY instructions for building a Dash Gauge panel, as seen in the Back To The Future movies, and a suitable firmware for that panel. This panel is meant as an add-on for the CircuitSetup [Time Circuits Display](https://tcd.backtothefutu.re) as it relies on the TCD's keypad to control many of its functions.

![Dash Gauges](https://github.com/realA10001986/Dash-Gauges/assets/76924199/e01aeb44-ea61-4685-95ee-d673b1e654ff)

The Panel consists of several parts:
- Aluminum (Aluminium for non-Americans) enclosure; the measurements are in the ["enclosure"](enclosure) folder of this repository.
- The gauges: The smaller gauges are H&P 631-14672 (built by Phaostron) and the "Plutonium chamber" gauge is a Simpson 49L VU Meter, all driven by a MCP4728 quad-DAC (on an Adafruit break-out board). Movie-accurate faces for these gauges are in the ["faces-labels"](faces-labels) folder of this repository.
- A Switch Board, which is mounted on the smaller gauges' back side; it carries the interface for the gauges, the lights and switches, as well as some connectors for external switches/buttons. This Switch Board is easy to assemble, it only carries some resistors and connectors. The gerbers as well as an EasyEDA file are in the ["electronics"](electronics) folder of this repository, JLCPCB can make this board for you. A BOM is available as well.
- A Control Board: In lack of a dedicated control board (which to design unfortunately is beyond my abilities), the heart of the device is a slightly modified TCD control board from [CircuitSetup](https://circuitsetup.us/product/time-circuits-display-control-board-with-keypad-trw-style-lenses/).

Firmware features include
- selectable "full" percentages per gauge (besides for fun, useful for adjusting inaccurate readings)
- [Time Travel](#time-travel) function, triggered by button, [Time Circuits Display](https://tcd.backtothefutu.re) (TCD) or via [MQTT](#home-assistant--mqtt)
- support for Side Switch to play "empty" and "refill" sequences
- Automatic refill timer, automatic alarm mute timer (both optional)
- easy expandability to support different gauges hardware
- Support for door switch for playing sounds when opening/closing the car doors
- Wireless communication ("[BTTF-Network](#bttf-network-bttfn)") with [Time Circuits Display](https://tcd.backtothefutu.re); used for synchonized time travels, alarm, night mode, fake power and remote control through TCD keypad
- [Music player](#the-music-player): Play mp3 files located on an SD card [requires TCD connected wirelessly for control]
- [SD card](#sd-card) support for custom audio files for effects, and music for the Music Player
- Advanced network-accessible [Config Portal](#the-config-portal) for setup with mDNS support for easy access (http://gauges.local, hostname configurable)
- [Home Assistant](#home-assistant--mqtt) (MQTT 3.1.1) support
- Built-in installer for default audio files in addition to OTA firmware updates

## Hardware

Note that this is a custom built prop; there is no kit available.

### Control board

The control board is a modified TCD control board from CircuitSetup:

- The ENTER button is removed from the PCB and its pins serve as the "Side Switch".
- The white LED serves as the "Empty" LED in the "Roentgens" meter; it needs to desoldered.
- The other LEDS are removed from the PCB as they stick out and are not needed.
- The volume pot can be removed; this is, of course, optional, but since this board is assumingly hidden somewhere, the pot would be inaccessible anyway.
- The i2c, audio and IO connectors were relocated to the front side of the PCB (they are normally on the back)

![ControlBoard](https://github.com/realA10001986/Dash-Gauges/assets/76924199/82f25c22-d60b-493c-a785-5c8b78f2ca32)

As regards the cabling, two remarks:
- I used each 2 wires for GND and 5V (black/white, green/blue), since the wires I used are quite thin.
- I put each a wire of GND (brown, orange) between and beside the SDA/SCL wires to avoid crosstalk.

### Switch board

In order to reduce cable chaos I designed a simple "switch board" which mounts on the smaller gauges. It carries the MCP4728 breakout board, a single channel relay board, all resistors for the gauges and LEDs, the LEDs for the smaller gauges and connectors for cables to the control board and to the "Roentgens" gauge. Also, it has connectors for an external Time Travel button and the door switches.

![SwitchBoard1.7](https://github.com/realA10001986/Dash-Gauges/assets/76924199/1adf1639-6e81-44df-b56d-0eba04ba26b8)

It carries this relay module for the lights (desolder the screw terminals, put in pin socket, pitch 5mm, 3 positions, for example Samtec HPF-03-01-T-S):

![relay](https://github.com/realA10001986/Dash-Gauges/assets/76924199/eef6cc62-21e1-4700-99df-499739c4ea61)

This module is not the most elegant solution but the easiest one if you, like me, are using real lamps instead of LEDs: also, it allows for an external power source for the lights (eg 12V).

The Switch Board also carries the Adafruit MCP4728 break-out board; just solder pin sockets to both lines of pads on the PCB (pitch 2.54mm, 6 positions, for example Samtec SSW-106-01-F-S). 

Connectors on the Switch Board:
- Audio: This allows connecting the audio from the Control Board; the signal is directed to the top-left audio connector, where a speaker is to be attached.
- Data (from left to right):
  * 3_3V: 3.3V from the Control Board
  * SW: From the ENTER button of the Control Board
  * E-LED: From the white LED on the Control Board
  * IO14: From "IO14" on the Control Board
  * IO13: From "IO13" on the Control Board
  * IO27: From "IO27" on the Control Board
 
- 5V/GND: 5V and GND from the Control Board. There are two connectors for power on the Switch Board; you can choose which one suits you better.

- SCL/SDA: The i2c signal from he Control Board.

A connection diagram is [here](electronics/connection%20diagram.png).

### Gauges

The gauges are connected through an Adafruit MCP4728 breakout board, which is an i2c-driven quad-DAC and mounted on the switch board. This DAC allows arbitrary voltages up to 5V, but the gauges used here use much lower voltages. If you use different gauges, you need to add your configuration to the firmware. More on this below.

*Important*: When using the MCP4728 in your project, do not connect the actual gauges until you selected the right hardware type in the Config Portal and have powered-down and powered-up the device once afterwards. This power-cycle is needed to reset the MCP4728's EEPROM to the correct settings and to avoid a power output too high at boot.

#### "Primary", "Percent Power"

For the smaller gauges I used two H&P 631-14672 (built by Phaostron), which are (were?) oddly available in volumes on ebay. The pointers on these are red, but an Edding permanent marker was quickly able to change them to black. These gauges have a scale from 0 to 0.5V, but - again: oddly - can't cope with 0.5V, they need a resistor in series. I found 8k2 + 470R to be the best value (R1+R2, R3+R4 on the Switch Board).

Those two gauges are driven by channels A and B of the MCP4728. They show nearly identical readings at 10% and at 90%, in the middle they differ a bit,despite the voltage being identical. But since the idea here is not to actually measure voltage, that is still ok, the "full" percentage is configurable after all.

The movie-accurate faces of those gauges are available in this repository.

For the backlight, I drilled a hole in the rear of the metal enclosure, center bottom, and put a yellow LED (590nm) on the Switch Board (LED1, LED2, with suitable resistors (R8/R9; depending on LED, probably around 150R). Most replicas use white LEDs, but I think on the A-Car as shown in the Petersen Museum, there are yellow ones used, and I found white ones too bright.

#### "Roentgens"

The big "Roentgens" gauge is more of a problem. I was lucky to score a Simpson 49L VU-meter, which is operated with a 3k6 resistor in series (R5+R6 on the Switch Board). Its lighting is through two 6V lamps which need no resistor; R11 on the Switch Board therefore needs to be bridged with a wire. If you prefer LEDs, put a suitable resistor in place instead.

Properly looking Simpson meters are obtainable, even on ebay, but there are many different ones. In the movie, they used one with lights (BA9S, 6V), which means that the enclosure is much thicker than on those without. That thickness, however, is needed for the "Empty" light. The second problem is that the meters mostly are for voltages beyond what we have available. Since I am more of a software guy, I can't give any further advice here; maybe the voltmeters can be modified somehow (I assume they have big resistors which can be removed or bridged), or a level shifter can be used. In the worst case, the meter can only show two values - 0 and "full" via a level shifter or a relay, using external power. For the purpose of the Dash Gauges that is not perfect, but enough for re-creating the movie scenes.

The "Empty" light: That was the easy part; I used a button like this one:

![emptybutton](https://github.com/realA10001986/Dash-Gauges/assets/76924199/53187b70-9399-44a4-bd78-090f055a3423)

The LED in those buttons is driven with 12V using a resistor; this resistor needs to be removed. No need to take out the blue part for this modification; just remove the red cover and pull out the LED; then desolder the LED and bridge the resistor.

Most Simpson meters have a drop-shaped pointer top which I was not able to remove (in fact, I didn't even try; I don't think the pointer top would have ended up properly straight); although I mounted the "Empty" light as high on the scale as possible, the pointer was still too long and collided with the light. My solution was to change the bends of the pointer where it leaves the driving mechanism more into an "S" shape, and I could thereby make it ever so short enough to pass the light. Another way would be to cut off the drop part, but that would make the pointer a tad too short in my opinion.

About the hole for the "Empty" light: Above button requires a 16mm hole. I started off drilling that hole into the face plate. Then I mounted the face plate into the meter and marked the hole on the plastic enclosure with a pencil. The next step was to remove the scale, wrap the mechanism in plastic to keep dust out, and drill a smaller hole (7mm) into the enclosure. Then I milled that hole into a bigger one, until the button fit. The plastic of the enclosure is quite brittle and breaks easily. Drilling a 16mm hole appeared too risky. Hence the drill/mill method.

### Enclosure

The enclosure consists of three parts: The front and two side pieces. Measurements are in the "enclosure" folder of this repository. The gauges are not marked completely in the drawing, but the distance is. The distance is important if you want to fit the Switch Board.

## Firmware Installation

There are different alternative ways to install this firmware:

1) If a previous version of the Dash Gauges firmware was installed on your device, you can upload the provided pre-compiled binary to update to the current version: Enter the [Config Portal](#the-config-portal), click on "Update" and select the pre-compiled binary file provided in this repository ("install/dashgauges-A10001986.ino.nodemcu-32s.bin").

If you, like me, use a TCD control board to drive your Dash Gauges, installation in this way is also possible if the TCD firmware are installed on your ESP32 before.

2) Using the Arduino IDE or PlatformIO: Download the sketch source code, all required libraries, compile and upload it. This method is the one for fresh ESP32 boards and/or folks familiar with the programming tool chain. Detailed build information is in [dashgauges-A10001986.ino](https://github.com/realA10001986/Dash-Gauges/blob/main/dashgauges-A10001986/dashgauges-A10001986.ino).

 *Important: After a firmware update, the "empty" LED might blink for up to a minute after reboot. Do NOT unplug the device during this time.*

### Audio file installation

The firmware comes with a number of sound files which need to be installed separately. These sound files are not updated as often as the firmware itself. If you have previously installed the latest version of the sound-pack, you normally don't have to re-install the audio files when you update the firmware. Only if either a new version of the sound-pack is released, or your device is quiet after a firmware update, a re-installation is needed.

- Download "install/sound-pack-xxxxxxxx.zip" and extract it to the root directory of of a FAT32 formatted SD card
- power down the Dash Gauges,
- insert this SD card into the device's slot and 
- power up the Dash Gauges.
 
If (and only if) the **exact and complete contents of sound-pack archive** is found on the SD card, the device will install the audio files (automatically).

After installation, the SD card can be re-used for [other purposes](#sd-card).

## Short summary of first steps

A good first step would be to establish access to the Config Portal in order to configure your Dash Gauges.

As long as the device is unconfigured, as is the case with a brand new one, or later if it for some reason fails to connect to a configured WiFi network, it starts in "access point" mode, i.e. it creates a WiFi network of its own named "DG-AP". This is called "Access point mode", or "AP-mode".

- Power up the device and wait until the startup sequence has completed.
- Connect your computer or handheld device to the WiFi network "DG-AP".
- Navigate your browser to http://gauges.local or http://192.168.4.1 to enter the Config Portal.
 
If you want your Dash Gauges to connect to another access point, such as your WiFi network, click on "Configure WiFi". The bare minimum is to select an SSID (WiFi network name) and a WiFi password.

Note that the device requests an IP address via DHCP, unless you entered valid data in the fields for static IP addresses (IP, gateway, netmask, DNS). 

After saving the WiFi network settings, the device reboots and tries to connect to your configured WiFi network. If that fails, it will again start in access point mode. 

If the device is inaccessible as a result of incorrect static IPs, 
- power-down the device,
- hold the Time Travel button,
- power-up the device (while still holding the Time Travel button)
- wait until the "Empty" LED flashes briefly,
- flip the Side Switch twice within 10 seconds,
- wait until the "Empty" LED lights up,
- then release the Time Travel button.

This procedure causes static IP data to be deleted; the device will return to DHCP after a reboot.

If you have your Dash Gauges, along with a Time Circuits Display, mounted in a car, see also [here](#car-setup).

### The Config Portal

The Config Portal is accessible exclusively through WiFi. As outlined above, if the device is not connected to a WiFi network, it creates its own WiFi network (named "DG-AP"), to which your WiFi-enabled hand held device or computer first needs to connect in order to access the Config Portal.

If the operating system on your handheld or computer supports Bonjour (a.k.a. "mDNS"), you can enter the Config Portal by directing your browser to http://gauges.local . (mDNS is supported on Windows 10 version TH2 (1511) [other sources say 1703] and later, Android 13 and later, MacOS, iOS)

If that fails, the way to enter the Config Portal depends on whether the device is in access point mode or not. 
- If it is in access point mode (and your handheld/computer is connected to the WiFi network "DG-AP"), navigate your browser to http://192.168.4.1 
- Otherwise .... FIXME TODO....  and listen, the IP address will be spoken out loud.

In the main menu, click on "Setup" to configure your Dash Gauges.
| ![The Config Portal](https://github.com/realA10001986/Dash-Gauges/assets/76924199/40fef271-bfd7-45b2-a7ba-9b00a7178409) |
|:--:| 
| *The Config Portal's Setup page* |

A full reference of the Config Portal is [here](#appendix-a-the-config-portal).

## Basic Operation

As mentioned, the Dash Gauges are an add-on for a Time Circuits Display. Their basic function is to show some values on its gauges, and to play an "empty" alarm after a time travel.

There is little to play with when the Dash Gauges aren't connected to a TCD:
- To quickly trigger the "empty" sequence, flip the side switch of your Dash Gauges. To "refill", flip that switch again.
- Press to the time travel button to trigger a simple "surge" sequence.

The Dash Gauges are way more fun when other props (TCD, FC, SID) are present as well. The TCD is of special importance: When connected through BTTFN, the TCD can act as a remote control for the Dash Gauges.

## TCD remote command reference

### Remote control reference

<table>
    <tr>
     <td align="center" colspan="2">Special sequences<br>(&#9166; = ENTER key)</td>
    </tr>
   <tr><td>Function</td><td>Code on TCD</td></tr>
    <tr>
     <td align="left">"Refill"</td>
     <td>009&#9166;</td>
    </tr>
   <tr>
     <td align="left">Set "full" percentage of "Primary" gauge</td>
     <td>9101&#9166; - 9199&#9166;</td>
    </tr>
    <tr>
     <td align="left">Reset "full" percentage of "Primary" gauge</td>
     <td align="left">9100&#9166;</td>
    </tr>
    <tr>
     <td align="left">Set "full" percentage of "Percent Power" gauge</td>
     <td align="left">9401&#9166; - 9499&#9166;</td>
    </tr>
    <tr>
     <td align="left">Reset "full" percentage of "Percent Power" gauge</td>
     <td align="left">9400&#9166;</td>
    </tr>
    <tr>
     <td align="left">Set "full" percentage of "Roentgens" gauge</td>
     <td align="left">9701&#9166; - 9799&#9166;</td>
    </tr>
    <tr>
     <td align="left">Reset "full" percentage of "Roentgens" gauge</td>
     <td align="left">9700&#9166;</td>
    </tr>
    <tr>
     <td align="left">Use volume knob (instead of fixed volume)</td>
     <td align="left">9399&#9166;</td>
    </tr>
    <tr>
     <td align="left">Set fixed volume level</td>
     <td align="left">9300&#9166; - 9319&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Play/Stop</td>
     <td align="left">9005&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Previous song</td>
     <td align="left">9002&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Next song</td>
     <td align="left">9008&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Select music folder</td>
     <td align="left">9050&#9166; - 9059&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Shuffle off</td>
     <td align="left">9222&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Shuffle on</td>
     <td align="left">9555&#9166;</td>
    </tr> 
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Go to song 0</td>
     <td align="left">9888&#9166;</td>
    </tr>
    <tr>
     <td align="left"><a href="#the-music-player">Music Player</a>: Go to song xxx</td>
     <td align="left">9888xxx&#9166;</td>
    </tr>
    <tr>
     <td align="left">Play "key3.mp3"</td>
     <td align="left">9003&#9166;</td>
    </tr>
    <tr>
     <td align="left">Play "key6.mp3"</td>
     <td align="left">9006&#9166;</td>
    </tr>
    <tr>
     <td align="left">Say current IP address</td>
     <td align="left">9090&#9166;</td>
    </tr>   
 <tr>
     <td align="left">Reboot the device</td>
     <td align="left">9064738&#9166;</td>
    </tr>
    <tr>
     <td align="left">Unlock "gauge type" selection in Config Portal</td>
     <td align="left">9317931&#9166;</td>
    </tr>
</table>

## Time travel

For "time travel", you need to install a "Time Travel" button. This button shortens "3_3V" and "IN" on the Switch Board's "TIME_TRAVEL" connector. Pressing that button briefly will let the Dash Gauges play their time travel sequence.

Other ways of triggering a time travel are available if a [Time Circuits Display](#connecting-a-time-circuits-display) is connected.

## SD card

Preface note on SD cards: For unknown reasons, some SD cards simply do not work with this device. For instance, I had no luck with Sandisk Ultra 32GB and  "Intenso" cards. If your SD card is not recognized, check if it is formatted in FAT32 format (not exFAT!). Also, the size must not exceed 32GB (as larger cards cannot be formatted with FAT32). I am currently using Transcend SDHC 4GB cards and those work fine.

The SD card, apart from being used to [install](#audio-file-installation) the default audio files, can be used for substituting default sounds and for music played back by the [Music player](#the-music-player).

Note that the SD card must be inserted before powering up the device. It is not recognized if inserted while the Dash Gauges are running. Furthermore, do not remove the SD card while the device is powered.

### Sound file substitution

The provided audio files ("sound-pack") are, after [proper installation](#audio-file-installation), integral part of the firmware and stored in the device's flash memory. 

These sounds can be substituted by your own sound files on a FAT32-formatted SD card. These files will be played back directly from the SD card during operation, so the SD card has to remain in the slot. The built-in [Audio file installer](#audio-file-installation) cannot be used to replace default sounds in the device's flash memory with custom sounds.

Your replacements need to be put in the root (top-most) directory of the SD card, be in mp3 format (128kbps max) and named as follows:
- "startup.mp3". Played when the Dash Gauges are connected to power and finished booting;
- "alarm.mp3". Played when the alarm sounds (triggered by a Time Circuits Display via BTTFN or MQTT);
- "0.mp3" through "9.mp3", "dot.mp3": Numbers for IP address read-out.
- "dooropen.mp3"/"doorclose.mp3": Played when the state of the door switch changes.

### Additional Custom Sounds

The firmware supports some additional user-provided sound effects, which it will load from the SD card. If the respective file is present, it will be used. If that file is absent, no sound will be played.

- "key3.mp3" and/or "key6.mp3": Will be played when you type 9003 / 9006 on the TCD (connected through BTTFN).

Those files are not provided here. You can use any mp3, with a bitrate of 128kpbs or less.

## The Music Player

The firmware contains a simple music player to play mp3 files located on the SD card. This player requires a TCD connected through BTTFN for control.

In order to be recognized, your mp3 files need to be organized in music folders named *music0* through *music9*. The folder number is 0 by default, ie the player starts searching for music in folder *music0*. This folder number can be changed using the remote control.

The names of the audio files must only consist of three-digit numbers, starting at 000.mp3, in consecutive order. No numbers should be left out. Each folder can hold up to 1000 files (000.mp3-999.mp3). *The maximum bitrate is 128kpbs.*

Since manually renaming mp3 files is somewhat cumbersome, the firmware can do this for you - provided you can live with the files being sorted in alphabetical order: Just copy your files with their original filenames to the music folder; upon boot or upon selecting a folder containing such files, they will be renamed following the 3-digit name scheme (as mentioned: in alphabetic order). You can also add files to a music folder later, they will be renamed properly; when you do so, delete the file "TCD_DONE.TXT" from the music folder on the SD card so that the firmware knows that something has changed. The renaming process can take a while (10 minutes for 1000 files in bad cases). Mac users are advised to delete the ._ files from the SD before putting it back into the control board as this speeds up the process.

To start and stop music playback, enter 9005 followed by ENTER on your TCD. Entering 9002 jumps to the previous song, 9008 to the next one.

By default, the songs are played in order, starting at 000.mp3, followed by 001.mp3 and so on. By entering 9555 on the TCD, you can switch to shuffle mode, in which the songs are played in random order. Type 9222 followed by ENTER to switch back to consecutive mode.

Entering 9888 followed by OK re-starts the player at song 000, and 9888xxx (xxx = three-digit number) jumps to song #xxx.

See [here](#ir-remote-reference) for a list of controls of the music player.

While the music player is playing music, other sound effects are disabled/muted. Initiating a time travel stops the music player. The TCD-triggered alarm will, if so configured, sound and stop the music player.

## Connecting a Time Circuits Display

### Connecting a TCD by wire

As mentioned, I am currently using a left-over TCD control board to run the Dash Gauges. The time travel function works through IO13 of that board.

<table>
    <tr>
     <td align="center">Dash Gauges:<br>"IO13" connector</td>
     <td align="center">TCD control board 1.2</td>
     <td align="center">TCD control board 1.3</td>
    </tr>
   <tr>
     <td align="center">GND of "IO13" connector</td>
     <td align="center">GND of "IO13" connector</td>
     <td align="center">GND on "Time Travel" connector</td>
    </tr>
    <tr>
     <td align="center">IO13 of "IO13" connector</td>
     <td align="center">IO13 of "IO13" connector</td>
     <td align="center">TT OUT on "Time Travel" connector</td>
    </tr>
</table>

Next, head to the Config Portal and set the option **_TCD connected by wire_**. On the TCD, the option "Control props connected by wire" must be set.

Note that a wired connection only allows for synchronized time travel sequences, no other communication takes place. Therefore I strongly recommend a wireless BTTFN connection, see immediately below.

### BTTF-Network ("BTTFN")

The TCD can communicate with the Dash Gauges wirelessly, via WiFi. It can send out information about a time travel and an alarm. Furthermore, the TCD's keypad can be used to remote-control the Dash Gauges.

Note that the TCD's firmware must be up to date for BTTFN. You can use [this](http://tcd.backtothefutu.re) one or CircuitSetup's release 2.9 or later.

![BTTFN connection](https://github.com/realA10001986/Dash-Gauges/assets/76924199/9342b658-b772-4106-9c57-56d135f6962b)

In order to connect your Dash Gauges to the TCD using BTTFN, just enter the TCD's IP address or hostname in the **_IP address or hostname of TCD_** field in the Dash Gauges's Config Portal. On the TCD, no special configuration is required. Note that you need TCD firmware 2.9.99 or later for using a hostname; previous versions only work with an IP address.
  
Afterwards, the Dash Gauges and the TCD can communicate wirelessly and 
- play time travel sequences in sync,
- both play an alarm-sequence when the TCD's alarm occurs,
- the Dash Gauges can be remote controlled through the TCD's keypad (command codes 9xxx),
- the Dash Gauges queries the TCD for fake power and night mode, in order to react accordingly if so configured.

You can use BTTF-Network and MQTT at the same time, see immediately below.

## Home Assistant / MQTT

The Dash Gauges support the MQTT protocol version 3.1.1 for the following features:

### Control the Dash Gauges via MQTT

The Dash Gauges can - to some extent - be controlled through messages sent to topic **bttf/dg/cmd**. Support commands are
- TIMETRAVEL: Start a [time travel](#time-travel)
- MP_PLAY: Starts the [Music Player](#the-music-player)
- MP_STOP: Stops the [Music Player](#the-music-player)
- MP_NEXT: Jump to next song
- MP_PREV: Jump to previous song
- MP_SHUFFLE_ON: Enables shuffle mode in [Music Player](#the-music-player)
- MP_SHUFFLE_OFF: Disables shuffle mode in [Music Player](#the-music-player)

### Receive commands from Time Circuits Display

If both TCD and Dash Gauges are connected to the same broker, and the option **_Send event notifications_** is checked on the TCD's side, the Dash Gauges will receive information on time travel and alarm and play their sequences in sync with the TCD. Unlike BTTFN, however, no other communication takes place.

![MQTT connection](https://github.com/realA10001986/Dash-Gauges/assets/76924199/f2a84eb4-ee50-44c4-9dc0-9a60345dbbaa)

MQTT and BTTFN can co-exist. However, the TCD only sends out time travel and alarm notifications through either MQTT or BTTFN, never both. If you have other MQTT-aware devices listening to the TCD's public topic (bttf/tcd/pub) in order to react to time travel or alarm messages, use MQTT (ie check **_Send event notifications_**). If only BTTFN-aware devices are to be used, uncheck this option to use BTTFN as it has less latency.

### Setup

In order to connect to a MQTT network, a "broker" (such as [mosquitto](https://mosquitto.org/), [EMQ X](https://www.emqx.io/), [Cassandana](https://github.com/mtsoleimani/cassandana), [RabbitMQ](https://www.rabbitmq.com/), [Ejjaberd](https://www.ejabberd.im/), [HiveMQ](https://www.hivemq.com/) to name a few) must be present in your network, and its address needs to be configured in the Config Portal. The broker can be specified either by domain or IP (IP preferred, spares us a DNS call). The default port is 1883. If a different port is to be used, append a ":" followed by the port number to the domain/IP, such as "192.168.1.5:1884". 

If your broker does not allow anonymous logins, a username and password can be specified.

Limitations: MQTT Protocol version 3.1.1; TLS/SSL not supported; ".local" domains (MDNS) not supported; server/broker must respond to PING (ICMP) echo requests. For proper operation with low latency, it is recommended that the broker is on your local network. 

## Car setup

If your Dash Gauges, along with a [Time Circuits Display](https://tcd.backtothefutu.re/), is mounted in a car, the following network configuration is recommended:

#### TCD

- Run your TCD in [*car mode*](https://tcd.backtothefutu.re/#car-mode);
- disable WiFi power-saving on the TCD by setting **_WiFi power save timer (AP-mode)_** to 0 (zero).

#### Dash Gauges

Enter the Config Portal on the Dash Gauges (as described above), click on *Setup* and
  - enter *192.168.4.1* into the field **_IP address or hostname of TCD_**
  - check the option **_Follow TCD fake power_** if you have a fake power switch for the TCD (like eg a TFC switch)
  - click on *Save*.

After the Dash Gauges have restarted, re-enter the Dash Gauges's Config Portal (while the TCD is powered and in *car mode*) and
  - click on *Configure WiFi*,
  - select the TCD's access point name in the list at the top or enter *TCD-AP* into the *SSID* field; if you password-protected your TCD's AP, enter this password in the *password* field. Leave all other fields empty,
  - click on *Save*.

Using this setup enables the Dash Gauges to receive notifications about time travel and alarm wirelessly, and to query the TCD for data. Also, the TCD keypad can be used to remote-control the Dash Gauges.

In order to access the Dash Gauges's Config Portal in your car, connect your hand held or computer to the TCD's WiFi access point ("TCD-AP"), and direct your browser to http://gauges.local ; if that does not work, go to the TCD's keypad menu, press ENTER until "BTTFN CLIENTS" is shown, hold ENTER, and look for the Dash Gauges's IP address there; then direct your browser to that IP by using the URL http://a.b.c.d (a-d being the IP address displayed on the TCD display).

## Flash Wear

Flash memory has a somewhat limited life-time. It can be written to only between 10.000 and 100.000 times before becoming unreliable. The firmware writes to the internal flash memory when saving settings and other data. Every time you change settings, data is written to flash memory.

In order to reduce the number of write operations and thereby prolong the life of your Dash Gauges, it is recommended to use a good-quality SD card and to check **_[Save secondary settings on SD](#-save-secondary-settings-on-sd)_** in the Config Portal; secondary settings (eg current volume) are then stored on the SD card (which also suffers from wear but is easy to replace). If you want to swap the SD card but preserve your secondary settings, go to the Config Portal while the old SD card is still in place, uncheck the **_Save secondary settings on SD_** option, click on Save and wait until the device has rebooted. You can then power down, swap the SD card and power-up again. Then go to the Config Portal, change the option back on and click on Save. Your settings are now on the new SD card.

## Appendix A: The Config Portal

### Main page

##### &#9654; Configure WiFi

Clicking this leads to the WiFi configuration page. On that page, you can connect your Dash Gauges to your WiFi network by selecting/entering the SSID (WiFi network name) as well as a password (WPA2). By default, the Dash Gauges requests an IP address via DHCP. However, you can also configure a static IP by entering the IP, netmask, gateway and DNS server. All four fields must be filled for a valid static IP configuration. If you want to stick to DHCP, leave those four fields empty.

Note that this page has nothing to do with Access Point mode; it is strictly for connecting your Dash Gauges to an existing WiFi network as a client.

##### &#9654; Setup

This leads to the [Setup page](#setup-page).

##### &#9654; Restart

This reboots the Dash Gauges. No confirmation dialog is displayed.

##### &#9654; Update

This leads to the firmware update page. You can select a locally stored firmware image file to upload (such as the ones published here in the install/ folder).

##### &#9654; Erase WiFi Config

Clicking this (and saying "yes" in the confirmation dialog) erases the WiFi configuration (WiFi network and password) and reboots the device; it will restart in "access point" mode. See [here](#short-summary-of-first-steps).

---

### Setup page

#### Basic settings

##### &#9654; Auto-refill timer

After a time travel, the plutonium is depleted, and the chamber needs to be refilled. This timer allows for an automatic "Refill" after the given number of seconds; 0 means never. In the latter case, a manual Refill is in order: Either flip the side switch, or enter "009" on the TCD (if connected wirelessly).

##### &#9654; Mute 'empty' alarm timer

The "empty" alarm's sound can be annoying if played for longer periods. This timer allows to mute it after the given number of seconds. 0 means never.

##### &#9654; Screen saver timer

Enter the number of minutes until the Screen Saver should become active when the Dash Gauges are idle.

The Screen Saver, when active, stops the "empty" alarm sound and disables all LEDs, until 
- the time travel button is briefly pressed (the first press when the screen saver is active will not trigger a time travel),
- on a connected TCD, a destination date is entered (only if TCD is wirelessly connected) or a time travel event is triggered (also when wired).

The music player will continue to run.
 
#### Hardware configuration settings

##### Use fixed volume

Your control board might have a volume knob; if you want this knob to be your volume selector, uncheck this option. If you want to control volume remotely via the TCD keypad, check this, and select a volume level below. 

Due to hardware restraints, the volume knob might cause distortions when the level is very low; a fixed value is a better choice in that case.

##### Fixed volume level (0-19)

If the option _Use fixed volume_ above is checked, enter a value between 0 (mute) or 19 (very loud) here. This is your starting point; you can change the volume via TCD (93xx) and that new volume will also be saved (and appear in this field when the page is reloaded in your browser).

#### Network settings

##### &#9654; Hostname

The device's hostname in the WiFi network. Defaults to 'gauges'. This also is the domain name at which the Config Portal is accessible from a browser in the same local network. The URL of the Config Portal then is http://<i>hostname</i>.local (the default is http://gauges.local)

If you have several Dash Gauges in your local network, please give them unique hostnames.

##### &#9654; AP Mode: Network name appendix

By default, if the Dash Gauges create a WiFi network of its own ("AP-mode"), this network is named "DG-AP". In case you have multiple Dash Gauges in your vicinity, you can have a string appended to create a unique network name. If you, for instance, enter "-ABC" here, the WiFi network name will be "DG-AP-ABC". Characters A-Z, a-z, 0-9 and - are allowed.

##### &#9654; AP Mode: WiFi password

By default, and if this field is empty, the Dash Gauges's own WiFi network ("AP-mode") will be unprotected. If you want to protect your access point, enter your password here. It needs to be 8 characters in length and only characters A-Z, a-z, 0-9 and - are allowed.

If you forget this password and are thereby locked out of your Dash Gauges, .. TODO FIXME ...; this deletes the WiFi password. Then power-down and power-up your Dash Gauges and the access point will start unprotected.

##### &#9654; WiFi connection attempts

Number of times the firmware tries to reconnect to a WiFi network, before falling back to AP-mode. See [here](#short-summary-of-first-steps)

##### &#9654; WiFi connection timeout

Number of seconds before a timeout occurs when connecting to a WiFi network. When a timeout happens, another attempt is made (see immediately above), and if all attempts fail, the device falls back to AP-mode. See [here](#short-summary-of-first-steps)

#### Settings for prop communication/synchronization

##### &#9654; TCD connected by wire

Check this if you have a Time Circuits Display connected by wire. You can only connect *either* a button *or* the TCD to the "time travel" connector on the Dash Gauges, but not both.

Note that a wired connection only allows for synchronized time travel sequences, no other communication takes place.

Do NOT check this option if your TCD is connected wirelessly (BTTFN, MQTT).

##### &#9654; TCD signals Time Travel without 5s lead

Usually, the TCD signals a time travel with a 5 seconds lead, in order to give a prop a chance to play an acceletation sequence before the actual time travel takes place. Since this 5 second lead is unique to CircuitSetup props, and people sometimes want to connect third party props to the TCD, the TCD has the option of skipping this 5 seconds lead. That that is the case, and your Dash Gauges are connected by wire, you need to set this option.

If your Dash Gauges are connected wirelessly, this option has no effect.

##### &#9654; IP address or hostname of TCD

If you want to have your Dash Gauges to communicate with a Time Circuits Display wirelessly ("BTTF-Network"), enter the IP address of the TCD here. If your TCD is running firmware version 2.9.99 or later, you can also enter the TCD's hostname here instead (eg. 'timecircuits').

If you connect your Dash Gauges to the TCD's access point ("TCD-AP"), the TCD's IP address is 192.168.4.1.

##### &#9654; Follow TCD night-mode

If this option is checked, and your TCD goes into night mode, the Dash Gauges will activate the Screen Saver with a very short timeout, and reduce its audio volume.

##### &#9654; Follow TCD fake power

If this option is checked, and your TCD is equipped with a fake power switch, the Dash Gauges will also fake-power up/down. If fake power is off, no LED is active and the Dash Gauges will ignore all input.

#### Audio-visual options

##### &#9654; 'Primary' full percentage

Here you can select the readiing the "Primary" meter should give when "full". You can enter a value between 0 and 100 here. 0 will reset the "full" percentage to a default; 1-100 select a specific percentage. Values below 10 don't really make sense, though.

The "full" percentage can be changed through the TCD keypad (91xx for the "Primary" gauge, 93xx for the "Pecent Power" one, and 97xx for the "Roentgens"). 9x00 resets the "full" position the a default value. Note that changing the "full" percentage through the TCD keypad is not persistent. The boot-up value are only set through the Config Portal.

##### &#9654; 'Primary' empty percentage

This allows to select the pointer position when the meter is supposed to show "empty". This should be 0 (zero), but if your hardware is either inaccurate or the pointer isn't exactly 0-adjusted, you can modify its "zero" position here. Values from 0-100 are allowed, but obviously only values < 20 make sense.

##### &#9654; 'Percent Power' full percentage

Same as [this](#-primary-full-percentage), but for the 'Percent Power' gauge

##### &#9654; 'Percent Power' empty percentage

Same as [this](#-primary-empty-percentage), but for the 'Percent Power' gauge

##### &#9654; 'Roentgens' full percentage

Same as [this](#-primary-full-percentage), but for the 'Roentgens' gauge

##### &#9654; 'Roentgens' empty percentage

Same as [this](#-primary-empty-percentage), but for the 'Roentgens' gauge

##### &#9654; Play TCD-alarm sounds

If a TCD is connected via BTTFN or MQTT, the Dash Gauges visually signals when the TCD's alarm sounds. If you want to play an alarm sound, check this option.

##### &#9654; Play door sounds

The Switch Board has a connector for door switches; these switches change state whenever a door is opened or closed. The firmware can play a sound for each such event. To enable door sounds, check this.

##### &#9654; Switch closes when door is closed

This selects what type of door switch is being used. Check this, if the switch closes contact when the door closes. Leave uncheck if the switch opens when the door closes.

##### &#9654; Door sound delay

Depending on the position of the switch and its reaction point, a delay for sound playback might be desired. You can configure such a delay here. Enter the number of milliseconds into the text field; 0 means no delay. The maximum is 5000ms (=5 seconds).

#### Home Assistant / MQTT settings

##### &#9654; Use Home Assistant (MQTT 3.1.1)

If checked, the Dash Gauges will connect to the broker (if configured) and send and receive messages via [MQTT](#home-assistant--mqtt)

##### &#9654; Broker IP[:port] or domain[:port]

The broker server address. Can be a domain (eg. "myhome.me") or an IP address (eg "192.168.1.5"). The default port is 1883. If different port is to be used, it can be specified after the domain/IP and a colon ":", for example: "192.168.1.5:1884". Specifiying the IP address is preferred over a domain since the DNS call adds to the network overhead. Note that ".local" (MDNS) domains are not supported.

##### &#9654; User[:Password]

The username (and optionally the password) to be used when connecting to the broker. Can be left empty if the broker accepts anonymous logins.

#### Music Player settings

##### &#9654; Shuffle at startup

When checked, songs are shuffled when the device is booted. When unchecked, songs will be played in order.

#### Other settings

##### &#9654; Save secondary settings on SD

If this is checked, some settings (volume, etc) are stored on the SD card (if one is present). This helps to minimize write operations to the internal flash memory and to prolong the lifetime of your Dash Gauges. See [Flash Wear](#flash-wear).

#### Gauge Hardware settings

##### &#9654; Gauges hardware type

This selects the type of gauge hardware and the way of connection. In order to protect your props, this is locked by default. To unlock this setting

- either hold the Time Travel button for 5 seconds, or
- enter 9317931 on a wirelessly connected TCD,

then reload the page in your browser.

## Appendix B: "Empty" LED signals

TODO














