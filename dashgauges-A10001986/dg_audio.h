/*
 * -------------------------------------------------------------------
 * Dash Gauges Panel
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/Dash-Gauges
 * https://dg.out-a-ti.me
 *
 * Sound handling
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

#ifndef _DG_AUDIO_H
#define _DG_AUDIO_H

// Default volume (index, or 255 for knob)
#ifdef DG_HAVEVOLKNOB
#define DEFAULT_VOLUME 6
#else
#define DEFAULT_VOLUME 6    // NOT 255!
#endif

#define PA_LOOP    0x0001
#define PA_INTRMUS 0x0002
#define PA_ALLOWSD 0x0004
#define PA_DYNVOL  0x0008
#define PA_ISEMPTY 0x0010
#define PA_WAV     0x0020
#define PA_DOOR    0x0040
#define PA_MASKA   (PA_LOOP|PA_INTRMUS|PA_ALLOWSD|PA_DYNVOL|PA_ISEMPTY)

void audio_setup();
void audio_loop();

void play_file(const char *audio_file, uint16_t flags, float volumeFactor = 1.0);
void append_file(const char *audio_file, uint16_t flags, float volumeFactor = 1.0);
bool checkAudioDone();
void stopAudio();
void stopAudioAtLoopEnd();
bool append_pending();

void play_empty();
//void append_empty();
void remove_appended_empty();

void     mp_init(bool isSetup);
void     mp_play(bool forcePlay = true);
bool     mp_stop();
void     mp_next(bool forcePlay = false);
void     mp_prev(bool forcePlay = false);
int      mp_gotonum(int num, bool force = false);
void     mp_makeShuffle(bool enable);
int      mp_checkForFolder(int num);
uint8_t* m(uint8_t *a, uint32_t s, int e);

extern bool audioInitDone;
extern bool audioMute;

extern bool haveMusic;
extern bool mpActive;

extern bool playingEmpty;
extern bool playingEmptyEnds;

extern bool playingDoor;

extern uint8_t curSoftVol;

#endif
