/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.backtothefutu.re
 *
 * WiFi and Config Portal handling
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

#ifndef _DG_WIFI_H
#define _DG_WIFI_H

extern bool gaugeTypeLocked;

extern bool wifiSetupDone;
extern bool wifiIsOff;
extern bool wifiAPIsOff;
extern bool wifiInAPMode;

void wifi_setup();
void wifi_setup2();
void wifi_loop();
void wifiOff();
void wifiOn(unsigned long newDelay = 0, bool alsoInAPMode = false, bool deferConfigPortal = false);
bool wifiIsOn();
void wifiStartCP();

void updateConfigPortalValues();
void updateConfigPortalVolValues();

bool wifi_getIP(uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d);
bool isIp(char *str);

#endif
