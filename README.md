# Adafruit_Arcada_GifDecoder

Display GIFs on TFT screens from Flash or SD, using Adafruit Arcada. GIFs reside in a directory "/gifs" on the filesystem. If using QSPI storage and TinyUSB stack, the filesystem will be available as a USB disk drive

This example has been deeply modified from https://github.com/prenticedavid/AnimatedGIFs_SD to be for use with Adafruit Arcada (SAMD51) boards only at this time. 

--------------------------------------

Original Credits (see also other readme)

Set DEBUG to 0, 2, 3 in GifDecoderImpl.h  

GIF optimisation via PC or online is not that clever.   
They should minimise the display rectangle to only cover animation changes.
They should make a compromisde between contiguous run of pixels or multiple transparent pixels.

Many thanks to Craig A. Lindley and Louis Beaudoin (Pixelmatix) for their original work on small LED matrix.
