/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Input
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

#include <Arduino.h>

#include "input.h"


/*
 * DGButton class
 * 
 * If a Long-Press-function is registered, a "press" is only reported only after
 * the button is released. If no such function is registered, a press is
 * reported immediately (after PressTicks have elapsed), regardless of a button
 * release. The latter mode is used for when the TCD is connected to trigger
 * time travels.
 */

/* pin: The pin to be used
 * activeLow: Set to true when the input level is LOW when the button is pressed, Default is true.
 * pullupActive: Activate the internal pullup when available. Default is true.
 */
DGButton::DGButton(const int pin, const boolean activeLow, const bool pullupActive)
{
    _pin = pin;

    _buttonPressed = activeLow ? LOW : HIGH;
  
    pinMode(pin, pullupActive ? INPUT_PULLUP : INPUT);
}


// Setup buttom timin:
// dticks: Number of millisec for a stable click to be assumed
// pticks: Number of millisec to pass for a short press 
// lticks: Number of millisec to pass for a long press
void DGButton::setTicks(const int dticks, const int pticks, const int lticks)
{
    _debounceTicks = dticks;
    _pressTicks = pticks;
    _longPressTicks = lticks;
}

// Register function for short press event
void DGButton::attachPress(void (*newFunction)(void))
{
    _pressFunc = newFunction;
}

// Register function for long press start event
void DGButton::attachLongPressStart(void (*newFunction)(void))
{
    _longPressStartFunc = newFunction;
}

// Register function for long press stop event
void DGButton::attachLongPressStop(void (*newFunction)(void))
{
    _longPressStopFunc = newFunction;
}

// Check input of the pin and advance the state machine
void DGButton::scan(void)
{
    unsigned long now = millis();
    unsigned long waitTime = now - _startTime;
    bool active = (digitalRead(_pin) == _buttonPressed);
    
    switch(_state) {
    case TCBS_IDLE:
        if(active) {
            transitionTo(TCBS_PRESSED);
            _startTime = now;
        }
        break;

    case TCBS_PRESSED:
        if((!active) && (waitTime < _debounceTicks)) {  // de-bounce
            transitionTo(_lastState);
        } else if(!active) {
            transitionTo(TCBS_RELEASED);
            _startTime = now;
        } else if(active) {
            if(!_longPressStartFunc) {
                if(waitTime > _pressTicks) {
                    if(_pressFunc) _pressFunc();
                    _pressNotified = true;
                }      
            } else if(waitTime > _longPressTicks) {
                if(_longPressStartFunc) _longPressStartFunc(); 
                transitionTo(TCBS_LONGPRESS);
            }
        }
        break;

    case TCBS_RELEASED:
        if((active) && (waitTime < _debounceTicks)) {  // de-bounce
            transitionTo(_lastState);
        } else if((!active) && (waitTime > _pressTicks)) {
            if(!_pressNotified && _pressFunc) _pressFunc();
            reset();
        }
        break;
  
    case TCBS_LONGPRESS:
        if(!active) {
            transitionTo(TCBS_LONGPRESSEND);
            _startTime = now;
        }
        break;

    case TCBS_LONGPRESSEND:
        if((active) && (waitTime < _debounceTicks)) { // de-bounce
            transitionTo(_lastState);
        } else if(waitTime >= _debounceTicks) {
            if(_longPressStopFunc) _longPressStopFunc();
            reset();
        }
        break;

    default:
        transitionTo(TCBS_IDLE);
        break;
    }
}

/*
 * Private
 */

void DGButton::reset(void)
{
    _state = TCBS_IDLE;
    _lastState = TCBS_IDLE;
    _startTime = 0;
    _pressNotified = false;
}

// Advance to new state
void DGButton::transitionTo(ButtonState nextState)
{
    _lastState = _state;
    _state = nextState;
}
 
