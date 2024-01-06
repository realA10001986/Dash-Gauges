/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.backtothefutu.re
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

#ifndef _DG_MAIN_H
#define _DG_MAIN_H

extern unsigned long powerupMillis;

extern bool TCDconnected;

extern bool FPBUnitIsOn;
extern bool dgNM;

extern bool TTrunning;

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

void showWaitSequence();
void endWaitSequence();

void showCopyError();

void allOff();

void mydelay(unsigned long mydel);

void prepareTT();
void refill_plutonium();
void wakeup();

void bttfn_loop();

#endif
