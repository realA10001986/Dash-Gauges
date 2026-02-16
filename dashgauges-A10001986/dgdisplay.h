/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * DGDisplay Class: Handles the Gauges
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

#ifndef _DGDISPLAY_H
#define _DGDISPLAY_H

/*
 * Empty LED class
 */

class EmptyLED {

    public:

        EmptyLED(uint8_t timer_no);
        void begin(uint8_t pin);
        
        void startBlink(uint16_t ticks, uint16_t delayTicks);
        void stopBlink();

        void specialSignal(uint8_t signum);
        bool specialDone();
        
    private:
        hw_timer_t *_DGTimer_Cfg = NULL;
        uint8_t _timer_no;
        
};

#define DGSEQ_NOAUDIO    1
#define DGSEQ_WAIT       2
#define DGSEQ_ALARM      3
#define DGSEQ_ERRCOPY    4
#define DGSEQ_UPDAVAIL   5
#define DGSEQ_MAX        DGSEQ_UPDAVAIL

/*
 * Gauges Class
 */

struct dacGauge {               // DGD_TYPE_MCP4728:
    uint16_t maxV;              // max (0-4095) (VRef = 4095)                
    uint8_t  VRefGain;          // Int/Ext VRef; Gain flags
};

struct binGauge {               // DGD_TYPE_DIGITAL:
    uint8_t gpioPin;            // GPIO pin for digitial output
};

struct ga_types {
    int  id;                    // ID
    const char *name;           // Name to show
    
    uint8_t connectionType;     // Connection type

    struct dacGauge aga;        // Data for analog gauges
    struct binGauge dga;        // Data for digital gauges
};

// Connection type
#define DGD_TYPE_NONE       0
#define DGD_TYPE_MCP4728    1
#define DGD_TYPE_DIGITAL    2

// DGD_TYPE_MCP4728: VRefGain
#define MCP4728_VREF_INT       0x80    // Vref = 2.048V / 4.096V (depending on GAIN)
#define MCP4728_VREF_EXT       0x00    // Vref = Vcc = 5V (GAIN ignored)
#define MCP4728_GAIN_HIGH      0x10
#define MCP4728_GAIN_LOW       0x00
#define MCP4728_POWER_DOWN     0x60
#define MCP4728_VREF_GAIN_MASK (MCP4728_VREF_INT|MCP4728_GAIN_HIGH)
#define MCP4728_DEFAULT        (MCP4728_VREF_INT|MCP4728_POWER_DOWN)
#define MCP4728_DEFAULT_MASK   (MCP4728_VREF_INT|MCP4728_POWER_DOWN|MCP4728_GAIN_HIGH)

// Minimum time between state changes for digital gauges
#define DIG_SWITCH_MIN_TIME 1500

class Gauges {

    public:

        Gauges();
        bool begin(uint8_t idA, uint8_t idB, uint8_t idC);

        void reset();

        void on();
        void off();

        void loop();

        void setBinGaugeThreshold(uint8_t index, uint8_t thres);

        bool supportVariablePercentage(uint8_t index);
        void setValuePercent(uint8_t index, uint8_t perc);
        uint8_t getValuePercent(uint8_t index);

        void UpdateAll();

        const struct ga_types *getGTStruct(bool isSmall, int index);

        int num_types_small, max_id_small;
        int num_types_large, max_id_large;
               
    private:
        void setMax(int8_t index, uint16_t maxVal);
        int findMaxId(const struct ga_types *gat, int num);
        const struct ga_types *findGauge(bool isSmall, int id);

        void setDigitalPin(uint8_t pin, uint8_t state);

        int  readEEPROM(uint8_t *buf);
        
        uint8_t _type[3];

        bool _haveMCP4728 = false;
        bool _haveMCP4728gauge = false;
        bool _allMCP4728 = false;
        int  _numDigPins = 0;

        bool _supportVarPerc[4] = { false, false, false, false };
        
        uint8_t _perc[4]     = { 0, 0, 0, 0 };
        uint16_t _values[4]  = { 0, 0, 0, 0 };

        // For DGD_TYPE_MCP4728:
        uint8_t _address     = 255;
        uint16_t _max[4]     = { 0, 0, 0, 0 };
        uint8_t _vrefGain[4] = { MCP4728_DEFAULT, MCP4728_DEFAULT, MCP4728_DEFAULT, MCP4728_DEFAULT };

        // For DGD_TYPE_DIGITAL
        uint8_t _pins[4]       = { 255, 255, 255, 255 };
        uint8_t _thresholds[4] = { 0, 0, 0, 0 };

        int8_t        _pinIndices[40]  = { -1 };
        uint8_t       _lastState[3]    = { 0, 0, 0 };
        uint8_t       _desiredState[3] = { 0, 0, 0 };
        unsigned long _lastStateChg[3] = { 0, 0, 0 };
        unsigned long _scheduled[3]    = { 0, 0, 0 };
};

#endif
