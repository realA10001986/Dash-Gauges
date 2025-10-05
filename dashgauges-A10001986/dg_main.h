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

#ifndef _DG_MAIN_H
#define _DG_MAIN_H

#include "dgdisplay.h"

extern unsigned long powerupMillis;

extern Gauges gauges;

extern bool sbv2;

extern bool TCDconnected;
#ifdef DG_HAVEDOORSWITCH
extern bool dsPlay;
#endif

extern bool FPBUnitIsOn;
extern bool dgNM;

extern bool TTrunning;
extern bool emptyAlarm;
extern bool startup;
extern bool startAlarm;
extern bool refill;
extern bool refillWA;

extern bool networkTimeTravel;
extern bool networkTCDTT;
extern bool networkReentry;
extern bool networkAbort;
extern bool networkAlarm;
extern uint16_t networkLead;
extern uint16_t networkP1;

void main_boot();
void main_boot2();
void main_setup();
void main_loop();

void flushDelayedSave();

void showWaitSequence();
void endWaitSequence();

void showCopyError();

void allOff();
void prepareReboot();

void mydelay(unsigned long mydel);

void prepareTT();
void refill_plutonium();
void set_empty();
void wakeup();

void switchMusicFolder(uint8_t nmf);

void sideSwitch_scan();
#ifdef DG_HAVEDOORSWITCH
void doorSwitch_scan();
void play_door_open(int doorNum, bool isOpen);
#endif

uint8_t read_port();
uint8_t read_port_debounce();

void bttfn_loop();

#endif
