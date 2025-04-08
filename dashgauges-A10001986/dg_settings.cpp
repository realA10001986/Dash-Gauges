/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Settings & file handling
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

#define ARDUINOJSON_USE_LONG_LONG 0
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_NAN 0
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#else
#define SPIFFS LittleFS
#include <LittleFS.h>
#endif

#include "dg_settings.h"
#include "dg_audio.h"
#include "dg_main.h"
#include "dgdisplay.h"

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2000
#if ARDUINOJSON_VERSION_MAJOR >= 7
#error "ArduinoJSON v7 not supported"
#define DECLARE_S_JSON(x,n) JsonDocument n;
#define DECLARE_D_JSON(x,n) JsonDocument n;
#else
#define DECLARE_S_JSON(x,n) StaticJsonDocument<x> n;
#define DECLARE_D_JSON(x,n) DynamicJsonDocument n(x);
#endif 

#define NUM_AUDIOFILES 11+9
#define SND_REQ_VERSION "DG02"
#define AC_FMTV 2
#define AC_TS   402225
#define AC_OHSZ (14 + ((NUM_AUDIOFILES+1)*(32+4)))

static const char *CONFN  = "/DGA.bin";
static const char *CONFND = "/DGA.old";
static const char *CONID  = "DGAA";
static uint32_t   soa = AC_TS;
static bool       ic = false;
static uint8_t*   f(uint8_t *d, uint32_t m, int y) { return d; }

static const char *cfgName    = "/dgconfig.json";   // Main config (flash)
static const char *ipCfgName  = "/dgipcfg.json";    // IP config (flash)
static const char *volCfgName = "/dgvolcfg.json";   // Volume config (flash/SD)
static const char *musCfgName = "/dgmcfg.json";     // Music config (SD)

static const char *fsNoAvail = "File System not available";
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
static const char *failFileWrite = "Failed to open file for writing";

/* If SPIFFS/LittleFS is mounted */
bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* Save secondary settings on SD? */
static bool configOnSD = false;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

/* If SD contains default audio files */
static bool allowCPA = false;

/* If current audio data is installed */
bool haveAudioFiles = false;

/* Music Folder Number */
uint8_t musFolderNum = 0;

/* Cache */
static uint8_t prevSavedVol = 255;

static uint8_t*  (*r)(uint8_t *, uint32_t, int);

static bool read_settings(File configFile);

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool copy_audio_files(bool& delIDfile);
static void open_and_copy(const char *fn, int& haveErr, int& haveWriteErr);
static bool filecopy(File source, File dest, int& haveWriteErr);
static void cfc(File& sfile, bool doCopy, int& haveErr, int& haveWriteErr);

static bool audio_files_present();

static void formatFlashFS();
static void rewriteSecondarySettings();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName);
static bool writeFileToSD(const char *fn, uint8_t *buf, int len);
static bool writeFileToFS(const char *fn, uint8_t *buf, int len);

/*
 * settings_setup()
 * 
 * Mount SPIFFS/LittleFS and SD (if available).
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 *
 */
void settings_setup()
{
    const char *funcName = "settings_setup";
    bool writedefault = false;
    bool SDres = false;

    // Pre-maturely use TT button and side switch (initialized again later)
    pinMode(TT_IN_PIN, INPUT);
    if(!sbv2) {
        pinMode(SIDESWITCH_PIN, INPUT);
    }
    delay(20);

    #ifdef DG_DBG
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(SPIFFS.begin()) {

        haveFS = true;

    } else {

        #ifdef DG_DBG
        Serial.print(F("failed, formatting... "));
        #endif

        // Show the user some action
        showWaitSequence();

        SPIFFS.format();
        if(SPIFFS.begin()) haveFS = true;

        endWaitSequence();

    }

    if(haveFS) {
      
        #ifdef DG_DBG
        Serial.println(F("ok, loading settings"));
        #endif
        
        if(SPIFFS.exists(cfgName)) {
            File configFile = SPIFFS.open(cfgName, "r");
            if(configFile) {
                writedefault = read_settings(configFile);
                configFile.close();
            } else {
                writedefault = true;
            }
        } else {
            writedefault = true;
        }

        // Write new config file after mounting SD and determining FlashROMode

    } else {

        Serial.println(F("failed.\nThis is very likely a hardware problem."));
        Serial.println(F("*** Since the internal storage is unavailable, all settings/states will be stored on"));
        Serial.println(F("*** the SD card (if available)")); 
    }

    // Set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;

    uint32_t sdfreq = (settings.sdFreq[0] == '0') ? 16000000 : 4000000;
    #ifdef DG_DBG
    Serial.printf("%s: SD/SPI frequency %dMHz\n", funcName, sdfreq / 1000000);
    #endif

    #ifdef DG_DBG
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    if(!(SDres = SD.begin(SD_CS_PIN, SPI, sdfreq))) {
        #ifdef DG_DBG
        Serial.printf("Retrying at 25Mhz... ");
        #endif
        SDres = SD.begin(SD_CS_PIN, SPI, 25000000);
    }

    if(SDres) {

        #ifdef DG_DBG
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef DG_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    } else {
      
        Serial.println(F("no SD card found"));
        
    }

    if(haveSD) {
        if(SD.exists("/DG_FLASH_RO") || !haveFS) {
            bool writedefault2 = false;
            FlashROMode = true;
            Serial.println(F("Flash-RO mode: All settings/states stored on SD. Reloading settings."));
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile);
                    configFile.close();
                } else {
                    writedefault2 = true;
                }
            } else {
                writedefault2 = true;
            }
            if(writedefault2) {
                Serial.printf("%s: %s\n", funcName, badConfig);
                write_settings();
            }
        }
    }
   
    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, badConfig);
        write_settings();
    }

    // Determine if secondary settings are to be stored on SD
    configOnSD = ((r = m) && haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));

    // Check if (current) audio data is installed
    haveAudioFiles = audio_files_present();
    
    // Check if SD contains our default sound files
    if(haveSD && (haveFS || FlashROMode)) {
        allowCPA = check_if_default_audio_present();
    }

    // Allow user to delete static IP data by holding time travel
    // while booting and switching side switch twice within 10 seconds
    if(digitalRead(TT_IN_PIN)) {
        delay(50);
        if(digitalRead(TT_IN_PIN)) {

            unsigned long mnow = millis();
            bool ssState, newSSState;
            int ssCount = 0;
            int ledpin = sbv2 ? EMPTY_LED_PIN2 : EMPTY_LED_PIN;

            if(sbv2) {
                ssState = (read_port() & (1 << SIDESWITCH_PIN)) ? false : true;
            } else {
                ssState = digitalRead(SIDESWITCH_PIN);
            }

            // Pre-maturely use empty led
            pinMode(ledpin, OUTPUT);
            digitalWrite(ledpin, HIGH);
            delay(200);
            digitalWrite(ledpin, LOW);

            while(1) {
                if((ssCount == 2) || (millis() - mnow > 10*1000)) break;

                if(sbv2) {

                    if((!!(read_port() & (1 << SIDESWITCH_PIN))) != ssState) {
                        delay(50);
                        if((newSSState = (!!(read_port() & (1 << SIDESWITCH_PIN)))) != ssState) {
                            ssCount++;
                            ssState = newSSState;
                        }
                    } else {
                        delay(50);
                    }

                } else {
                  
                    if(digitalRead(SIDESWITCH_PIN) != ssState) {
                        delay(50);
                        if((newSSState = digitalRead(SIDESWITCH_PIN)) != ssState) {
                            ssCount++;
                            ssState = newSSState;
                        }
                    } else {
                        delay(50);
                    }

                }
                
            }

            if(ssCount == 2) {
    
                Serial.printf("%s: Deleting ip config; temporarily clearing AP mode WiFi password\n", funcName);
    
                deleteIpSettings();
    
                // Set AP mode password to empty (not written, only until reboot!)
                settings.appw[0] = 0;
    
                digitalWrite(ledpin, HIGH);
                while(digitalRead(TT_IN_PIN)) {  }
                digitalWrite(ledpin, LOW);
            }
        }
    }
}

void unmount_fs()
{
    if(haveFS) {
        SPIFFS.end();
        #ifdef DG_DBG
        Serial.println(F("Unmounted Flash FS"));
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef DG_DBG
        Serial.println(F("Unmounted SD card"));
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    DECLARE_D_JSON(JSON_SIZE,json);
    /*
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    */
    
    DeserializationError error = readJSONCfgFile(json, configFile, funcName);

    #if ARDUINOJSON_VERSION_MAJOR < 7
    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("%s: ERROR: Config file too large (%d vs %d), memory corrupted, awaiting doom.\n", funcName, jsonSize, JSON_SIZE);
    }
    
    #ifdef DG_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif
    #endif

    if(!error) {

        wd |= CopyCheckValidNumParm(json["aRef"], settings.autoRefill, sizeof(settings.autoRefill), 0, 360, DEF_AUTO_REFILL);
        wd |= CopyCheckValidNumParm(json["aMut"], settings.autoMute, sizeof(settings.autoMute), 0, 360, DEF_AUTO_MUTE);
        wd |= CopyCheckValidNumParm(json["ssTimer"], settings.ssTimer, sizeof(settings.ssTimer), 0, 999, DEF_SS_TIMER);
        
        if(json["hostName"]) {
            memset(settings.hostName, 0, sizeof(settings.hostName));
            strncpy(settings.hostName, json["hostName"], sizeof(settings.hostName) - 1);
        } else wd = true;
        if(json["systemID"]) {
            memset(settings.systemID, 0, sizeof(settings.systemID));
            strncpy(settings.systemID, json["systemID"], sizeof(settings.systemID) - 1);
        } else wd = true;

        if(json["appw"]) {
            memset(settings.appw, 0, sizeof(settings.appw));
            strncpy(settings.appw, json["appw"], sizeof(settings.appw) - 1);
        } else wd = true;
        
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);

        wd |= CopyCheckValidNumParm(json["TCDpresent"], settings.TCDpresent, sizeof(settings.TCDpresent), 0, 1, DEF_TCD_PRES);
        wd |= CopyCheckValidNumParm(json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);

        if(json["tcdIP"]) {
            memset(settings.tcdIP, 0, sizeof(settings.tcdIP));
            strncpy(settings.tcdIP, json["tcdIP"], sizeof(settings.tcdIP) - 1);
        } else wd = true;
        //wd |= CopyCheckValidNumParm(json["useGPSS"], settings.useGPSS, sizeof(settings.useGPSS), 0, 1, DEF_USE_GPSS);
        wd |= CopyCheckValidNumParm(json["useNM"], settings.useNM, sizeof(settings.useNM), 0, 1, DEF_USE_NM);
        wd |= CopyCheckValidNumParm(json["useFPO"], settings.useFPO, sizeof(settings.useFPO), 0, 1, DEF_USE_FPO);
        wd |= CopyCheckValidNumParm(json["bttfnTT"], settings.bttfnTT, sizeof(settings.bttfnTT), 0, 1, DEF_BTTFN_TT);

        wd |= CopyCheckValidNumParm(json["lIdle"], settings.lIdle, sizeof(settings.lIdle), 0, 100, DEF_L_GAUGE_IDLE);
        wd |= CopyCheckValidNumParm(json["cIdle"], settings.cIdle, sizeof(settings.cIdle), 0, 100, DEF_C_GAUGE_IDLE);
        wd |= CopyCheckValidNumParm(json["rIdle"], settings.rIdle, sizeof(settings.rIdle), 0, 100, DEF_R_GAUGE_IDLE);
        wd |= CopyCheckValidNumParm(json["lEmpty"], settings.lEmpty, sizeof(settings.lEmpty), 0, 100, DEF_L_GAUGE_EMPTY);
        wd |= CopyCheckValidNumParm(json["cEmpty"], settings.cEmpty, sizeof(settings.cEmpty), 0, 100, DEF_C_GAUGE_EMPTY);
        wd |= CopyCheckValidNumParm(json["rEmpty"], settings.rEmpty, sizeof(settings.rEmpty), 0, 100, DEF_R_GAUGE_EMPTY);

        wd |= CopyCheckValidNumParm(json["drPri"], settings.drPri, sizeof(settings.drPri), 0, 1, DEF_DR_PRI);
        wd |= CopyCheckValidNumParm(json["drPPo"], settings.drPPo, sizeof(settings.drPPo), 0, 1, DEF_DR_PPO);
        wd |= CopyCheckValidNumParm(json["drRoe"], settings.drRoe, sizeof(settings.drRoe), 0, 1, DEF_DR_ROE);

        wd |= CopyCheckValidNumParm(json["lTh"], settings.lThreshold, sizeof(settings.lThreshold), 0, 99, 0);
        wd |= CopyCheckValidNumParm(json["cTh"], settings.cThreshold, sizeof(settings.cThreshold), 0, 99, 0);
        wd |= CopyCheckValidNumParm(json["rTh"], settings.rThreshold, sizeof(settings.rThreshold), 0, 99, 0);

        wd |= CopyCheckValidNumParm(json["playALsnd"], settings.playALsnd, sizeof(settings.playALsnd), 0, 1, DEF_PLAY_ALM_SND);

        #ifdef DG_HAVEDOORSWITCH
        wd |= CopyCheckValidNumParm(json["dsPlay"], settings.dsPlay, sizeof(settings.dsPlay), 0, 1, DEF_DS_PLAY);
        wd |= CopyCheckValidNumParm(json["dsCOnC"], settings.dsCOnC, sizeof(settings.dsCOnC), 0, 1, DEF_DS_NC);
        wd |= CopyCheckValidNumParm(json["dsDelay"], settings.dsDelay, sizeof(settings.dsDelay), 0, 5000, DEF_DS_DELAY);
        #endif
        
        #ifdef DG_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        if(json["mqttServer"]) {
            memset(settings.mqttServer, 0, sizeof(settings.mqttServer));
            strncpy(settings.mqttServer, json["mqttServer"], sizeof(settings.mqttServer) - 1);
        } else wd = true;
        if(json["mqttUser"]) {
            memset(settings.mqttUser, 0, sizeof(settings.mqttUser));
            strncpy(settings.mqttUser, json["mqttUser"], sizeof(settings.mqttUser) - 1);
        } else wd = true;
        #endif

        wd |= CopyCheckValidNumParm(json["shuffle"], settings.shuffle, sizeof(settings.shuffle), 0, 1, DEF_SHUFFLE);
        
        wd |= CopyCheckValidNumParm(json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);

        wd |= CopyCheckValidNumParm(json["gaugeIDA"], settings.gaugeIDA, sizeof(settings.gaugeIDA), 0, gauges.max_id_small, DEF_GAUGE_TYPE);
        wd |= CopyCheckValidNumParm(json["gaugeIDB"], settings.gaugeIDB, sizeof(settings.gaugeIDB), 0, gauges.max_id_small, DEF_GAUGE_TYPE);
        wd |= CopyCheckValidNumParm(json["gaugeIDC"], settings.gaugeIDC, sizeof(settings.gaugeIDC), 0, gauges.max_id_large, DEF_GAUGE_TYPE);

    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    const char *funcName = "write_settings";
    DECLARE_D_JSON(JSON_SIZE,json);
    /*
    DynamicJsonDocument json(JSON_SIZE);
    //StaticJsonDocument<JSON_SIZE> json;
    */

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef DG_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    json["aRef"] = (const char *)settings.autoRefill;
    json["aMut"] = (const char *)settings.autoMute;
    json["ssTimer"] = (const char *)settings.ssTimer;
    
    json["hostName"] = (const char *)settings.hostName;
    json["systemID"] = (const char *)settings.systemID;
    json["appw"] = (const char *)settings.appw;
    json["wifiConRetries"] = (const char *)settings.wifiConRetries;
    json["wifiConTimeout"] = (const char *)settings.wifiConTimeout;

    json["TCDpresent"] = (const char *)settings.TCDpresent;
    json["noETTOLead"] = (const char *)settings.noETTOLead;

    json["tcdIP"] = (const char *)settings.tcdIP;
    //json["useGPSS"] = (const char *)settings.useGPSS;
    json["useNM"] = (const char *)settings.useNM;
    json["useFPO"] = (const char *)settings.useFPO;
    json["bttfnTT"] = (const char *)settings.bttfnTT;

    json["lIdle"] = (const char *)settings.lIdle;
    json["cIdle"] = (const char *)settings.cIdle;
    json["rIdle"] = (const char *)settings.rIdle;
    json["lEmpty"] = (const char *)settings.lEmpty;
    json["cEmpty"] = (const char *)settings.cEmpty;
    json["rEmpty"] = (const char *)settings.rEmpty;
    json["drPri"] =  (const char *)settings.drPri;
    json["drPPo"] =  (const char *)settings.drPPo;
    json["drRoe"] =  (const char *)settings.drRoe;

    json["lTh"] = (const char *)settings.lThreshold;
    json["cTh"] = (const char *)settings.cThreshold;
    json["rTh"] = (const char *)settings.rThreshold;

    json["playALsnd"] = (const char *)settings.playALsnd;

    #ifdef DG_HAVEDOORSWITCH
    json["dsPlay"] = (const char *)settings.dsPlay;
    json["dsCOnC"] = (const char *)settings.dsCOnC;
    json["dsDelay"] = (const char *)settings.dsDelay;
    #endif
    
    #ifdef DG_HAVEMQTT
    json["useMQTT"] = (const char *)settings.useMQTT;
    json["mqttServer"] = (const char *)settings.mqttServer;
    json["mqttUser"] = (const char *)settings.mqttUser;
    #endif

    json["shuffle"] = (const char *)settings.shuffle;
    
    json["CfgOnSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;

    json["gaugeIDA"] = (const char *)settings.gaugeIDA;
    json["gaugeIDB"] = (const char *)settings.gaugeIDB;
    json["gaugeIDC"] = (const char *)settings.gaugeIDC;

    writeJSONCfgFile(json, cfgName, FlashROMode, funcName);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && SPIFFS.exists(cfgName));
}


/*
 *  Helpers for parm copying & checking
 */

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParm(text, lowerLim, upperLim, setDefault);
}

static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
}

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);

    if(len == 0) {
        sprintf(text, "%d", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] < '0' || text[i] > '9') {
            sprintf(text, "%d", setDefault);
            return true;
        }
    }

    i = atoi(text);

    if(i < lowerLim) {
        sprintf(text, "%d", lowerLim);
        return true;
    }
    if(i > upperLim) {
        sprintf(text, "%d", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "000")
    sprintf(text, "%d", i);

    return false;
}

static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault)
{
    int i, len = strlen(text);
    float f;

    if(len == 0) {
        sprintf(text, "%.1f", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
            sprintf(text, "%.1f", setDefault);
            return true;
        }
    }

    f = atof(text);

    if(f < lowerLim) {
        sprintf(text, "%.1f", lowerLim);
        return true;
    }
    if(f > upperLim) {
        sprintf(text, "%.1f", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%.1f", f);

    return false;
}

static bool openCfgFileRead(const char *fn, File& f, bool SDonly = false)
{
    bool haveConfigFile = false;
    
    if(configOnSD || SDonly) {
        if(SD.exists(fn)) {
            haveConfigFile = (f = SD.open(fn, "r"));
        }
    } 
    if(!haveConfigFile && !SDonly && haveFS) {
        if(SPIFFS.exists(fn)) {
            haveConfigFile = (f = SPIFFS.open(fn, "r"));
        }
    }

    return haveConfigFile;
}

/*
 *  Load/save the Volume
 */

#ifdef DG_HAVEVOLKNOB
#define T_V_MAX 255
#else
#define T_V_MAX 19
#endif

bool loadCurVolume()
{
    const char *funcName = "loadCurVolume";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        #ifdef DG_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    #ifdef DG_DBG
    Serial.printf("%s: Loading from %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    if(openCfgFileRead(volCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, funcName)) {
            if(!CopyCheckValidNumParm(json["volume"], temp, sizeof(temp), 0, T_V_MAX, DEFAULT_VOLUME)) {
                uint8_t ncv = atoi(temp);
                if((ncv >= 0 && ncv <= 19) || ncv == T_V_MAX) {
                    curSoftVol = ncv;
                } 
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    #ifdef DG_DBG
    Serial.printf("%s: Loaded volume %d\n", funcName, curSoftVol);
    #endif

    prevSavedVol = curSoftVol;

    return true;
}

void saveCurVolume(bool useCache)
{
    const char *funcName = "saveCurVolume";
    char buf[6];
    File configFile;
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(useCache && (prevSavedVol == curSoftVol)) {
        #ifdef DG_DBG
        Serial.printf("%s: Prev. saved vol identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(buf, "%d", curSoftVol);
    json["volume"] = (const char *)buf;

    if(writeJSONCfgFile(json, volCfgName, configOnSD, funcName)) {
        prevSavedVol = curSoftVol;
    }
}

#undef T_V_MAX

/*
 * Load/save Music Folder Number
 */

bool loadMusFoldNum()
{
    bool writedefault = true;
    char temp[4];

    if(!haveSD)
        return false;

    if(SD.exists(musCfgName)) {

        File configFile = SD.open(musCfgName, "r");
        if(configFile) {
            DECLARE_S_JSON(512,json);
            //StaticJsonDocument<512> json;
            if(!readJSONCfgFile(json, configFile, "loadMusFoldNum")) {
                if(!CopyCheckValidNumParm(json["folder"], temp, sizeof(temp), 0, 9, 0)) {
                    musFolderNum = atoi(temp);
                    writedefault = false;
                }
            } 
            configFile.close();
        }

    }

    if(writedefault) {
        musFolderNum = 0;
        saveMusFoldNum();
    }

    return true;
}

void saveMusFoldNum()
{
    const char *funcName = "saveMusFoldNum";
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;
    char buf[4];

    if(!haveSD)
        return;

    sprintf(buf, "%1d", musFolderNum);
    json["folder"] = (const char *)buf;

    writeJSONCfgFile(json, musCfgName, true, funcName);
}

/*
 * Load/save/delete settings for static IP configuration
 */

bool loadIpSettings()
{
    bool invalid = false;
    bool haveConfig = false;

    if(!haveFS && !FlashROMode)
        return false;

    if( (!FlashROMode && SPIFFS.exists(ipCfgName)) ||
        (FlashROMode && SD.exists(ipCfgName)) ) {

        File configFile = FlashROMode ? SD.open(ipCfgName, "r") : SPIFFS.open(ipCfgName, "r");

        if(configFile) {

            DECLARE_S_JSON(512,json);
            //StaticJsonDocument<512> json;
            
            DeserializationError error = readJSONCfgFile(json, configFile, "loadIpSettings");

            #ifdef DG_DBG
            serializeJson(json, Serial);
            Serial.println(F(" "));
            #endif

            if(!error) {

                invalid |= CopyIPParm(json["IpAddress"], ipsettings.ip, sizeof(ipsettings.ip));
                invalid |= CopyIPParm(json["Gateway"], ipsettings.gateway, sizeof(ipsettings.gateway));
                invalid |= CopyIPParm(json["Netmask"], ipsettings.netmask, sizeof(ipsettings.netmask));
                invalid |= CopyIPParm(json["DNS"], ipsettings.dns, sizeof(ipsettings.dns));
                
                haveConfig = !invalid;

            } else {

                invalid = true;

            }

            configFile.close();

        }

    }

    if(invalid) {

        // config file is invalid - delete it

        Serial.println(F("loadIpSettings: IP settings invalid; deleting file"));

        deleteIpSettings();

        memset(ipsettings.ip, 0, sizeof(ipsettings.ip));
        memset(ipsettings.gateway, 0, sizeof(ipsettings.gateway));
        memset(ipsettings.netmask, 0, sizeof(ipsettings.netmask));
        memset(ipsettings.dns, 0, sizeof(ipsettings.dns));

    }

    return haveConfig;
}

static bool CopyIPParm(const char *json, char *text, uint8_t psize)
{
    if(!json) return true;

    if(strlen(json) == 0) 
        return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return false;
}

void writeIpSettings()
{
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;
    
    json["IpAddress"] = (const char *)ipsettings.ip;
    json["Gateway"]   = (const char *)ipsettings.gateway;
    json["Netmask"]   = (const char *)ipsettings.netmask;
    json["DNS"]       = (const char *)ipsettings.dns;

    writeJSONCfgFile(json, ipCfgName, FlashROMode, "writeIpSettings");
}

void deleteIpSettings()
{
    #ifdef DG_DBG
    Serial.println(F("deleteIpSettings: Deleting ip config"));
    #endif

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else if(haveFS) {
        SPIFFS.remove(ipCfgName);
    }
}

/*
 * Audio file installer
 *
 * Copies our default audio files from SD to flash FS.
 * The is restricted to the original default audio
 * files that came with the software. If you want to
 * customize your sounds, put them on a FAT32 formatted
 * SD card and leave this SD card in the slot.
 */

bool check_allow_CPA()
{
    return allowCPA;
}

static uint32_t getuint32(uint8_t *buf)
{
    uint32_t t = 0;
    for(int i = 3; i >= 0; i--) {
      t <<= 8;
      t += buf[i];
    }
    return t;
}

bool check_if_default_audio_present()
{
    uint8_t dbuf[16];
    File file;
    size_t ts;
    int i;

    ic = false;

    if(!haveSD)
        return false;

    if(SD.exists(CONFN)) {
        if(file = SD.open(CONFN, FILE_READ)) {
            ts = file.size();
            file.read(dbuf, 14);
            file.close();
            if((!memcmp(dbuf, CONID, 4))             && 
               ((*(dbuf+4) & 0x7f) == AC_FMTV)       &&
               (!memcmp(dbuf+5, SND_REQ_VERSION, 4)) &&
               (*(dbuf+9) == (NUM_AUDIOFILES+1))     &&
               (getuint32(dbuf+10) == soa)           &&
               (ts > soa + AC_OHSZ)) {
                ic = true;
                if(!(*(dbuf+4) & 0x80)) r  = f;
            }
        }
    }

    return ic;
}

/*
 * Install default audio files from SD to flash FS #############
 */

bool prepareCopyAudioFiles()
{
    int i, haveErr = 0, haveWriteErr = 0;
    
    if(!ic)
        return true;

    File sfile;
    if(sfile = SD.open(CONFN, FILE_READ)) {
        sfile.seek(14);
        for(i = 0; i < NUM_AUDIOFILES+1; i++) {
           cfc(sfile, false, haveErr, haveWriteErr);
           if(haveErr) break;
        }
        sfile.close();
    } else {
        return false;
    }

    return (haveErr == 0);
}

void doCopyAudioFiles()
{
    bool delIDfile = false;

    if((!copy_audio_files(delIDfile)) && !FlashROMode) {
        // If copy fails because of a write error, re-format flash FS
        formatFlashFS();            // Format
        rewriteSecondarySettings(); // Re-write secondary settings
        #ifdef DG_DBG 
        Serial.println("Re-writing general settings");
        #endif
        write_settings();           // Re-write general settings
        copy_audio_files(delIDfile);// Retry copy
    }

    if(haveSD) {
        SD.remove("/_installing.mp3");
    }

    if(delIDfile) {
        delete_ID_file();
    } else {
        showCopyError();
        mydelay(5000);
    }
    
    mydelay(500);

    flushDelayedSave();

    unmount_fs();
    delay(500);
    
    esp_restart();
}

// Returns false if copy failed because of a write error (which 
//    might be cured by a reformat of the FlashFS)
// Returns true if ok or source error (file missing, read error)
// Sets delIDfile to true if copy fully succeeded
static bool copy_audio_files(bool& delIDfile)
{
    int i, haveErr = 0, haveWriteErr = 0;

    if(!allowCPA) {
        delIDfile = false;
        return true;
    }

    if(ic) {
        File sfile;
        if(sfile = SD.open(CONFN, FILE_READ)) {
            sfile.seek(14);
            for(i = 0; i < NUM_AUDIOFILES+1; i++) {
               cfc(sfile, true, haveErr, haveWriteErr);
               if(haveErr) break;
            }
            sfile.close();
        } else {
            haveErr++;
        }
    } else {
        haveErr++;
    }

    delIDfile = (haveErr == 0);

    return (haveWriteErr == 0);
}

static void cfc(File& sfile, bool doCopy, int& haveErr, int& haveWriteErr)
{
    const char *funcName = "cfc";
    uint8_t buf1[1+32+4];
    uint8_t buf2[1024];
    uint32_t s;
    bool skip = false, tSD = false;
    File dfile;

    buf1[0] = '/';
    sfile.read(buf1 + 1, 32+4);   
    s = getuint32((*r)(buf1 + 1, soa, 32) + 32);
    if(buf1[1] == '_') {
        tSD = true;
        skip = doCopy;
    } else {
        skip = !doCopy;
    }
    if(!skip) {
        if((dfile = (tSD || FlashROMode) ? SD.open((const char *)buf1, FILE_WRITE) : SPIFFS.open((const char *)buf1, FILE_WRITE))) {
            uint32_t t = 1024;
            #ifdef DG_DBG
            Serial.printf("%s: Opened destination file: %s, length %d\n", funcName, (const char *)buf1, s);
            #endif
            while(s > 0) {
                t = (s < t) ? s : t;
                if(sfile.read(buf2, t) != t) {
                    haveErr++;
                    break;
                }
                if(dfile.write((*r)(buf2, soa, t), t) != t) {
                    haveErr++;
                    haveWriteErr++;
                    break;
                }
                s -= t;
            }
        } else {
            haveErr++;
            haveWriteErr++;
            Serial.printf("%s: Error opening destination file: %s\n", funcName, buf1);
        }
    } else {
        #ifdef DG_DBG
        Serial.printf("%s: Skipped file: %s, length %d\n", funcName, (const char *)buf1, s);
        #endif
        sfile.seek(sfile.position() + s);
    }
}

static bool audio_files_present()
{
    File file;
    uint8_t buf[4];
    const char *fn = "/VER";
    
    if(FlashROMode || !haveFS)
        return true;

    if(!SPIFFS.exists(fn))
        return false;
    if(!(file = SPIFFS.open(fn, FILE_READ)))
        return false;
    file.read(buf, 4);
    file.close();

    return (!memcmp(buf, SND_REQ_VERSION, 4));
}

void delete_ID_file()
{
    if(haveSD && ic) {
        SD.remove(CONFND);
        SD.rename(CONFN, CONFND);
    }
}

/*
 * Various helpers
 */

static void formatFlashFS()
{
    #ifdef DG_DBG
    Serial.println(F("Formatting flash FS"));
    #endif
    SPIFFS.format();
}

/* Copy secondary settings from/to SD if user
 * changed "save to SD"-option in CP
 */

void copySettings()
{
    if(!haveSD || !haveFS)
        return;

    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef DG_DBG
        Serial.println(F("copySettings: Copying secondary settings to other medium"));
        #endif
        saveCurVolume(false);
    }

    configOnSD = !configOnSD;
}

// Re-write secondary settings
// Used during audio file installation when flash FS needs
// to be re-formatted.
static void rewriteSecondarySettings()
{
    bool oldconfigOnSD = configOnSD;
    
    #ifdef DG_DBG
    Serial.println("Re-writing secondary settings");
    #endif
    
    writeIpSettings();

    // Create current settings on flash FS
    // regardless of SD-option
    configOnSD = false;
       
    saveCurVolume(false);
    
    configOnSD = oldconfigOnSD;
}

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName)
{
    const char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return DeserializationError::NoMemory;
    }

    memset((void *)buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef DG_DBG
    Serial.println(buf);
    #endif
    
    ret = deserializeJson(json, buf);

    free((void *)buf);

    return ret;
}

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return false;
    }

    memset(buf, 0, bufSize + 1);
    serializeJson(json, buf, bufSize);

    #ifdef DG_DBG
    Serial.printf("Writing %s to %s\n", fn, useSD ? "SD" : "FS");
    Serial.println((const char *)buf);
    #endif

    if(useSD) {
        success = writeFileToSD(fn, (uint8_t *)buf, (int)bufSize);
    } else {
        success = writeFileToFS(fn, (uint8_t *)buf, (int)bufSize);
    }

    free(buf);

    if(!success) {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }

    return success;
}

static bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

static bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveFS)
        return false;

    File myFile = SPIFFS.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

bool openACFile(File& file)
{
    if(haveSD) {
        if(file = SD.open(CONFN, FILE_WRITE)) {
            return true;
        }
    }

    return false;
}

size_t writeACFile(File& file, uint8_t *buf, size_t len)
{
    return file.write(buf, len);
}

void closeACFile(File& file)
{
    file.close();
}

void removeACFile()
{
    if(haveSD) {
        SD.remove(CONFN);
    }
}
