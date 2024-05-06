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
 * Gauges hardware definition table
 * 
 * The first three items are:
 * - id: An ID that identifies the gauge type. Must be unique per table.
 * - name: The name of the gauge type for display in the Config Portal   
 * - connection type:
 * 
 * DGD_TYPE_MCP4728: 
 * 
 * For a gauge connected to the MCP4728 DAC. This identifies "analog" gauges, 
 * which means that they can move their pointer by changing voltage. The DAC 
 * has four channels: A, C, B, D. Channel D is unused.
 * DAC channels A-C are allocated as follows: 
 * left gauge = Channel A, center gauge = channel B, right gauge = channel C
 * This allocation is fixed; even if, for instance, only the left and right
 * gauges are analog ones, and the center gauge is, say, digital, channels
 * A and C will be used.
 * 
 * Analog gauges have further config data in the first struct that follows:
 * 
 * maxV: The "maxV" parameter defines the maximum voltage ever sent to the gauges, 
 * given in 1/4095s of reference voltage. 
 *    100% of reference voltage = 4095
 *    50% of reference voltage = 2048
 * 
 * VRefGain: This parameter defines the reference voltage: This is either
 * - (MCP4728_VREF_INT|MCP4728_GAIN_LOW)  for 2.048V, or
 * - (MCP4728_VREF_INT|MCP4728_GAIN_HIGH) for 4.096V, or
 * - MCP4728_VREF_EXT                     for 5V.
 * 
 * DGD_TYPE_DIGITAL:
 * 
 * This defines a digital gauge. Digital gauges only know "full" and "empty" states,
 * they can't move their pointer arbitrarily by software. This type is used when, 
 * for example, 
 * - when the Control Board cannot supply a suitable voltage/amperage for the gauge, 
 *   and it is driven with external power and switched through a relay;
 * - for gauge replicas using a stepper motor to move the pointer to a fixed position
 *   on power, and resetting the pointer position to "Empty" on power loss.
 * 
 * Digital gauges are configured in the second struct following the connection type:
 * 
 * gpioPin: GPIO pin to ge switched HIGH for "full" and LOW for "empty". On the OEM
 * Control Board, only one GPIO pin is reserved for digital gauges, so they all use
 * the same. Custom variations of that board could allow different ones.
 * 
 * For digital gauges, there is a "Threshold" setting in the Config Portal.
 * This defines the "virtual percentage" (0% being the left end of the scale, 100% 
 * being the right end of the scale) at which the gauge should switch from "full" 
 * to "empty" in animations. This value depends on the speed of pointer movement: 
 * In the time travel sequence, the pointers are slowly moved towards "Empty" in 
 * sync with the length of the sequence. If your pointers "jump" to zero quickly, 
 * a threshold of 0 is ok. If the pointers move more slowly, the threshold should 
 * by adjusted so that the pointers are at "empty" at the end of the sequence, when 
 * the "Empty" alarm goes off. Start with "50" and work your way from there.
 * 
 */
 
static const struct ga_types gaugeTypesSmall[] = {
    
    // None
    { 0, "None", DGD_TYPE_NONE, {}, {} },

    // Type 1: Analog gauge: 0-0.5V through MCP4728
    //         Phaostron/H&P 631-14672 with 8.6K resistors in series
    //         Using MCP4728's built-in Vref (2.048V), limited by "maxV" parameter to 0.5V (1000/4095*2.048).
    { 1, "Phaostron/H&P 631-14672 (0-0.5V)", DGD_TYPE_MCP4728, 
      { 1000, MCP4728_VREF_INT|MCP4728_GAIN_LOW }, {} },

    // Type 2: Digital gauge: 0 or 12V, switched through DIGITAL_GAUGE_PIN. On the OEM Control
    // Board, the output voltage is 12V, but can be adjusted by putting a resistor
    // instead of a bridge at LEG1 (for "Percent Power") or LEG2 (for "Primary").
    { 2, "Digital / Legacy (0/12V)", DGD_TYPE_DIGITAL, 
      {}, { DIGITAL_GAUGE_PIN } },

    { 3, "Generic Analog 0-5V)", DGD_TYPE_MCP4728, 
      { 4095, MCP4728_VREF_EXT }, {} },

    { 4, "Generic Analog 0-4.095V)", DGD_TYPE_MCP4728, 
      { 4095, (MCP4728_VREF_INT|MCP4728_GAIN_HIGH) }, {} },

    { 5, "Generic Analog 0-2.048V)", DGD_TYPE_MCP4728, 
      { 4095, (MCP4728_VREF_INT|MCP4728_GAIN_LOW) }, {} }
};

static const struct ga_types gaugeTypesLarge[] = {
    
    // None
    { 0, "None", DGD_TYPE_NONE, {}, {} },  

    // Type 1: Analog gauge: 0-0.014V through MCP4728
    //         Simpson Roentgens meter (with no resistor) 
    //         Using MCP4728's built-in Vref (2.048V), limited by "max" parameter to 0.014V (28/4095*2.048)
    //         FIXME - need a resistor value to keep this within reasonable limits - 14mV is too low
    { 1, "Simpson Roentgens (0-0.014V)", DGD_TYPE_MCP4728,
      {   28, MCP4728_VREF_INT|MCP4728_GAIN_LOW }, {} },
    
    // Type 2: Analog gauge: 0-1.6V through MCP4728
    //         Simpson 49L VU meter with 3k6 resistor in series
    //         Using MCP4728's built-in Vref (2.048V), limited by "maxV" parameter to 1.6V (3200/4095*2.048)
    { 2, "Standard VU-Meter (0-1.6V)", DGD_TYPE_MCP4728,
      { 3200, MCP4728_VREF_INT|MCP4728_GAIN_LOW }, {} },

    // Type 3: Digital gauge: 0 or 12V, switched through DIGITAL_GAUGE_PIN. On the OEM control
    // board, the output voltage is 12V, but can be adjusted by putting a resistor
    // instead of a bridge at LEG5. A digital "Roentgens" meter must be connected to the 
    // "Digital/Legacy" connector, pins 1 (+) and 2 (-).
    { 3, "Digital / Legacy (0/12V)", DGD_TYPE_DIGITAL,
      {}, { DIGITAL_GAUGE_PIN } },

    { 4, "Generic Analog (0-5V)", DGD_TYPE_MCP4728, 
      { 4095, MCP4728_VREF_EXT }, {} },

    { 5, "Generic Analog (0-4.095V)", DGD_TYPE_MCP4728, 
      { 4095, (MCP4728_VREF_INT|MCP4728_GAIN_HIGH) }, {} },

    { 6, "Generic Analog (0-2.048V)", DGD_TYPE_MCP4728, 
      { 4095, (MCP4728_VREF_INT|MCP4728_GAIN_LOW) }, {} }

    /*
    // Example:
    // 50V DC Simpson voltmeter with the internal resistor bridged, and a 4k7
    // resistor added on the Control Board (R5/R6) so that full scale is at about 5V. 
    // We use Vcc (which is 5V) as reference, and can go to maximum (4095).
    // This is covered by the "Generic 0-5V" entry above.
    { 7, "Simpson DC 0-50V modified (0-5V)", DGD_TYPE_MCP4728,
      { 4095, MCP4728_VREF_EXT }, {} }
    */
};


/*
 * Empty LED class
 */

// Timer interrupt
#define TMR_TIME      0.01    // 0.01s = 10ms
#define TMR_PRESCALE  80
#define TMR_TICKS     (uint64_t)(((double)TMR_TIME * 80000000.0) / (double)TMR_PRESCALE)
#define TME_TIMEUS    (TMR_TIME * 1000000)

static volatile int      _pin = 0;
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
    digitalWrite(_pin, HIGH);
}

static void IRAM_ATTR _el_off()
{
    digitalWrite(_pin, LOW);
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

void EmptyLED::begin(uint8_t pin)
{   
    _pin = pin;
    
    pinMode(_pin, OUTPUT);
    
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
    // Set up number of defined types
    num_types_small = sizeof(gaugeTypesSmall) / sizeof(gaugeTypesSmall[0]);
    num_types_large = sizeof(gaugeTypesLarge) / sizeof(gaugeTypesLarge[0]);
}

bool Gauges::begin(uint8_t idA, uint8_t idB, uint8_t idC)
{
    int numMCP4728 = 0;
    uint8_t idArray[3] = { idA, idB, idC };
    int pinIdx = 0;

    // Check for MCP4728
    Wire.beginTransmission((uint8_t)0x60);
    if(Wire.endTransmission(true)) {
        Wire.beginTransmission((uint8_t)0x64);
        if(Wire.endTransmission(true)) {
            Serial.println("MCP4728 not found");
        } else {
            _address = 0x64;
        }
    } else {
        _address = 0x60;
    }
    _haveMCP4728 = (_address != 255);

    for(int i = 0; i < 3; i++) {
     
        const struct ga_types *gat = findGauge((i < 2), idArray[i]);

        if(!gat) {
            #ifdef DG_DBG
            Serial.printf("Index %d: Bad gauge id %d, setting to NONE\n", i, idArray[i]);
            #endif
            gat = &gaugeTypesSmall[0];
        }

        _type[i] = gat->connectionType;
        _values[i] = _perc[i] = 0;
        
        switch(_type[i]) {
        case DGD_TYPE_NONE:
            break;
        case DGD_TYPE_MCP4728:
            if(_haveMCP4728) {
                setMax(i, gat->aga.maxV);
                _vrefGain[i] = gat->aga.VRefGain & MCP4728_VREF_GAIN_MASK;
                _supportVarPerc[i] = true;
                _haveMCP4728gauge = true;
                numMCP4728++;
            } else {
                _type[i] = DGD_TYPE_NONE;
            }
            break;
        case DGD_TYPE_DIGITAL:
            if((_pins[i] = gat->dga.gpioPin) < 40) {
                if(_pinIndices[_pins[i]] < 0) {
                    _pinIndices[_pins[i]] = pinIdx++;
                }
                pinMode(_pins[i], OUTPUT);
                setDigitalPin(_pins[i], LOW);
                _supportVarPerc[i] = false;
            } else {
                Serial.printf("Bad GPIO number: %d", _pins[i]);
                _type[i] = DGD_TYPE_NONE;
            }
            break;
        }
          
    }

    _allMCP4728 = (numMCP4728 == 3);

    if(_haveMCP4728) {
        uint8_t readBack[24];

        readEEPROM(readBack);
        
        if(_haveMCP4728gauge) {
                
            if( ((readBack[(0*6)+4] & MCP4728_DEFAULT_MASK) != (_vrefGain[0] | MCP4728_POWER_DOWN)) ||
                ((readBack[(1*6)+4] & MCP4728_DEFAULT_MASK) != (_vrefGain[1] | MCP4728_POWER_DOWN)) ||
                ((readBack[(2*6)+4] & MCP4728_DEFAULT_MASK) != (_vrefGain[2] | MCP4728_POWER_DOWN)) ||
                ((readBack[(3*6)+4] & MCP4728_DEFAULT_MASK) != MCP4728_DEFAULT) ) {
                #ifdef DG_DBG
                Serial.println("Resetting MCP4728 EEPROM for current gauge config");
                #endif
                Wire.beginTransmission(_address);
                Wire.write(0b01010000);   // Sequ write with EEPROM
                for(int i = 0; i < 4; i++) {
                    Wire.write(_vrefGain[i] | MCP4728_POWER_DOWN);
                    Wire.write(0x00);
                }
                Wire.endTransmission(true);
            }
            
        } else {
          
            if( ((readBack[(0*6)+4] & MCP4728_DEFAULT_MASK) != MCP4728_DEFAULT) ||
                ((readBack[(1*6)+4] & MCP4728_DEFAULT_MASK) != MCP4728_DEFAULT) ||
                ((readBack[(2*6)+4] & MCP4728_DEFAULT_MASK) != MCP4728_DEFAULT) ||
                ((readBack[(3*6)+4] & MCP4728_DEFAULT_MASK) != MCP4728_DEFAULT) ) {
                #ifdef DG_DBG
                Serial.println("Resetting MCP4728 EEPROM to all disabled");
                #endif
                Wire.beginTransmission(_address);
                Wire.write(0b01010000);   // Sequ write with EEPROM
                for(int i = 0; i < 4; i++) {
                    Wire.write(MCP4728_DEFAULT);
                    Wire.write(0x00);
                }
                Wire.endTransmission(true);
            }

        }

        // Disable channel D
        Wire.beginTransmission(_address);
        Wire.write(0b01000110);    // Multi-write
        Wire.write(MCP4728_DEFAULT);
        Wire.write(0x00);
        Wire.endTransmission(true);
    }

    UpdateAll();

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
    if(_allMCP4728) {
        Wire.beginTransmission(_address);
        for(int i = 0; i < 3; i++) {
            Wire.write(0b01000000 | ((i << 1) & 0x06));    // Multi-write
            Wire.write(_vrefGain[i]);
            Wire.write(0x00);
        }
        Wire.endTransmission(true);
    } else {
        for(int i = 0; i < 3; i++) {
            switch(_type[i]) {
            case DGD_TYPE_MCP4728:
                Wire.beginTransmission(_address);
                Wire.write(0b01000000 | ((i << 1) & 0x06));    // Multi-write
                Wire.write(_vrefGain[i]);
                Wire.write(0x00);
                Wire.endTransmission(true);
                break;
            case DGD_TYPE_DIGITAL:
                setDigitalPin(_pins[i], LOW);
                break;
            }
        }
    }
}

// Restore to previous values from buffer
void Gauges::on()
{
    UpdateAll();
}

void Gauges::setBinGaugeThreshold(uint8_t index, uint8_t thres)
{
    index &= 0x03;

    if(thres > 99) thres = 99;
    
    _thresholds[index] = thres;
}

bool Gauges::supportVariablePercentage(uint8_t index)
{
    index &= 0x03;
    
    return _supportVarPerc[index];
}

// Set percent value for gauge in buffer (do not update display)
void Gauges::setValuePercent(uint8_t index, uint8_t perc)
{
    uint16_t newVal;

    index &= 0x03;

    if(perc > 100) perc = 100;
    _perc[index] = perc;

    switch(_type[index]) {
    case DGD_TYPE_MCP4728:
        newVal = _max[index] * perc / 100;
        if(newVal > _max[index]) newVal = _max[index];
        _values[index] = newVal;
        #ifdef DG_DBG
        Serial.printf("Gauge %d: %d%%, val %x\n", index, perc, newVal);
        #endif
        break;
    case DGD_TYPE_DIGITAL:
        _values[index] = _perc[index];
        break;
    }
}

uint8_t Gauges::getValuePercent(uint8_t index)
{    
    index &= 0x03;

    return _perc[index];
}

// Update the display
void Gauges::UpdateAll()
{
    if(_allMCP4728) {
        Wire.beginTransmission(_address);   // Send all values to DACs
        for(int i = 0; i < 3; i++) {
            Wire.write(0b01000000 | ((i << 1) & 0x06));    // Multi-write
            Wire.write((_values[i] >> 8) | _vrefGain[i]);
            Wire.write(_values[i] & 0xff);
        }
        Wire.endTransmission(true);
    } else {
        for(int i = 0; i < 3; i++) {
            switch(_type[i]) {
            case DGD_TYPE_MCP4728:
                Wire.beginTransmission(_address);   // Send value to DAC
                Wire.write(0b01000000 | ((i << 1) & 0x06));    // Multi-write
                Wire.write((_values[i] >> 8) | _vrefGain[i]);
                Wire.write(_values[i] & 0xff);
                Wire.endTransmission(true);
                break;
            case DGD_TYPE_DIGITAL:
                setDigitalPin(_pins[i], (_values[i] > _thresholds[i]) ? HIGH : LOW);
                break;
            }
        }
    }
}

const struct ga_types *Gauges::getGTStruct(bool isSmall, int index)
{
    if(isSmall) {
        if(index >= num_types_small) return NULL;
        return &gaugeTypesSmall[index];
    } else {
        if(index >= num_types_large) return NULL;
        return &gaugeTypesLarge[index];
    }
}

void Gauges::loop()
{
    for(int i = 0; i < 3; i++) {
        if(_type[i] == DGD_TYPE_DIGITAL) {
            int pidx = _pinIndices[_pins[i]];
            if(pidx >= 0 && _scheduled[pidx]) {
                if(millis() >= _scheduled[pidx]) {
                    digitalWrite(_pins[i], _desiredState[pidx]);
                    _lastState[pidx] = _desiredState[pidx];
                    _lastStateChg[pidx] = millis();
                    _scheduled[pidx] = 0;
                }
            }
        }
    }
}


// Private


void Gauges::setMax(int8_t index, uint16_t maxVal)
{
    switch(_type[index]) {
    case DGD_TYPE_MCP4728:    // Set max value written to DAC
        if(maxVal > 0xfff) maxVal = 0xfff;

        index &= 0x03;
        
        _max[index] = maxVal;
        break;
    }
}

const struct ga_types *Gauges::findGauge(bool isSmall, int id)
{
    if(isSmall) {
        for(int i = 0; i < num_types_small; i++) {
            if(id == gaugeTypesSmall[i].id) {
                return &gaugeTypesSmall[i];
            }
        }
    } else {
        for(int i = 0; i < num_types_large; i++) {
            if(id == gaugeTypesLarge[i].id) {
                return &gaugeTypesLarge[i];
            }
        }
    }

    return NULL;
}

void Gauges::setDigitalPin(uint8_t pin, uint8_t state)
{
    int pidx = _pinIndices[pin];
    unsigned long now = millis();
    unsigned long elapsed;

    if(pidx < 0) {
        #ifdef DG_DBG
        Serial.printf("setDigitalPin: Bad pin! %d\n", pin);
        #endif
        return;
    }
    
    elapsed = now - _lastStateChg[pidx];

    if(state != _lastState[pidx]) {
        if(elapsed < DIG_SWITCH_MIN_TIME) {
            _desiredState[pidx] = state;
            if(!_scheduled[pidx]) {
                _scheduled[pidx] = now + (DIG_SWITCH_MIN_TIME - elapsed);
                #ifdef DG_DBG
                Serial.printf("Scheduled switch for gauge pin %d\n", pin);
                #endif
            } else {
                #ifdef DG_DBG
                Serial.printf("Switch for gauge pin %d already scheduled\n", pin);
                #endif
            }
            return;
        }
    }
    
    digitalWrite(pin, state);

    if(state != _lastState[pidx]) {
        _lastState[pidx] = state;
        _lastStateChg[pidx] = now;
    }
    _scheduled[pidx] = 0;
}

int Gauges::readEEPROM(uint8_t *buf)
{
    // Read EEPROM
    int i2clen = Wire.requestFrom(_address, (uint8_t)24);
    if(i2clen < 24) {
        Serial.printf("Error: Can only read back %d bytes from MCP4728 DAC\n", i2clen);
    }
    for(int i = 0; i < min(24, i2clen); i++) {
        buf[i] = Wire.read();
        #ifdef DG_DBG
        Serial.printf("0x%02x ", buf[i]);
        #endif
    }
    #ifdef DG_DBG
    Serial.println("");
    #endif

    return i2clen;
}
