/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Input
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

#ifndef _DGINPUT_H
#define _DGINPUT_H

/*
 * DGButton class
 */

typedef enum {
    TCBS_IDLE,
    TCBS_PRESSED,
    TCBS_RELEASED,
    TCBS_LONGPRESS,
    TCBS_LONGPRESSEND
} ButtonState;

class DGButton {
  
    public:
        DGButton(const int pin, const bool activeLow = true, const bool pullupActive = true);

        void begin();
        
        void setTiming(const int dticks, const int pticks, const int lticks);
      
        void attachPress(void (*newFunction)(void));
        void attachLongPressStart(void (*newFunction)(void));
        void attachLongPressStop(void (*newFunction)(void));

        void scan(void);

    private:

        void reset(void);
        void transitionTo(ButtonState nextState);

        void (*_pressFunc)(void) = NULL;
        void (*_longPressStartFunc)(void) = NULL;
        void (*_longPressStopFunc)(void) = NULL;

        int _pin;
        bool _pullupActive;
        
        unsigned int _debounceDur = 50;
        unsigned int _pressDur = 400;
        unsigned int _longPressDur = 800;
      
        int _buttonPressed;
      
        ButtonState _state     = TCBS_IDLE;
        ButtonState _lastState = TCBS_IDLE;
      
        unsigned long _startTime;

        bool _pressNotified = false;
};

#endif
