/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Global definitions
 */

#ifndef _DG_GLOBAL_H
#define _DG_GLOBAL_H

// Version strings
#define DG_VERSION       "V1.06"
#define DG_VERSION_EXTRA "AUG292024"

//#define DG_DBG              // debug output on Serial

/*************************************************************************
 ***                     mDNS (Bonjour) support                        ***
 *************************************************************************/

// Supply mDNS service 
// Allows accessing the Config Portal via http://hostname.local
// <hostname> is configurable in the Config Portal
// This needs to be commented if WiFiManager provides mDNS
#define DG_MDNS
// Uncomment this if WiFiManager has mDNS enabled
//#define DG_WM_HAS_MDNS 

/*************************************************************************
 ***             Configuration for peripherals/features                ***
 *************************************************************************/

// Uncomment for support of door switches/door sounds
// Comment if using DOOR_SWITCH_PIN for something else
#define DG_HAVEDOORSWITCH
// Comment if only one door switch is supported by hardware
#define DG_HAVEDOORSWITCH2

// Uncomment for HomeAssistant MQTT protocol support
#define DG_HAVEMQTT

// Uncomment if hardware has a volume knob
//#define DG_HAVEVOLKNOB

// Version of Control Board
#define CB_VERSION 4

// --- end of config options

/*************************************************************************
 ***                           Miscellaneous                           ***
 *************************************************************************/

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
//#define USE_SPIFFS

// External time travel lead time, as defined by TCD firmware
// If DG are connected by wire, and the option "Signal Time Travel without 5s 
// lead" is set on the TCD, the DG option "TCD signals without lead" must
// be set, too.
#define ETTO_LEAD 5000

// Uncomment to include BTTFN discover support (multicast)
#define BTTFN_MC

/*************************************************************************
 ***                  esp32-arduino version detection                  ***
 *************************************************************************/

#if defined __has_include && __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#ifdef ESP_ARDUINO_VERSION_MAJOR
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2,0,8)
    #define HAVE_GETNEXTFILENAME
    #endif
#endif
#endif

/*************************************************************************
 ***                            Sanitation                             ***
 *************************************************************************/

 #ifdef DG_HAVEVOLKNOB
 #undef DG_HAVEDOORSWITCH2
 #endif

/*************************************************************************
 ***                             GPIO pins                             ***
 *************************************************************************/

#define STATUS_LED_PIN     2  // Status LED (on ESP)

#define DIGITAL_GAUGE_PIN 12  // [OUT] "Digital" gauges (all three)                 

#define TT_IN_PIN         13  // [IN]  Time Travel button (or TCD tt trigger input) (has internal PU/PD; PD on CB)
#define BUTTON1_PIN       36  // [IN]  Button 1                                     (has no internal PU; PD on CB)

#define SIDESWITCH_PIN    16  // [IN]  SBv1/CBv1: Toggle switch on side             (has no internal PU; PU on CB)
#if CB_VERSION < 4
#define DOOR_SWITCH_PIN   27  // [IN]  SBv1/CBv1.03: Door switch for door 1         (has internal PU)
#else
#define DOOR_SWITCH_PIN   14  // [IN]  SBv1/CBv1.04: Door switch for door 1         (has internal PU/PD)
#endif
#define DOOR2_SWITCH_PIN  32  // [IN]  CBv1.03+: Door switch for door 2             (has no internal PU?; PU on CB)
#if CB_VERSION < 4
#define BACKLIGHTS_PIN    14  // [OUT] SBv1/CBv1.03: Gauges' backlights
#else
#define BACKLIGHTS_PIN    27  // [OUT] SBv1/CBv1.04: Gauges' backlights
#endif

#define EMPTY_LED_PIN     17  // [OUT] SBv1/CBv1: "Empty" LED
#define EMPTY_LED_PIN2    14  // [OUT] SBv2:      "Empty" LED

// I2S audio pins
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18 

// Analog input for volume (unused on A10001986 Control Boards)
#define VOLUME_PIN        32

/* 
 *  Switchboard v2: Bits on PCA8574 port expander
 */
#define SIDESWITCH_BIT     7    // input    Toggle switch on side
#define DOORSWITCH_BIT     0    // input    Door switch
#define BACKLIGHTS_BIT     6    // output   Gauges' backlights (via relay)

#define PX_READ_MASK       ((1<<SIDESWITCH_BIT)|(1<<DOORSWITCH_BIT))
#define PX_WRITE_MASK      ((1<<BACKLIGHTS_BIT))

#endif
