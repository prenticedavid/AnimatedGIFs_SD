# AnimatedGIFs_SD
Display GIFs on TFT screens from Flash, SD or SPIFFS 

GIFs require at least 32kB of SRAM.   So this example can only run on Zero, Due, ESP8266, ESP32, Teensy3.x, STM32x4xx ...

All targets can use SPI libraries e.g TFT_eSPI, Adafruit_ST7735, Adafruit_ILI9341, ...
Most targets can use MCUFRIEND_kbv (except ESP8266)

Most chips can store some sample GIFs in Flash
All targets can render unlimited number of GIFs from SD  Card.
ESP8266 and ESP32 can also store some samples with SPIFFS.  Enable with USE_SPIFFS in FileNameFunctions.h

Note that you need a current ESP32 for SPIFFS.   And ESP is a little fussy about SD Card type.

Set DEBUG to 0, 2, 3 in GifDecoderImpl.h  

GIF optimisation via PC or online is not that clever.   
They should minimise the display rectangle to only cover animation changes.
They should make a compromisde between contiguous run of pixels or multiple transparent pixels.

Many thanks to Craig A. Lindley and Louis Beaudoin (Pixelmatix) for their original work on small LED matrix.
