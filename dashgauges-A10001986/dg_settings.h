/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Settings handling
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

#ifndef _DG_SETTINGS_H
#define _DG_SETTINGS_H

extern bool haveFS;
extern bool haveSD;
extern bool FlashROMode;

extern bool haveAudioFiles;

extern uint8_t musFolderNum;

#define MS(s) XMS(s)
#define XMS(s) #s

// Default settings
#define DEF_AUTO_REFILL     0     // Default auto-refill: 0=Never (1-360 seconds)
#define DEF_AUTO_MUTE       0     // Default audio mute: 0=Never (1-360 seconds)
#define DEF_SS_TIMER        0     // "Screen saver" timeout in minutes; 0=off

#define DEF_HOSTNAME        "gauges"
#define DEF_WIFI_RETRY      3     // 1-10; Default: 3 retries
#define DEF_WIFI_TIMEOUT    7     // 7-25; Default: 7 seconds

#define DEF_TCD_PRES        0     // 0: No TCD connected, 1: connected via GPIO
#define DEF_NO_ETTO_LEAD    0     // 0: TCD signals TT with ETTO_LEAD lead time; 1 without

#define DEF_TCD_IP          ""    // TCD ip address for BTTFN
#define DEF_USE_GPSS        0     // 0: Ignore GPS speed; 1: Use it for chase speed
#define DEF_USE_NM          0     // 0: Ignore TCD night mode; 1: Follow TCD night mode
#define DEF_USE_FPO         0     // 0: Ignore TCD fake power; 1: Follow TCD fake power
#define DEF_BTTFN_TT        1     // 0: '0' TT button triggers stand-alone TT; 1: Button triggers BTTFN-wide TT

#define DEF_L_GAUGE_IDLE    28    // Default "full" percentages of analog gauges
#define DEF_C_GAUGE_IDLE    28
#define DEF_R_GAUGE_IDLE    65
#define DEF_L_GAUGE_EMPTY   0     // Default "empty" percentages of analog gauges
#define DEF_C_GAUGE_EMPTY   0
#define DEF_R_GAUGE_EMPTY   0

#define DEF_DR_PRI          1     // 0: Meter jumps to zero after TT; 1: slowly drain during TT
#define DEF_DR_PPO          1     // 0: Meter jumps to zero after TT; 1: slowly drain during TT
#define DEF_DR_ROE          1     // 0: Meter jumps to zero after TT; 1: slowly drain during TT

#define DEF_PLAY_ALM_SND    0     // 1: Play TCD-alarm sound, 0: do not

#define DEF_DS_PLAY         1     // 0: don't play door sounds, 1: do
#define DEF_DS_NC           0     // 0: door switch is NO, 1: door switch is NC
#define DEF_DS_DELAY        0     // door switch sound delay

#define DEF_SHUFFLE         0     // Music Player: Do not shuffle by default

#define DEF_CFG_ON_SD       1     // Default: Save secondary settings on SD card
#define DEF_SD_FREQ         0     // SD/SPI frequency: Default 16MHz

#define DEF_GAUGE_TYPE      0     // Default gauge type; to protect the hardware, this is zero by default (=NONE)

struct Settings {
    char autoRefill[6]      = MS(DEF_AUTO_REFILL);
    char autoMute[6]        = MS(DEF_AUTO_MUTE);
    char ssTimer[6]         = MS(DEF_SS_TIMER);
    
    char hostName[32]       = DEF_HOSTNAME;
    char systemID[8]        = "";
    char appw[10]           = "";
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char wifiConTimeout[4]  = MS(DEF_WIFI_TIMEOUT);

    char TCDpresent[4]      = MS(DEF_TCD_PRES);
    char noETTOLead[4]      = MS(DEF_NO_ETTO_LEAD);

    char tcdIP[16]          = DEF_TCD_IP;
    //char useGPSS[4]         = MS(DEF_USE_GPSS);
    char useNM[4]           = MS(DEF_USE_NM);
    char useFPO[4]          = MS(DEF_USE_FPO);
    char bttfnTT[4]         = MS(DEF_BTTFN_TT);

    char lIdle[6]           = MS(DEF_L_GAUGE_IDLE);
    char cIdle[6]           = MS(DEF_C_GAUGE_IDLE);
    char rIdle[6]           = MS(DEF_R_GAUGE_IDLE);
    char lEmpty[6]          = MS(DEF_L_GAUGE_EMPTY);
    char cEmpty[6]          = MS(DEF_C_GAUGE_EMPTY);
    char rEmpty[6]          = MS(DEF_R_GAUGE_EMPTY);
    char drPri[4]           = MS(DEF_DR_PRI);
    char drPPo[4]           = MS(DEF_DR_PPO);
    char drRoe[4]           = MS(DEF_DR_ROE);

    char lThreshold[6]      = "0";
    char cThreshold[6]      = "0";
    char rThreshold[6]      = "0";

    char playALsnd[4]       = MS(DEF_PLAY_ALM_SND);

    char dsPlay[4]          = MS(DEF_DS_PLAY);
    char dsCOnC[4]          = MS(DEF_DS_NC);
    char dsDelay[6]         = MS(DEF_DS_DELAY);

#ifdef DG_HAVEMQTT  
    char useMQTT[4]         = "0";
    char mqttServer[80]     = "";  // ip or domain [:port]  
    char mqttUser[128]      = "";  // user[:pass] (UTF8)
#endif     

    char shuffle[4]         = MS(DEF_SHUFFLE);

    char CfgOnSD[4]         = MS(DEF_CFG_ON_SD);
    char sdFreq[4]          = MS(DEF_SD_FREQ);

    char gaugeIDA[4]        = MS(DEF_GAUGE_TYPE);
    char gaugeIDB[4]        = MS(DEF_GAUGE_TYPE);
    char gaugeIDC[4]        = MS(DEF_GAUGE_TYPE);

#ifdef DG_HAVEVOLKNOB
    char FixV[4];   // Dynamically set for CP, not saved
#endif
    char Vol[6];
    char musicFolder[6];
};

struct IPSettings {
    char ip[20]       = "";
    char gateway[20]  = "";
    char netmask[20]  = "";
    char dns[20]      = "";
};

extern struct Settings settings;
extern struct IPSettings ipsettings;

void settings_setup();

void unmount_fs();

void write_settings();
bool checkConfigExists();

bool loadCurVolume();
void saveCurVolume(bool useCache = true);

bool loadMusFoldNum();
void saveMusFoldNum();

void copySettings();

bool loadIpSettings();
void writeIpSettings();
void deleteIpSettings();

bool check_if_default_audio_present();
bool prepareCopyAudioFiles();
void doCopyAudioFiles();

bool check_allow_CPA();
void delete_ID_file();

#include <FS.h>
bool openACFile(File& file);
size_t writeACFile(File& file, uint8_t *buf, size_t len);
void closeACFile(File& file);
void removeACFile();

#endif
