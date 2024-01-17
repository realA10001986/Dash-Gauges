/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * DGDisplay: Handle the Gauges & the "empty" LED
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
#include <Wire.h>

#include "dgdisplay.h"

/*
 * Empty LED class
 */

// Timer interrupt
#define TMR_TIME      0.01    // 0.01s = 10ms
#define TMR_PRESCALE  80
#define TMR_TICKS     (uint64_t)(((double)TMR_TIME * 80000000.0) / (double)TMR_PRESCALE)
#define TME_TIMEUS    (TMR_TIME * 1000000)

static volatile int32_t  _ticks = 0;
static volatile bool     _critical = false;
static volatile bool     _blinkEnable = false;
static volatile bool     _blinkWasOff = false;
static volatile bool     _emptyLED = false;
static volatile int16_t  _tick_interval = 400; // random, will be overwritten
static volatile uint16_t _blinkDelay = 0;

#define SS_ONESHOT 0xfe
#define SS_LOOP    0
#define SS_END     0xff
static volatile bool     _specialsig = false;
static volatile bool     _wasSpecial = false;
static volatile bool     _specialOS = false;
static volatile uint8_t  _specialsignum = 0;
static volatile uint8_t  _specialidx = 0;
static volatile int16_t  _specialticks = 0;

static const DRAM_ATTR uint8_t _specialArray[DGSEQ_MAX][64] = {
    {                                               // 1: error: no audio files installed ("SOS")
      SS_ONESHOT,
      1, 15, 0, 15, 1, 15, 0, 15, 1, 15, 0, 15,
      1, 40, 0, 40, 1, 40, 0, 40, 1, 40, 0, 40,
      1, 15, 0, 15, 1, 15, 0, 15, 1, 15, 0, 15,
      SS_END
    },
    {                                               // 2: Wait
      SS_LOOP,
      1, 50, 0, 50,
      SS_END
    },
    {                                               // 3: Alarm (BTTFN/MQTT)
      SS_ONESHOT,
      1, 50, 0, 50, 1, 50, 0, 50,
      1, 50, 0, 50, 1, 50, 0, 50,
      SS_END
    },
    {                                               // 4: Error when copying audio files
      SS_LOOP,
      1, 20, 0, 20, 1, 20, 0, 100,
      SS_END
    }
};

// ISR: "Empty" LED blinking

static void IRAM_ATTR _el_on()
{
    digitalWrite(EMPTY_LED_PIN, HIGH);
}

static void IRAM_ATTR _el_off()
{
    digitalWrite(EMPTY_LED_PIN, LOW);
}

static void IRAM_ATTR ISR_DG_Empty()
{
    if(_critical)
        return;

    if(_specialsig) {
      
        // Special sequence for signalling
        _wasSpecial = true;
        if(_specialticks == 0) {
            if(_specialArray[_specialsignum][_specialidx] == SS_END) {
                 if(_specialOS) {
                    _specialsig = _wasSpecial = false; 
                    _el_off();
                 } else {
                    _specialidx = 1;
                 }
            }
            if(_specialsig) {
                (_specialArray[_specialsignum][_specialidx]) ? _el_on() : _el_off();
            }
        }
        if(_specialsig) {
            _specialticks++;
            if(_specialticks >= _specialArray[_specialsignum][_specialidx + 1]) {
                _specialticks = 0;
                _specialidx += 2;
            }
        }

    } else if(_wasSpecial) {
      
        _el_off();
        _wasSpecial = false;
        
    }

    // We need to continue our blink pace even
    // when a specialSig is running in order not to
    // lose sync

    if(!_blinkEnable) {
        if(_blinkWasOff) return;
        if(!_specialsig) _el_off();
        _emptyLED = false;
        _blinkWasOff = true;
        return;
    }

    if(_blinkDelay) {
        _blinkDelay--;
        return;
    }

    if(_blinkWasOff) {
        _ticks = 0;
        if(!_specialsig) _el_on();
        _emptyLED = true;
        _blinkWasOff = false;
    }

    _ticks += 10;
    if(_ticks >= _tick_interval) {
        _ticks -= _tick_interval;
        _emptyLED = !_emptyLED;
        if(!_specialsig) {
            _emptyLED ? _el_on() : _el_off();
        }
    }
}

EmptyLED::EmptyLED(uint8_t timer_no)
{
    _timer_no = timer_no;
}

void EmptyLED::begin()
{   
    pinMode(EMPTY_LED_PIN, OUTPUT);
    
    // Switch off
    _el_off();

    // Install & enable timer interrupt
    _DGTimer_Cfg = timerBegin(_timer_no, TMR_PRESCALE, true);
    timerAttachInterrupt(_DGTimer_Cfg, &ISR_DG_Empty, true);
    timerAlarmWrite(_DGTimer_Cfg, TMR_TICKS, true);
    timerAlarmEnable(_DGTimer_Cfg);
}

void EmptyLED::startBlink(uint16_t milliSecs, uint16_t delayTicks)
{
    _critical = true;
    _blinkDelay = delayTicks;
    _tick_interval = milliSecs;
    _blinkEnable = true;
    _critical = false;
}

void EmptyLED::stopBlink()
{
    _critical = true;
    _blinkEnable = false;
    _critical = false;
}

void EmptyLED::specialSignal(uint8_t signum) 
{
    _critical = true;
    _specialsig = false;
    if(signum) {
        _specialsignum = signum - 1;
        _specialOS = (_specialArray[_specialsignum][0] == SS_ONESHOT);
        _specialidx = 1;
        _specialticks = 0;
        _specialsig = true;
    }
    _critical = false;
}

bool EmptyLED::specialDone()
{
    return !_specialsig;
}

/*
 * Gauges Class
 */

Gauges::Gauges()
{
}

bool Gauges::begin(uint16_t *parmArray)
{
    uint8_t readBack[24];
    int i2clen;

     _type = parmArray[0];
    
    switch(_type) {

    case DGD_TYPE_NONE:
        break;
      
    case DGD_TYPE_MCP4728:

        // Parms:
        // i2c address1, i2c address2,                                              [0x64, 0x60]
        // (max val for DAC A), (max val for DAC B), (max val for DAC C),           [0-4095]
        // (Vref|Gain for DAC A), (Vref|Gain for DAC B), (Vref|Gain for DAC C) .    [VRef = 0x80, Gain = 0x10]

        // Max voltage limiter:
        // - 0 - 4095 = 0 - 2.048V if Vref Int, Gain low,
        // - 0 - 4095 = 0 - 4.096V if Vref Int, Gain high,
        // - 0 - 4095 = 0 - 5V if Vref Ext [gain irrelevant])
        setMax(0, parmArray[3]); 
        setMax(1, parmArray[4]);
        setMax(2, parmArray[5]);
    
        // Reset: Clear values, set up Vref/Gain
        for(int i = 0; i < 3; i++) {
            _values[i] = _perc[i] = 0;
            _vrefGain[i] = parmArray[6+i] & MCP4728_VREF_GAIN_MASK;
        }
        _vrefGain[3] = MCP4728_VREF_INT;    // DAC D unused, set to lowest
    
        // Check for DAC module on i2c bus
        Wire.beginTransmission((uint8_t)parmArray[1]);
        if(Wire.endTransmission(true)) {
            Wire.beginTransmission((uint8_t)parmArray[2]);
            if(Wire.endTransmission(true)) {
                Serial.println("MCP4728 not found");
                return false;
            }
            _address = parmArray[2];
        } else {
            _address = parmArray[1];
        }

        // Read EEPROM and set it up to our values if not already so
        i2clen = Wire.requestFrom(_address, (uint8_t)24);
        if(i2clen < 24) {
            Serial.printf("Error: Can only read back %d bytes from MCP4728 DAC\n", i2clen);
        }
        for(int i = 0; i < min(24, i2clen); i++) {
            readBack[i] = Wire.read();
            #ifdef DG_DBG
            Serial.printf("0x%02x ", readBack[i]);
            #endif
        }
        #ifdef DG_DBG
        Serial.println("");
        #endif
        
        if( (readBack[(0*6)+4] & MCP4728_VREF_GAIN_MASK != _vrefGain[0]) ||
            (readBack[(1*6)+4] & MCP4728_VREF_GAIN_MASK != _vrefGain[1]) ||
            (readBack[(2*6)+4] & MCP4728_VREF_GAIN_MASK != _vrefGain[2]) ||
            (readBack[(3*6)+4] & 0xe0 != 0xe0) ) {
            #ifdef DG_DBG
            Serial.println("Resetting MCP4728 EEPROM");
            #endif
            Wire.beginTransmission(_address);
            Wire.write(0b01010000);   // Sequ write with EEPROM
            for(int i = 0; i < 4; i++) {
                Wire.write((i == 3) ? 0b11100000 : _vrefGain[i]);
                Wire.write(0x00);
            }
            Wire.endTransmission(true);
        }
    
        UpdateAll();
    
        // Disable channel D
        Wire.beginTransmission(_address);
        Wire.write(0b01000110);    // Multi-write
        Wire.write(0b11100000);
        Wire.write(0x00);
        Wire.endTransmission(true);
        break;

    case DGD_TYPE_BINARY_ALL:
        _pins[0] = parmArray[2];
        _thresholds[0] = parmArray[3];
        pinMode(_pins[0], OUTPUT);
        digitalWrite(_pins[0], LOW);
        break;

    case DGD_TYPE_BINARY_SEP:
        for(int i = 0; i < 3; i++) {
            _pins[i] = parmArray[1 + i];
            _thresholds[i] = parmArray[4 + i];
            pinMode(_pins[i], OUTPUT);
            digitalWrite(_pins[i], LOW);
        }
        break;
    }

    return true;
}

// Reset values to 0 and update display
void Gauges::reset()
{
    for(int i = 0; i < 4; i++) {
        _values[i] = _perc[i] = 0;
    }
    UpdateAll();
}

// Display all 0, bypass buffer
void Gauges::off()
{
    switch(_type) {
    case DGD_TYPE_NONE:
        break;
    case DGD_TYPE_MCP4728:
        Wire.beginTransmission(_address);
        for(int i = 0; i < 3; i++) {
            Wire.write(0b01000000 | ((i << 1) & 0x06));    // Multi-write
            Wire.write(_vrefGain[i]);
            Wire.write(0x00);
        }
        Wire.endTransmission(true);
        break;
    case DGD_TYPE_BINARY_ALL:
        digitalWrite(_pins[0], LOW);
        break;
    case DGD_TYPE_BINARY_SEP:
        for(int i = 0; i < 3; i++) {
            digitalWrite(_pins[i], LOW);
        }
        break;
    }
}

// Restore to previous values from buffer
void Gauges::on()
{
    UpdateAll();
}

// Set percent value for gauge in buffer (do not update display)
void Gauges::setValuePercent(uint8_t index, uint8_t perc)
{
    uint16_t newVal;

    index &= 0x03;

    if(perc > 100) perc = 100;

    _perc[index] = perc;

    switch(_type) {
    case DGD_TYPE_NONE:
        break;
    case DGD_TYPE_MCP4728:
        newVal = _max[index] * perc / 100;
        if(newVal > _max[index]) newVal = _max[index];
        _values[index] = newVal;
        #ifdef DG_DBG
        Serial.printf("Gauge %d: %d%%, val %x\n", index, perc, newVal);
        #endif
        break;
    case DGD_TYPE_BINARY_ALL:
        if(!index) _values[0] = _perc[0];
        break;
    case DGD_TYPE_BINARY_SEP:
        _values[index] = _perc[index];
        break;
    }
}

uint8_t Gauges::getValuePercent(uint8_t index)
{    
    index &= 0x03;

    return _perc[index];
}

// Set percent value in buffer and update displays
void Gauges::setValueDirectPercent(uint8_t index, uint8_t perc)
{
    setValuePercent(index, perc);
    UpdateAll();
}

// Update the display
void Gauges::UpdateAll()
{
    switch(_type) {
    case DGD_TYPE_NONE:
        break;
    case DGD_TYPE_MCP4728:
        Wire.beginTransmission(_address);   // Send all values to DACs
        for(int i = 0; i < 3; i++) {
            Wire.write(0b01000000 | ((i << 1) & 0x06));    // Multi-write
            Wire.write((_values[i] >> 8) | _vrefGain[i]);
            Wire.write(_values[i] & 0xff);
        }
        Wire.endTransmission(true);
        break;
    case DGD_TYPE_BINARY_ALL:
        digitalWrite(_pins[0], (_values[0] > _thresholds[0]) ? HIGH : LOW);
        break;
    case DGD_TYPE_BINARY_SEP:
        for(int i = 0; i < 3; i++) {       
            digitalWrite(_pins[i], (_values[i] > _thresholds[i]) ? HIGH : LOW);
        }
        break;
    }
}


// Private


void Gauges::setMax(int8_t index, uint16_t maxVal)
{
    int s, e;

    switch(_type) {
    case DGD_TYPE_NONE:
    case DGD_TYPE_BINARY_ALL:
    case DGD_TYPE_BINARY_SEP:
        break;
    case DGD_TYPE_MCP4728:    // Set max value written to DAC
        if(maxVal > 0xfff) maxVal = 0xfff;
        
        if(index >= 0) {
            s = e = index & 0x03;
        } else {
            s = 0; e = 2;
        }
    
        for(int i = s; i <= e; i++) {
            _max[i] = maxVal;
        }
        break;
    }
}
