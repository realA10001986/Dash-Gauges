/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.backtothefutu.re
 *
 * Settings & file handling
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

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 1600

/* If SPIFFS/LittleFS is mounted */
bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* If SD contains default audio files */
static bool allowCPA = false;

/* Music Folder Number */
uint8_t musFolderNum = 0;

/* Cache for volume */
static uint8_t prevSavedVol = 255;

/* Save alarm/volume on SD? */
static bool configOnSD = false;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

#define SND_KEY_LEN 73386
#define SND_KEY_IDX 11+2
#define NUM_AUDIOFILES 13+6
static const char *audioFiles[NUM_AUDIOFILES] = {
      "/0.mp3", "/1.mp3", "/2.mp3", "/3.mp3", "/4.mp3", 
      "/5.mp3", "/6.mp3", "/7.mp3", "/8.mp3", "/9.mp3", 
      "/dot.mp3", 
      "/startup.mp3",
      "/refill.mp3",
      "/empty.wav",   // key
      "/alarm.mp3",
      "/dooropen.mp3",
      "/doorclose.mp3",
      "/renaming.mp3",
      "/installing.mp3"
};
static const char *IDFN = "/DG_def_snd.txt";

static const char *cfgName    = "/dgconfig.json";   // Main config (flash)
static const char *ipCfgName  = "/dgipcfg.json";    // IP config (flash)
static const char *volCfgName = "/dgvolcfg.json";   // Volume config (flash/SD)
static const char *musCfgName = "/dgmcfg.json";     // Music config (SD)

static const char *fsNoAvail = "File System not available";
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
static const char *failFileWrite = "Failed to open file for writing";

static bool read_settings(File configFile);

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static void open_and_copy(const char *fn, int& haveErr);
static bool filecopy(File source, File dest);
static bool check_if_default_audio_present();

static void formatFlashFS();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

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
    pinMode(SIDESWITCH_PIN, INPUT);
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
    configOnSD = (haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));
    
    // Check if SD contains our default sound files
    if(haveFS && haveSD && !FlashROMode) {
        allowCPA = check_if_default_audio_present();
    }

    // Allow user to delete static IP data by holding time travel
    // while booting and switching side switch twice within 10 seconds
    if(digitalRead(TT_IN_PIN)) {
        delay(50);
        if(digitalRead(TT_IN_PIN)) {

            unsigned long mnow = millis();
            bool ssState = digitalRead(SIDESWITCH_PIN), newSSState;
            int ssCount = 0;

            // Pre-maturely use empty led
            pinMode(EMPTY_LED_PIN, OUTPUT);
            digitalWrite(EMPTY_LED_PIN, HIGH);
            delay(200);
            digitalWrite(EMPTY_LED_PIN, LOW);

            while(1) {
                if((ssCount == 2) || (millis() - mnow > 10*1000)) break;

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

            if(ssCount == 2) {
    
                Serial.printf("%s: Deleting ip config; temporarily clearing AP mode WiFi password\n", funcName);
    
                deleteIpSettings();
    
                // Set AP mode password to empty (not written, only until reboot!)
                settings.appw[0] = 0;
    
                digitalWrite(EMPTY_LED_PIN, HIGH);
                while(digitalRead(TT_IN_PIN)) {  }
                digitalWrite(EMPTY_LED_PIN, LOW);
            }
        }
    }
}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    
    DeserializationError error = deserializeJson(json, configFile);

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

    if(!error) {

        wd |= CopyCheckValidNumParm(json["lIdle"], settings.lIdle, sizeof(settings.lIdle), 0, 100, DEF_L_GAUGE_IDLE);
        wd |= CopyCheckValidNumParm(json["cIdle"], settings.cIdle, sizeof(settings.cIdle), 0, 100, DEF_C_GAUGE_IDLE);
        wd |= CopyCheckValidNumParm(json["rIdle"], settings.rIdle, sizeof(settings.rIdle), 0, 100, DEF_R_GAUGE_IDLE);
        wd |= CopyCheckValidNumParm(json["lEmpty"], settings.lEmpty, sizeof(settings.lEmpty), 0, 100, DEF_L_GAUGE_EMPTY);
        wd |= CopyCheckValidNumParm(json["cEmpty"], settings.cEmpty, sizeof(settings.cEmpty), 0, 100, DEF_C_GAUGE_EMPTY);
        wd |= CopyCheckValidNumParm(json["rEmpty"], settings.rEmpty, sizeof(settings.rEmpty), 0, 100, DEF_R_GAUGE_EMPTY);
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

        wd |= CopyCheckValidNumParm(json["playALsnd"], settings.playALsnd, sizeof(settings.playALsnd), 0, 1, DEF_PLAY_ALM_SND);

        wd |= CopyCheckValidNumParm(json["dsPlay"], settings.dsPlay, sizeof(settings.dsPlay), 0, 1, DEF_DS_PLAY);
        wd |= CopyCheckValidNumParm(json["dsCOnC"], settings.dsCOnC, sizeof(settings.dsCOnC), 0, 1, DEF_DS_NC);
        wd |= CopyCheckValidNumParm(json["dsDelay"], settings.dsDelay, sizeof(settings.dsDelay), 0, 5000, DEF_DS_DELAY);

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

        wd |= CopyCheckValidNumParm(json["gaugeType"], settings.gaugeType, sizeof(settings.gaugeType), 0, GA_NUM_TYPES-1, DEF_SHUFFLE);

    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    const char *funcName = "write_settings";
    DynamicJsonDocument json(JSON_SIZE);
    //StaticJsonDocument<JSON_SIZE> json;

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef DG_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    json["lIdle"] = settings.lIdle;
    json["cIdle"] = settings.cIdle;
    json["rIdle"] = settings.rIdle;
    json["lEmpty"] = settings.lEmpty;
    json["cEmpty"] = settings.cEmpty;
    json["rEmpty"] = settings.rEmpty;
    json["aRef"] = settings.autoRefill;
    json["aMut"] = settings.autoMute;
    json["ssTimer"] = settings.ssTimer;

    json["hostName"] = settings.hostName;
    json["systemID"] = settings.systemID;
    json["appw"] = settings.appw;
    json["wifiConRetries"] = settings.wifiConRetries;
    json["wifiConTimeout"] = settings.wifiConTimeout;

    json["TCDpresent"] = settings.TCDpresent;
    json["noETTOLead"] = settings.noETTOLead;

    json["tcdIP"] = settings.tcdIP;
    //json["useGPSS"] = settings.useGPSS;
    json["useNM"] = settings.useNM;
    json["useFPO"] = settings.useFPO;

    json["playALsnd"] = settings.playALsnd;

    json["dsPlay"] = settings.dsPlay;
    json["dsCOnC"] = settings.dsCOnC;
    json["dsDelay"] = settings.dsDelay;

    #ifdef DG_HAVEMQTT
    json["useMQTT"] = settings.useMQTT;
    json["mqttServer"] = settings.mqttServer;
    json["mqttUser"] = settings.mqttUser;
    #endif

    json["shuffle"] = settings.shuffle;
    
    json["CfgOnSD"] = settings.CfgOnSD;
    //json["sdFreq"] = settings.sdFreq;

    json["gaugeType"] = settings.gaugeType;

    File configFile = FlashROMode ? SD.open(cfgName, FILE_WRITE) : SPIFFS.open(cfgName, FILE_WRITE);

    if(configFile) {

        #ifdef DG_DBG
        serializeJson(json, Serial);
        Serial.println(F(" "));
        #endif
        
        serializeJson(json, configFile);
        configFile.close();

    } else {

        Serial.printf("%s: %s\n", funcName, failFileWrite);

    }
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
        sprintf(text, "%1.1f", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
            sprintf(text, "%1.1f", setDefault);
            return true;
        }
    }

    f = atof(text);

    if(f < lowerLim) {
        sprintf(text, "%1.1f", lowerLim);
        return true;
    }
    if(f > upperLim) {
        sprintf(text, "%1.1f", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "0.")
    sprintf(text, "%1.1f", f);

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

static bool openCfgFileWrite(const char *fn, File& f, bool SDonly = false)
{
    bool haveConfigFile;
    
    if(configOnSD || SDonly) {
        haveConfigFile = (f = SD.open(fn, FILE_WRITE));
    } else if(haveFS) {
        haveConfigFile = (f = SPIFFS.open(fn, FILE_WRITE));
    }

    return haveConfigFile;
}

/*
 *  Load/save the Volume
 */

bool loadCurVolume()
{
    #ifdef DG_DBG
    const char *funcName = "loadCurVolume";
    #endif
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
        StaticJsonDocument<512> json;
        if(!deserializeJson(json, configFile)) {
            if(!CopyCheckValidNumParm(json["volume"], temp, sizeof(temp), 0, 255, 255)) {
                uint8_t ncv = atoi(temp);
                if((ncv >= 0 && ncv <= 19) || ncv == 255) {
                    curSoftVol = ncv;
                } 
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedVol = curSoftVol;

    return true;
}

void saveCurVolume(bool useCache)
{
    const char *funcName = "saveCurVolume";
    char buf[6];
    File configFile;
    StaticJsonDocument<512> json;

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

    #ifdef DG_DBG
    Serial.printf("%s: Writing to %s\n", funcName, configOnSD ? "SD" : "flash FS");
    #endif

    sprintf(buf, "%d", curSoftVol);
    json["volume"] = (char *)buf;

    #ifdef DG_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(openCfgFileWrite(volCfgName, configFile)) {
        serializeJson(json, configFile);
        configFile.close();
        prevSavedVol = curSoftVol;
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
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
            StaticJsonDocument<512> json;
            if(!deserializeJson(json, configFile)) {
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
    StaticJsonDocument<512> json;
    char buf[4];

    if(!haveSD)
        return;

    sprintf(buf, "%1d", musFolderNum);
    json["folder"] = buf;
    
    File configFile = SD.open(musCfgName, FILE_WRITE);

    if(configFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }
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

            StaticJsonDocument<512> json;
            DeserializationError error = deserializeJson(json, configFile);

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
    StaticJsonDocument<512> json;

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;

    #ifdef DG_DBG
    Serial.println(F("writeIpSettings: Writing file"));
    #endif
    
    json["IpAddress"] = ipsettings.ip;
    json["Gateway"] = ipsettings.gateway;
    json["Netmask"] = ipsettings.netmask;
    json["DNS"] = ipsettings.dns;

    File configFile = FlashROMode ? SD.open(ipCfgName, FILE_WRITE) : SPIFFS.open(ipCfgName, FILE_WRITE);

    #ifdef DG_DBG
    serializeJson(json, Serial);
    Serial.println(F(" "));
    #endif

    if(configFile) {
        serializeJson(json, configFile);
        configFile.close();
    } else {
        Serial.printf("writeIpSettings: %s\n", failFileWrite);
    }
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


static bool check_if_default_audio_present()
{
    File file;
    size_t ts;
    int i, idx = 0;
    size_t sizes[NUM_AUDIOFILES] = {
      9404, 7523, 5642, 6582, 6582,         // 0-4
      7836, 8463, 8463, 5015, 8777,         // 5-9
      5955,                                 // dot
      24658,        // startup
      30510,        // refill
      SND_KEY_LEN,  // empty
      65230,        // alarm
      20479,        // door open
      24658,        // door close
      43153,        // renaming
      42212         // installing (not copied)
    };

    if(!haveSD)
        return false;

    // If identifier missing, quit now
    if(!(SD.exists(IDFN))) {
        #ifdef DG_DBG
        Serial.println("SD: ID file not present");
        #endif
        return false;
    }

    for(i = 0; i < NUM_AUDIOFILES; i++) {
        if(!SD.exists(audioFiles[i])) {
            #ifdef DG_DBG
            Serial.printf("missing: %s\n", audioFiles[i]);
            #endif
            return false;
        }
        if(!(file = SD.open(audioFiles[i])))
            return false;
        ts = file.size();
        file.close();
        #ifdef DG_DBG
        sizes[idx++] = ts;
        #else
        if(sizes[idx++] != ts)
            return false;
        #endif
    }

    #ifdef DG_DBG
    for(i = 0; i < (NUM_AUDIOFILES); i++) {
        Serial.printf("%d, ", sizes[i]);
    }
    Serial.println("");
    #endif

    return true;
}

/*
 * Install default audio files from SD to flash FS #############
 */

void doCopyAudioFiles()
{
    bool delIDfile = false;

    if(!copy_audio_files()) {
        // If copy fails, re-format flash FS
        formatFlashFS();            // Format
        rewriteSecondarySettings(); // Re-write secondary settings
        #ifdef DG_DBG 
        Serial.println("Re-writing general settings");
        #endif
        write_settings();           // Re-write general settings
        if(!copy_audio_files()) {   // Retry copy
            showCopyError();
            mydelay(5000);
        } else {
            delIDfile = true;
        }
    } else {
        delIDfile = true;
    }

    if(delIDfile)
        delete_ID_file();

    mydelay(500);
    
    esp_restart();
}


bool copy_audio_files()
{
    int i, haveErr = 0;

    if(!allowCPA) {
        return false;
    }

    for(i = 0; i < NUM_AUDIOFILES - 1; i++) {
        open_and_copy(audioFiles[i], haveErr);
    }

    return (haveErr == 0);
}

static void open_and_copy(const char *fn, int& haveErr)
{
    const char *funcName = "copy_audio_files";
    File sFile, dFile;

    if((sFile = SD.open(fn, FILE_READ))) {
        #ifdef DG_DBG
        Serial.printf("%s: Opened source file: %s\n", funcName, fn);
        #endif
        if((dFile = SPIFFS.open(fn, FILE_WRITE))) {
            #ifdef DG_DBG
            Serial.printf("%s: Opened destination file: %s\n", funcName, fn);
            #endif
            if(!filecopy(sFile, dFile)) {
                haveErr++;
            }
            dFile.close();
        } else {
            Serial.printf("%s: Error opening destination file: %s\n", funcName, fn);
            haveErr++;
        }
        sFile.close();
    } else {
        Serial.printf("%s: Error opening source file: %s\n", funcName, fn);
        haveErr++;
    }
}

static bool filecopy(File source, File dest)
{
    uint8_t buffer[1024];
    size_t bytesr, bytesw;

    while((bytesr = source.read(buffer, 1024))) {
        if((bytesw = dest.write(buffer, bytesr)) != bytesr) {
            Serial.println(F("filecopy: Error writing data"));
            return false;
        }
    }

    return true;
}

bool audio_files_present()
{
    File file;
    size_t ts;
    
    if(FlashROMode || !haveFS)
        return true;

    if(!SPIFFS.exists(audioFiles[SND_KEY_IDX]))
        return false;
      
    if(!(file = SPIFFS.open(audioFiles[SND_KEY_IDX])))
        return false;
      
    ts = file.size();
    file.close();

    if(ts != SND_KEY_LEN)
        return false;

    return true;
}

void delete_ID_file()
{
    if(!haveSD)
        return;

    #ifdef DG_DBG
    Serial.printf("Deleting ID file %s\n", IDFN);
    #endif
        
    if(SD.exists(IDFN)) {
        SD.remove(IDFN);
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

// Re-write secondary settings
// Used during audio file installation when flash FS needs
// to be re-formatted.
void rewriteSecondarySettings()
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
