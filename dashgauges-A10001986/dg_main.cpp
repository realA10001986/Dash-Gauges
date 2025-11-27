/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Main controller
 *
 * -------------------------------------------------------------------
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

#include "dg_global.h"

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include "dgdisplay.h"
#include "input.h"

#include "dg_main.h"
#include "dg_settings.h"
#include "dg_audio.h"
#include "dg_wifi.h"

unsigned long powerupMillis = 0;
#ifdef DG_DBG
unsigned long debugTimer = 0;
#endif

// Gauges

// The gauges object
Gauges gauges = Gauges();

static uint8_t left_gauge_idle = 30;
static uint8_t center_gauge_idle = 30;
static uint8_t right_gauge_idle = 60;

static uint8_t left_gauge_empty = 0;
static uint8_t center_gauge_empty = 0;
static uint8_t right_gauge_empty = 0;

// The emptyLED object
static EmptyLED emptyLED = EmptyLED(0);

// The side switch
static DGButton sideSwitch = DGButton(SIDESWITCH_PIN,
    #if CB_VERSION < 4
    false,    // Switch is active HIGH (CB <= 1.03)
    false     // Disable internal pull-up resistor
    #else
    true,     // Switch is active LOW (CB 1.04)
    true      // Enable internal pull-up resistor (doesn't have one though)
    #endif
);

static bool          isSSwitchPressed = false;
static bool          isSSwitchChange = false;
static unsigned long isSSwitchChangeNow = 0;
static int           sideSwitchStatus = -1;

// The tt button / TCD tt trigger
static DGButton TTKey = DGButton(TT_IN_PIN,
    false,    // Button is active HIGH
    false     // Disable internal pull-up resistor
);

#define TT_DEBOUNCE    50    // tt button debounce time in ms
#define TT_PRESS_TIME 200    // tt button will register a short press
#define TT_HOLD_TIME 5000    // time in ms holding the tt button will count as a long press
static bool isTTKeyPressed = false;
static bool isTTKeyHeld = false;

// "Button 1"
#define B1_DEBOUNCE    50    // button debounce time in ms
#define B1_PRESS_TIME 200    // button will register a short press
#define B1_HOLD_TIME 2000    // time in ms holding the button will count as a long press
static DGButton Button1 = DGButton(BUTTON1_PIN,
    false,    // Button is active HIGH
    false     // Disable internal pull-up resistor (non-existent on GPIO36 anyway)
);

static bool isB1Pressed = false;
static bool isB1Held = false;

// The door switch
#ifdef DG_HAVEDOORSWITCH
static DGButton doorSwitch = DGButton(DOOR_SWITCH_PIN,
    true,    // Switch is active LOW
    true     // Enable internal pull-up resistor
);
static bool          isDSwitchPressed = false;
static bool          isDSwitchChange = false;
static int           doorSwitchStatus = -1;
static unsigned long isDSwitchChangeNow = 0;
static bool          dsCloseOnClose = false;
#ifdef DG_HAVEDOORSWITCH2
static DGButton door2Switch = DGButton(DOOR2_SWITCH_PIN,
    true,    // Switch is active LOW
    true     // Enable internal pull-up resistor (does not have one, though)
);
static bool          isD2SwitchPressed = false;
static bool          isD2SwitchChange = false;
static int           door2SwitchStatus = -1;
static unsigned long isD2SwitchChangeNow = 0;
#endif
// Doorswitch sequence
bool                 dsPlay = false;
static unsigned long dsNow = 0;
static bool          dsTimer = false;
static unsigned long dsDelay = 0;
static unsigned long dsADelay = 0;
static bool          dsOpen = false;
static unsigned long lastDoorSoundNow = 0;
static int           lastDoorNum = 0;
#ifdef DG_HAVEDOORSWITCH2
static unsigned long d2sNow = 0;
static bool          d2sTimer = false;
static unsigned long d2sADelay = 0;
static bool          d2sOpen = false;
#endif
uint16_t             doPlayDoorSound = 0;
#endif

static unsigned long swInitNow = 0;

#define STARTUP_DELAY 2300
bool                 startup = false;
static unsigned long startupNow = 0;

#define REFILL_DELAY 1235
bool                 refill = false;
static unsigned long refillNow = 0;
bool                 refillWA = false;

static unsigned long autoRefill = 0;
static unsigned long autoMute = 0;

#define ALARM_DELAY 1000
bool                 startAlarm = false;
static unsigned long startAlarmNow = 0;

bool networkTimeTravel = false;
bool networkTCDTT      = false;
bool networkReentry    = false;
bool networkAbort      = false;
bool networkAlarm      = false;
uint16_t networkLead   = ETTO_LEAD;
uint16_t networkP1     = 6600;

static bool tcdIsBusy  = false;
bool        dgBusy     = false;

static int16_t gpsSpeed = -1;
static int16_t oldGpsSpeed = -1;
static bool    spdIsRotEnc = false;

static bool useNM = false;
static bool tcdNM = false;
bool        dgNM  = false;

static bool useFPO = false;
static bool tcdFPO = false;

static bool bttfnTT = true;

bool doPrepareTT = false;
bool doWakeup = false;

// Time travel status flags etc.
bool                 TTrunning = false;  // TT sequence is running
static bool          extTT = false;      // TT was triggered by TCD
static unsigned long TTstart = 0;
static unsigned long TTfUpdNow = 0;
static unsigned long P0duration = ETTO_LEAD;
static unsigned long P1_maxtimeout = 10000;
static bool          TTP0 = false;
static bool          TTP1 = false;
static bool          TTP2 = false;
static unsigned long TTfUpdLNow = 0;
static unsigned long TTfUpdCNow = 0; 
static unsigned long TTfUpdRNow = 0;
static unsigned long TTFIntL = 0;
static unsigned long TTFIntC = 0;
static unsigned long TTFIntR = 0;
static int           TTStepL = 1, TTStepC = 1, TTStepR = 1;
static bool          TTdrPri = true, TTdrPPo = true, TTdrRoe = true;
static bool          playTTsounds = true; // for stand-alone TT

// Durations of tt phases for stand-alone tt
#define TT_P1_TOTAL     8000
#define TT_P1_DELAY_P1  1400  
#define TT_P1_POINT88   1400
#define TT_P1_EXT       TT_P1_TOTAL - TT_P1_DELAY_P1
#define P0_DUR          TT_P1_POINT88  // acceleration phase (stand-alone)
#define P1_DUR_TCD      6600           // time tunnel phase (synced; overruled by TCD network commands) 
#define P1_DUR          TT_P1_EXT      // time tunnel phase (stand-alone)
#define P2_ALARM_DELAY  2000           // Delay for alarm after reentry (sync'd)
#define P2_ALARM_DLY_SA 2300           // Delay for alarm after reentry (stand-alone)

// Volume-factor for "travelstart" sounds
#define TT_SOUND_FACT 1.2f

bool         TCDconnected = false;
static bool  noETTOLead = false;

static bool          volchanged = false;
static unsigned long volchgnow = 0;

static unsigned long ssLastActivity = 0;
static unsigned long ssDelay = 0;
static unsigned long ssOrigDelay = 0;
static bool          ssActive = false;

static bool          nmOld = false;
static bool          fpoOld = false;
bool                 FPBUnitIsOn = true;

#define EMPTY_INTERVAL   (832/2)
bool                 emptyAlarm = false;
static unsigned long emptyAlarmNow = 0;

static bool          FPOffemptyAlarm = false;
static unsigned long FPOffemptyAlarmNow = 0;

static int           mprengauge = -1;
static bool          mprengaugetouched = false;

// BTTF network
#define BTTFN_VERSION              1
#define BTTFN_SUP_MC            0x80
#define BTTF_PACKET_SIZE          48
#define BTTF_DEFAULT_LOCAL_PORT 1338
#define BTTFN_POLL_INT          1100
#define BTTFN_NOT_PREPARE  1
#define BTTFN_NOT_TT       2
#define BTTFN_NOT_REENTRY  3
#define BTTFN_NOT_ABORT_TT 4
#define BTTFN_NOT_ALARM    5
#define BTTFN_NOT_REFILL   6
#define BTTFN_NOT_FLUX_CMD 7
#define BTTFN_NOT_SID_CMD  8
#define BTTFN_NOT_PCG_CMD  9
#define BTTFN_NOT_WAKEUP   10
#define BTTFN_NOT_AUX_CMD  11
#define BTTFN_NOT_VSR_CMD  12
#define BTTFN_NOT_SPD      15
#define BTTFN_NOT_BUSY     16
#define BTTFN_TYPE_ANY     0    // Any, unknown or no device
#define BTTFN_TYPE_FLUX    1    // Flux Capacitor
#define BTTFN_TYPE_SID     2    // SID
#define BTTFN_TYPE_PCG     3    // Dash Gauges
#define BTTFN_TYPE_VSR     4    // VSR
#define BTTFN_TYPE_AUX     5    // Aux (user custom device)
#define BTTFN_TYPE_REMOTE  6    // Futaba remote control
#define BTTFN_SSRC_NONE         0
#define BTTFN_SSRC_GPS          1
#define BTTFN_SSRC_ROTENC       2
#define BTTFN_SSRC_REM          3
#define BTTFN_SSRC_P0           4
#define BTTFN_SSRC_P1           5
#define BTTFN_SSRC_P2           6
static const uint8_t BTTFUDPHD[4] = { 'B', 'T', 'T', 'F' };
static bool          useBTTFN = false;
static WiFiUDP       bttfUDP;
static UDP*          dgUDP;
static WiFiUDP       bttfMcUDP;
static UDP*          dgMcUDP;
static byte          BTTFUDPBuf[BTTF_PACKET_SIZE];
static byte          BTTFUDPTBuf[BTTF_PACKET_SIZE];
static unsigned long BTTFNUpdateNow = 0;
static unsigned long BTFNTSAge = 0;
static unsigned long BTTFNTSRQAge = 0;
static bool          BTTFNPacketDue = false;
static bool          BTTFNWiFiUp = false;
static uint8_t       BTTFNfailCount = 0;
static uint32_t      BTTFUDPID = 0;
static unsigned long lastBTTFNpacket = 0;
static bool          BTTFNBootTO = false;
static bool          haveTCDIP = false;
static IPAddress     bttfnTcdIP;
static uint32_t      bttfnTCDSeqCnt = 0;
static uint8_t       bttfnReqStatus = 0x52; // Request capabilities, status, speed
static uint32_t      tcdHostNameHash = 0;
static byte          BTTFMCBuf[BTTF_PACKET_SIZE];
static IPAddress     bttfnMcIP(224, 0, 0, 224);

static int      iCmdIdx = 0;
static int      oCmdIdx = 0;
static uint32_t commandQueue[16] = { 0 };

#define GET32(a,b)          \
    (((a)[b])            |  \
    (((a)[(b)+1]) << 8)  |  \
    (((a)[(b)+2]) << 16) |  \
    (((a)[(b)+3]) << 24))
#define SET32(a,b,c)                        \
    (a)[b]       = ((uint32_t)(c)) & 0xff;  \
    ((a)[(b)+1]) = ((uint32_t)(c)) >> 8;    \
    ((a)[(b)+2]) = ((uint32_t)(c)) >> 16;   \
    ((a)[(b)+3]) = ((uint32_t)(c)) >> 24;  

// Forward declarations ------

static void timeTravel(bool TCDtriggered, uint16_t P0Dur, uint16_t P1Dur = 0);

static void execute_remote_command();
static void say_ip_address();

static void startEmptyAlarm();
static void stopEmptyAlarm();
static bool checkGauges();

static void gauge_lights_on();
static void gauge_lights_off();

static void blink_empty();
static void start_blink_empty();

static void sideSwitchLongPress();
static void sideSwitchLongPressStop();
#ifdef DG_HAVEDOORSWITCH
static void dsScan();
static void doorSwitchLongPress();
static void doorSwitchLongPressStop();
#ifdef DG_HAVEDOORSWITCH2
static void door2SwitchLongPress();
static void door2SwitchLongPressStop();
#endif
#endif
static void ttkeyScan();
static void TTKeyPressed();
static void TTKeyHeld();

static void Button1Scan();
static void Button1Pressed();
static void Button1Held();

static void ssStart();
static void ssEnd();
static void ssRestartTimer();

static void waitAudioDone();

static uint8_t restrict_gauge_idle(int val, int minimum, int maximum, uint8_t def);
static uint8_t restrict_gauge_empty(int val, int minimum, int maximum, uint8_t def);

static void write_port(uint8_t val);
static void set_port_pin(uint8_t bitnum);
static void clr_port_pin(uint8_t bitnum);

static void bttfn_setup();
static void bttfn_loop_quick();
static bool bttfn_checkmc();
static void BTTFNCheckPacket();
static bool BTTFNTriggerUpdate();
static void BTTFNPreparePacketTemplate();
static void BTTFNSendPacket();

static bool bttfn_trigger_tt();

// Boot

void main_boot()
{
    while(millis() - powerupMillis < 100) {
        delay(10);
    }
        
    // Some init
    
    // Setup pin for lights
    pinMode(BACKLIGHTS_PIN, OUTPUT);
    
    gauge_lights_off();
    
    // Set up "empty" LED
    emptyLED.begin(EMPTY_LED_PIN);

    // Init side switch
    sideSwitch.begin();
    sideSwitch.setTiming(50, 10, 50);
    sideSwitch.attachLongPressStart(sideSwitchLongPress);
    sideSwitch.attachLongPressStop(sideSwitchLongPressStop);
    sideSwitch_scan();

    // Init door switch
    #ifdef DG_HAVEDOORSWITCH
    doorSwitch.begin();
    doorSwitch.setTiming(50, 10, 50);
    doorSwitch.attachLongPressStart(doorSwitchLongPress);
    doorSwitch.attachLongPressStop(doorSwitchLongPressStop);
    #ifdef DG_HAVEDOORSWITCH2
    door2Switch.begin();
    door2Switch.setTiming(50, 10, 50);
    door2Switch.attachLongPressStart(door2SwitchLongPress);
    door2Switch.attachLongPressStop(door2SwitchLongPressStop);
    #endif
    doorSwitch_scan();
    #endif

    swInitNow = millis();
}

void main_boot2()
{
    int8_t gaugeIDA = 0, gaugeIDB = 0, gaugeIDC = 0;
    
    gaugeIDA = atoi(settings.gaugeIDA);
    gaugeIDB = atoi(settings.gaugeIDB);
    gaugeIDC = atoi(settings.gaugeIDC);
      
    gauges.begin(gaugeIDA, gaugeIDB, gaugeIDC);
    
    gauges.off();

    gauges.setValuePercent(0, 0);
    gauges.setValuePercent(1, 0);
    gauges.setValuePercent(2, 0);
}

void main_setup()
{
    int temp;
    unsigned long tempu;
    bool waitShown = false;
    
    Serial.println("Dash Gauges version " DG_VERSION " " DG_VERSION_EXTRA);

    if(gauges.supportVariablePercentage(0)) {
        left_gauge_idle = restrict_gauge_idle(atoi(settings.lIdle), 0, 100, DEF_L_GAUGE_IDLE);
        left_gauge_empty = restrict_gauge_empty(atoi(settings.lEmpty), 0, left_gauge_idle, DEF_L_GAUGE_EMPTY);
        if(left_gauge_empty >= left_gauge_idle) { left_gauge_idle = DEF_L_GAUGE_IDLE; left_gauge_empty = DEF_L_GAUGE_EMPTY; }
        TTdrPri = (atoi(settings.drPri) > 0);
    } else {
        left_gauge_idle = 100;
        left_gauge_empty = 0;
        TTdrPri = true;
    }

    if(gauges.supportVariablePercentage(1)) {
        center_gauge_idle = restrict_gauge_idle(atoi(settings.cIdle), 0, 100, DEF_C_GAUGE_IDLE);
        center_gauge_empty = restrict_gauge_empty(atoi(settings.cEmpty), 0, center_gauge_idle, DEF_C_GAUGE_EMPTY);
        if(center_gauge_empty >= center_gauge_idle) { center_gauge_idle = DEF_C_GAUGE_IDLE; center_gauge_empty = DEF_C_GAUGE_EMPTY; }
        TTdrPPo = (atoi(settings.drPPo) > 0);
    } else {
        center_gauge_idle = 100;
        center_gauge_empty = 0;
        TTdrPPo = true;
    }

    if(gauges.supportVariablePercentage(2)) {
        right_gauge_idle = restrict_gauge_idle(atoi(settings.rIdle), 0, 100, DEF_R_GAUGE_IDLE);
        right_gauge_empty = restrict_gauge_empty(atoi(settings.rEmpty), 0, right_gauge_idle, DEF_R_GAUGE_EMPTY);
        if(right_gauge_empty >= right_gauge_idle) { right_gauge_idle = DEF_R_GAUGE_IDLE; right_gauge_empty = DEF_R_GAUGE_EMPTY; }
        TTdrRoe = (atoi(settings.drRoe) > 0);
    } else {
        right_gauge_idle = 100;
        right_gauge_empty = 0;
        TTdrRoe = true;
    }

    // Thresholds for digital gauges
    gauges.setBinGaugeThreshold(0, atoi(settings.lThreshold));
    gauges.setBinGaugeThreshold(1, atoi(settings.cThreshold));
    gauges.setBinGaugeThreshold(2, atoi(settings.rThreshold));

    // Other options
    ssDelay = ssOrigDelay = atoi(settings.ssTimer) * 60 * 1000;    
    useNM = (atoi(settings.useNM) > 0);
    useFPO = (atoi(settings.useFPO) > 0);
    bttfnTT = (atoi(settings.bttfnTT) > 0);

    #ifdef DG_HAVEDOORSWITCH
    dsPlay = (atoi(settings.dsPlay) > 0);
    dsCloseOnClose = (atoi(settings.dsCOnC) > 0);
    temp = atoi(settings.dsDelay);
    if(temp < 0) temp = 0;
    else if(temp > 5000) temp = 5000;
    dsDelay = temp;
    #endif
    
    temp = atoi(settings.autoRefill);
    if(temp < 0) temp = 0;
    else if(temp > 360) temp = 360;
    autoRefill = temp * 1000;
    temp = atoi(settings.autoMute);
    if(temp < 0) temp = 0;
    else if(temp > 360) temp = 360;
    autoMute = temp * 1000;
    
    // [Formerly started CP here]

    // Determine if Time Circuits Display is connected
    // via wire, and is source of GPIO tt trigger
    TCDconnected = (atoi(settings.TCDpresent) > 0);
    noETTOLead = (atoi(settings.noETTOLead) > 0);

    // Set up TT button / TCD trigger
    TTKey.begin();
    TTKey.attachPress(TTKeyPressed);
    if(!TCDconnected) {
        // If we have a physical button, we need
        // reasonable values for debounce and press
        TTKey.setTiming(TT_DEBOUNCE, TT_PRESS_TIME, TT_HOLD_TIME);
        TTKey.attachLongPressStart(TTKeyHeld);
    } else {
        // If the TCD is connected, we can go more to the edge
        TTKey.setTiming(5, 50, 100000);
        // Long press ignored when TCD is connected
    }

    // Set up Button 1
    Button1.begin();
    Button1.attachPress(Button1Pressed);
    Button1.attachLongPressStart(Button1Held);
    Button1.setTiming(B1_DEBOUNCE, B1_PRESS_TIME, B1_HOLD_TIME);

    // Invoke audio file installer if SD content qualifies
    #ifdef DG_DBG
    Serial.println("Probing for audio files on SD");
    #endif
    if(check_allow_CPA()) {
        showWaitSequence();
        if(prepareCopyAudioFiles()) {
            play_file("/_installing.mp3", PA_ALLOWSD, 1.0);
            waitAudioDone();
        }
        doCopyAudioFiles();
        // We never return here. The ESP is rebooted.
    }

    if(!haveAudioFiles) {
        #ifdef DG_DBG
        Serial.println("Current audio data not installed");
        #endif
        emptyLED.specialSignal(DGSEQ_NOAUDIO);
        while(emptyLED.specialDone()) {
            mydelay(100);
        }
    }

    // Init music player (don't check for SD here)
    for(mprengauge = 2; mprengauge >= 0 ; mprengauge--) {
       if(gauges.supportVariablePercentage(mprengauge))
           break;
    }
    switchMusicFolder(musFolderNum, true);
        
    // Reset gauges to idle percentages
    gauges.setValuePercent(0, left_gauge_idle);
    gauges.setValuePercent(1, center_gauge_idle);
    gauges.setValuePercent(2, right_gauge_idle);
    

    // Initialize BTTF network
    bttfn_setup();
    bttfn_loop();

    // Init side and door switch position
    // Do not trigger event until NEXT change
    if((tempu = millis() - swInitNow) < 60) {
        delay(60 - tempu);
    }
    sideSwitch_scan();
    isSSwitchChange = false;
    #ifdef DG_HAVEDOORSWITCH
    doorSwitch_scan();
    isDSwitchChange = false;
    #ifdef DG_HAVEDOORSWITCH2
    isD2SwitchChange = false;
    #endif
    #endif

    #ifdef DG_DBG
    Serial.println("main_setup() done");
    #endif

    // If "Follow TCD fake power" is set,
    // stay silent and dark

    if(useBTTFN && useFPO && (WiFi.status() == WL_CONNECTED)) {

        FPBUnitIsOn = false;
        tcdFPO = fpoOld = true;

        Serial.println("Waiting for TCD fake power on");

    } else {

        // Otherwise boot:
        FPBUnitIsOn = true;
        
        // Play startup
        gauge_lights_on();
        startup = true;
        startupNow = millis();
        
        ssRestartTimer();

    }
}

void main_loop()
{
    unsigned long now = millis();

    // Execute scheduled digital gauge changes
    gauges.loop();

    // Scan door switch
    #ifdef DG_HAVEDOORSWITCH
    dsScan();
    #endif

    // Follow TCD fake power
    if(useFPO && (tcdFPO != fpoOld)) {
        if(tcdFPO) {

            // Power off:
            FPBUnitIsOn = false;

            // Back up "empty" alarm state
            FPOffemptyAlarm = emptyAlarm;
            FPOffemptyAlarmNow = emptyAlarmNow;

            TTrunning = false;
            startup = false;
            startAlarm = false;
            refill = false;
            refillWA = false;

            stopEmptyAlarm();
            gauges.off(); 
            gauge_lights_off();

            mp_stop();
            if(!playingDoor) {
                stopAudio();
                flushDelayedSave();
            }

            doPrepareTT = false;
            doWakeup = false;

            // FIXME - anything else?
            
        } else {

            // Power on: 
            FPBUnitIsOn = true;

            // Check if autoRefill timer ran out during our "off" period
            if(autoRefill && FPOffemptyAlarm && (millis() - FPOffemptyAlarmNow >= autoRefill)) {
                // If so, refill
                gauges.setValuePercent(0, left_gauge_idle);
                gauges.setValuePercent(1, center_gauge_idle);
                gauges.setValuePercent(2, right_gauge_idle);
                FPOffemptyAlarm = false;   
            }

            isTTKeyHeld = isTTKeyPressed = false;
            networkTimeTravel = false;
            isSSwitchChange = false;

            ssRestartTimer();
            ssActive = false;

            // Turn on lights & gauges (with delay)
            gauge_lights_on();            
            
            startup = true;
            startupNow = millis();
            
            // FIXME - anything else?
 
        }
        fpoOld = tcdFPO;
    }

    // Eval flags set in handle_tcd_notification
    if(doPrepareTT) {
        if(FPBUnitIsOn && !TTrunning) {
            prepareTT();
        }
        doPrepareTT = false;
    }
    if(doWakeup) {
        if(FPBUnitIsOn && !TTrunning) {
            wakeup();
        }
        doWakeup = false;
    }

    // Timers
    if(FPBUnitIsOn) {
        // Turn display on after startup delay
        if(startup && (millis() - startupNow >= STARTUP_DELAY)) {
            gauges.on();
            if(checkGauges()) {    // Check if empty, and trigger alarm if so
                play_file("/startup.mp3", PA_INTRMUS|PA_ALLOWSD, 1.0);
            }
            startup = false;
        }
        // Start alarm (after delay to give gauges time to go to 0)
        if(startAlarm && (millis() - startAlarmNow >= ALARM_DELAY)) {
            startEmptyAlarm();
            startAlarm = false;
        }
        // Initiate refill after audio has finished
        if(refillWA && (!playingEmpty || checkAudioDone())) {
            play_file("/refill.mp3", PA_INTRMUS|PA_ALLOWSD, 0.6);
            refillWA = false;
            if(!ssActive) {
                refill = true;
                refillNow = millis();
            }
        }
        // Update gauges after refill (sound-sync'd)
        if(refill && (millis() - refillNow >= REFILL_DELAY)) {
            gauges.UpdateAll();
            refill = false;
        }
        if(!TTrunning && !startup && !startAlarm && !refill && !refillWA) {
            if(autoRefill && emptyAlarm && (millis() - emptyAlarmNow >= autoRefill)) {
                refill_plutonium();
            }
            if(autoMute && playingEmpty && (millis() - emptyAlarmNow >= autoMute)) {
                stopAudioAtLoopEnd();
            }
        }
    }

    // Side switch handling
    if(FPBUnitIsOn && !TTrunning && !startup && !startAlarm && !refill && !refillWA) {
        if(isSSwitchChange) {       // don't care about isSSwitchPressed
            if(ssActive) {
                ssEnd();
            } else if(emptyAlarm) {
                refill_plutonium();
            } else {
                set_empty(); 
            }
            isSSwitchChange = false;
            ssRestartTimer();
        }
    }

    // Door switch/sound handling
    #ifdef DG_HAVEDOORSWITCH
    now = millis();
    if(dsTimer && now - dsNow >= dsADelay) {
        // delay door sound by max 500ms, otherwise effect is lost and we skip it
        if(now - dsNow <= dsADelay + 500) {
            play_door_open(1, dsOpen);
        }
        dsTimer = false;
    }
    #ifdef DG_HAVEDOORSWITCH2
    if(d2sTimer && now - d2sNow >= d2sADelay) {
        // delay door sound by max 500ms, otherwise effect is lost and we skip it
        if(now - d2sNow <= d2sADelay + 500) {
            play_door_open(2, d2sOpen);
        }
        d2sTimer = false;
    }
    #endif
    
    if(dsPlay && isDSwitchChange) {
        if(!refillWA) {
            unsigned long timePassed = millis() - isDSwitchChangeNow;
            dsOpen = (isDSwitchPressed != dsCloseOnClose);
            if(dsDelay && (dsDelay > timePassed + 100)) {
                dsTimer = true;
                dsADelay = dsDelay;
                dsNow = isDSwitchChangeNow;
            } else if(timePassed < 500) {
                play_door_open(1, dsOpen);
            }
        }
        isDSwitchChange = false;
    }
    #ifdef DG_HAVEDOORSWITCH2
    if(dsPlay && isD2SwitchChange) {
        if(!refillWA) {
            unsigned long timePassed = millis() - isD2SwitchChangeNow;
            d2sOpen = (isD2SwitchPressed != dsCloseOnClose);
            if(dsDelay && (dsDelay > timePassed + 100)) {
                d2sTimer = true;
                d2sADelay = dsDelay;
                d2sNow = isD2SwitchChangeNow;
            } else if(timePassed < 500) {
                play_door_open(2, d2sOpen);
            }
        }
        isD2SwitchChange = false;
    }
    #endif
    // Eval MQTT command
    if(!dsPlay && doPlayDoorSound) {
        play_door_open(1, !!(doPlayDoorSound & 0xff));
        doPlayDoorSound = 0;
    }
    #endif

    // Button 1 evaluation
    if(!TTrunning && !startup && !startAlarm && !refill && !refillWA) {
        Button1Scan();
        if(isB1Held) {
            // Say IP address
            isB1Held = isB1Pressed = false;
            flushDelayedSave();
            say_ip_address();
        } else if(isB1Pressed) {
            // WiFi re-connect / restart from Power Save
            bool wasActiveM = false, waitShown = false;
            isB1Pressed = false;
            if(wifiOnWillBlock()) {
                if(haveMusic && mpActive) {
                    mp_stop();
                    wasActiveM = true;
                }
                stopAudio();
                showWaitSequence();
                waitShown = true;
                flushDelayedSave();
            }
            // Enable WiFi / even if in AP mode / with CP
            wifiOn(0, true, false);
            if(waitShown) endWaitSequence();
            // Restart mp if active before
            if(wasActiveM) mp_play();
        }
    }
    
    // TT button evaluation
    if(FPBUnitIsOn && !TTrunning && !startup && !startAlarm && !refill && !refillWA) {
        ttkeyScan();
        if(isTTKeyHeld) {
            ssEnd();
            isTTKeyHeld = isTTKeyPressed = false;
            // TT button hold unlocks gaugeType in Config Portal
            if(gaugeTypeLocked) {
                gaugeTypeLocked = false;
                updateConfigPortalValues();
            }
            //if() {
                play_file("/buttonl.mp3", PA_ALLOWSD, 1.0);
            //}
        } else if(isTTKeyPressed) {
            isTTKeyPressed = false;
            if(!TCDconnected && ssActive) {
                // First button press when ss is active only deactivates ss
                ssEnd();
            } else {
                if(TCDconnected) {
                    ssEnd();
                }
                if(TCDconnected || !bttfnTT || !bttfn_trigger_tt()) {
                    // stand-alone TT with P0_DUR lead, not ETTO_LEAD;
                    // if no sound to be played, even 0.
                    timeTravel(TCDconnected, (TCDconnected && noETTOLead) ? 
                                              0 : (TCDconnected ? ETTO_LEAD : 
                                                    (playTTsounds ? P0_DUR : 0)));
                }
            }
        }
    
        // Check for BTTFN/MQTT-induced TT
        if(networkTimeTravel) {
            networkTimeTravel = false;
            ssEnd();
            timeTravel(networkTCDTT, networkLead, networkP1);
        }
    }

    now = millis();

    // The time travel sequences
    
    if(TTrunning) {

        if(extTT) {

            // ***********************************************************************************
            // TT triggered by TCD (BTTFN, GPIO or MQTT) *****************************************
            // ***********************************************************************************

            if(TTP0) {   // Acceleration - runs for ETTO_LEAD ms by default

                if(!networkAbort && (now - TTstart < P0duration)) {

                    // Nothing.
                             
                } else if(networkAbort) {

                    // If we are aborted in P0,
                    // the entire TT is over for us.
                    TTP0 = false;
                    TTrunning = false;
                    isTTKeyPressed = false;
                    ssRestartTimer();

                } else {

                    TTP0 = false;
                    TTP1 = true;
                    TTstart = TTfUpdLNow = TTfUpdCNow = TTfUpdRNow = now;

                }
            }
            if(TTP1) {   // Peak/"time tunnel" - ends with pin going LOW or BTTFN/MQTT "REENTRY" (or a long timeout)

                if(((networkTCDTT && (!networkReentry && !networkAbort)) || 
                    (!networkTCDTT && digitalRead(TT_IN_PIN)))               &&
                    (millis() - TTstart < P1_maxtimeout) ) {

                    bool doUpd = false;
                    
                    if(TTdrPri && TTFIntL && (now - TTfUpdLNow >= TTFIntL)) {
                        int t = gauges.getValuePercent(0);
                        if(t >= left_gauge_empty + TTStepL) t -= TTStepL;
                        else t = left_gauge_empty;
                        gauges.setValuePercent(0, t);
                        TTfUpdLNow = now;
                        doUpd = true;
                    }
                    if(TTdrPPo && TTFIntC && (now - TTfUpdCNow >= TTFIntC)) {
                        int t = gauges.getValuePercent(1);
                        if(t >= center_gauge_empty + TTStepC) t -= TTStepC;
                        else t = center_gauge_empty;
                        gauges.setValuePercent(1, t);
                        TTfUpdCNow = now;
                        doUpd = true;
                    }
                    if(TTdrRoe && TTFIntR && (now - TTfUpdRNow >= TTFIntR)) {
                        int t = gauges.getValuePercent(2);
                        if(t >= right_gauge_empty + TTStepR) t -= TTStepR;
                        else t = right_gauge_empty;
                        gauges.setValuePercent(2, t);
                        TTfUpdRNow = now;
                        doUpd = true;
                    }
                    if(doUpd) {
                        gauges.UpdateAll();
                    }
                    
                } else {

                    TTP1 = false;
                    TTP2 = true;

                    // P1 ends with empty; even if we were aborted in P1.
                    // There is no "half empty".
                    gauges.setValuePercent(0, left_gauge_empty);
                    gauges.setValuePercent(1, center_gauge_empty);
                    gauges.setValuePercent(2, right_gauge_empty);
                    gauges.UpdateAll();
                    
                    TTstart = now;
                    
                }
            }
            if(TTP2) {   // Reentry - up to us

                if(networkAbort || (now - TTstart > P2_ALARM_DELAY)) {

                    // At very end:
                    checkGauges();
                    TTP2 = false;
                    TTrunning = false;
                    isTTKeyPressed = false;
                    ssRestartTimer();

                }
            }
          
        } else {

            // ****************************************************************************
            // TT triggered by button (if TCD not connected) or MQTT **********************
            // ****************************************************************************
          
            if(TTP0) {   // Acceleration - runs for P0_DUR ms

                if(now - TTstart < P0_DUR) {

                    // Nothing.
                             
                } else {

                    TTP0 = false;
                    TTP1 = true;
                    TTstart = TTfUpdLNow = TTfUpdCNow = TTfUpdRNow = now;
                    
                }
            }
            
            if(TTP1) {   // Peak/"time tunnel" - runs for P1_DUR ms

                if(now - TTstart < P1_DUR) {

                    bool doUpd = false;
                    
                    if(TTdrPri && TTFIntL && (now - TTfUpdLNow >= TTFIntL)) {
                        int t = gauges.getValuePercent(0);
                        if(t >= left_gauge_empty + TTStepL) t -= TTStepL;
                        else t = left_gauge_empty;
                        gauges.setValuePercent(0, t);
                        TTfUpdLNow = now;
                        doUpd = true;
                    }
                    if(TTdrPPo && TTFIntC && (now - TTfUpdCNow >= TTFIntC)) {
                        int t = gauges.getValuePercent(1);
                        if(t >= center_gauge_empty + TTStepC) t -= TTStepC;
                        else t = center_gauge_empty;
                        gauges.setValuePercent(1, t);
                        TTfUpdCNow = now;
                        doUpd = true;
                    }
                    if(TTdrRoe && TTFIntR && (now - TTfUpdRNow >= TTFIntR)) {
                        int t = gauges.getValuePercent(2);
                        if(t >= right_gauge_empty + TTStepR) t -= TTStepR;
                        else t = right_gauge_empty;
                        gauges.setValuePercent(2, t);
                        TTfUpdRNow = now;
                        doUpd = true;
                    }
                    if(doUpd) {
                        gauges.UpdateAll();
                    }
                    
                } else {

                    if(playTTsounds) {
                        play_file("/timetravel.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
                    }

                    TTP1 = false;
                    TTP2 = true;
                    gauges.setValuePercent(0, left_gauge_empty);
                    gauges.setValuePercent(1, center_gauge_empty);
                    gauges.setValuePercent(2, right_gauge_empty);
                    gauges.UpdateAll();
                    TTstart = now;
                    
                }
            }
            
            if(TTP2) {   // Reentry - up to us

                if(now - TTstart > P2_ALARM_DLY_SA) {

                    // At very end:
                    checkGauges();
                    TTP2 = false;
                    TTrunning = false;
                    isTTKeyPressed = false;
                    ssRestartTimer();

                }
            }
          
        }
    }

    // Follow TCD night mode
    if(useNM && (tcdNM != nmOld)) {
        if(tcdNM) {
            // NM on: Set Screen Saver timeout to 10 seconds
            ssDelay = 10 * 1000;
            dgNM = true;
        } else {
            // NM off: End Screen Saver; reset timeout to old value
            ssEnd();  // Doesn't do anything if fake power is off
            ssDelay = ssOrigDelay;
            dgNM = false;
        }
        nmOld = tcdNM;
    }

    // Execute remote commands from TCD or MQTT
    if(FPBUnitIsOn) {
        execute_remote_command();
    }

    // Wake up on RotEnc/Remote speed changes; on GPS only if old speed was <=0
    if(gpsSpeed != oldGpsSpeed) {
        if(FPBUnitIsOn && !TTrunning && (spdIsRotEnc || oldGpsSpeed <= 0) && gpsSpeed >= 0) {
            wakeup();
        }
        oldGpsSpeed = gpsSpeed;
    }
    
    // "Screen saver"
    if(FPBUnitIsOn) {
        if(!ssActive && !TTrunning && !startup && !startAlarm && !refill && !refillWA && 
           ssDelay && (millis() - ssLastActivity > ssDelay)) {
            ssStart();
        }
    }

    now = millis();

    // If network is interrupted, return to stand-alone
    if(useBTTFN) {
        if( (lastBTTFNpacket && (now - lastBTTFNpacket > 30*1000)) ||
            (!BTTFNBootTO && !lastBTTFNpacket && (now - powerupMillis > 60*1000)) ) {
            tcdNM = false;
            tcdFPO = false;
            gpsSpeed = -1;
            lastBTTFNpacket = 0;
            BTTFNBootTO = true;
        }
    }

    if(networkAlarm && !TTrunning && !startup && !startAlarm && !refill && !refillWA) {
        networkAlarm = false;
        if(atoi(settings.playALsnd) > 0) {
            play_file("/alarm.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
        }
        // No special handling for !FPBUnitIsOn needed, when
        // special signal ends, it restores the old state
        emptyLED.specialSignal(DGSEQ_ALARM);
    }

    if(!TTrunning && !startup && !startAlarm && !refill && !refillWA) {
        // Save volume 10 seconds after last change
        if(volchanged && (now - volchgnow > 10000)) {
            volchanged = false;
            saveCurVolume();
        }
    }

    #ifdef DG_DBG
    if(millis() - debugTimer > 10*1000) {
        debugTimer = millis();
        Serial.printf("WiFi status %d (Connected = %d)\n", WiFi.status(), WL_CONNECTED);
        Serial.printf("ssActive %d\n", ssActive);
        Serial.printf("FPBUnitIsOn %d\n", FPBUnitIsOn);
        Serial.printf("now %d, lastBTTFNpacket %d, ssLastActivity %d\n", millis(), lastBTTFNpacket, ssLastActivity);
    }
    #endif
}

void flushDelayedSave()
{
    if(volchanged) {
        volchanged = false;
        saveCurVolume();
    }
}

/*
 * Time travel
 */

static void timeTravel(bool TCDtriggered, uint16_t P0Dur, uint16_t P1Dur)
{
    int i = 0, tspd;
    
    if(TTrunning)
        return;

    //dbgcnt = 0;

    flushDelayedSave();
    
    if(!TCDtriggered) {
        // If "empty", stand-alone TT is refused
        if(emptyAlarm) return;
    } else {
        // If "empty" when TCD triggers TT, do a "quick refill"
        if(emptyAlarm) {
            prepareTT();
        }
    }

    // If refill sequence is currently running, cancel it
    // and update needles immediately.
    if(refill || refillWA) {
        gauges.UpdateAll();
        refill = refillWA = false;
    }

    // Cancel startAlarm sequence
    startAlarm = false;

    //if(playTTsounds) {
        mp_stop();
    //}

    if(!TCDtriggered && playTTsounds) {
        play_file("/travelstart.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, TT_SOUND_FACT);
    }
        
    TTrunning = true;
    TTstart = TTfUpdNow = millis();
    TTP0 = true;   // phase 0
    TTP1 = TTP2 = false;

    // P1Dur, even if coming from TCD, is not used for timing, 
    // but only to calculate steps  and for a max timeout
    if(!P1Dur) {
        P1Dur = TCDtriggered ? P1_DUR_TCD : P1_DUR;
    }
    P1_maxtimeout = P1Dur + 3000;
    
    TTStepL = TTStepC = TTStepR = 1;          // Stepping is 1
    TTFIntL = gauges.getValuePercent(0);
    if(TTFIntL < left_gauge_empty + (TTStepL * 2)) TTFIntL = 0;
    else {
        TTFIntL = P1Dur / (((TTFIntL - left_gauge_empty) / TTStepL) + 2);
    }
    TTFIntC = gauges.getValuePercent(1);
    if(TTFIntC < center_gauge_empty + (TTStepC * 2)) TTFIntC = 0;
    else {
        TTFIntC = P1Dur / (((TTFIntC - center_gauge_empty) / TTStepC) + 2);
    }
    TTFIntR = gauges.getValuePercent(2);
    if(TTFIntR < right_gauge_empty + (TTStepR * 2)) TTFIntR = 0;
    else {
        TTFIntR = P1Dur / (((TTFIntR - right_gauge_empty) / TTStepR) + 2);
    }
    
    if(TCDtriggered) {    // TCD-triggered TT (GPIO, BTTFN, MQTT-pub) (synced with TCD)
        extTT = true;
        P0duration = P0Dur;
        #ifdef DG_DBG
        Serial.printf("P0 duration is %d, P1 %d\n", P0duration, P1Dur);
        #endif
    } else {              // button/MQTT-cmd triggered TT (stand-alone)
        extTT = false;
    }
}

void prepareTT()
{
    ssEnd();

    // Abort any running sequences
    refill = refillWA = startAlarm = false;

    stopEmptyAlarm();

    if( (gauges.getValuePercent(0) == left_gauge_idle) &&
        (gauges.getValuePercent(1) == center_gauge_idle) &&
        (gauges.getValuePercent(2) == right_gauge_idle) ) {
        return;     
    }
    
    gauges.setValuePercent(0, left_gauge_idle);
    gauges.setValuePercent(1, center_gauge_idle);
    gauges.setValuePercent(2, right_gauge_idle);
    gauges.UpdateAll();

    doPrepareTT = false;
}

// Wakeup: Sent by TCD upon entering dest date,
// return from tt, triggering delayed tt via ETT
// For audio-visually synchronized behavior
// Also called when GPS/RotEnc speed is changed
void wakeup()
{
    // End screen saver
    ssEnd();

    doWakeup = false;
}

void refill_plutonium()
{
    // Triggered by TCD via BTTFN/MQTT, sideswitch or autoRefill timeout.
    // Can come at any time

    if(!FPBUnitIsOn || TTrunning)
        return;

    // Bail if already full.
    if( (gauges.getValuePercent(0) == left_gauge_idle) &&
        (gauges.getValuePercent(1) == center_gauge_idle) &&
        (gauges.getValuePercent(2) == right_gauge_idle) ) {
        return;     
    }

    // Otherwise set percentages, but do not yet Update
    gauges.setValuePercent(0, left_gauge_idle);
    gauges.setValuePercent(1, center_gauge_idle);
    gauges.setValuePercent(2, right_gauge_idle);

    // Abort any running sequences
    refill = refillWA = startAlarm = false;

    // Start two-staged refill if currently on empty alarm
    // refillWA waits for audio to finish, and then triggers timed refill
    if(emptyAlarm) {
        refillWA = playingEmpty;
        stopEmptyAlarm();
        if(refillWA) return;
    }

    // If not currently in alarm state (or sound was stopped), do timed refill
    play_file("/refill.mp3", (ssActive ? 0 : PA_INTRMUS)|PA_ALLOWSD, 0.6);

    // Avoid updating the needles if ss is active
    if(ssActive)
        return;

    // Trigger timed needle-update
    refill = true;
    refillNow = millis();
}

void set_empty()
{
    // Triggered by (user)MQTT or sideswitch
    // Must not run when TTrunning or !FPBUnitIsOn

    if(!FPBUnitIsOn || TTrunning)
        return;

    // Bail if already empty.
    if( (gauges.getValuePercent(0) == left_gauge_empty) &&
        (gauges.getValuePercent(1) == center_gauge_empty) &&
        (gauges.getValuePercent(2) == right_gauge_empty) ) {
        return;     
    }
        
    gauges.setValuePercent(0, left_gauge_empty);
    gauges.setValuePercent(1, center_gauge_empty);
    gauges.setValuePercent(2, right_gauge_empty);

    // Avoid updating the needles if ss is active
    if(!ssActive) {
        gauges.UpdateAll();
    }
    
    startAlarm = true;
    startAlarmNow = millis();
    
    emptyAlarm = true;  // Set this here already for checks elsewhere
    
    refill = refillWA = false;
    
    #ifdef DG_HAVEDOORSWITCH
    dsTimer = false;
    #ifdef DG_HAVEDOORSWITCH2
    d2sTimer = false;
    #endif
    #endif
}     

static void execute_remote_command()
{
    uint32_t command = commandQueue[oCmdIdx];
    bool pE = playingEmpty && !playingEmptyEnds;
    bool injected = false;

    // We are only called if ON
    // To check here:
    // No command execution during time travel
    // No command execution during timed sequences
    // ssActive checked by individual command

    if(!command || TTrunning || startup || startAlarm || refill || refillWA)
        return;

    commandQueue[oCmdIdx] = 0;
    oCmdIdx++;
    oCmdIdx &= 0x0f;

    if(command & 0x80000000) {
        injected = true;
        command &= ~0x80000000;
        // Allow user to directly use TCD code
        if(command >= 9000 && command <= 9999) {
            command -= 9000;
        } else if(command >= 9000000 && command <= 9999999) {
            command -= 9000000;
        }
        if(!command) return;
    }

    if(command < 10) {                                // 900x
        switch(command) {
        case 1:
            play_key(1);
            break;
        case 2:
            if(haveMusic) {
                mp_prev(mpActive);
            }
            break;
        case 3:
            play_key(3);
            break;
        case 4:
            play_key(4);
            break;
        case 5:
            if(haveMusic) {
                if(mpActive) {
                    mp_stop();
                    //if(emptyAlarm) play_empty(); // would be async
                } else {
                    mp_play();
                }
            }
            break;
        case 6:
            play_key(6);
            break;
        case 7:
            play_key(7);
            break;
        case 8:
            if(haveMusic) {
                mp_next(mpActive);
            }
            break;
        case 9:
            play_key(9);
            break;
        }
      
    } else if(command < 100) {                        // 90xx

        if(command == 90) {

            flushDelayedSave();
            say_ip_address();
          
        } else if(command >= 50 && command <= 59) {

            if(haveSD) {
                ssEnd();
                switchMusicFolder((uint8_t)command - 50);
                ssRestartTimer();
            }

        }
        
    } else if (command < 1000) {                      // 9xxx

        if(command >= 100 && command <= 199) {    

            ssEnd();
            command -= 100;                       // 100-199: Set "full" percentage of "Primary" gauge
            if(gauges.supportVariablePercentage(0)) {
                if(!command) command = DEF_L_GAUGE_IDLE;
                if(command > left_gauge_empty) {
                    left_gauge_idle = command;
                    if(!emptyAlarm) {
                        gauges.setValuePercent(0, left_gauge_idle);
                        gauges.UpdateAll();
                    }
                }
            }
            ssRestartTimer();
          
        } else if(command >= 400 && command <= 499) {

            ssEnd();
            command -= 400;                       // 400-499: Set "full" percentage of "Power percent" gauge
            if(gauges.supportVariablePercentage(1)) {
                if(!command) command = DEF_C_GAUGE_IDLE;
                if(command > center_gauge_empty) {
                    center_gauge_idle = command;
                    if(!emptyAlarm) {
                        gauges.setValuePercent(1, center_gauge_idle);
                        gauges.UpdateAll();
                    }
                }
            }
            ssRestartTimer();
          
        } else if(command >= 700 && command <= 799) {

            ssEnd();
            command -= 700;                       // 700-799: Set "full" percentage of "roentgens" gauge
            if(gauges.supportVariablePercentage(2)) {
                if(!command) command = DEF_R_GAUGE_IDLE;
                if(command > right_gauge_empty) {
                    right_gauge_idle = command;
                    if(!emptyAlarm) {
                        gauges.setValuePercent(2, right_gauge_idle);
                        gauges.UpdateAll();
                    }
                }
            }
            ssRestartTimer();

        } else if(command >= 300 && command <= 399) {

            command -= 300;                       // 300-319/399: Set fixed volume level / enable knob
            if(command == 99) {
                #ifdef DG_HAVEVOLKNOB
                curSoftVol = 255;
                volchanged = true;
                volchgnow = millis();
                updateConfigPortalVolValues();
                #endif
            } else if(command <= 19) {
                curSoftVol = command;
                volchanged = true;
                volchgnow = millis();
                updateConfigPortalVolValues();
            }

        } else if(command >= 501 && command <= 509) {

            play_key(command - 500);              // 501-509: Play keyX

        } else {

            switch(command) {
            case 222:                             // 222/555 Disable/enable shuffle
            case 555:
                if(haveMusic) {
                    mp_makeShuffle((command == 555));
                }
                break;
            case 888:                             // 888 go to song #0
                if(haveMusic) {
                    mp_gotonum(0, mpActive);
                }
                break;
            }
        }

    } else if(command < 10000) {                  // 1000-9999 MQTT commands

        // All below only when we're ON
        // Commands allowed while off must be handled in mqttCallback()

        if(!injected) {

            command -= 1000;
    
            switch(command) {
            case 0:
                // Trigger stand-alone Time Travel
                ssEnd();
                timeTravel(false, ETTO_LEAD);
                break;
            case 1:
                ssEnd();
                set_empty();
                break;
            case 2:
                ssEnd();
                refill_plutonium();
                break;
            case 5:    
                if(haveMusic) mp_play();
                break;
            case 6:
                if(haveMusic && mpActive) {
                    mp_stop();
                }
                break;
            case 7:
                if(haveMusic) mp_next(mpActive);
                break;
            case 8:
                if(haveMusic) mp_prev(mpActive);
                break;
            case 13:
                stop_key();
                break;
            }
        }

    } else {
      
        switch(command) {
        case 64738:                               // 64738: reboot
            if(!injected) {
                prepareReboot();
                delay(500);
                esp_restart();
            }
            break;
        case 123456:
            flushDelayedSave();
            if(!injected) {
                deleteIpSettings();               // 123456: deletes IP settings
                if(settings.appw[0]) {
                    settings.appw[0] = 0;         // and clears AP mode WiFi password
                    write_settings();
                }
                ssRestartTimer();
            }
            break;
        case 317931:                              // 317931: Unlock Gauge Type selection in Config Portal
            if(gaugeTypeLocked) {
                gaugeTypeLocked = false;
                updateConfigPortalValues();
            }
            ssRestartTimer();
            break;
        case 1000000:
            if(!injected) {
                refill_plutonium();
            }
            break;
        default:                                  // 888xxx: goto song #xxx
            if((command / 1000) == 888) {
                uint16_t num = command - 888000;
                num = mp_gotonum(num, mpActive);
            }
            break;
        }

    }
}

static void say_ip_address()
{
    uint8_t a, b, c, d;
    bool wasActive = false;
    char ipbuf[16];
    char numfname[] = "/x.mp3";
    if(haveMusic && mpActive) {
        mp_stop();
        wasActive = true;
    }
    stopAudio();
    wifi_getIP(a, b, c, d);
    sprintf(ipbuf, "%d.%d.%d.%d", a, b, c, d);
    numfname[1] = ipbuf[0];
    play_file(numfname, PA_INTRMUS|PA_ALLOWSD);
    for(int i = 1; i < strlen(ipbuf); i++) {
        if(ipbuf[i] == '.') {
            append_file("/dot.mp3", PA_INTRMUS|PA_ALLOWSD);
        } else {
            numfname[1] = ipbuf[i];
            append_file(numfname, PA_INTRMUS|PA_ALLOWSD);
        }
        while(append_pending()) {
            mydelay(10);
        }
    }
    waitAudioDone();
    if(wasActive) {
        mp_play();
    }
}

bool switchMusicFolder(uint8_t nmf, bool isSetup)
{
    bool waitShown = false;

    if(nmf > 9) return false;

    if((musFolderNum != nmf) || isSetup) {
        uint8_t tempperc;

        dgBusy = true;
        
        if(!isSetup) {
            musFolderNum = nmf;
            // Initializing the MP can take a while;
            // need to stop all audio before calling
            // mp_init()
            if(haveMusic && mpActive) {
                mp_stop();
            }
            stopAudio();
        }
        if(haveSD) {
            if(mp_checkForFolder(musFolderNum) == -1) {
                if(!isSetup) flushDelayedSave();
                showWaitSequence();
                waitShown = true;
                play_file("/renaming.mp3", PA_INTRMUS|PA_ALLOWSD);
                waitAudioDone();
            }
        }
        if(!isSetup) {
            saveMusFoldNum();
            updateConfigPortalMFValues();
        }
        if(mprengauge >= 0) {
            tempperc = isSetup ? 0 : gauges.getValuePercent(mprengauge);
        }
        mprengaugetouched = false;
        mp_init(isSetup);
        if(mprengaugetouched && mprengauge >= 0) {
            gauges.setValuePercent(mprengauge, tempperc);
            gauges.UpdateAll();
        }
        if(waitShown) {
            endWaitSequence();
        }

        dgBusy = false;
    }

    return waitShown;
} 

void showMPRPrecDone(unsigned int perc)
{
    if(mprengauge < 0)
        return;

    gauges.setValuePercent(mprengauge, perc);
    gauges.UpdateAll();
    mprengaugetouched = true;
}

/*
 * Helpers
 */

static bool checkGauges()
{
    if( (gauges.getValuePercent(0) == left_gauge_empty) ||
        (gauges.getValuePercent(1) == center_gauge_empty) ||
        (gauges.getValuePercent(2) == right_gauge_empty) ) {

        gauges.setValuePercent(0, left_gauge_empty);
        gauges.setValuePercent(1, center_gauge_empty);
        gauges.setValuePercent(2, right_gauge_empty);
        gauges.UpdateAll();

        startEmptyAlarm();
        return false;
    }

    return true;
}

static void startEmptyAlarm()
{
    play_empty();
    emptyLED.startBlink(EMPTY_INTERVAL, 0);
    emptyAlarmNow = millis();
    emptyAlarm = true;
    startAlarm = false;                 // cancel timed sequence
}

static void stopEmptyAlarm()
{
    if(playingEmpty) stopAudioAtLoopEnd();
    remove_appended_empty();
    emptyLED.stopBlink();
    emptyAlarm = startAlarm = false;    // also cancel startAlarm timed sequence
}

static void gauge_lights_on()
{
    digitalWrite(BACKLIGHTS_PIN, HIGH);
}

static void gauge_lights_off()
{
    digitalWrite(BACKLIGHTS_PIN, LOW);
}

/* Keys & buttons */

void sideSwitch_scan()
{
    sideSwitch.scan();
}

static void sideSwitchLongPress()
{
    isSSwitchPressed = true;
    isSSwitchChange = true;
    Serial.println("Side switch is now pressed");
}

static void sideSwitchLongPressStop()
{
    isSSwitchPressed = false;
    isSSwitchChange = true;
    Serial.println("Side switch is now released");
}

#ifdef DG_HAVEDOORSWITCH
void play_door_open(int doorNum, bool isOpen)
{
    // Sounds from same door may interrupt themselves; if sound from
    // other door is to be played while first door's is running, we 
    // only interrupt if resonable part of the first door's sound is already 
    // played back.
    if((!lastDoorNum || lastDoorNum == doorNum) || (millis() - lastDoorSoundNow > 750)) {
        if(playingDoor || checkAudioDone()) {
            play_file(isOpen ? "/dooropen.mp3" : "/doorclose.mp3", PA_ALLOWSD|PA_DOOR, 1.0);
            lastDoorSoundNow = millis();
            lastDoorNum = doorNum;
        }
    }
}

void doorSwitch_scan()
{
    doorSwitch.scan();
    #ifdef DG_HAVEDOORSWITCH2
    door2Switch.scan();
    #endif
}
static void doorSwitchLongPress()
{
    isDSwitchPressed = true;
    isDSwitchChange = true;
    isDSwitchChangeNow = millis();
}

static void doorSwitchLongPressStop()
{
    isDSwitchPressed = false;
    isDSwitchChange = true;
    isDSwitchChangeNow = millis();
}
#ifdef DG_HAVEDOORSWITCH2
static void door2SwitchLongPress()
{
    isD2SwitchPressed = true;
    isD2SwitchChange = true;
    isD2SwitchChangeNow = millis();
}

static void door2SwitchLongPressStop()
{
    isD2SwitchPressed = false;
    isD2SwitchChange = true;
    isD2SwitchChangeNow = millis();
}
#endif
#endif

static void ttkeyScan()
{
    TTKey.scan();
    sideSwitch_scan();
}

static void Button1Scan()
{
    Button1.scan();
}

#ifdef DG_HAVEDOORSWITCH
static void dsScan()
{
    if(dsPlay) doorSwitch_scan();
}
#endif

static void TTKeyPressed()
{
    isTTKeyPressed = true;
}

static void TTKeyHeld()
{
    isTTKeyHeld = true;
}

static void Button1Pressed()
{
    isB1Pressed = true;
}

static void Button1Held()
{
    isB1Held = true;
}

/* 
 *  "Screen saver"
 *  
 *  - switches off backlight 
 *  - set gauges to zero
 *  - stops empty alarm sound (but not alarm itself)
 *  
 */

static void ssStart()
{
    if(ssActive)
        return;

    // Stop empty alarm sound
    if(emptyAlarm && playingEmpty) {
        stopAudioAtLoopEnd();
    }

    gauge_lights_off();
    gauges.off();

    ssActive = true;
}

static void ssRestartTimer()
{
    ssLastActivity = millis();
}

static void ssEnd()
{
    if(!FPBUnitIsOn)
        return;

    ssRestartTimer();

    if(!ssActive)
        return;

    gauge_lights_on();
    gauges.on();

    //if(emptyAlarm && !mpActive) play_empty(); // would be async

    ssActive = false;
}

/*
 * Show special signals
 */

void showWaitSequence()
{
    emptyLED.specialSignal(DGSEQ_WAIT);
}

void endWaitSequence()
{
    emptyLED.specialSignal(0);
}

void showCopyError()
{
    emptyLED.specialSignal(DGSEQ_ERRCOPY);
}

void allOff()
{
    gauge_lights_off();
    gauges.off();
    emptyLED.stopBlink();
    emptyLED.specialSignal(0);
}

void prepareReboot()
{
    mp_stop();
    stopAudio();
    allOff();
    flushDelayedSave();
    delay(500);
    unmount_fs();
    delay(100);
}

/*
 * Others
 */

static void waitAudioDone()
{
    int timeout = 400;

    while(!checkAudioDone() && timeout--) {
        mydelay(10);
    }
}

/*
 *  Do this whenever we are caught in a while() loop
 *  This allows other stuff to proceed
 */
static void myloop()
{
    wifi_loop();
    audio_loop();
    #ifdef DG_HAVEDOORSWITCH
    dsScan();
    #endif
    bttfn_loop_quick();
}

/*
 * MyDelay:
 * Calls myloop() periodically
 */
void mydelay(unsigned long mydel)
{
    unsigned long startNow = millis();
    myloop();
    while(millis() - startNow < mydel) {
        delay(10);
        myloop();
    }
}

static uint8_t restrict_gauge_idle(int val, int minimum, int maximum, uint8_t def)
{
    if(val <= minimum) return def;
    if(val > maximum)  return 100;
    return val;
}

static uint8_t restrict_gauge_empty(int val, int minimum, int maximum, uint8_t def)
{
    if(val < minimum)  return 0;
    if(val >= maximum) return def;
    return val;
}

/*
 * Basic Telematics Transmission Framework (BTTFN)
 */

void addCmdQueue(uint32_t command)
{
    if(!command) return;

    commandQueue[iCmdIdx] = command;
    iCmdIdx++;
    iCmdIdx &= 0x0f;
}

static void bttfn_setup()
{
    useBTTFN = false;

    // string empty? Disable BTTFN.
    if(!settings.tcdIP[0])
        return;

    haveTCDIP = isIp(settings.tcdIP);
    
    if(!haveTCDIP) {
        tcdHostNameHash = 0;
        unsigned char *s = (unsigned char *)settings.tcdIP;
        for ( ; *s; ++s) tcdHostNameHash = 37 * tcdHostNameHash + tolower(*s);
    } else {
        bttfnTcdIP.fromString(settings.tcdIP);
    }
    
    dgUDP = &bttfUDP;
    dgUDP->begin(BTTF_DEFAULT_LOCAL_PORT);

    dgMcUDP = &bttfMcUDP;
    dgMcUDP->beginMulticast(bttfnMcIP, BTTF_DEFAULT_LOCAL_PORT + 2);
    
    BTTFNPreparePacketTemplate();
    
    BTTFNfailCount = 0;
    useBTTFN = true;
}

void bttfn_loop()
{
    int t = 100;
    
    if(!useBTTFN)
        return;

    while(bttfn_checkmc() && t--) {}
            
    BTTFNCheckPacket();
    
    if(!BTTFNPacketDue) {
        // If WiFi status changed, trigger immediately
        if(!BTTFNWiFiUp && (WiFi.status() == WL_CONNECTED)) {
            BTTFNUpdateNow = 0;
        }
        if((!BTTFNUpdateNow) || (millis() - BTTFNUpdateNow > BTTFN_POLL_INT)) {
            BTTFNTriggerUpdate();
        }
    }
}

static void bttfn_loop_quick()
{
    int t = 100;
    
    if(!useBTTFN)
        return;

    while(bttfn_checkmc() && t--) {}
}

static bool check_packet(uint8_t *buf)
{
    // Basic validity check
    if(memcmp(BTTFUDPBuf, BTTFUDPHD, 4))
        return false;

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    if(BTTFUDPBuf[BTTF_PACKET_SIZE - 1] != a)
        return false;

    return true;
}

static void handle_tcd_notification(uint8_t *buf)
{
    uint32_t seqCnt;

    // Note: This might be called while we are in a
    // wait-delay-loop. Best to just set flags here
    // that are evaluated synchronously (=later).
    // Do not stuff that messes with display, input,
    // etc.
    
    switch(buf[5]) {
    case BTTFN_NOT_SPD:
        seqCnt = GET32(buf, 12);
        if(seqCnt > bttfnTCDSeqCnt || seqCnt == 1) {
            switch(buf[8] | (buf[9] << 8)) {
            case BTTFN_SSRC_GPS:
                spdIsRotEnc = false;
                break;
            case BTTFN_SSRC_P1:
                // If packets come out-of-order, we might
                // get this one before TTrunning, and we
                // don't want a switch to usingGPSS only 
                // because of P1 speed
                if(!TTrunning) return;
                // fall through
            default:
                spdIsRotEnc = true;
            }
            gpsSpeed = (int16_t)(buf[6] | (buf[7] << 8));
            if(gpsSpeed > 88) gpsSpeed = 88;
        } else {
            #ifdef DG_DBG
            Serial.printf("Out-of-sequence packet received from TCD %d %d\n", seqCnt, bttfnTCDSeqCnt);
            #endif
        }
        bttfnTCDSeqCnt = seqCnt;
        break;
    case BTTFN_NOT_PREPARE:
        // Prepare for TT. Comes at some undefined point,
        // an undefined time before the actual tt, and
        // may not come at all.
        // We disable our Screen Saver
        // We don't ignore this if TCD is connected by wire,
        // because this signal does not come via wire.
        doPrepareTT = true;
        break;
    case BTTFN_NOT_TT:
        // Trigger Time Travel (if not running already)
        // Ignore command if TCD is connected by wire
        if(!TCDconnected && !TTrunning && !dgBusy) {
            networkTimeTravel = true;
            networkTCDTT = true;
            networkReentry = false;
            networkAbort = false;
            networkLead = buf[6] | (buf[7] << 8);
            networkP1 = buf[8] | (buf[9] << 8);
        }
        break;
    case BTTFN_NOT_REENTRY:
        // Start re-entry (if TT currently running)
        // Ignore command if TCD is connected by wire
        if(!TCDconnected && TTrunning && networkTCDTT) {
            networkReentry = true;
        }
        break;
    case BTTFN_NOT_ABORT_TT:
        // Abort TT (if TT currently running)
        // Ignore command if TCD is connected by wire
        if(!TCDconnected && TTrunning && networkTCDTT) {
            networkAbort = true;
        }
        break;
    case BTTFN_NOT_ALARM:
        networkAlarm = true;
        break;
    case BTTFN_NOT_REFILL:
        if(!dgBusy) {
            addCmdQueue(1000000);
        }
        break;
    case BTTFN_NOT_PCG_CMD:
        if(!dgBusy) {
            addCmdQueue(GET32(buf, 6));
        }
        break;
    case BTTFN_NOT_WAKEUP:
        doWakeup = true;
        break;
    case BTTFN_NOT_BUSY:
        tcdIsBusy = !!(buf[8]);
        break;
    }
}

static bool bttfn_checkmc()
{
    int psize = dgMcUDP->parsePacket();

    if(!psize) {
        return false;
    }

    // This returns true as long as a packet was received
    // regardless whether it was for us or not. Point is
    // to clear the receive buffer.
    
    dgMcUDP->read(BTTFMCBuf, BTTF_PACKET_SIZE);

    #ifdef DG_DBG
    Serial.printf("Received multicast packet from %s\n", dgMcUDP->remoteIP().toString());
    #endif

    if(haveTCDIP) {
        if(bttfnTcdIP != dgMcUDP->remoteIP())
            return true;
    } else {
        // Do not use tcdHostNameHash; let DISCOVER do its work
        // and wait for a result.
        return true;
    }

    if(!check_packet(BTTFMCBuf))
        return true;

    if((BTTFMCBuf[4] & 0x4f) == (BTTFN_VERSION | 0x40)) {

        // A notification from the TCD
        handle_tcd_notification(BTTFMCBuf);
    
    }

    return true;
}

// Check for pending packet and parse it
static void BTTFNCheckPacket()
{
    unsigned long mymillis = millis();
    
    int psize = dgUDP->parsePacket();
    if(!psize) {
        if(BTTFNPacketDue) {
            if((mymillis - BTTFNTSRQAge) > 700) {
                // Packet timed out
                BTTFNPacketDue = false;
                // Immediately trigger new request for
                // the first 10 timeouts, after that
                // the new request is only triggered
                // in greater intervals via bttfn_loop().
                if(haveTCDIP && BTTFNfailCount < 10) {
                    BTTFNfailCount++;
                    BTTFNUpdateNow = 0;
                }
            }
        }
        return;
    }
    
    dgUDP->read(BTTFUDPBuf, BTTF_PACKET_SIZE);

    if(!check_packet(BTTFUDPBuf))
        return;

    if((BTTFUDPBuf[4] & 0x4f) == (BTTFN_VERSION | 0x40)) {

        // A notification from the TCD
        handle_tcd_notification(BTTFUDPBuf);

    } else {

        // (Possibly) a response packet
    
        if(GET32(BTTFUDPBuf, 6) != BTTFUDPID)
            return;
    
        // Response marker missing or wrong version, bail
        if((BTTFUDPBuf[4] & 0x8f) != (BTTFN_VERSION | 0x80))
            return;

        BTTFNfailCount = 0;
    
        // If it's our expected packet, no other is due for now
        BTTFNPacketDue = false;

        if(BTTFUDPBuf[5] & 0x80) {
            if(!haveTCDIP) {
                bttfnTcdIP = dgUDP->remoteIP();
                haveTCDIP = true;
                #ifdef DG_DBG
                Serial.printf("Discovered TCD IP %d.%d.%d.%d\n", bttfnTcdIP[0], bttfnTcdIP[1], bttfnTcdIP[2], bttfnTcdIP[3]);
                #endif
            } else {
                #ifdef DG_DBG
                Serial.println("Internal error - received unexpected DISCOVER response");
                #endif
            }
        }

        if(BTTFUDPBuf[5] & 0x02) {
            gpsSpeed = (int16_t)(BTTFUDPBuf[18] | (BTTFUDPBuf[19] << 8));
            if(gpsSpeed > 88) gpsSpeed = 88;
            spdIsRotEnc = (BTTFUDPBuf[26] & (0x80|0x20)) ? true : false;    // Speed is from RotEnc or Remote
        }

        if(BTTFUDPBuf[5] & 0x10) {
            tcdNM  = (BTTFUDPBuf[26] & 0x01) ? true : false;
            tcdFPO = (BTTFUDPBuf[26] & 0x02) ? true : false;   // 1 means fake power off
            tcdIsBusy = (BTTFUDPBuf[26] & 0x10) ? true : false;
        } else {
            tcdNM = false;
            tcdFPO = false;
        }

        if(BTTFUDPBuf[5] & 0x40) {
            bttfnReqStatus &= ~0x40;     // Do no longer poll capabilities
            if(BTTFUDPBuf[31] & 0x01) {
                bttfnReqStatus &= ~0x02; // Do no longer poll speed, comes over multicast
            }
        }

        lastBTTFNpacket = mymillis;
    }
}

// Send a new data request
static bool BTTFNTriggerUpdate()
{
    BTTFNPacketDue = false;

    BTTFNUpdateNow = millis();

    if(WiFi.status() != WL_CONNECTED) {
        BTTFNWiFiUp = false;
        return false;
    }

    BTTFNWiFiUp = true;

    // Send new packet
    BTTFNSendPacket();
    BTTFNTSRQAge = millis();
    
    BTTFNPacketDue = true;
    
    return true;
}

static void BTTFNPreparePacketTemplate()
{
    memset(BTTFUDPTBuf, 0, BTTF_PACKET_SIZE);

    // ID
    memcpy(BTTFUDPTBuf, BTTFUDPHD, 4);

    // Tell the TCD about our hostname
    // 13 bytes total. If hostname is longer, last in buf is '.'
    memcpy(BTTFUDPTBuf + 10, settings.hostName, 13);
    if(strlen(settings.hostName) > 13) BTTFUDPTBuf[10+12] = '.';

    BTTFUDPTBuf[10+13] = BTTFN_TYPE_PCG;

    // Version, MC-marker
    BTTFUDPTBuf[4] = BTTFN_VERSION | BTTFN_SUP_MC;
}

static void BTTFNPreparePacket()
{
    memcpy(BTTFUDPBuf, BTTFUDPTBuf, BTTF_PACKET_SIZE);
} 

static void BTTFNDispatch()
{
    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;

    if(haveTCDIP) {
        dgUDP->beginPacket(bttfnTcdIP, BTTF_DEFAULT_LOCAL_PORT);
    } else {
        #ifdef DG_DBG
        Serial.printf("Sending multicast (hostname hash %x)\n", tcdHostNameHash);
        #endif
        dgUDP->beginPacket(bttfnMcIP, BTTF_DEFAULT_LOCAL_PORT + 1);
    }
    dgUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
    dgUDP->endPacket();
}


static void BTTFNSendPacket()
{   
    BTTFNPreparePacket();

    // Serial
    BTTFUDPID = (uint32_t)millis();
    SET32(BTTFUDPBuf, 6, BTTFUDPID);

    // Request status and speed
    BTTFUDPBuf[5] = bttfnReqStatus;

    if(!haveTCDIP) {
        BTTFUDPBuf[5] |= 0x80;
        SET32(BTTFUDPBuf, 31, tcdHostNameHash);
    }

    BTTFNDispatch();
}

static bool BTTFNConnected()
{
    if(!useBTTFN)
        return false;

    if(!haveTCDIP)
        return false;
    
    if(WiFi.status() != WL_CONNECTED)
        return false;

    if(!lastBTTFNpacket)
        return false;

    return true;
}

static bool bttfn_trigger_tt()
{
    if(!BTTFNConnected())
        return false;

    if(TTrunning || tcdIsBusy)
        return false;

    BTTFNPreparePacket();
    
    BTTFUDPBuf[5] = 0x80;           // Trigger BTTFN-wide TT

    BTTFNDispatch();

    #ifdef DG_DBG
    Serial.println("Triggered BTTFN-wide TT");
    #endif

    return true;
}
