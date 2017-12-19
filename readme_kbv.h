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

I have attached two GIF sketches.  One for rendering animations from Flash.
One for rendering animations from SD card.  Copy example files to /GIF directory.

David.
