// please read credits at the bottom of file

#define USE_TFT_LIB 0xE8266

#ifndef min
#define min(a, b) (((a) <= (b)) ? (a) : (b))
#endif
#include "GifDecoder.h"
#include "FilenameFunctions.h"    //defines USE_SPIFFS

#define DISPLAY_TIME_SECONDS 100  //
#define NUMBER_FULL_CYCLES     3  //
#define GIFWIDTH             480  //228 fails on COW_PAINT.  Edit class_implementation.cpp
#define FLASH_SIZE      512*1024  //     

/*  template parameters are maxGifWidth, maxGifHeight, lzwMaxBits

    The lzwMaxBits value of 12 supports all GIFs, but uses 16kB RAM
    lzwMaxBits can be set to 10 or 11 for small displays, 12 for large displays
    All 32x32-pixel GIFs tested work with 11, most work with 10
*/

GifDecoder<GIFWIDTH, 320, 12> decoder;

#if defined(USE_SPIFFS)
#define GIF_DIRECTORY "/"     //ESP8266 SPIFFS
#define DISKCOLOUR   CYAN
#else
#define GIF_DIRECTORY "/gifs"
//#define GIF_DIRECTORY "/gifs32"
//#define GIF_DIRECTORY "/gifsdbg"
#define DISKCOLOUR   BLUE
#endif

#include "USE_TFT_LIB.h"

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
long rowCount;  //.kbv
long plotCount; //.kbv
long skipCount; //.kbv
long lineTime;  //.kbv
int32_t parse_start; //.kbv

#include "gifs_128.h"
#include "wrong_gif.h"
#include "llama_gif.h"
#include "mad_man_gif.h"
#include "mad_race_gif.h"


#define M0(x) {x, #x, sizeof(x)}
typedef struct {
    const unsigned char *data;
    const char *name;
    uint32_t sz;
} gif_detail_t;
gif_detail_t gifs[] = {
#if FLASH_SIZE >= 1024 * 1024      //Teensy4.0, ESP32, F767, L476

    M0(llama_driver_gif),          //758945
    M0(teakettle_128x128x10_gif),  // 21155
    M0(bottom_128x128x17_gif),     // 51775
    M0(globe_rotating_gif),        // 90533

    //    M0(mad_man_gif),               //711166
    M0(mad_race_gif),              //173301
    //    M0(marilyn_240x240_gif),       // 40843
    //    M0(cliff_100x100_gif),   //406564
#elif FLASH_SIZE >= 512 * 1024     // Due, F446, ESP8266
    M0(teakettle_128x128x10_gif),  // 21155
    M0(bottom_128x128x17_gif),     // 51775
    M0(globe_rotating_gif),        // 90533
    M0(mad_race_gif),              //173301
    //    M0(marilyn_240x240_gif),       // 40843
    M0(horse_128x96x8_gif),        //  7868
#elif FLASH_SIZE >= 256 * 1024     //Teensy3.2, Zero
    M0(teakettle_128x128x10_gif),  // 21155
    M0(bottom_128x128x17_gif),     // 51775
    //    M0(globe_rotating_gif),        // 90533
    M0(mad_race_gif),              //173301
    M0(horse_128x96x8_gif),        //  7868
#else
    M0(teakettle_128x128x10_gif),  // 21155
    M0(globe_rotating_gif),        // 90533
    M0(bottom_128x128x17_gif),     // 51775
    M0(irish_cows_green_beer_gif), // 29798
    M0(horse_128x96x8_gif),        //  7868
#endif
};

const uint8_t *g_gif;
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

void screenClearCallback(void) {
    //    tft.fillRect(0, 0, 128, 128, 0x0000);
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
void updateScreenCallback(void) {
    ;
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    tft.drawPixel(x, y, tft.color565(red, green, blue));
    plotCount++;
    rowCount = 1;
}

void drawLineCallback(int16_t x, int16_t y, uint8_t *buf, int16_t w, uint16_t *palette, int16_t skip) {
    uint8_t pixel;
    bool first;
    int32_t t = micros();
    if (y >= tft.height() || x >= tft.width() ) return;
    if (x + w > tft.width()) w = tft.width() - x;
    if (w <= 0) return;
    int16_t endx = x + w - 1;
    uint16_t buf565[w];
    for (int i = 0; i < w; ) {
        int n = 0;
        while (i < w) {
            pixel = buf[i++];
            if (pixel == skip) {
                skipCount++;
                break;
            }
            buf565[n++] = palette[pixel];
        }
        if (n) {
            tft.setAddrWindow(x + i - n, y, endx, y);
            first = true;
            tft.pushColors(buf565, n, first);
        }
    }
    plotCount += w;  //count total pixels (including skipped)
    rowCount += 1;   //count number of drawLines
    lineTime += micros() - t;
}

// Setup method runs once, when the sketch starts
void setup() {
    char msg[80];
    Serial.begin(115200);
    while (!Serial) ;
    Serial.println("\nAnimatedGIFs_SD");
    tft.begin(tft.readID());
    tft.setRotation(1);
    tft.fillScreen(BLACK);
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(drawPixelCallback);
    decoder.setDrawLineCallback(drawLineCallback);

    int ret = initSdCard(SD_CS);
    if (ret == 0) {
        decoder.setFileSeekCallback(fileSeekCallback);
        decoder.setFilePositionCallback(filePositionCallback);
        decoder.setFileReadCallback(fileReadCallback);
        decoder.setFileReadBlockCallback(fileReadBlockCallback);
        num_files = enumerateGIFFiles(GIF_DIRECTORY, true);
    }
    if (ret != 0 || num_files == 0) {
        if (num_files == 0) sprintf(msg, "No GIF files on SD card");
        else sprintf(msg, "No SD card on CS:%d", SD_CS);
        Serial.println(msg);
        decoder.setFileSeekCallback(fileSeekCallback_P);
        decoder.setFilePositionCallback(filePositionCallback_P);
        decoder.setFileReadCallback(fileReadCallback_P);
        decoder.setFileReadBlockCallback(fileReadBlockCallback_P);
        g_gif = gifs[0].data;
        for (num_files = 0; num_files < sizeof(gifs) / sizeof(*gifs); num_files++) {
            Serial.println(gifs[num_files].name);
        }
    }

    sprintf(msg, "Animated GIF files Found: %d", num_files);
    if (num_files < 0) sprintf(msg, "No directory on %s", GIF_DIRECTORY);
    if (num_files == 0) sprintf(msg, "No GIFs on %s", GIF_DIRECTORY);
    Serial.println(msg);
    if (num_files > 0) return;
    tft.fillScreen(RED);
    tft.println(msg);
    while (1) delay(10);  //does a yield()
}

void loop() {
    static unsigned long futureTime, cycle_start, nextFrameTime, frame_time, frames;

    //    int index = random(num_files);
    static int index = -1;

    int32_t now = millis();
    if (now >= futureTime || decoder.getCycleNo() > NUMBER_FULL_CYCLES) {
        char buf[100];
        int32_t frameCount = decoder.getFrameCount();
        if (frameCount > 0) {   //complete animation sequence
            int32_t framedelay = decoder.getFrameDelay_ms();
            int32_t cycle_design = framedelay * frameCount;
            int32_t cycle_time = now - cycle_start;
            int32_t percent = (100 * cycle_design) / cycle_time;
            int32_t skipcent = plotCount ? (100 * skipCount) / (plotCount) : 0;
            int32_t avg_wid = rowCount ? plotCount / rowCount : 0;
            int32_t avg_ht = rowCount / frames;
            percent = (100 * frames * framedelay) / (frame_time / 1000);
            if (percent > 100) percent = 100;

            sprintf(buf, "%3d frames @%3dms %3d%% [%3d] typ: %3dx%-3d ",
                    frameCount, framedelay, percent, frames,
                    avg_wid, avg_ht);
            Serial.print(buf);

            float map = 0.001 / frames;
            char ft[10], dt[10];
            dtostrf(frame_time * map, 5, 1, ft);
            dtostrf(lineTime * map, 5, 1, dt);
            sprintf(buf, "avg:%sms draw:%sms %d%%", ft, dt, skipcent);
            Serial.println(buf);
        }
        skipCount = plotCount = rowCount = lineTime = frames = frame_time = 0L;

        cycle_start = now;
        // Calculate time in the future to terminate animation
        futureTime = now + (DISPLAY_TIME_SECONDS * 1000);

        if (++index >= num_files) {
            index = 0;
        }

        int good;
        if (g_gif) good = (openGifFilenameByIndex_P(GIF_DIRECTORY, index) >= 0);
        else good = (openGifFilenameByIndex(GIF_DIRECTORY, index) >= 0);
        if (good >= 0) {
            tft.fillScreen(g_gif ? MAGENTA : DISKCOLOUR);
            tft.fillRect(GIFWIDTH, 0, 1, tft.height(), WHITE);
            tft.fillRect(278, 0, 1, tft.height(), WHITE);

            decoder.startDecoding();

        }
    }

    parse_start = micros();
    decoder.decodeFrame();
    yield();
    frame_time += micros() - parse_start; //count it even if housekeeping block
    if (decoder.getFrameNo() != 0) {  //don't count the header blocks.
        frames++;
        nextFrameTime = now + decoder.getFrameDelay_ms();
        while (millis() <= nextFrameTime) yield();
    }
    yield();

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
