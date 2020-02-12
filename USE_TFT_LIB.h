// 08.02.2020 added 0xF37735 for ST7735_t3
// 08.02.2020 pushColors(... bigend) for ILI9341_due
// 09.02.2020 SD_CS=10 for MCUFRIEND_kbv 

#if 0
#elif defined(STM32F1xx) && defined(BLUEPILL)
#define TFT_CS  PA15
#define TFT_DC  PA1
#define TFT_RST PA0
#define SD_CS   PA4   //BLUEPILL
#elif defined(STM32F1xx) && defined(MY_BLUEPILL)
#define TFT_CS  PB12
#define TFT_DC  PA10
#define TFT_RST PA9
#define SD_CS   PA0   //MY_BLUEPILL
#elif defined(ESP32)
#define TFT_CS  5
#define TFT_DC  13
#define TFT_RST 12
#define SD_CS 17
#elif defined(ESP8266)
#define TFT_CS  D10
#define TFT_DC  D9
#define TFT_RST D8
#define SD_CS   D4
#else
#define TFT_CS  10
#define TFT_DC  9
#define TFT_RST 8
#define TFT_NO_CS 7
#define SD_CS   4
#endif

#include "SPI.h"
#if 0

#elif USE_TFT_LIB == 0x7735
#include <ST7789_kbv.h>
ST7789_kbv tft(0x7735, 128, 160);
#elif USE_TFT_LIB == 0x7789
#include <ST7789_kbv.h>
ST7789_kbv tft;
#elif USE_TFT_LIB == 0x8347
#include <HX8347D_kbv.h>
HX8347D_kbv tft;
#define SD_CS  5
#elif USE_TFT_LIB == 0x9101
#include <ST7789_kbv.h>
ST7789_kbv tft(0x9101, 128, 128);
#elif USE_TFT_LIB == 0x9163
#include <ST7789_kbv.h>
ST7789_kbv tft(0x9163, 128, 128);
#elif USE_TFT_LIB == 0x9225
#include <ILI9225_kbv.h>
ILI9225_kbv tft(TFT_CS, TFT_DC, TFT_RST);
#elif USE_TFT_LIB == 0x9341
#include <ST7789_kbv.h>
ST7789_kbv tft(0x9341, 240, 320);
#elif USE_TFT_LIB == 0x9481
#include <ST7789_kbv.h>
ST7789_kbv tft(0x9481, 320, 480);
#elif USE_TFT_LIB == 0x9488
#include <ST7789_kbv.h>
ST7789_kbv tft(0x9488, 320, 480);

#elif USE_TFT_LIB == 0x0000 //MCUFRIEND detects the controller
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#define SD_CS  10

#elif USE_TFT_LIB == 0xA7735   //repeat for 7735, 7789, 9341, ...
#include <Adafruit_ST7735.h>
class KBVAdafruit_ST7735 : public Adafruit_ST7735
{
    public:
        KBVAdafruit_ST7735(uint8_t CS, uint8_t RS, uint8_t RST) : Adafruit_ST7735(CS, RS, RST) {}
        void     begin(uint16_t ID = 0x7735) {
            Adafruit_ST7735::initR(INITR_BLACKTAB);
        }
        uint16_t readID(void) {
            return 0x7735;
        }
        void     setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1) {
            startWrite();
            Adafruit_ST7735::setAddrWindow(x, y, x1 - x + 1, y1 - y + 1);
            endWrite();
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            startWrite();
            writePixels(block, n, true, false);
            endWrite();
        }
        void     pushColors(const uint8_t *block, int16_t n, bool first, bool bigend = false) {
            uint8_t h, l;
            uint16_t color;
            while (n-- > 0) {
                l = pgm_read_byte(block++); //all targets are little-endian
                h = pgm_read_byte(block++);
                if (bigend) color = (l << 8) | h;
                else color = (h << 8) | l;
                pushColor(color);
            }
        }
};
KBVAdafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

#elif USE_TFT_LIB == 0xA8357   //repeat for 7735, 7789, 9341, ...
#include <Adafruit_HX8357.h>
class KBVAdafruit_HX8357 : public Adafruit_HX8357
{
    public:
        KBVAdafruit_HX8357(uint8_t CS, uint8_t RS, uint8_t RST) : Adafruit_HX8357(CS, RS, RST) {}
        void     begin(uint16_t ID = 0x8357) {
            Adafruit_HX8357::begin();
        }
        uint16_t readID(void) {
            return 0x8357;
        }
        void     setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1) {
            startWrite();
            Adafruit_HX8357::setAddrWindow(x, y, x1 - x + 1, y1 - y + 1);
            endWrite();
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            startWrite();
            writePixels(block, n, true, false);
            endWrite();
        }
};
KBVAdafruit_HX8357 tft(TFT_CS, TFT_DC, TFT_RST);

#elif USE_TFT_LIB == 0xA9341   //repeat for 7735, 7789, 9341, ...
//#include <Adafruit_ZeroDMA.h>  //.kbv
#include <Adafruit_ILI9341.h>
class KBVAdafruit_ILI9341 : public Adafruit_ILI9341
{
    public:
        KBVAdafruit_ILI9341(uint8_t CS, uint8_t RS, uint8_t RST) : Adafruit_ILI9341(CS, RS, RST) {}
        void     begin(uint16_t ID = 0x9341) {
            Adafruit_ILI9341::begin();
        }
        uint16_t readID(void) {
            return 0x9341;
        }
        void     setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1) {
            startWrite();
            Adafruit_ILI9341::setAddrWindow(x, y, x1 - x + 1, y1 - y + 1);
            endWrite();
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            startWrite();
            writePixels(block, n, true, false);
            endWrite();
        }
};
KBVAdafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

#elif USE_TFT_LIB == 0xD9341   //
#include <ILI9341_due.h>
#include "fonts\SystemFont5x7.h"

class KBVILI9341_due: public ILI9341_due
{
    public:
        KBVILI9341_due(uint8_t CS, uint8_t RS, uint8_t RST) : ILI9341_due(CS, RS, RST) {}
        void begin(uint16_t ID = 0x9341) {
            ILI9341_due::begin();
            ILI9341_due::setFont(SystemFont5x7);
            ILI9341_due::setTextLineSpacing(1);
            ILI9341_due::setTextLetterSpacing(1);
        }
        uint16_t readID(void) {
            return 0x9341;    //.kbv
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            ILI9341_due::pushColors(block, 0, n);
        }
        void     pushColors(const uint8_t *block, int16_t n, bool first, bool bigend = false) {
            //ILI9341_due::pushColors((const uint16_t*)block, 0, n);
            uint8_t h, l;
            uint16_t color;
            while (n-- > 0) {
                if (!bigend) l = pgm_read_byte(block++);
                h = pgm_read_byte(block++);
                if (bigend) l = pgm_read_byte(block++);
                color = (h << 8) | l;
                pushColor(color);
            }
        }
        void setTextSize(uint8_t sz) {
            ILI9341_due::setTextScale(sz);
        }
        void setCursor(uint16_t x, uint16_t y) {
            ILI9341_due::cursorToXY(x, y);
        }
        void setRotation(uint8_t aspect) {
            ILI9341_due::setRotation((iliRotation)aspect);
        }
};
KBVILI9341_due tft(TFT_CS, TFT_DC, TFT_RST);

#elif USE_TFT_LIB == 0xE8266   //Bodmer
#include <TFT_eSPI.h>
class KBVTFT_eSPI : public TFT_eSPI
{
    public:
        KBVTFT_eSPI() : TFT_eSPI() {}
        void     begin(uint16_t ID = 0x9341) {
            TFT_eSPI::begin();
        }
        uint16_t readID(void) {
            return 0x9341;
        }
        void     setAddrWindow(int16_t x, int16_t y, int16_t x1, int16_t y1) {
            TFT_eSPI::setAddrWindow(x, y, x1 - x + 1, y1 - y + 1);
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            TFT_eSPI::pushColors(block, n);
        }
};
KBVTFT_eSPI tft;

#elif USE_TFT_LIB == 0xF37735
#include <ST7735_t3.h>

class KBVST7735_t3 : public ST7735_t3
{
    public:
        //GLUEST7735_t3(uint8_t CS, uint8_t RS, uint8_t SID, uint8_t SCLK, uint8_t RST, uint8_t MISO) : ST7735_t3(CS, RS, RST, SID, SCLK, MISO) {}
        KBVST7735_t3(uint8_t CS, uint8_t RS, uint8_t RST) : ST7735_t3(CS, RS, RST) {}
        void begin(uint16_t ID = 0x7735) {
            ST7735_t3::initR(INITR_GREENTAB);
        }
        uint16_t readID(void) {
            return 0x7735;
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            beginSPITransaction();
            if (first) writecommand(0x2C);
            while (n > 1) {
                n--;
                writedata16(*block++);
            }
            writedata16(*block);
            endSPITransaction();
        }
        void     pushColors(const uint8_t *block, int16_t n, bool first, bool bigend = false) {
            uint8_t h, l;
            uint16_t color;
            beginSPITransaction();
            if (first) writecommand(0x2C);
            while (n-- > 0) {
                if (!bigend) l = pgm_read_byte(block++);
                h = pgm_read_byte(block++);
                if (bigend) l = pgm_read_byte(block++);
                color = (h << 8) | l;
                if (n) writedata16(color);
                else writedata16(color);
            }
            endSPITransaction();
        }
        uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
            return ST7735_t3::Color565(r, g, b);
        }
};
KBVST7735_t3 tft(TFT_CS, TFT_DC, TFT_RST);

#elif USE_TFT_LIB == 0xF39341
#include <ILI9341_t3.h>

class KBVILI9341_t3 : public ILI9341_t3
{
    public:
        //GLUEILI9341_t3(uint8_t CS, uint8_t RS, uint8_t SID, uint8_t SCLK, uint8_t RST, uint8_t MISO) : ILI9341_t3(CS, RS, RST, SID, SCLK, MISO) {}
        KBVILI9341_t3(uint8_t CS, uint8_t RS, uint8_t RST) : ILI9341_t3(CS, RS, RST) {}
        void begin(uint16_t ID = 0x9341) {
            ILI9341_t3::begin();
        }
        uint16_t readID(void) {
            return 0x9341;
        }
        void     pushColors(uint16_t *block, int16_t n, bool first) {
            beginSPITransaction();
            if (first) writecommand_cont(0x2C);
            while (n > 1) {
                n--;
                writedata16_cont(*block++);
            }
            writedata16_last(*block);
            endSPITransaction();
        }
        void     pushColors(const uint8_t *block, int16_t n, bool first, bool bigend = false) {
            uint8_t h, l;
            uint16_t color;
            beginSPITransaction();
            if (first) writecommand_cont(0x2C);
            while (n-- > 0) {
                if (!bigend) l = pgm_read_byte(block++);
                h = pgm_read_byte(block++);
                if (bigend) l = pgm_read_byte(block++);
                color = (h << 8) | l;
                if (n) writedata16_cont(color);
                else writedata16_last(color);
            }
            endSPITransaction();
        }
};
KBVILI9341_t3 tft(TFT_CS, TFT_DC, TFT_RST);

#else
#error Please specify TFT library
#endif


#ifndef TFT_BLACK
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F
#endif
