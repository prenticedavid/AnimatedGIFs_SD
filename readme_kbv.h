Displaying images on a TFT with Arduino has several issues.

1. RAW image file on SD disk.  e.g. 16-bit pixels in 565
2. BMP file on disk.  e.g. 24-bit colour format (common)
3. BMP file on disk.  e.g. 16-bit colour 
4. BMP file on disk.  e.g. 8-bit palette (256 colours)
5. BMP file on disk.  e.g. 4-bit (16 colours) or 1-bit (monochrome)
6. JPG file on disk.  compresses very well.  
7. GIF file on disk.  compresses.  animations.

You can use tools on the PC to resize an image and store as BMP, JPG, GIF
SD cards are cheap and convenient.   Unlimited number of images.

RAW and BMP files can be rendered with a Uno or any small Arduino with 2kB SRAM.

JPG images require about 4kB SRAM.
GIF images require about 20kB SRAM.

Only the "bigger" ARM and Expressif boards have enough SRAM for GIF

Modern controllers have plenty of Flash.   You can store several JPG or GIF images.  
BMP (and RAW) images are very wasteful.   Only practical for small icons.

Storing a graphics image in a C array in PROGMEM has serious problem with AVR.
Arrays are limited to 32kB in size on an AVR.   
PROGMEM > 64kB needs pgm_read_byte_far().   No problem for rendering image.  
"Library" PROGMEM data must be kept < 64kB e.g. Fonts

ARM has no restriction on array size.  3.3V GPIO is kind to SD cards
Expressif can even use some Flash for a SPIFFS Filesystem

[code]
Uno:          32kB Flash   2kB SRAM
Mega2560:    256kB Flash   8kB SRAM
Zero:        256kB Flash  32kB SRAM
Due:         512kB Flash  90kB SRAM
Teensy3.2:   256kB Flash  64kB SRAM
ESP8266:    1024kB Flash  80kB SRAM
ESP32:      1024kB Flash 256kB SRAM
STM32F103RB  128kB Flash  20kB SRAM
STM32L476RE 1024kB Flash 128kB SRAM    
[/code]

Single AnimatedGIFs_SD sketch:
If SD or SPIFFS is found,  GIF is rendered from SD (BLUE background)
Else GIFis rendered from PROGMEM (MAGENTA background)

Debug output @ 115200 baud on Serial Terminal.
Most GIFs do not display Frames at design rate.

Each Frame is stored as a LZW image containing the minimal rectangle with changed pixels.
Transparent pixels are skipped.  Changed pixels are plotted.

Frame data must be read from SD or PROGMEM
Each Frame has a small header + LZW compressed image.
LZW must be decoded to N rows of uncompressed bytes.
Each byte represents a pixel from a 255 colour Palette (or special transparent byte)

It is expensive to read file from SD or SPIFFS
It is expensive for LZW decoder to read bytes via Callback function.
It is expensive to skip pixels.   You have to reset the Window after each skip.
It is expensive to lookup each colour from the Palette.

The UUP115.gif file was rendered from SD with a STM32F446 on a MCUFRIEND_kbv display.
Pathname: /gifsdbg2/UUP115.GIF
Logical Screen [LZW=8 320x240 P:0xF7 B:255 A:0 F:40ms] frames:0 pass=1
[72 frames = 2880ms] actual: 50635ms speed: 5% plot: 5504880 [14%] w=318

Each frame is different.  It takes a 720ms to decode and render each frame.   
It is clearly not possible to achieve a 40ms frame rate.

It might be possible to render uncompressed frames directly from SD.
It needs to read 154k SPI bytes per frame.   
This requires at least 42MHz SPI from SD card to get the raw data.
And a similar time for SPI data to be sent to the TFT.
So I doubt if you could do a 320x240 frame in less than 80ms.

David.
