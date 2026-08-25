/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Main controller
 *
 * -------------------------------------------------------------------
 * License: Modified MIT NON-AI
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
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
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

#ifndef _DG_MAIN_H
#define _DG_MAIN_H

#include "dgdisplay.h"

void main_boot();
void main_boot2();
void main_setup();
void main_loop();

void flushDelayedSave();

void showWaitSequence();
void endWaitSequence();

void showCopyError();

void gaugesCompleteOff();
void allOff();
void prepareReboot();

void mydelay(unsigned long mydel);
unsigned long millisNonZero();

bool switchMusicFolder(uint8_t nmf, bool isSetup = false);
void showMPRPrecDone(unsigned int perc);

void addCmdQueue(uint32_t command);

void bttfn_loop();

extern unsigned long powerupMillis;

extern Gauges gauges;

extern uint32_t csf;
#define CSF_OFF           0x00000001
#define CSF_REFILL        0x00000002
#define CSF_REFILLWA      0x00000004
#define CSF_EMPTYALM      0x00000008
#define CSF_STARTALM      0x00000010
#define CSF_EALM          0x00000020
#define CSF_ST            0x00000040
#define CSF_TT            0x00000100
#define CSF_NM            0x00000200

#define CSF_TTP0          0x01000000
#define CSF_TTP1          0x02000000
#define CSF_TTP2          0x04000000
#define CSF_EXTTT         0x08000000

extern bool TCDbyWire;
#ifdef DG_HAVEDOORSWITCH
extern bool dsPlay;
extern uint16_t doPlayDoorSound;
extern unsigned long doPlayDoorSoundNow;
#endif

extern bool showUpdAvail;

extern int  dgBusy;

extern bool networkTimeTravel;
extern bool networkReentry;
extern bool networkAbort;
extern bool networkAlarm;
extern uint16_t networkLead;
extern uint16_t networkP1;

extern bool doPrepareTT;
extern bool doWakeup;

extern int     bttfnHaveTCDSSID;
extern char    TCDSSID[];
extern uint8_t TCDpwMarker;

#endif
