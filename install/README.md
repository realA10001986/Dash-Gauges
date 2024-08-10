This folder holds all files necessary for immediate installation on your Time Circuits Display. Here you'll find
- a binary of the current firmware, ready for upload to the device;
- the latest audio data

## Firmware Installation

If a previous version of the Dash Gauges firmware is installed on your device's ESP32, you can update easily using the pre-compiled binary. Enter the [Config Portal](#the-config-portal), click on "Update" and select the pre-compiled binary file provided in this repository ([install/dashgauges-A10001986.ino.nodemcu-32s.bin](https://github.com/realA10001986/Dash-Gauges/blob/main/install/dashgauges-A10001986.ino.nodemcu-32s.bin)).

If you are using a fresh ESP32 board, please see [dashgauges-A10001986.ino](https://github.com/realA10001986/Dash-Gauges/blob/main/dashgauges-A10001986/dashgauges-A10001986.ino) for detailed build and upload information, or, if you don't want to deal with source code, compilers and all that nerd stuff, go [here](https://install.out-a-ti.me) and follow the instructions.

 *Important: After a firmware update, the "empty" LED might blink for short while after reboot. Do NOT unplug the device during this time.*

## Audio data installation

The audio data is not updated as often as the firmware itself. If you have previously installed the latest version of the sound-pack, you normally don't have to re-install the audio data when you update the firmware. Only if either a new version of the sound-pack is released, or your device is quiet after a firmware update, a re-installation is needed.

The first step is to download "install/sound-pack-xxxxxxxx.zip" and extract it. It contains one file named "DGA.bin".

Then there are two alternative ways to proceed. Note that both methods *require an SD card*.

1) Through the [Config Portal](#the-config-portal). Click on *Update*, select the "DGA.bin" file in the bottom file selector and click on *Upload*. Note that an SD card must be present during this operation.

2) Via SD card:
- Copy "DGA.bin" to the root directory of of a FAT32 formatted SD card;
- power down the Dash Gauges,
- insert this SD card into the slot and 
- power up the Dash Gauges; the audio data will be installed automatically.
