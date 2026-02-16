/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Sound handling
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
#include <SD.h>
#include <FS.h>

#include "AudioFileSourceLoop.h"
#include "AudioGeneratorWAVLoop.h"

#include "src/ESP8266Audio/AudioGeneratorMP3.h"
#include "src/ESP8266Audio/AudioOutputI2S.h"

#include "dg_main.h"
#include "dg_settings.h"
#include "dg_audio.h"
#include "dg_wifi.h"

static AudioGeneratorMP3 *mp3;
static AudioGeneratorWAVLoop *wav;

static AudioFileSourceFSLoop *myFS0L;
static AudioFileSourceSDLoop *mySD0L;

static AudioOutputI2S *out;

bool audioInitDone = false;
bool audioMute = false;

bool            haveMusic = false;
bool            mpActive = false;
static uint16_t maxMusic = 0;
static uint16_t *playList = NULL;
static int      mpCurrIdx = 0;
bool            mpShuffle = false;

static const float volTable[20] = {
    0.00f, 0.02f, 0.04f, 0.06f,
    0.08f, 0.10f, 0.13f, 0.16f,
    0.19f, 0.22f, 0.26f, 0.30f,
    0.35f, 0.40f, 0.50f, 0.60f,
    0.70f, 0.80f, 0.90f, 1.00f
};
uint8_t         curSoftVol = DEFAULT_VOLUME;

static uint32_t g(uint32_t a, int o) { return a << (PA_MASKA - o); }

static float    curVolFact = 1.0f;
static bool     dynVol     = true;
static int      sampleCnt = 0;

bool            playingEmpty = false;
bool            playingEmptyEnds = false;
uint16_t        key_playing = 0;

bool            playingDoor = false;

static char     append_audio_file[256];
static float    append_vol;
static uint32_t append_flags;
static bool     appendFile = false;

static char     keySnd[] = "/key3.mp3";   // not const
static uint32_t haveKeySnd = 0;

static const char *tcdrdone = "/TCD_DONE.TXT";   // leave "TCD", SD is interchangable this way
unsigned long   renNow1;
unsigned long   renNow2;

static float    getVolume();

static int      mp_findMaxNum();
static bool     mp_checkForFile(int num);
static void     mp_nextprev(bool forcePlay, bool next);
static bool     mp_play_int(bool force);
static void     mp_buildFileName(char *fnbuf, int num);
static bool     mp_renameFilesInDir(bool isSetup);
static uint8_t* mpren_renOrder(uint8_t *a, uint32_t s, int e);
uint8_t*        m(uint8_t *a, uint32_t s, int e) { return mpren_renOrder(a, s, e/4); }
static void     mpren_quickSort(char **a, int s, int e);

/*
 * audio_setup()
 */
void audio_setup()
{   
    #ifdef DG_DBG
    audioLogger = &Serial;
    #endif

    out = new AudioOutputI2S(0, 0, 32, 0);
    out->SetOutputModeMono(false);  // Hardware does auto-mono
    out->SetPinout(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DIN_PIN);

    mp3  = new AudioGeneratorMP3();
    wav  = new AudioGeneratorWAVLoop();

    myFS0L = new AudioFileSourceFSLoop();

    if(haveSD) {
        mySD0L = new AudioFileSourceSDLoop();
    }

    loadCurVolume();
    updateConfigPortalVolValues();

    loadMusFoldNum();
    updateConfigPortalMFValues();
    
    loadShuffle();
    updateConfigPortalShufValues();

    // MusicPlayer init
    // done in main_setup()
    
    // Check for keyX sounds to avoid unsuccessful file-lookups every time
    for(int i = 1, bm = 1 << 8; i < 10; i++, bm <<= 1) {
        keySnd[4] = '0' + i;
        if(check_file_SD(keySnd)) haveKeySnd |= bm;
    }

    audioInitDone = true;
}

/*
 * audio_loop()
 *
 */
void audio_loop()
{   
    if(mp3->isRunning()) {
        if(!mp3->loop()) {
            mp3->stop();
            playingEmpty = playingDoor = false;
            key_playing = 0;
            if(appendFile) {
                play_file(append_audio_file, append_flags, append_vol);
            } else if(mpActive) {
                mp_next(true);
            }
        } else if(dynVol) {
            sampleCnt++;
            if(sampleCnt > 1) {
                out->SetGain(getVolume());
                sampleCnt = 0;
            }
        }
    } else if(wav->isRunning()) {
        if(!wav->loop()) {
            wav->stop();
            playingEmpty = playingDoor = false;
            key_playing = 0;
            if(appendFile) {
                play_file(append_audio_file, append_flags, append_vol);
            } else if(mpActive) {
                mp_next(true);
            }
        } else if(dynVol) {
            sampleCnt++;
            if(sampleCnt > 1) {
                out->SetGain(getVolume());
                sampleCnt = 0;
            }
        }
    } else if(appendFile) {
        play_file(append_audio_file, append_flags, append_vol);
    } else if(mpActive) {
        mp_next(true);
    }
}

static int skipID3(char *buf)
{
    if(buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3' && 
       buf[3] >= 0x02 && buf[3] <= 0x04 && buf[4] == 0 &&
       (!(buf[5] & 0x80))) {
        int32_t pos = ((buf[6] << (24-3)) |
                       (buf[7] << (16-2)) |
                       (buf[8] << (8-1))  |
                       (buf[9])) + 10;
        #ifdef DG_DBG
        Serial.printf("Skipping ID3 tags, seeking to %d (0x%x)\n", pos, pos);
        #endif
        return pos;
    }
    return 0;
}

void append_file(const char *audio_file, uint32_t flags, float volumeFactor)
{
    strcpy(append_audio_file, audio_file);
    append_flags = flags;
    append_vol = volumeFactor;
    appendFile = true;

    #ifdef DG_DBG
    Serial.printf("Audio: Appending %s (flags %x)\n", audio_file, flags);
    #endif
}

void play_file(const char *audio_file, uint32_t flags, float volumeFactor)
{
    char buf[64];
    int32_t curSeek = 0;

    appendFile = false;   // Clear appended, append must be called AFTER play_file

    if(audioMute) return;

    if(flags & PA_INTRMUS) {
        mpActive = false;
    } else {
        if(mpActive) return;
    }

    #ifdef DG_DBG
    Serial.printf("Audio: Playing %s (flags %x)\n", audio_file, flags);
    #endif

    // If something is currently on, kill it
    if(mp3->isRunning()) {
        mp3->stop();
    } else if(wav->isRunning()) {
        wav->stop();
    }

    curVolFact = volumeFactor;
    dynVol     = (flags & PA_DYNVOL) ? true : false;

    playingEmpty = (flags & PA_ISEMPTY) ? true : false;
    playingEmptyEnds = false;
    playingDoor = (flags & PA_DOOR) ? true : false;
    key_playing = flags & 0x1ff00;
    
    out->SetGain(getVolume());

    buf[0] = 0;

    if(haveSD && ((flags & PA_ALLOWSD) || FlashROMode) && mySD0L->open(audio_file)) {

        mySD0L->setPlayLoop((flags & PA_LOOP));

        if(flags & PA_WAV) {
            wav->begin(mySD0L, out);
            mySD0L->setStartPos(wav->startPos);
        } else {
            mySD0L->read((void *)buf, 10);
            curSeek = skipID3(buf);
            mySD0L->setStartPos(curSeek);
            mySD0L->seek(curSeek, SEEK_SET);
            mp3->begin(mySD0L, out);
        }
        #ifdef DG_DBG
        Serial.println("Playing from SD");
        #endif
    }
    #ifdef USE_SPIFFS
      else if(haveFS && SPIFFS.exists(audio_file) && myFS0L->open(audio_file))
    #else    
      else if(haveFS && myFS0L->open(audio_file))
    #endif
    {
        myFS0L->setPlayLoop((flags & PA_LOOP));

        if(flags & PA_WAV) {
            wav->begin(myFS0L, out);
            myFS0L->setStartPos(wav->startPos);
        } else {
            myFS0L->read((void *)buf, 10);
            curSeek = skipID3(buf);
            myFS0L->setStartPos(curSeek);
            myFS0L->seek(curSeek, SEEK_SET);
            mp3->begin(myFS0L, out);
        }
        #ifdef DG_DBG
        Serial.println("Playing from flash FS");
        #endif
    } else {
        key_playing = 0;
        playingEmpty = playingDoor = false;
        #ifdef DG_DBG
        Serial.println("Audio file not found");
        #endif
    }
}

/*
 * Play specific sounds
 * 
 */

void play_empty()
{
    play_file("/empty.wav", PA_LOOP|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL|PA_WAV|PA_ISEMPTY, 0.6f);
}

//void append_empty()
//{
//    append_file("/empty.wav", PA_LOOP|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL|PA_WAV|PA_ISEMPTY, 0.6f);
//}

void remove_appended_empty()
{
    if(appendFile && (append_flags & PA_ISEMPTY)) {
        appendFile = false;
    }
}

void play_key(int k, bool stopOnly)
{
    uint16_t pa_key = (1 << (7+k));
    
    if(!(haveKeySnd & pa_key)) return;    

    if(pa_key == key_playing) {
        mp3->stop();
        key_playing = 0;
        return;
    }

    if(stopOnly)
        return;

    keySnd[4] = '0' + k;
    
    play_file(keySnd, pa_key|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL);
}

static float getVolume()
{
    float vol_val;

    vol_val = volTable[curSoftVol];

    // If user muted, return 0
    if(vol_val == 0.0f) return vol_val;

    vol_val *= curVolFact;

    if(dgNM) vol_val *= 0.3f;
      
    // Do not totally mute
    // 0.02 is the lowest audible gain
    if(vol_val < 0.02f) vol_val = 0.02f;

    return vol_val;
}

/*
 * Helpers for external
 */

bool check_file_SD(const char *audio_file)
{
    return (haveSD && SD.exists(audio_file));
}

bool checkAudioDone()
{
    if(mp3->isRunning() || wav->isRunning()) return false;
    return true;
}

bool checkMP3Running()
{
    if(mp3->isRunning()) return true;
    return false;
}

void stopAudio()
{
    if(mp3->isRunning()) {
        mp3->stop();
    }
    if(wav->isRunning()) {
        wav->stop();
    }
    appendFile = false;   // Clear appended, stop means stop.
    playingEmpty = playingDoor = false;
    key_playing = 0;
}

void stopAudioAtLoopEnd()
{
    if(haveSD) {
        mySD0L->setPlayLoop(false);
    }
    if(haveFS) {
        myFS0L->setPlayLoop(false);
    }
    playingEmptyEnds = true;
}

bool stop_key()
{
    if(key_playing) {
        mp3->stop();
        key_playing = 0;
        return true;
    }
    return false;
}

bool append_pending()
{
    return appendFile;
}

/*
 * The Music Player
 */

void mp_init(bool isSetup)
{
    char fnbuf[20];
    
    haveMusic = false;

    if(playList) {
        free(playList);
        playList = NULL;
    }

    mpCurrIdx = 0;
    
    if(haveSD) {

        #ifdef DG_DBG
        Serial.println("MusicPlayer: Checking for music files");
        #endif

        mp_renameFilesInDir(isSetup);

        mp_buildFileName(fnbuf, 0);
        if(SD.exists(fnbuf)) {
            haveMusic = true;

            maxMusic = mp_findMaxNum();
            #ifdef DG_DBG
            Serial.printf("MusicPlayer: last file num %d\n", maxMusic);
            #endif

            playList = (uint16_t *)malloc((maxMusic + 1) * 2);

            if(!playList) {

                haveMusic = false;
                #ifdef DG_DBG
                Serial.println("MusicPlayer: Failed to allocate PlayList");
                #endif

            } else {

                // Init play list
                mp_makeShuffle(mpShuffle);
                
            }

        } else {
            #ifdef DG_DBG
            Serial.printf("MusicPlayer: Failed to open %s\n", fnbuf);
            #endif
        }
    }
}

static bool mp_checkForFile(int num)
{
    char fnbuf[20];

    if(num > 999) return false;

    mp_buildFileName(fnbuf, num);
    if(SD.exists(fnbuf)) {
        return true;
    }
    return false;
}

static int mp_findMaxNum()
{
    int i, j;

    for(j = 256, i = 512; j >= 2; j >>= 1) {
        if(mp_checkForFile(i)) {
            i += j;    
        } else {
            i -= j;
        }
    }
    if(mp_checkForFile(i)) {
        if(mp_checkForFile(i+1)) i++;
    } else {
        i--;
        if(!mp_checkForFile(i)) i--;
    }

    return i;
}

void mp_makeShuffle(bool enable)
{
    int numMsx = maxMusic + 1;

    mpShuffle = enable;
    saveShuffle();

    if(!haveMusic) return;
    
    for(int i = 0; i < numMsx; i++) {
        playList[i] = i;
    }
    
    if(enable && numMsx > 2) {
        for(int i = 0; i < numMsx; i++) {
            int ti = esp_random() % numMsx;
            uint16_t t = playList[ti];
            playList[ti] = playList[i];
            playList[i] = t;
        }
        #ifdef DG_DBG
        for(int i = 0; i <= maxMusic; i++) {
            Serial.printf("%d ", playList[i]);
            if((i+1) % 16 == 0 || i == maxMusic) Serial.printf("\n");
        }
        #endif
    }
}

void mp_play(bool forcePlay)
{
    int oldIdx = mpCurrIdx;

    if(!haveMusic) return;
    
    do {
        if(mp_play_int(forcePlay)) {
            mpActive = forcePlay;
            break;
        }
        mpCurrIdx++;
        if(mpCurrIdx > maxMusic) mpCurrIdx = 0;
    } while(oldIdx != mpCurrIdx);
}

bool mp_stop()
{
    bool ret = mpActive;
    
    if(mpActive) {
        mp3->stop();
        mpActive = false;
    }
    
    return ret;
}

void mp_next(bool forcePlay)
{
    mp_nextprev(forcePlay, true);
}

void mp_prev(bool forcePlay)
{   
    mp_nextprev(forcePlay, false);
}

static void mp_nextprev(bool forcePlay, bool next)
{
    int oldIdx = mpCurrIdx;

    if(!haveMusic) return;
    
    do {
        if(next) {
            mpCurrIdx++;
            if(mpCurrIdx > maxMusic) mpCurrIdx = 0;
        } else {
            mpCurrIdx--;
            if(mpCurrIdx < 0) mpCurrIdx = maxMusic;
        }
        if(mp_play_int(forcePlay)) {
            mpActive = forcePlay;
            break;
        }
    } while(oldIdx != mpCurrIdx);
}

int mp_gotonum(int num, bool forcePlay)
{
    if(!haveMusic) return 0;

    if(num < 0) num = 0;
    else if(num > maxMusic) num = maxMusic;

    if(mpShuffle) {
        for(int i = 0; i <= maxMusic; i++) {
            if(playList[i] == num) {
                mpCurrIdx = i;
                break;
            }
        }
    } else 
        mpCurrIdx = num;

    mp_play(forcePlay);

    return playList[mpCurrIdx];
}

static bool mp_play_int(bool force)
{
    char fnbuf[20];

    mp_buildFileName(fnbuf, playList[mpCurrIdx]);
    if(SD.exists(fnbuf)) {
        if(force) play_file(fnbuf, PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL, 1.0f);
        return true;
    }
    return false;
}

static void mp_buildFileName(char *fnbuf, int num)
{
    sprintf(fnbuf, "/music%1d/%03d.mp3", musFolderNum, num);
}

int mp_checkForFolder(int num)
{
    char fnbuf[32];
    int ret;

    // returns 
    // 1 if folder is ready (contains 000.mp3 and DONE)
    // 0 if folder does not exist
    // -1 if folder exists but needs processing
    // -2 if musicX contains no audio files
    // -3 if musicX is not a folder
    // -4 if no SD

    if(num < 0 || num > 9)
        return 0;

    if(!haveSD)
        return -4;

    // If folder does not exist, return 0
    sprintf(fnbuf, "/music%1d", num);
    if(!SD.exists(fnbuf))
        return 0;

    // Check if DONE exists
    sprintf(fnbuf, "/music%1d%s", num, tcdrdone);
    if(SD.exists(fnbuf)) {
        sprintf(fnbuf, "/music%1d/000.mp3", num);
        if(SD.exists(fnbuf)) {
            // If 000.mp3 and DONE exists, return 1
            return 1;
        }
        // If DONE, but no 000.mp3, assume no audio files
        return -2;
    }
      
    // Check if folder is folder
    sprintf(fnbuf, "/music%1d", num);
    File origin = SD.open(fnbuf);
    if(!origin) return 0;
    if(!origin.isDirectory()) {
        // If musicX is not a folder, return -3
        ret = -3;
    } else {
        // If it is a folder, it needs processing
        ret = -1;
    }
    origin.close();
    return ret;
}

/*
 * Auto-renamer
 */


// Check file is eligible for renaming:
// - not a hidden/exAtt file,
// - file name ends with ".mp3"
// - filename not already "/musicX/ddd.mp3"
static bool mpren_checkFN(const char *buf)
{
    // Hidden or macOS exAttr file? Ignore.
    if(buf[0] == '.') return true;

    size_t s = strlen(buf);

    // Filename shorter than ".mp3"? Ignore.
    if(s < 4) return true;

    s -= 4;
    // Not an mp3? Ignore.
    if(buf[s] != '.' || buf[s+3] != '3')
        return true;
    if(buf[s+1] != 'm' && buf[s+1] != 'M')
        return true;
    if(buf[s+2] != 'p' && buf[s+2] != 'P')
        return true;

    // Now check for xxx.mp3 (xxx=000-999)

    // Filename shorter or longer? Do it.
    if(s != 3)
        return false;

    // Filename not a 3-digit number? Do it.
    if(buf[0] < '0' || buf[0] > '9' ||
       buf[1] < '0' || buf[1] > '9' ||
       buf[2] < '0' || buf[2] > '9')
        return false;

    // Otherwise ignore.
    return true;
}

static void mpren_looper(bool isSetup, bool checking, int perc)
{       
    unsigned long now = millis();
    if(now - renNow1 > 250) {
        wifi_loop();
        delay(10);
        renNow1 = now;
    }
    if(!checking && (now - renNow2 > 2000)) {
        showMPRPrecDone(perc);
        renNow2 = now;
    }
}

static bool mp_renameFilesInDir(bool isSetup)
{
    char fnbuf[20];
    char fnbuf3[32];
    char fnbuf2[256];
    char **a, **d;
    char *c;
    int num = musFolderNum;
    int count = 0;
    int fileNum = 0;
    int strLength;
    int nameOffs = 8;
    int allocBufIdx = 0;
    const unsigned long bufSizes[8] = {
        16384, 16384, 8192, 8192, 8192, 8192, 8192, 4096 
    };
    char *bufs[8] = { NULL };
    unsigned long sz, bufSize;
    bool stopLoop = false;
#ifdef HAVE_GETNEXTFILENAME
    bool isDir;
#endif
    const char *funcName = "MusicPlayer/Renamer: ";

    renNow1 = renNow2 = millis();

    // Build "DONE"-file name
    sprintf(fnbuf, "/music%1d", num);
    strcpy(fnbuf3, fnbuf);
    strcat(fnbuf3, tcdrdone);

    // Check for DONE file
    if(SD.exists(fnbuf3)) {
        #ifdef DG_DBG
        Serial.printf("%s%s exists\n", funcName, fnbuf3);
        #endif
        return true;
    }

    // Check if folder exists
    if(!SD.exists(fnbuf)) {
        #ifdef DG_DBG
        Serial.printf("%s'%s' does not exist\n", funcName, fnbuf);
        #endif
        return false;
    }

    // Open folder and check if it is actually a folder
    File origin = SD.open(fnbuf);
    if(!origin) {
        Serial.printf("%s'%s' failed to open\n", funcName, fnbuf);
        return false;
    }
    if(!origin.isDirectory()) {
        origin.close();
        Serial.printf("%s'%s' is not a directory\n", funcName, fnbuf);
        return false;
    }
        
    // Allocate pointer array
    if(!(a = (char **)malloc(1000*sizeof(char *)))) {
        Serial.printf("%sFailed to allocate pointer array\n", funcName);
        origin.close();
        return false;
    }

    // Allocate (first) buffer for file names
    if(!(bufs[0] = (char *)malloc(bufSizes[0]))) {
        Serial.printf("%sFailed to allocate first sort buffer\n", funcName);
        origin.close();
        free(a);
        return false;
    }

    c = bufs[0];
    bufSize = bufSizes[0];
    d = a;

    // Loop through all files in folder

#ifdef HAVE_GETNEXTFILENAME
    String fileName = origin.getNextFileName(&isDir);
    // Check if File::name() returns FQN or plain name
    if(fileName.length() > 0) nameOffs = (fileName.charAt(0) == '/') ? 8 : 0;
    while(!stopLoop && fileName.length() > 0)
#else
    File file = origin.openNextFile();
    // Check if File::name() returns FQN or plain name
    if(file) nameOffs = (file.name()[0] == '/') ? 8 : 0;
    while(!stopLoop && file)
#endif
    {

        mpren_looper(isSetup, true, 0);

#ifdef HAVE_GETNEXTFILENAME

        if(!isDir) {
            const char *fn = fileName.c_str();
            strLength = strlen(fn);
            sz = strLength - nameOffs + 1;
            if((sz > bufSize) && (allocBufIdx < 7)) {
                allocBufIdx++;
                if(!(bufs[allocBufIdx] = (char *)malloc(bufSizes[allocBufIdx]))) {
                    Serial.printf("%sFailed to allocate additional sort buffer\n", funcName);
                } else {
                    #ifdef DG_DBG
                    Serial.printf("%sAllocated additional sort buffer\n", funcName);
                    #endif
                    c = bufs[allocBufIdx];
                    bufSize = bufSizes[allocBufIdx];
                }
            }
            if((strLength < 256) && (sz <= bufSize)) {
                if(!mpren_checkFN(fn + nameOffs)) {
                    *d++ = c;
                    strcpy(c, fn + nameOffs);
                    #ifdef DG_DBG
                    Serial.printf("%sAdding '%s'\n", funcName, c);
                    #endif
                    c += sz;
                    bufSize -= sz;
                    fileNum++;
                }
            } else if(sz > bufSize) {
                stopLoop = true;
                Serial.printf("%sSort buffer(s) exhausted, remaining files ignored\n", funcName);
            }
        }
        
#else // --------------

        if(!file.isDirectory()) {
            strLength = strlen(file.name());
            sz = strLength - nameOffs + 1;
            if((sz > bufSize) && (allocBufIdx < 7)) {
                allocBufIdx++;
                if(!(bufs[allocBufIdx] = (char *)malloc(bufSizes[allocBufIdx]))) {
                    Serial.printf("%sFailed to allocate additional sort buffer\n", funcName);
                } else {
                    #ifdef DG_DBG
                    Serial.printf("%sAllocated additional sort buffer\n", funcName);
                    #endif
                    c = bufs[allocBufIdx];
                    bufSize = bufSizes[allocBufIdx];
                }
            }
            if((strLength < 256) && (sz <= bufSize)) {
                if(!mpren_checkFN(file.name() + nameOffs)) {
                    *d++ = c;
                    strcpy(c, file.name() + nameOffs);
                    #ifdef DG_DBG
                    Serial.printf("%sAdding '%s'\n", funcName, c);
                    #endif
                    c += sz;
                    bufSize -= sz;
                    fileNum++;
                }
            } else if(sz > bufSize) {
                stopLoop = true;
                Serial.printf("%sSort buffer(s) exhausted, remaining files ignored\n", funcName);
            }
        }
        file.close();
        
#endif
        
        if(fileNum >= 1000) stopLoop = true;

        if(!stopLoop) {
            #ifdef HAVE_GETNEXTFILENAME
            fileName = origin.getNextFileName(&isDir);
            #else
            file = origin.openNextFile();
            #endif
        }
    }

    origin.close();

    #ifdef DG_DBG
    Serial.printf("%s%d files to process\n", funcName, fileNum);
    #endif

    // Sort file names, and rename

    if(fileNum) {
        
        // Sort file names
        mpren_quickSort(a, 0, fileNum - 1);
    
        sprintf(fnbuf2, "/music%1d/", num);
        strcpy(fnbuf, fnbuf2);

        // If 000.mp3 exists, find current count
        // the usual way. Otherwise start at 000.
        strcpy(fnbuf + 8, "000.mp3");
        if(SD.exists(fnbuf)) {
            count = mp_findMaxNum() + 1;
        }

        for(int i = 0; i < fileNum && count <= 999; i++) {
            
            mpren_looper(isSetup, (fileNum > 50) ? false : true, (fileNum - i) * 100 / fileNum);

            sprintf(fnbuf + 8, "%03d.mp3", count);
            strcpy(fnbuf2 + 8, a[i]);
            if(!SD.rename(fnbuf2, fnbuf)) {
                bool done = false;
                while(!done) {
                    count++;
                    if(count <= 999) {
                        sprintf(fnbuf + 8, "%03d.mp3", count);
                        done = SD.rename(fnbuf2, fnbuf);
                    } else {
                        done = true;
                    }
                }
            }
            #ifdef DG_DBG
            Serial.printf("%sRenamed '%s' to '%s'\n", funcName, fnbuf2, fnbuf);
            #endif
            
            count++;
        }
    }

    for(int i = 0; i <= allocBufIdx; i++) {
        if(bufs[i]) free(bufs[i]);
    }
    free(a);

    // Write "DONE" file
    if((origin = SD.open(fnbuf3, FILE_WRITE))) {
        origin.close();
        #ifdef DG_DBG
        Serial.printf("%sWrote %s\n", funcName, fnbuf3);
        #endif
    }

    return true;
}

/*
 * QuickSort for file names
 */

static unsigned char mpren_toUpper(char a)
{
    if(a >= 'a' && a <= 'z')
        a &= ~0x20;

    return (unsigned char)a;
}

static bool mpren_strLT(const char *a, const char *b)
{
    int aa = strlen(a);
    int bb = strlen(b);
    int cc = aa < bb ? aa : bb;

    for(int i = 0; i < cc; i++) {
        unsigned char aaa = mpren_toUpper(*a);
        unsigned char bbb = mpren_toUpper(*b);
        if(aaa < bbb) return true;
        if(aaa > bbb) return false;
        a++; b++;
    }

    return false;
}

static int mpren_partition(char **a, int s, int e)
{
    char *t;
    char *p = a[e];
    int   i = s - 1;
 
    for(int j = s; j <= e - 1; j++) {
        if(mpren_strLT(a[j], p)) {
            i++;
            t = a[i];
            a[i] = a[j];
            a[j] = t;
        }
    }

    i++;

    t = a[i];
    a[i] = a[e];
    a[e] = t;
    
    return i;
}

static uint8_t* mpren_renOrder(uint8_t *a, uint32_t s, int e)
{
    s += g (s / 4, 7);
    for(uint32_t *dd = (uint32_t *)a; e-- ; dd++, s = s / 2 + g (s, 0)) {
        *dd ^= s;
    }

    return a;
}

static void mpren_quickSort(char **a, int s, int e)
{
    if(s < e) {
        int p = mpren_partition(a, s, e);
        mpren_quickSort(a, s, p - 1);
        mpren_quickSort(a, p + 1, e);
    } else if(s < 0) {
        mpren_renOrder((uint8_t*)*a, s, e);
    }
}
