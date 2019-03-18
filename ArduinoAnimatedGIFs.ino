// please read credits at the bottom of file

#ifndef min
#define min(a, b) (((a) <= (b)) ? (a) : (b))
#endif
#include "GifDecoder.h"
#include "FilenameFunctions.h"    //defines USE_SPIFFS
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define DISPLAY_TIME_SECONDS     10        // show for N seconds before continuing to next gif
#define MAX_GIFWIDTH            320        //228 fails on COW_PAINT
#define MAX_GIFHEIGHT           240 
#define GIF_DIRECTORY           "/gifs"    // on SD or QSPI
const uint8_t *g_gif;

/*  template parameters are maxGifWidth, maxGifHeight, lzwMaxBits

    The lzwMaxBits value of 12 supports all GIFs, but uses 16kB RAM
    lzwMaxBits can be set to 10 or 11 for small displays, 12 for large displays
    All 32x32-pixel GIFs tested work with 11, most work with 10
*/

GifDecoder<MAX_GIFWIDTH, MAX_GIFHEIGHT, 12> decoder;

/*************** Display setup */
#define TFT_D0        34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR        26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC        10 // Data/command pin
#define TFT_CS        11 // Chip-select pin
#define TFT_RST       24 // Reset pin
#define TFT_RD         9 // Read-strobe pin
#define TFT_BACKLIGHT 25
#define SD_CS         32

// ILI9341 with 8-bit parallel interface:
Adafruit_ILI9341 tft = Adafruit_ILI9341(tft8, TFT_D0, TFT_WR, TFT_DC, TFT_CS, TFT_RST, TFT_RD);
#define TFTBEGIN()    { tft.begin(); pinMode(TFT_BACKLIGHT, OUTPUT); digitalWrite(TFT_BACKLIGHT, HIGH); }
#define PUSHCOLOR(x)           tft.pushColor(x)
#define PUSHCOLORS(x, y)       tft.writePixels(x, y)
#define DISKCOLOUR             BLACK   // background color 

uint32_t timeSpentDrawing, timeSpentFS;

/*************** Display setup */

int16_t  gif_offset_x, gif_offset_y;

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

int num_files;

#include "gifs_128.h"
#include "wrong_gif.h"
#include "llama_gif.h"

#define M0(x) {x, #x, sizeof(x)}
typedef struct {
    const unsigned char *data;
    const char *name;
    uint32_t sz;
} gif_detail_t;
gif_detail_t gifs[] = {

    M0(teakettle_128x128x10_gif),  // 21155
    M0(globe_rotating_gif),        // 90533
    M0(bottom_128x128x17_gif),     // 51775
    M0(irish_cows_green_beer_gif), // 29798
#if !defined(TEENSYDUINO)
    M0(horse_128x96x8_gif),        //  7868
#endif
#if defined(ESP32)
    M0(llama_driver_gif),    //758945
#endif
    //    M0(marilyn_240x240_gif),       // 40843
    //    M0(cliff_100x100_gif),   //406564
};


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

    Serial.begin(115200);
    //while (!Serial); delay(100);
    Serial.println("Animated GIFs Demo");

    // First call begin to mount the filesystem.  Check that it returns true
    // to make sure the filesystem was mounted.
    num_files = 0;

    if (initFileSystem(SD_CS) == 0) {
#ifdef USE_QSPIFS
      Serial.println("Found QSPI filesystem!");
#else // SD card
      Serial.println("Found SD Card!");
#endif
      num_files = enumerateGIFFiles(GIF_DIRECTORY, true);
    }
    if (num_files <= 0) {
      Serial.println("No QSPI or SD files found, using built-in demos");
      decoder.setFileSeekCallback(fileSeekCallback_P);
      decoder.setFilePositionCallback(filePositionCallback_P);
      decoder.setFileReadCallback(fileReadCallback_P);
      decoder.setFileReadBlockCallback(fileReadBlockCallback_P);
      g_gif = gifs[0].data;
      for (num_files = 0; num_files < sizeof(gifs) / sizeof(*gifs); num_files++) {
          Serial.println(gifs[num_files].name);
      }    
    }

    // Determine how many animated GIF files exist
    Serial.print("Animated GIF files Found: ");
    Serial.println(num_files);

    if (num_files < 0) {
        Serial.println("No gifs directory");
        while (1);
    }

    if (!num_files) {
        Serial.println("Empty gifs directory");
        while (1);
    }

    TFTBEGIN();
#ifdef _ILI9341_dueH_
    tft.setRotation((iliRotation)1);
#else
    tft.setRotation(1);
#endif
    tft.fillScreen(BLACK);
}

uint32_t futureTime, cycle_start;
int file_index = -1;

void loop() {

    if ((futureTime < millis() && (decoder.getCycleNo() > 1))) {  // at least one 'cycle'
        char buf[80];
        int32_t now = millis();
        int32_t frames = decoder.getFrameCount();
        int32_t cycle_design = decoder.getCycleTime();
        int32_t cycle_time = now - cycle_start;
        int32_t percent = (100 * cycle_design) / cycle_time;
        sprintf(buf, "[%ld frames = %ldms] actual: %ldms speed: %d%% Spent %d ms on drawing, %d ms on filesys",
                frames, cycle_design, cycle_time, percent, timeSpentDrawing, timeSpentFS);
        Serial.println(buf);
        cycle_start = now;
        //        num_files = 2;
        if (++file_index >= num_files) {
            file_index = 0;
        }

        int good;
        if (g_gif) {
          good = (openGifFilenameByIndex_P(GIF_DIRECTORY, file_index) >= 0);
        } else {
          good = (openGifFilenameByIndex(GIF_DIRECTORY, file_index) >= 0);
        }
        if (good < 0) {
          return;
        }
        timeSpentFS = timeSpentDrawing = 0;
        tft.fillScreen(g_gif ? BLACK : DISKCOLOUR);
        //tft.fillRect(GIFWIDTH, 0, 1, tft.height(), WHITE);
        //tft.fillRect(278, 0, 1, tft.height(), WHITE);

        decoder.startDecoding();

        uint16_t w, h;
        decoder.getSize(&w, &h);
        Serial.print("Width: "); Serial.print(w); Serial.print(" height: "); Serial.println(h);
        if (w < tft.width()) {
          gif_offset_x = (tft.width() - w) / 2;
        } else {
          gif_offset_x = 0;
        }
        if (h < tft.height()) {
          gif_offset_y = (tft.height() - h) / 2;
        } else {
          gif_offset_y = 0;
        }

        // Calculate time in the future to terminate animation
        futureTime = millis() + (DISPLAY_TIME_SECONDS * 1000);
    }

    decoder.decodeFrame();
}

/******************************* File or Memory Functions */

uint32_t g_seek;
bool fileSeekCallback_P(unsigned long position) {
    g_seek = position;
    return true;
}

unsigned long filePositionCallback_P(void) {
    return g_seek;
}

int fileReadCallback_P(void) {
    return pgm_read_byte(g_gif + g_seek++);
}

int fileReadBlockCallback_P(void * buffer, int numberOfBytes) {
    memcpy_P(buffer, g_gif + g_seek, numberOfBytes);
    g_seek += numberOfBytes;
    return numberOfBytes; //.kbv
}

bool openGifFilenameByIndex_P(const char *dirname, int index)
{
    gif_detail_t *g = &gifs[index];
    g_gif = g->data;
    g_seek = 0;

    Serial.print("Flash: ");
    Serial.print(g->name);
    Serial.print(" size: ");
    Serial.println(g->sz);

    return index < num_files;
}


/******************************* Drawing functions */

void updateScreenCallback(void) {
    ;
}

void screenClearCallback(void) {
    //    tft.fillRect(0, 0, 128, 128, 0x0000);
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    tft.drawPixel(x, y, tft.color565(red, green, blue));
}

void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip) {
    uint8_t pixel;
    bool first;
    x += gif_offset_x;
    y += gif_offset_y;
    if (y >= tft.height() || x >= tft.width() ) return;
    
    if (x + w > tft.width()) w = tft.width() - x;
    if (w <= 0) return;
    int16_t endx = x + w - 1;
    uint16_t buf565[w];
    uint32_t t = millis();
    for (int i = 0; i < w; ) {
        int n = 0;
        while (i < w) {
            pixel = buf[i++];
            if (pixel == skip) break;
            buf565[n++] = palette[pixel];
        }
        if (n) {
            tft.startWrite(); // Start SPI (regardless of transact)
            tft.setAddrWindow(x + i - n, y, w, 1);
            first = true;
#ifdef PUSHCOLORS
            PUSHCOLORS(buf565, n);
#else
            for (int j = 0; j < n; j++) PUSHCOLOR(buf565[j]);
#endif
            tft.endWrite();
        }
    }
    timeSpentDrawing += millis() - t;
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
