/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Global definitions
 */

#ifndef _DG_GLOBAL_H
#define _DG_GLOBAL_H

/*************************************************************************
 ***                          Version Strings                          ***
 *************************************************************************/

#define DG_VERSION       "V1.30"
#define DG_VERSION_EXTRA "FEB162026"

/*************************************************************************
 ***             Configuration for peripherals/features                ***
 *************************************************************************/

// Uncomment for support of door switches/door sounds
// Comment if using DOORx_SWITCH_PINs for something else
#define DG_HAVEDOORSWITCH

// Uncomment for HomeAssistant MQTT protocol support
#define DG_HAVEMQTT

// Version of Control Board
#define CB_VERSION 4

/*************************************************************************
 ***                           Miscellaneous                           ***
 *************************************************************************/

// External time travel lead time, as defined by TCD firmware
// If DG are connected by wire, and the option "Signal Time Travel without 
// 5s lead" is set on the TCD, the DG option "TCD signals without lead" must
// be set, too.
#define ETTO_LEAD 5000

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
//#define USE_SPIFFS

/*************************************************************************
 ***                               Debug                               ***
 *************************************************************************/

//#define DG_DBG              // Generic except below
//#define DG_DBG_NET          // Prop network related

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


/*************************************************************************
 ***                             GPIO pins                             ***
 *************************************************************************/

#define STATUS_LED_PIN     2  // Status LED (on ESP)

#define DIGITAL_GAUGE_PIN 12  // [OUT] "Digital" gauges (all three)                 

#define TT_IN_PIN         13  // [IN]  Time Travel button (or TCD tt trigger input) (has internal PU/PD; PD on CB)
#define BUTTON1_PIN       36  // [IN]  Button 1                                     (has no internal PU; PD on CB)

#define SIDESWITCH_PIN    16  // [IN]  CBv1: Toggle switch on side                  (has no internal PU; PU on CB)
#define DOOR_SWITCH_PIN   32  // [IN]  CBv1.03+: Door switch for door 1             (has no internal PU?; PU on CB)
#if CB_VERSION < 4
#define DOOR2_SWITCH_PIN  27  // [IN]  CBv1.03: Door switch for door 2              (has internal PU)
#define BACKLIGHTS_PIN    14  // [OUT] CBv1.03: Gauges' backlights
#else
#define DOOR2_SWITCH_PIN  14  // [IN]  CBv1.04: Door switch for door 2              (has internal PU/PD)
#define BACKLIGHTS_PIN    27  // [OUT] CBv1.04: Gauges' backlights
#endif

#define EMPTY_LED_PIN     17  // [OUT] CBv1: "Empty" LED

// I2S audio pins
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18 

#endif
