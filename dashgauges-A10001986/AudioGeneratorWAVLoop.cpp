/*
  AudioGeneratorWAVLoop
  Audio output generator that reads 8 and 16-bit WAV files
  
  Copyright (C) 2017  Earle F. Philhower, III
  Adapted by Thomas Winischhofer (A10001986), 2023/2025

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "AudioGeneratorWAVLoop.h"

#define DBG_OUT
//define DBG_OUT audioLogger->printf_P

AudioGeneratorWAVLoop::AudioGeneratorWAVLoop()
{
    running = false;
    file = NULL;
    output = NULL;
    buffSize = 128;
    buff = NULL;
    buffPtr = 0;
    buffLen = 0;
}

AudioGeneratorWAVLoop::~AudioGeneratorWAVLoop()
{
    freeBuf();
}

bool AudioGeneratorWAVLoop::stop()
{
    if(!running) return true;
    running = false;
    freeBuf();
    output->stop();
    return file->close();
}

bool AudioGeneratorWAVLoop::isRunning()
{
    return running;
}

bool AudioGeneratorWAVLoop::freeBuf()
{
    if(buff) free(buff);
    buff = NULL;
    return false;
}

// Handle buffered reading, reload each time we run out of data
bool AudioGeneratorWAVLoop::GetBufferedData16x2(int16_t& destL, int16_t& destR)
{
    if(buffPtr >= buffLen) {
        buffPtr = 0;
        //uint32_t toRead = availBytes > buffSize ? buffSize : availBytes;
        //buffLen = file->read( buff, toRead );
        //availBytes -= buffLen;
        buffLen = file->read( buff, buffSize );
        if(buffPtr >= buffLen)
            return false; // No data left!
    }
    destL = *(int16_t *)(buff+buffPtr);
    buffPtr += 2;
    destR = *(int16_t *)(buff+buffPtr);
    buffPtr += 2;
    return true;
}

bool AudioGeneratorWAVLoop::GetBufferedData16(int16_t& dest)
{
    if(buffPtr >= buffLen) {
        buffPtr = 0;
        //uint32_t toRead = availBytes > buffSize ? buffSize : availBytes;
        //buffLen = file->read( buff, toRead );
        //availBytes -= buffLen;
        buffLen = file->read( buff, buffSize );
        if(buffPtr >= buffLen)
            return false; // No data left!
    }
    dest = *(int16_t *)(buff+buffPtr);
    buffPtr += 2;
    return true;
}

bool AudioGeneratorWAVLoop::GetBufferedData8(uint8_t& dest)
{
    if(buffPtr >= buffLen) {
        buffPtr = 0;
        //uint32_t toRead = availBytes > buffSize ? buffSize : availBytes;
        //buffLen = file->read( buff, toRead );
        //availBytes -= buffLen;
        buffLen = file->read( buff, buffSize );
        if(buffPtr >= buffLen)
            return false; // No data left!
    }
    dest = (uint8_t)buff[buffPtr++];
    return true;
}

bool AudioGeneratorWAVLoop::loop()
{
    if(!running) goto done; // Nothing to do here!

    // First, try and push in the stored sample.  If we can't, then punt and try later
    if(!output->ConsumeSample(sL, sR)) goto done; // Can't send, but no error detected

    // Try and stuff the buffer one sample at a time
    // TW: Unroll this
    if(bitsPerSample == 16) {
        if(channels == 2) {
            do
            {
                if(!GetBufferedData16x2(sL, sR)) stop();
            } while (running && output->ConsumeSample(sL, sR));
        } else {
            sR = 0;
            do
            {
                if(!GetBufferedData16(sL)) stop();
            } while (running && output->ConsumeSample(sL, sR));
        }
    } else if(bitsPerSample == 8) {
        uint8_t l, r = 0;
        do
        {
            if(!GetBufferedData8(l)) stop();
            sL = ((int16_t)l - 128) << 8;
            if(channels == 2) {
                if(!GetBufferedData8(r)) stop();
                sR = ((int16_t)r - 128) << 8;
            }
        } while (running && output->ConsumeSample(sL, sR));
    }

done:
    file->loop();
    output->loop();

    return running;
}

bool AudioGeneratorWAVLoop::ReadWAVInfo()
{
    uint32_t u32;
    uint16_t u16;
    int toSkip;
  
    // WAV specification document:
    // https://www.aelius.com/njh/wavemetatools/doc/riffmci.pdf
  
    // Header == "RIFF"
    if(!ReadU32(&u32)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if(u32 != 0x46464952) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, invalid RIFF header, got: %08X \n"), (uint32_t) u32);
        return false;
    }
  
    // Skip ChunkSize
    if(!ReadU32(&u32)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
  
    // Format == "WAVE"
    if(!ReadU32(&u32)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if(u32 != 0x45564157) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, invalid WAVE header, got: %08X \n"), (uint32_t) u32);
        return false;
    }
  
    // there might be JUNK or PAD - ignore it by continuing reading until we get to "fmt "
    while (1) {
        if(!ReadU32(&u32)) {
            DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
            return false;
        };
        if(u32 == 0x20746d66) break; // 'fmt '
    };
  
    // subchunk size
    if(!ReadU32(&u32)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if(u32 == 16) { toSkip = 0; }
    else if(u32 == 18) { toSkip = 18 - 16; }
    else if(u32 == 40) { toSkip = 40 - 16; }
    else {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, appears not to be standard PCM \n"));
        return false;
    } // we only do standard PCM
  
    // AudioFormat
    if(!ReadU16(&u16)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if(u16 != 1) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, AudioFormat appears not to be standard PCM \n"));
        return false;
    } // we only do standard PCM
  
    // NumChannels
    if(!ReadU16(&channels)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if((channels<1) || (channels>2)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, only mono and stereo are supported \n"));
        return false;
    } // Mono or stereo support only
  
    // SampleRate
    if(!ReadU32(&sampleRate)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if(sampleRate < 1) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, unknown sample rate \n"));
        return false;
    }  // Weird rate, punt.  Will need to check w/DAC to see if supported
  
    // Ignore byterate and blockalign
    if(!ReadU32(&u32)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if(!ReadU16(&u16)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
  
    // Bits per sample
    if(!ReadU16(&bitsPerSample)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    if((bitsPerSample!=8) && (bitsPerSample != 16)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, only 8 or 16 bits is supported \n"));
        return false;
    }  // Only 8 or 16 bits
  
    // Skip any extra header
    while (toSkip) {
        uint8_t ign;
        if(!ReadU8(&ign)) {
            DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
            return false;
        };
        toSkip--;
    }
  
    // look for data subchunk
    do {
      // id == "data"
      if(!ReadU32(&u32)) {
          DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
          return false;
      };
      if(u32 == 0x61746164) break; // "data"
      // Skip size, read until end of chunk
      if(!ReadU32(&u32)) {
          DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
          return false;
      };
      if(!file->seek(u32, SEEK_CUR)) {
          DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data, seek failed\n"));
          return false;
      }
    } while (1);
    if(!file->isOpen()) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, file is not open\n"));
        return false;
    };
  
    // Skip size, read until end of file...
    if(!ReadU32(&u32)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: failed to read WAV data\n"));
        return false;
    };
    //availBytes = u32;
  
    // TW: Set current pos as loop start pos
    startPos = file->getPos();
  
    // Now set up the buffer or fail
    buff = reinterpret_cast<uint8_t *>(malloc(buffSize));
    if(!buff) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::ReadWAVInfo: cannot read WAV, failed to set up buffer \n"));
        return false;
    };
    buffPtr = 0;
    buffLen = 0;

    // loop starts by pushing out samples, clear them here
    sL = sR = 0;
  
    return true;
}

bool AudioGeneratorWAVLoop::begin(AudioFileSource *source, AudioOutput *output)
{
    if(!source) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: failed: invalid source\n"));
        return false;
    }
    file = source;
    if(!output) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: invalid output\n"));
        return false;
    }
    this->output = output;
    if(!file->isOpen()) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: file not open\n"));
        return false;
    } // Error
  
    if(!ReadWAVInfo()) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: failed during ReadWAVInfo\n"));
        return false;
    }
  
    if(!output->SetRate(sampleRate)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: failed to SetRate in output\n"));
        return freeBuf();
    }
    // Output is always 16bit
    if(!output->SetBitsPerSample(16)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: failed to SetBitsPerSample in output\n"));
        return freeBuf();
    }
    if(!output->SetChannels(channels)) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: failed to SetChannels in output\n"));
        return freeBuf();
    }
    if(!output->begin()) {
        DBG_OUT(PSTR("AudioGeneratorWAVLoop::begin: output's begin did not return true\n"));
        return freeBuf();
    }
  
    running = true;
  
    return true;
}

bool AudioGeneratorWAVLoop::beginQuick(AudioFileSource *source, AudioOutput *output, int chnls, uint32_t stPos)
{
    file = source;
    this->output = output;
    
    bitsPerSample = 16;
    channels = chnls;
    sampleRate = 44100;
    startPos = stPos;
  
    //availBytes = 999999;  // unused
    
    file->seek(startPos, SEEK_SET);
  
    // Now set up the buffer or fail
    buff = reinterpret_cast<uint8_t *>(malloc(buffSize));
    if(!buff) {
        return false;
    }
    buffPtr = 0;
    buffLen = 0;

    // loop starts by pushing out samples, clear them here
    sL = sR = 0;
  
    if(!output->SetRate(sampleRate)) {
        return freeBuf();
    }
    if(!output->SetBitsPerSample(bitsPerSample)) {
        return freeBuf();
    }
    if(!output->SetChannels(channels)) {
        return freeBuf();
    }
  
    if(!output->begin()) {
        return freeBuf();
    }
  
    running = true;
  
    return true; 
}
