/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.backtothefutu.re
 *
 * Global definitions
 */

#ifndef _DG_GLOBAL_H
#define _DG_GLOBAL_H

// Version strings.
#define DG_VERSION       "V0.12"
#define DG_VERSION_EXTRA "NOV012023"

#define DG_DBG              // debug output on Serial

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

// Number of gauge type configurations (including NONE)
#define GA_NUM_TYPES 4

// Uncomment for HomeAssistant MQTT protocol support
#define DG_HAVEMQTT

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
 ***                             GPIO pins                             ***
 *************************************************************************/

#define STATUS_LED_PIN     2  // Status LED (on ESP)

#define ALL_G_BIN_PIN     15  // Placeholder for "binary" gauge Panel (all gauges only full and empty at same time)

#define L_G_BIN_PIN       15  // Placeholder for "binary" gauge Panel (left gauge, only full and empty)
#define C_G_BIN_PIN       12  // Placeholder for "binary" gauge Panel (center gauge, only full and empty)
#define R_G_BIN_PIN        2  // Placeholder for "binary" gauge Panel (right gauge, only full and empty)

#define SIDESWITCH_PIN    16  // Switch on side
#define EMPTY_LED_PIN     17  // LED in "empty" button

#define TT_IN_PIN         13  // Time Travel button (or TCD tt trigger input)

#define BACKLIGHTS_PIN    14  // Gauges' backlights (via relay)

#define DOOR_SWITCH_PIN   27  // (optional) door switch

// I2S audio pins
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18 

// analog input for volume   
#define VOLUME_PIN        32

#endif
