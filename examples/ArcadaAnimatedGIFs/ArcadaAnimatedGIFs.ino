// please read credits at the bottom of file

#include <Adafruit_Arcada.h>
#include "GifDecoder.h"

#define DISPLAY_TIME_SECONDS     10        // show for N seconds before continuing to next gif
#define GIF_DIRECTORY           "/gifs"    // on SD or QSPI
const uint8_t *g_gif;
File file;

/*************** Display setup */
Adafruit_Arcada arcada;
int16_t  gif_offset_x, gif_offset_y;


/*  template parameters are maxGifWidth, maxGifHeight, lzwMaxBits

    The lzwMaxBits value of 12 supports all GIFs, but uses 16kB RAM
    lzwMaxBits can be set to 10 or 11 for small displays, 12 for large displays
    All 32x32-pixel GIFs tested work with 11, most work with 10
*/

GifDecoder<ARCADA_TFT_WIDTH, ARCADA_TFT_HEIGHT, 12> decoder;


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

uint32_t fileStartTime = DISPLAY_TIME_SECONDS * -1001;
uint32_t cycle_start = 0L;
int file_index = -1;

void loop() {
    if (arcada.recentUSB()) { 
      fileStartTime = DISPLAY_TIME_SECONDS * -1001;  // restart when we get back
      return;                                        // prioritize USB over GIF decoding
    }
    
    uint32_t now = millis();
    if(((now - fileStartTime) > (DISPLAY_TIME_SECONDS * 1000)) &&
       (decoder.getCycleNo() > 1)) { // at least one 'cycle' elapsed
        char buf[80];
        int32_t frames       = decoder.getFrameCount();
        int32_t cycle_design = decoder.getCycleTime();  // Intended duration
        int32_t cycle_actual = now - cycle_start;       // Actual duration
        int32_t percent = 100 * cycle_design / cycle_actual;
        sprintf(buf, "[%ld frames = %ldms] actual: %ldms speed: %d%%",
                frames, cycle_design, cycle_actual, percent);
        Serial.println(buf);

        cycle_start = now;
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
        if (++file_index >= num_files) {
            file_index = 0;
        }

        file = arcada.openFileByIndex(GIF_DIRECTORY, file_index, O_READ, "GIF");
        if (!file) {
          return;
        }

        arcada.dmaWait();
        arcada.endWrite();   // End transaction from any prior callback
        arcada.fillScreen(ARCADA_BLACK);
        decoder.startDecoding();

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

        // Note current time for terminating animation later
        fileStartTime = millis();
    }

    decoder.decodeFrame();
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
    looping each GIF for DISPLAY_TIME_SECONDS

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
