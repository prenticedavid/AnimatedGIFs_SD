// please read credits at the bottom of file

#include <Adafruit_Arcada.h>
#include "GifDecoder.h"

// Rotary encoder hardware connected on FeatherWing
#define PIN_ENCODER_SWITCH 5
#define PIN_ENCODER_B      6   // switch A and B pins if rotary encoder is 'backwards'
#define PIN_ENCODER_A      9
static uint8_t enc_prev_pos   = 0;
static uint8_t enc_flags      = 0;

#define MAX_GIF_FRAMES    250   // guess how many frames we'll expect!
uint32_t frameLocations[MAX_GIF_FRAMES];
uint16_t frameCount = 0;

/*************** Display setup */
Adafruit_Arcada arcada;
GifDecoder<ARCADA_TFT_WIDTH, ARCADA_TFT_HEIGHT, 12> decoder;
File file;
int16_t  gif_offset_x, gif_offset_y;

#define GIF_DIRECTORY        "/gifs"    // on SD or QSPI

// Setup method runs once, when the sketch starts
void setup() {
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);
  decoder.setDrawLineCallback(drawLineCallback);

  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);

  // Start arcada!
  arcada.begin();
  // If we are using TinyUSB & QSPI we will have the filesystem show up!
  arcada.filesysBeginMSD();

  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SWITCH, INPUT_PULLUP);
  // get an initial reading on the encoder pins
  if (digitalRead(PIN_ENCODER_A) == LOW) {
    enc_prev_pos |= (1 << 0);
  }
  if (digitalRead(PIN_ENCODER_B) == LOW) {
    enc_prev_pos |= (1 << 1);
  }
    
  //while (!Serial) delay(10);
  
  Serial.begin(115200);
  Serial.println("Animated GIFs Demo");
  arcada.displayBegin();
  arcada.fillScreen(ARCADA_BLUE);
  arcada.setBacklight(255);

  if (arcada.filesysBegin()) {
    Serial.println("Found filesystem!");
  } else {
    arcada.haltBox("No filesystem found! For QSPI flash, load CircuitPython. For SD cards, format with FAT");
  }
}

int file_index = -1;
int8_t nextGIF = 1;       // 0 for no change, +1 for next gif, -1 for previous gif
int currFrame, lastFrame = -1;
int currButton, lastButton = true;

void loop() {
  if (arcada.recentUSB()) { 
    nextGIF = 1;  // restart when we get back
    return;       // prioritize USB over GIF decoding
  }

  // Check button presses
  currButton = digitalRead(PIN_ENCODER_SWITCH);
  if (! lastButton && currButton) {
    nextGIF = 1;   // forward      
  }
  lastButton = currButton;

  int buttons = 0;
  
  if (nextGIF != 0) {
      if (! arcada.chdir(GIF_DIRECTORY)) {
        arcada.errorBox("No '" GIF_DIRECTORY "' directory found!\nPlease create it & continue");
        return;
      } 

      int num_files = arcada.filesysListFiles(GIF_DIRECTORY, "GIF");
      if (num_files == 0) {
        arcada.errorBox("No GIF files found! Please add some & press (A) to continue");
        return;
      }

      // Determine how many animated GIF files exist
      Serial.print("Animated GIF files Found: ");  Serial.println(num_files);
      file_index += nextGIF;
      if (file_index >= num_files) {
        file_index = 0;                // wrap around to first file
      }
      if (file_index < 0) {
        file_index = num_files-1;      // wrap around to last file
      }
      nextGIF = 0; // and we're done moving between GIFs
      
      file = arcada.openFileByIndex(GIF_DIRECTORY, file_index, O_READ, "GIF");
      if (!file) {
        return;
      }

      arcada.dmaWait();
      arcada.endWrite();   // End transaction from any prior callback
      arcada.fillScreen(ARCADA_BLACK);
      decoder.startDecoding();

      // Center the GIF
      uint16_t w, h;
      decoder.getSize(&w, &h);
      Serial.print("Width: "); Serial.print(w); Serial.print(" height: "); Serial.println(h);
      if (w < arcada.width()) {
        gif_offset_x = (arcada.width() - w) / 2;
      } else {
        gif_offset_x = 0;
      }
      if (h < arcada.height()) {
        gif_offset_y = (arcada.height() - h) / 2;
      } else {
        gif_offset_y = 0;
      }

      // start by decoding whole GIF once to get the points
      frameCount = 0;
      lastFrame = -1;
      while (1) {
        currFrame = decoder.getFrameNumber();
        if (currFrame == lastFrame) 
          break;
        frameLocations[currFrame] = file.position();
        frameCount = max(frameCount, currFrame);
        Serial.printf("Frame #%d & File location: %d\n", currFrame, file.position());
        if (frameCount >= MAX_GIF_FRAMES) {
          arcada.haltBox("Too many frames in this GIF to handle - please update MAX_GIF_FRAMES!");
        }
        decoder.decodeFrame();
        lastFrame = currFrame;
      }
      currFrame = lastFrame = 0;
  }

  int rotaryDirection = readRotaryEncoder();
  if (rotaryDirection) {
    Serial.printf("Rotary encoder moved: %d\n", rotaryDirection);

    currFrame += rotaryDirection;
    if (currFrame >= frameCount) {
      currFrame = 0;
    }
    if (currFrame < 0) {
      currFrame = frameCount-1;
    }
    Serial.printf("Playing frame #%d at location %d\n", currFrame, frameLocations[currFrame]);
    file.seek(frameLocations[currFrame]);
    decoder.decodeFrame(false); // return immediately
  }
  
}

/******************************* Drawing functions */

void updateScreenCallback(void) {  }

void screenClearCallback(void) {  }

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    arcada.drawPixel(x, y, arcada.color565(red, green, blue));
}

void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip) {
    uint8_t pixel;
    uint32_t t = millis();
    x += gif_offset_x;
    y += gif_offset_y;
    if (y >= arcada.height() || x >= arcada.width() ) return;
    
    if (x + w > arcada.width()) w = arcada.width() - x;
    if (w <= 0) return;

    uint16_t buf565[2][w];
    bool first = true; // First write op on this line?
    uint8_t bufidx = 0;
    uint16_t *ptr;

    for (int i = 0; i < w; ) {
        int n = 0, startColumn = i;
        ptr = &buf565[bufidx][0];
        // Handle opaque span of pixels (stop at end of line or first transparent pixel)
        while((i < w) && ((pixel = buf[i++]) != skip)) {
            ptr[n++] = palette[pixel];
        }
        if (n) {
            arcada.dmaWait(); // Wait for prior DMA transfer to complete
            if (first) {
              arcada.endWrite();   // End transaction from prior callback
              arcada.startWrite(); // Start new display transaction
              first = false;
            }
            arcada.setAddrWindow(x + startColumn, y, n, 1);
            arcada.writePixels(ptr, n, false, true);
            bufidx = 1 - bufidx;
        }
    }
    arcada.dmaWait(); // Wait for last DMA transfer to complete
}


bool fileSeekCallback(unsigned long position) {
  return file.seek(position);
}

unsigned long filePositionCallback(void) { 
  return file.position(); 
}

int fileReadCallback(void) {
    return file.read(); 
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
    return file.read((uint8_t*)buffer, numberOfBytes); //.kbv
}


bool bit_is_set(uint32_t var, uint8_t bitno) {
  return var & (1 << bitno);
}
bool bit_is_clear(uint32_t var, uint8_t bitno) {
  return !( var & (1 << bitno));
}

int8_t readRotaryEncoder(void) {
  int8_t enc_action = 0; // 1 or -1 if moved, sign is direction
 
  uint8_t enc_cur_pos = 0;
  // read in the encoder state first
  if (!digitalRead(PIN_ENCODER_A)) {
    enc_cur_pos |= (1 << 0);
  }
  if (!digitalRead(PIN_ENCODER_B)) {
    enc_cur_pos |= (1 << 1);
  }
 
  // if any rotation at all
  if (enc_cur_pos != enc_prev_pos)
  {
    if (enc_prev_pos == 0x00)
    {
      // this is the first edge
      if (enc_cur_pos == 0x01) {
        enc_flags |= (1 << 0);
      }
      else if (enc_cur_pos == 0x02) {
        enc_flags |= (1 << 1);
      }
    }
 
    if (enc_cur_pos == 0x03)
    {
      // this is when the encoder is in the middle of a "step"
      enc_flags |= (1 << 4);
    }
    else if (enc_cur_pos == 0x00)
    {
      // this is the final edge
      if (enc_prev_pos == 0x02) {
        enc_flags |= (1 << 2);
      }
      else if (enc_prev_pos == 0x01) {
        enc_flags |= (1 << 3);
      }
 
      // check the first and last edge
      // or maybe one edge is missing, if missing then require the middle state
      // this will reject bounces and false movements
      if (bit_is_set(enc_flags, 0) && (bit_is_set(enc_flags, 2) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 2) && (bit_is_set(enc_flags, 0) || bit_is_set(enc_flags, 4))) {
        enc_action = 1;
      }
      else if (bit_is_set(enc_flags, 1) && (bit_is_set(enc_flags, 3) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }
      else if (bit_is_set(enc_flags, 3) && (bit_is_set(enc_flags, 1) || bit_is_set(enc_flags, 4))) {
        enc_action = -1;
      }
 
      enc_flags = 0; // reset for next time
    }
  }
 
  enc_prev_pos = enc_cur_pos;

  return enc_action; 
}

/*
    Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels

    Uses SmartMatrix Library for Teensy 3.1 written by Louis Beaudoin at pixelmatix.com

    Written by: Craig A. Lindley

    Copyright (c) 2014 Craig A. Lindley
    Refactoring by Louis Beaudoin (Pixelmatix)

    Permission is hereby granted, free of charge, to any person obtaining a copy of
    this software and associated documentation files (the "Software"), to deal in
    the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
    the Software, and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
    This example displays 32x32 GIF animations loaded from a SD Card connected to the Teensy 3.1
    The GIFs can be up to 32 pixels in width and height.
    This code has been tested with 32x32 pixel and 16x16 pixel GIFs, but is optimized for 32x32 pixel GIFs.

    Wiring is on the default Teensy 3.1 SPI pins, and chip select can be on any GPIO,
    set by defining SD_CS in the code below
    Function     | Pin
    DOUT         |  11
    DIN          |  12
    CLK          |  13
    CS (default) |  15

    This code first looks for .gif files in the /gifs/ directory
    (customize below with the GIF_DIRECTORY definition) then plays random GIFs in the directory,
    looping each GIF for displayTimeSeconds

    This example is meant to give you an idea of how to add GIF playback to your own sketch.
    For a project that adds GIF playback with other features, take a look at
    Light Appliance and Aurora:
    https://github.com/CraigLindley/LightAppliance
    https://github.com/pixelmatix/aurora

    If you find any GIFs that won't play properly, please attach them to a new
    Issue post in the GitHub repo here:
    https://github.com/pixelmatix/AnimatedGIFs/issues
*/

/*
    CONFIGURATION:
    - If you're using SmartLED Shield V4 (or above), uncomment the line that includes <SmartMatrixShieldV4.h>
    - update the "SmartMatrix configuration and memory allocation" section to match the width and height and other configuration of your display
    - Note for 128x32 and 64x64 displays with Teensy 3.2 - need to reduce RAM:
      set kRefreshDepth=24 and kDmaBufferRows=2 or set USB Type: "None" in Arduino,
      decrease refreshRate in setup() to 90 or lower to get good an accurate GIF frame rate
    - Set the chip select pin for your board.  On Teensy 3.5/3.6, the onboard microSD CS pin is "BUILTIN_SDCARD"
*/
