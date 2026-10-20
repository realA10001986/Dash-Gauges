/**
 * wm_local.h - DG
 *
 */

#ifndef wm_local_h
#define wm_local_h

#define DEVNAME "Dash Gauges"
#define WEBFORMID "G"

#define WM_MDNS

#define WM_PARAM2

#ifdef WM_PARAM2
#define WM_PARAM2_CAPTION "HA/MQTT Settings"
#define WM_PARAM2_TITLE "HA/MQTT Settings"
#endif

#define WM_UPLOAD

#define WM_CCM

#define WM_FWPROT "GAUGES"
#define WM_FWPROTSP "DGA"

//#define _A10001986_DBG
//#define _A10001986_V_DBG

// #define WM_AP_STATIC_IP
// #define WM_APCALLBACK
// #define WM_PRECONNECTCB
// #define WM_PRESAVECB
// #define WM_EVENTCB
// #define WM_ADDLGETTERS
// #define WM_ADDLSETTERS

#endif