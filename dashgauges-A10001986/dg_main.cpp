/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Main controller
 *
 * -------------------------------------------------------------------
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

#include "dg_global.h"

#include <Arduino.h>
#include <WiFi.h>
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
static Gauges gauges = Gauges();

/* 
 * Gauges hardware definition 
 * 
 * The first value is the Type (#define in dgdisplay.h). This 
 * is not an index, but the top-most selector for hardware
 * access in dgdisplay.
 * All other values are parameters for the specific Type (which
 * differ in meaning depending on the Type).
 * Types can be reused in the array, with different parameters.
 * 
 * DGD_TYPE_MCP4728:
 * The first two paramaters after the Type are the i2c addresses
 * under which the MCP4728 is searched for.
 * The following three parameters are the maximum voltage ever sent
 * to the gauges, given in 1/4095s. Full voltage = 4095 (=100% of
 * reference voltage).
 * The last three parameters define the reference voltage: This is either
 * - MCP4728_VREF_INT|MCP4728_GAIN_LOW  for 2048V, or
 * - MCP4728_VREF_INT|MCP4728_GAIN_HIGH for 4096V, or
 * - MCP4728_VREF_EXT                   for 5V.
 * 
 */
static int8_t gaugeType = 0;
static const uint16_t gaugesParmArray[GA_NUM_TYPES][16] = {
    { 0 },

    // Config 1: MCP4728 (searched for at two i2c addresses) + 2 x 0-0.5V + VUMeter
    // 1+2: Phaostron/H&P 631-14672 with 8.6K resistors in series (DAC channels A and B). 
    //      Using MCP4728's built-in Vref (2.048V), limited by "max" parameter to 0.5V (1000/4095*2.048).
    // 3:   Simpson 49L VU meter with 3k6 resistor in series (DAC channel C)
    //      Using MCP4728's built-in Vref (2.048V), limited by "max" parameter to 1.6V (3200/4095*2.048)
    // Parameters:      i2c-1 i2c-2 maxA  maxB  maxC  Voltage ref A     Voltage ref B     Voltage ref C
    { DGD_TYPE_MCP4728, 0x64, 0x60, 
            1000, 1000, 3200, 
            MCP4728_VREF_INT|MCP4728_GAIN_LOW, 
            MCP4728_VREF_INT|MCP4728_GAIN_LOW, 
            MCP4728_VREF_INT|MCP4728_GAIN_LOW },
            
    // Config 2: MCP4728 (searched for at two i2c addresses) + 2 x 0-0.5V + original Roentgens meter
    // 1+2: Phaostron/H&P 631-14672 with 8.6K resistors in series (DAC channels A and B). 
    //      Using MCP4728's built-in Vref (2.048V), limited by "max" parameter to 0.5V (1000/4095*2.048).
    // 3:   Simpson Roentgens meter (with no resistor) (DAC channel C)
    //      Using MCP4728's built-in Vref (2.048V), limited by "max" parameter to 0.014V (28/4095*2.048)
    //      FIXME - need a resistor value to keep this within reasonable limits - 14mV is too low
    // Parameters:      i2c-1 i2c-2 maxA  maxB  maxC  Voltage ref A     Voltage ref B     Voltage ref C    
    { DGD_TYPE_MCP4728, 0x64, 0x60, 
            1000, 1000, 28, 
            MCP4728_VREF_INT|MCP4728_GAIN_LOW, 
            MCP4728_VREF_INT|MCP4728_GAIN_LOW, 
            MCP4728_VREF_INT|MCP4728_GAIN_LOW },

    // Config 3: Binary gauges (with separate pins for each gauge; pins defined in dg_global.h)
    // Type,               pin left,    pin center,  pin right,   thresholds for "empty" for each gauge
    { DGD_TYPE_BINARY_SEP, L_G_BIN_PIN, C_G_BIN_PIN, R_G_BIN_PIN, 0, 0, 0 },

    // Config 4: Binary gauges (with one pin for all gauges; pin defined in dg_global.h)
    // Type,               pin,           threshold for empty
    { DGD_TYPE_BINARY_ALL, ALL_G_BIN_PIN, 0 }

    // Example 1: Meters 1+2 are driven with 0-2V output (which naturally means either a different
    // voltmeter (0-2V), or the Phaostrons with a higher resistor value).
    // Meter 3 is a modified 50V DC Simpson voltmeter with the internal resistor bridged, and a 
    // 5k6 resistor added externally so that full scale is at about 5V. We use Vcc (which is 5V) as 
    // reference in that case, and can go to max (4095).
    //{ DGD_TYPE_MCP4728, 0x64, 0x60, 
    //        4000, 4000, 4095, 
    //        MCP4728_VREF_INT|MCP4728_GAIN_LOW, 
    //        MCP4728_VREF_INT|MCP4728_GAIN_LOW, 
    //        MCP4728_VREF_EXT },

    // Example 2: Like config 1, but 1+2 are driven with 0-5V output (using Vcc as VRef on DAC)
    //{ DGD_TYPE_MCP4728, 0x64, 0x60, 
    //        4095, 4095, 3200, 
    //        MCP4728_VREF_EXT, 
    //        MCP4728_VREF_EXT, 
    //        MCP4728_VREF_INT|MCP4728_GAIN_LOW }

    

    // Add yours here and in dgdisplay (hardware access, if new type is required) and 
    // in dgwifi (name for selection menu in CP; must be in same order as this array)
};

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
    false,    // Switch is active HIGH
    false     // Disable internal pull-up resistor
);

static bool isSSwitchPressed = false;
static bool isSSwitchChange = false;

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

// The door switch
#ifdef DG_HAVEDOORSWITCH
static DGButton doorSwitch = DGButton(DOOR_SWITCH_PIN,
    true,    // Switch is active LOW
    true     // Enable internal pull-up resistor
);
#endif

static bool isDSwitchPressed = false;
static bool isDSwitchChange = false;
static unsigned long isDSwitchChangeNow = 0;
static bool dsCloseOnClose = false;
static unsigned long swInitNow = 0;

#define STARTUP_DELAY 2300
static bool          startup = false;
static unsigned long startupNow = 0;

#define REFILL_DELAY 1235
static bool          refill = false;
static unsigned long refillNow = 0;
static bool          refillWA = false;

static unsigned long autoRefill = 0;
static unsigned long autoMute = 0;

#define ALARM_DELAY 1000
static bool          startAlarm = false;
static unsigned long startAlarmNow = 0;

#ifdef DG_HAVEDOORSWITCH
static bool          dsPlay = false;
static unsigned long dsNow = 0;
static bool          dsTimer = false;
static unsigned long dsDelay = 0;
static unsigned long dsADelay = 0;
static bool          dsOpen = false;
#endif

bool networkTimeTravel = false;
bool networkTCDTT      = false;
bool networkReentry    = false;
bool networkAbort      = false;
bool networkAlarm      = false;
uint16_t networkLead   = ETTO_LEAD;
uint16_t networkP1     = 6600;

static int16_t gpsSpeed = -1;
static int16_t oldGpsSpeed = -1;
static bool    spdIsRotEnc = false;

static bool useNM = false;
static bool tcdNM = false;
bool        dgNM = false;
static bool useFPO = false;
static bool tcdFPO = false;

static bool bttfnTT = true;

// Time travel status flags etc.
bool                 TTrunning = false;  // TT sequence is running
static bool          extTT = false;      // TT was triggered by TCD
static unsigned long TTstart = 0;
static unsigned long TTfUpdNow = 0;
static unsigned long P0duration = ETTO_LEAD;
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

// Durations of tt phases for internal tt
#define P0_DUR          5000    // acceleration phase
#define P1_DUR          5000    // time tunnel phase
#define P2_ALARM_DELAY  2000    // Delay for alarm after reentry
#define TT_SNDLAT        400    // DO NOT CHANGE (latency for sound/mp3)

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
static bool          emptyAlarm = false;
static unsigned long emptyAlarmNow = 0;

static bool          FPOffemptyAlarm = false;
static unsigned long FPOffemptyAlarmNow = 0;

// BTTF network
#define BTTFN_VERSION              1
#define BTTF_PACKET_SIZE          48
#define BTTF_DEFAULT_LOCAL_PORT 1338
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
#define BTTFN_TYPE_ANY     0    // Any, unknown or no device
#define BTTFN_TYPE_FLUX    1    // Flux Capacitor
#define BTTFN_TYPE_SID     2    // SID
#define BTTFN_TYPE_PCG     3    // Plutonium chamber gauge panel
static const uint8_t BTTFUDPHD[4] = { 'B', 'T', 'T', 'F' };
static bool          useBTTFN = false;
static WiFiUDP       bttfUDP;
static UDP*          dgUDP;
static byte          BTTFUDPBuf[BTTF_PACKET_SIZE];
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
#ifdef BTTFN_MC
static uint32_t      tcdHostNameHash = 0;
#endif  

static int      iCmdIdx = 0;
static int      oCmdIdx = 0;
static uint32_t commandQueue[16] = { 0 };

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
#endif
static void ttkeyScan();
static void TTKeyPressed();
static void TTKeyHeld();

static void ssStart();
static void ssEnd();
static void ssRestartTimer();

static void waitAudioDone();

static uint8_t restrict_gauge_idle(int val, int minimum, int maximum, uint8_t def);
static uint8_t restrict_gauge_empty(int val, int minimum, int maximum, uint8_t def);

static void bttfn_setup();
static void BTTFNCheckPacket();
static bool BTTFNTriggerUpdate();
static void BTTFNSendPacket();
static bool BTTFNTriggerTT();

// Boot

void main_boot()
{
    // Setup pin for lights relay
    pinMode(BACKLIGHTS_PIN, OUTPUT);
    gauge_lights_off();
    
    // Set up "empty" LED
    emptyLED.begin();

    // Init side switch
    sideSwitch.setPressTicks(10);     // ms after short press is assumed
    sideSwitch.setLongPressTicks(50); // ms after long press is assumed
    sideSwitch.setDebounceTicks(50);
    sideSwitch.attachLongPressStart(sideSwitchLongPress);
    sideSwitch.attachLongPressStop(sideSwitchLongPressStop);
    sideSwitch.scan();

    // Init door switch
    #ifdef DG_HAVEDOORSWITCH
    doorSwitch.setPressTicks(10);     // ms after short press is assumed
    doorSwitch.setLongPressTicks(50); // ms after long press is assumed
    doorSwitch.setDebounceTicks(50);
    doorSwitch.attachLongPressStart(doorSwitchLongPress);
    doorSwitch.attachLongPressStop(doorSwitchLongPressStop);
    doorSwitch.scan();
    #endif

    swInitNow = millis();
}

void main_boot2()
{
    gaugeType = atoi(settings.gaugeType);
    if(gaugeType < 0 || gaugeType >= GA_NUM_TYPES) {
        Serial.printf("Invalid gauge type %d, resetting to NONE\n", gaugeType);
        gaugeType = 0;
    }
      
    gauges.begin((uint16_t *)&gaugesParmArray[gaugeType]);
    
    gauges.off();

    gauges.setValuePercent(0, 0);
    gauges.setValuePercent(1, 0);
    gauges.setValuePercent(2, 0);
}

void main_setup()
{
    int temp;
    unsigned long tempu;
    
    Serial.println(F("Dash Gauges version " DG_VERSION " " DG_VERSION_EXTRA));

    left_gauge_idle = restrict_gauge_idle(atoi(settings.lIdle), 0, 100, DEF_L_GAUGE_IDLE);
    center_gauge_idle = restrict_gauge_idle(atoi(settings.cIdle), 0, 100, DEF_C_GAUGE_IDLE);
    right_gauge_idle = restrict_gauge_idle(atoi(settings.rIdle), 0, 100, DEF_R_GAUGE_IDLE);

    left_gauge_empty = restrict_gauge_empty(atoi(settings.lEmpty), 0, left_gauge_idle, DEF_L_GAUGE_EMPTY);
    center_gauge_empty = restrict_gauge_empty(atoi(settings.cEmpty), 0, center_gauge_idle, DEF_C_GAUGE_EMPTY);
    right_gauge_empty = restrict_gauge_empty(atoi(settings.rEmpty), 0, right_gauge_idle, DEF_R_GAUGE_EMPTY);

    if(left_gauge_empty >= left_gauge_idle) { left_gauge_idle = DEF_L_GAUGE_IDLE; left_gauge_empty = DEF_L_GAUGE_EMPTY; } 
    if(center_gauge_empty >= center_gauge_idle) { center_gauge_idle = DEF_C_GAUGE_IDLE; center_gauge_empty = DEF_C_GAUGE_EMPTY; } 
    if(right_gauge_empty >= right_gauge_idle) { right_gauge_idle = DEF_R_GAUGE_IDLE; right_gauge_empty = DEF_R_GAUGE_EMPTY; } 

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
    TTKey.attachPress(TTKeyPressed);
    if(!TCDconnected) {
        // If we have a physical button, we need
        // reasonable values for debounce and press
        TTKey.setDebounceTicks(TT_DEBOUNCE);
        TTKey.setPressTicks(TT_PRESS_TIME);
        TTKey.setLongPressTicks(TT_HOLD_TIME);
        TTKey.attachLongPressStart(TTKeyHeld);
    } else {
        // If the TCD is connected, we can go more to the edge
        TTKey.setDebounceTicks(5);
        TTKey.setPressTicks(50);
        TTKey.setLongPressTicks(100000);
        // Long press ignored when TCD is connected
    }

    // Invoke audio file installer if SD content qualifies
    #ifdef DG_DBG
    Serial.println(F("Probing for audio files on SD"));
    #endif
    if(check_allow_CPA()) {
        showWaitSequence();
        play_file("/installing.mp3", PA_ALLOWSD, 1.0);
        waitAudioDone();
        doCopyAudioFiles();
        // We never return here. The ESP is rebooted.
    }

    if(!audio_files_present()) {
        #ifdef DG_DBG
        Serial.println(F("Audio files not installed"));
        #endif
        emptyLED.specialSignal(DGSEQ_NOAUDIO);
        while(emptyLED.specialDone()) {
            mydelay(100);
        }
    }

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
    sideSwitch.scan();
    isSSwitchChange = false;
    #ifdef DG_HAVEDOORSWITCH
    doorSwitch.scan();
    isDSwitchChange = false;
    #endif

    #ifdef DG_DBG
    Serial.println(F("main_setup() done"));
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
            stopAudio();

            flushDelayedSave();
            
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
                gauges.setValuePercent(0, left_gauge_empty);
                gauges.setValuePercent(1, center_gauge_empty);
                gauges.setValuePercent(2, right_gauge_empty);
                gauges.UpdateAll();
                startAlarm = true;
                startAlarmNow = millis();
                emptyAlarm = true;  // Set this here already for checks elsewhere
                refill = refillWA = false;
                #ifdef DG_HAVEDOORSWITCH
                dsTimer = false;
                #endif
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
            if(checkAudioDone()) {
                play_file(dsOpen ? "/dooropen.mp3" : "/doorclose.mp3", PA_ALLOWSD, 1.0);
            }
        }
        dsTimer = false;
    }
    
    if(dsPlay && isDSwitchChange) {
        if(!refillWA) {
            unsigned long timePassed = millis() - isDSwitchChangeNow;
            dsOpen = (isDSwitchPressed != dsCloseOnClose);
            if(dsDelay && (dsDelay > timePassed + 100)) {
                dsTimer = true;
                dsADelay = dsDelay;
                dsNow = isDSwitchChangeNow;
            } else if(timePassed < 500 && checkAudioDone()) {
                play_file(dsOpen ? "/dooropen.mp3" : "/doorclose.mp3", PA_ALLOWSD, 1.0);
            }
        }
        isDSwitchChange = false;
    }
    #endif
    
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
        } else if(isTTKeyPressed) {
            isTTKeyPressed = false;
            if(!TCDconnected && ssActive) {
                // First button press when ss is active only deactivates ss
                ssEnd();
            } else {
                if(TCDconnected) {
                    ssEnd();
                }
                if(!bttfnTT || !BTTFNTriggerTT()) {
                    timeTravel(TCDconnected, noETTOLead ? 0 : ETTO_LEAD);
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
                             
                } else {

                    TTP0 = false;
                    TTP1 = true;
                    TTfUpdLNow = TTfUpdCNow = TTfUpdRNow = now;

                }
            }
            if(TTP1) {   // Peak/"time tunnel" - ends with pin going LOW or BTTFN/MQTT "REENTRY"

                if( (networkTCDTT && (!networkReentry && !networkAbort)) || (!networkTCDTT && digitalRead(TT_IN_PIN))) 
                {

                    bool doUpd = false;
                    
                    if(TTFIntL && (now - TTfUpdLNow >= TTFIntL)) {
                        int t = gauges.getValuePercent(0);
                        if(t >= left_gauge_empty + TTStepL) t -= TTStepL;
                        else t = left_gauge_empty;
                        gauges.setValuePercent(0, t);
                        TTfUpdLNow = now;
                        doUpd = true;
                    }
                    if(TTFIntC && (now - TTfUpdCNow >= TTFIntC)) {
                        int t = gauges.getValuePercent(1);
                        if(t >= center_gauge_empty + TTStepC) t -= TTStepC;
                        else t = center_gauge_empty;
                        gauges.setValuePercent(1, t);
                        TTfUpdCNow = now;
                        doUpd = true;
                    }
                    if(TTFIntR && (now - TTfUpdRNow >= TTFIntR)) {
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
                    gauges.setValuePercent(0, left_gauge_empty);
                    gauges.setValuePercent(1, center_gauge_empty);
                    gauges.setValuePercent(2, right_gauge_empty);
                    gauges.UpdateAll();
                    TTfUpdNow = now;
                    
                }
            }
            if(TTP2) {   // Reentry - up to us

                if(now - TTfUpdNow > P2_ALARM_DELAY) {

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
            // TT triggered by button (if TCD not connected), MQTT or IR ******************
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
                    
                    if(TTFIntL && (now - TTfUpdLNow >= TTFIntL)) {
                        int t = gauges.getValuePercent(0);
                        if(t >= left_gauge_empty + TTStepL) t -= TTStepL;
                        else t = left_gauge_empty;
                        gauges.setValuePercent(0, t);
                        TTfUpdLNow = now;
                        doUpd = true;
                    }
                    if(TTFIntC && (now - TTfUpdCNow >= TTFIntC)) {
                        int t = gauges.getValuePercent(1);
                        if(t >= center_gauge_empty + TTStepC) t -= TTStepC;
                        else t = center_gauge_empty;
                        gauges.setValuePercent(1, t);
                        TTfUpdCNow = now;
                        doUpd = true;
                    }
                    if(TTFIntR && (now - TTfUpdRNow >= TTFIntR)) {
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
                    gauges.setValuePercent(0, left_gauge_empty);
                    gauges.setValuePercent(1, center_gauge_empty);
                    gauges.setValuePercent(2, right_gauge_empty);
                    gauges.UpdateAll();
                    TTfUpdNow = now;
                    
                }
            }
            
            if(TTP2) {   // Reentry - up to us

                if(now - TTfUpdNow > P2_ALARM_DELAY) {

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

    // Execute remote commands from TCD
    if(FPBUnitIsOn) {
        execute_remote_command();
    }

    // Wake up on GPS/RotEnc speed changes
    if(gpsSpeed != oldGpsSpeed) {
        if(FPBUnitIsOn && !TTrunning && spdIsRotEnc && gpsSpeed >= 0) {
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
            lastBTTFNpacket = 0;
            BTTFNBootTO = true;
        }
    }

    if(networkAlarm && !TTrunning && !startup && !startAlarm && !refill && !refillWA) {
        //bool pE = playingEmpty && !playingEmptyEnds;
        networkAlarm = false;
        if(atoi(settings.playALsnd) > 0) {
            play_file("/alarm.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0);
            //if(pE) {    // would be async
            //    append_file("/empty.wav", PA_LOOP|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL|PA_WAV|PA_ISEMPTY, 0.5);
            //}
        }
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

    flushDelayedSave();
    
    if(!TCDtriggered) {
        // If "empty", button-triggered TT is refused (?)
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
        
    TTrunning = true;
    TTstart = TTfUpdNow = millis();
    TTP0 = true;   // phase 0
    TTP1 = TTP2 = false;

    // P1Dur, even if coming from TCD, is not used for timing, 
    // but only to calculate steps.
    if(!P1Dur) {
        P1Dur = TCDtriggered ? 6600 : P1_DUR;
    }
    TTStepL = TTStepC = TTStepR = 1;          // Stepping is 1
    TTFIntL = gauges.getValuePercent(0);
    if(TTFIntL < left_gauge_empty + (TTStepL * 2)) TTFIntL = 0;
    else {
        TTFIntL = P1Dur / (((TTFIntL - left_gauge_empty) / TTStepL) + 1);
    }
    TTFIntC = gauges.getValuePercent(1);
    if(TTFIntC < center_gauge_empty + (TTStepC * 2)) TTFIntC = 0;
    else {
        TTFIntC = P1Dur / (((TTFIntC - center_gauge_empty) / TTStepC) + 1);
    }
    TTFIntR = gauges.getValuePercent(2);
    if(TTFIntR < right_gauge_empty + (TTStepR * 2)) TTFIntR = 0;
    else {
        TTFIntR = P1Dur / (((TTFIntR - right_gauge_empty) / TTStepR) + 1);
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
}

// Wakeup: Sent by TCD upon entering dest date,
// return from tt, triggering delayed tt via ETT
// For audio-visually synchronized behavior
// Also called when GPS/RotEnc speed is changed
void wakeup()
{

    // End screen saver
    ssEnd();
    
    // Anything else?
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

static void execute_remote_command()
{
    uint32_t command = commandQueue[oCmdIdx];
    bool pE = playingEmpty && !playingEmptyEnds;

    // We are only called if ON
    // No command execution during time travel
    // No command execution during timed sequences
    // ssActive checked by individual command

    if(!command || TTrunning || startup || startAlarm || refill || refillWA)
        return;

    commandQueue[oCmdIdx] = 0;
    oCmdIdx++;
    oCmdIdx &= 0x0f;

    if(command < 10) {                                // 900x
        switch(command) {
        case 2:
            if(haveMusic) {
                mp_prev(mpActive);
            }
            break;
        case 3:
            play_file("/key3.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
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
            play_file("/key6.mp3", PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
            //if(pE) append_empty();    // would be async
            break;
        case 8:
            if(haveMusic) {
                mp_next(mpActive);
            }
            break;
        }
      
    } else if(command < 100) {                        // 90xx

        if(command == 90) {

            flushDelayedSave();
            say_ip_address();
          
        } else if(command >= 50 && command <= 59) {
            if(haveSD) {
                bool wasActive = false;
                bool waitShown = false;
                musFolderNum = (int)command - 50;
                // Initializing the MP can take a while;
                // need to stop all audio before calling
                // mp_init()
                if(haveMusic && mpActive) {
                    mp_stop();
                    wasActive = true;
                }
                stopAudio();
                if(mp_checkForFolder(musFolderNum) == -1) {
                    flushDelayedSave();
                    showWaitSequence();
                    waitShown = true;
                    play_file("/renaming.mp3", PA_INTRMUS|PA_ALLOWSD);
                    waitAudioDone();
                }
                saveMusFoldNum();
                mp_init(false);
                if(waitShown) {
                    endWaitSequence();
                }
                //if(pE) append_empty(); // would be async
            }
        }
        
    } else if (command < 1000) {                      // 9xxx

        if(command >= 100 && command <= 199) {    

            ssEnd();
            command -= 100;                       // 100-199: Set "full" percentage of "Primary" gauge
            if(!command) command = DEF_L_GAUGE_IDLE;
            if(command > left_gauge_empty) {
                left_gauge_idle = command;
                if(!emptyAlarm) {
                    gauges.setValuePercent(0, left_gauge_idle);
                    gauges.UpdateAll();
                }
            }
            ssRestartTimer();
          
        } else if(command >= 400 && command <= 499) {

            ssEnd();
            command -= 400;                       // 400-499: Set "full" percentage of "Power percent" gauge
            if(!command) command = DEF_C_GAUGE_IDLE;
            if(command > center_gauge_empty) {
                center_gauge_idle = command;
                if(!emptyAlarm) {
                    gauges.setValuePercent(1, center_gauge_idle);
                    gauges.UpdateAll();
                }
            }
            ssRestartTimer();
          
        } else if(command >= 700 && command <= 799) {

            ssEnd();
            command -= 700;                       // 700-799: Set "full" percentage of "roentgens" gauge
            if(!command) command = DEF_R_GAUGE_IDLE;
            if(command > right_gauge_empty) {
                right_gauge_idle = command;
                if(!emptyAlarm) {
                    gauges.setValuePercent(2, right_gauge_idle);
                    gauges.UpdateAll();
                }
            }
            ssRestartTimer();

        } else if(command >= 300 && command <= 399) {

            command -= 300;                       // 300-319/399: Set fixed volume level
            if(command == 99) {
                curSoftVol = 255;
                volchanged = true;
                updateConfigPortalVolValues();
            } else if(command <= 19) {
                curSoftVol = command;
                volchanged = true;
                updateConfigPortalVolValues();
            }
            volchgnow = millis();

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
        
    } else {
      
        switch(command) {
        case 64738:                               // 64738: reboot
            allOff();
            mp_stop();
            stopAudio();
            flushDelayedSave();
            unmount_fs();
            delay(500);
            esp_restart();
            break;
        case 123456:
            flushDelayedSave();
            deleteIpSettings();                   // 123456: deletes IP settings
            settings.appw[0] = 0;                 // and clears AP mode WiFi password
            write_settings();
            ssRestartTimer();
            break;
        case 317931:                              // 317931: Unlock Gauge Type selection in Config Portal
            if(gaugeTypeLocked) {
                gaugeTypeLocked = false;
                updateConfigPortalValues();
            }
            ssRestartTimer();
            break;
        case 1000000:
            refill_plutonium();
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
    char numfname[8] = "/x.mp3";
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

static void sideSwitchLongPress()
{
    isSSwitchPressed = true;
    isSSwitchChange = true;
}

static void sideSwitchLongPressStop()
{
    isSSwitchPressed = false;
    isSSwitchChange = true;
}

#ifdef DG_HAVEDOORSWITCH
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
#endif

static void ttkeyScan()
{
    TTKey.scan();
    sideSwitch.scan();
}

#ifdef DG_HAVEDOORSWITCH
static void dsScan()
{
    if(dsPlay) doorSwitch.scan();
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
    bttfn_loop();
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

static void addCmdQueue(uint32_t command)
{
    if(!command) return;

    #ifdef DG_DBG
    Serial.printf("Remote command %d added to queue\n", command);
    #endif

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
        #ifdef BTTFN_MC
        tcdHostNameHash = 0;
        unsigned char *s = (unsigned char *)settings.tcdIP;
        for ( ; *s; ++s) tcdHostNameHash = 37 * tcdHostNameHash + tolower(*s);
        #else
        return;
        #endif
    } else {
        bttfnTcdIP.fromString(settings.tcdIP);
    }
    
    dgUDP = &bttfUDP;
    dgUDP->begin(BTTF_DEFAULT_LOCAL_PORT);
    BTTFNfailCount = 0;
    useBTTFN = true;
}

void bttfn_loop()
{
    if(!useBTTFN)
        return;
        
    BTTFNCheckPacket();
    
    if(!BTTFNPacketDue) {
        // If WiFi status changed, trigger immediately
        if(!BTTFNWiFiUp && (WiFi.status() == WL_CONNECTED)) {
            BTTFNUpdateNow = 0;
        }
        if((!BTTFNUpdateNow) || (millis() - BTTFNUpdateNow > 1100)) {
            BTTFNTriggerUpdate();
        }
    }
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

    // Basic validity check
    if(memcmp(BTTFUDPBuf, BTTFUDPHD, 4))
        return;

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    if(BTTFUDPBuf[BTTF_PACKET_SIZE - 1] != a)
        return;

    if(BTTFUDPBuf[4] == (BTTFN_VERSION | 0x40)) {

        // A notification from the TCD

        switch(BTTFUDPBuf[5]) {
        case BTTFN_NOT_PREPARE:
            // Prepare for TT. Comes at some undefined point,
            // an undefined time before the actual tt, and
            // may not come at all.
            // We disable our Screen Saver
            // We don't ignore this if TCD is connected by wire,
            // because this signal does not come via wire.
            if(!TTrunning) {
                prepareTT();
            }
            break;
        case BTTFN_NOT_TT:
            // Trigger Time Travel (if not running already)
            // Ignore command if TCD is connected by wire
            if(!TCDconnected && !TTrunning) {
                networkTimeTravel = true;
                networkTCDTT = true;
                networkReentry = false;
                networkAbort = false;
                networkLead = BTTFUDPBuf[6] | (BTTFUDPBuf[7] << 8);
                networkP1 = BTTFUDPBuf[8] | (BTTFUDPBuf[9] << 8);
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
            // Eval this at our convenience
            break;
        case BTTFN_NOT_REFILL:
            addCmdQueue(1000000);
            break;
        case BTTFN_NOT_PCG_CMD:
            addCmdQueue( BTTFUDPBuf[6] | (BTTFUDPBuf[7] << 8) |
                        (BTTFUDPBuf[8] | (BTTFUDPBuf[9] << 8)) << 16);
            break;
        case BTTFN_NOT_WAKEUP:
            if(!TTrunning) {
                wakeup();
            }
            break;
        }
      
    } else {

        // (Possibly) a response packet
    
        if(*((uint32_t *)(BTTFUDPBuf + 6)) != BTTFUDPID)
            return;
    
        // Response marker missing or wrong version, bail
        if(BTTFUDPBuf[4] != (BTTFN_VERSION | 0x80))
            return;

        BTTFNfailCount = 0;
    
        // If it's our expected packet, no other is due for now
        BTTFNPacketDue = false;

        #ifdef BTTFN_MC
        if(BTTFUDPBuf[5] & 0x80) {
            if(!haveTCDIP) {
                bttfnTcdIP = dgUDP->remoteIP();
                haveTCDIP = true;
                #ifdef SID_DBG
                Serial.printf("Discovered TCD IP %d.%d.%d.%d\n", bttfnTcdIP[0], bttfnTcdIP[1], bttfnTcdIP[2], bttfnTcdIP[3]);
                #endif
            } else {
                #ifdef SID_DBG
                Serial.println("Internal error - received unexpected DISCOVER response");
                #endif
            }
        }
        #endif

        if(BTTFUDPBuf[5] & 0x02) {
            gpsSpeed = (int16_t)(BTTFUDPBuf[18] | (BTTFUDPBuf[19] << 8));
            if(gpsSpeed > 88) gpsSpeed = 88;
            spdIsRotEnc = (BTTFUDPBuf[26] & 0x80) ? true : false;
        }
        if(BTTFUDPBuf[5] & 0x10) {
            tcdNM  = (BTTFUDPBuf[26] & 0x01) ? true : false;
            tcdFPO = (BTTFUDPBuf[26] & 0x02) ? true : false;   // 1 means fake power off
        } else {
            tcdNM = false;
            tcdFPO = false;
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

static void BTTFNSendPacket()
{   
    memset(BTTFUDPBuf, 0, BTTF_PACKET_SIZE);

    // ID
    memcpy(BTTFUDPBuf, BTTFUDPHD, 4);

    // Serial
    *((uint32_t *)(BTTFUDPBuf + 6)) = BTTFUDPID = (uint32_t)millis();

    // Tell the TCD about our hostname (0-term., 13 bytes total)
    strncpy((char *)BTTFUDPBuf + 10, settings.hostName, 12);
    BTTFUDPBuf[10+12] = 0;

    BTTFUDPBuf[10+13] = BTTFN_TYPE_PCG;

    BTTFUDPBuf[4] = BTTFN_VERSION;  // Version
    BTTFUDPBuf[5] = 0x12;           // Request status and GPS speed

    #ifdef BTTFN_MC
    if(!haveTCDIP) {
        BTTFUDPBuf[5] |= 0x80;
        memcpy(BTTFUDPBuf + 31, (void *)&tcdHostNameHash, 4);
    }
    #endif

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;
    
    #ifdef BTTFN_MC
    if(haveTCDIP) {
    #endif  
        dgUDP->beginPacket(bttfnTcdIP, BTTF_DEFAULT_LOCAL_PORT);
    #ifdef BTTFN_MC    
    } else {
        #ifdef SID_DBG
        Serial.printf("Sending multicast (hostname hash %x)\n", tcdHostNameHash);
        #endif
        dgUDP->beginPacket("224.0.0.224", BTTF_DEFAULT_LOCAL_PORT + 1);
    }
    #endif
    dgUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
    dgUDP->endPacket();
}

static bool BTTFNTriggerTT()
{
    if(!useBTTFN)
        return false;

    #ifdef BTTFN_MC
    if(!haveTCDIP)
        return false;
    #endif

    if(WiFi.status() != WL_CONNECTED)
        return false;

    if(!lastBTTFNpacket)
        return false;

    if(TCDconnected || TTrunning)
        return false;

    memset(BTTFUDPBuf, 0, BTTF_PACKET_SIZE);

    // ID
    memcpy(BTTFUDPBuf, BTTFUDPHD, 4);

    // Tell the TCD about our hostname (0-term., 13 bytes total)
    strncpy((char *)BTTFUDPBuf + 10, settings.hostName, 12);
    BTTFUDPBuf[10+12] = 0;

    BTTFUDPBuf[10+13] = BTTFN_TYPE_PCG;

    BTTFUDPBuf[4] = BTTFN_VERSION;  // Version
    BTTFUDPBuf[5] = 0x80;           // Trigger BTTFN-wide TT

    uint8_t a = 0;
    for(int i = 4; i < BTTF_PACKET_SIZE - 1; i++) {
        a += BTTFUDPBuf[i] ^ 0x55;
    }
    BTTFUDPBuf[BTTF_PACKET_SIZE - 1] = a;
        
    dgUDP->beginPacket(bttfnTcdIP, BTTF_DEFAULT_LOCAL_PORT);
    dgUDP->write(BTTFUDPBuf, BTTF_PACKET_SIZE);
    dgUDP->endPacket();

    return true;
}
