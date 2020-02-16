/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to decompress the LZW encoded animated GIF data
 *
 * Written by: Craig A. Lindley, Fabrice Bellard and Steven A. Bennett
 * See my book, "Practical Image Processing in C", John Wiley & Sons, Inc.
 *
 * Copyright (c) 2014 Craig A. Lindley
 * Minor modifications by Louis Beaudoin (pixelmatix)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define LZWDEBUG 0

#if defined (ARDUINO)
#include <Arduino.h>
#elif defined (SPARK)
#include "application.h"
#endif

#include "GifDecoder.h"

template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::lzw_setTempBuffer(uint8_t * tempBuffer) {
    temp_buffer = tempBuffer;
}

// Initialize LZW decoder
//   csize initial code size in bits
//   buf input data
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
void GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::lzw_decode_init (int csize) {

    // Initialize read buffer variables
    bbuf = 0;
    bbits = 0;
    bs = 0;
    bcnt = 0;

    // Initialize decoder variables
    codesize = csize;
    cursize = codesize + 1;
    curmask = mask[cursize];
    top_slot = 1 << cursize;
    clear_code = 1 << codesize;
    end_code = clear_code + 1;
    slot = newcodes = clear_code + 2;
    oc = fc = -1;
    sp = stack;
}

//  Get one code of given number of bits from stream
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
inline int GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::lzw_get_code() {

    while (bbits < cursize) {
        if (bcnt == bs) {
            if (bs == 0) // first time through, we don't know the next block size
            {
                readIntoBuffer(temp_buffer, 1);
                bs = temp_buffer[0];
            }
            else
            {
                bs = temp_buffer[bs];
            }
            readIntoBuffer(temp_buffer, bs+1);
            bcnt = 0;
        }
        bbuf |= temp_buffer[bcnt] << bbits;
        bbits += 8;
        bcnt++;
    }
    int c = bbuf;
    bbuf >>= cursize;
    bbits -= cursize;
    return c & curmask;
}

#define ADD_STACK_BYTE(b) accum <<= 8; accum |= b; accum_count--; if (accum_count == 0) {*(uint32_t *)sp_l = accum; sp_l += 4; accum=0; accum_count=4;}

#define DUMP_STACK_BYTES {*(uint32_t *)sp_l = accum; sp_l += 4-accum_count; accum=0; accum_count=4;}
// Decode given number of bytes
//   buf 8 bit output buffer
//   len number of pixels to decode
//   returns the number of bytes decoded
// .kbv add optional number of pixels to skip i.e. align
template <int maxGifWidth, int maxGifHeight, int lzwMaxBits>
int GifDecoder<maxGifWidth, maxGifHeight, lzwMaxBits>::lzw_decode(uint8_t *buf, int len, uint8_t *bufend) /*, int align)*/ {
    int l, c, code;
    // Local copies of class member vars allows the compiler to save a few cycles
    const int newcode_l = newcodes;
    uint8_t *sp_l = sp;
    int top_slot_l = top_slot;
    int slot_l = slot;
    int bbuf_l = bbuf;
    uint8_t *suf_pre = &suffix_prefix[0]; // suffix and prefix data in a single buffer (eliminates a variable)
    
#if LZWDEBUG == 1
    unsigned char debugMessagePrinted = 0;
#endif

    if (end_code < 0) {
        return 0;
    }
    l = len;

    for (;;) {
        while (sp_l > stack) {
            uint8_t q = *(--sp_l);     //.kbv pull off stack anyway
            l--;
            // load buf with data if we're still within bounds
            if (buf < bufend) {
                *buf++ = q; // a decent amount of time is spent here, but it's needed to reverse the output of the lzw compressed strings
            } else {
                // out of bounds, keep incrementing the pointers, but don't use the data
#if LZWDEBUG == 1
                // only print this message once per call to lzw_decode
                if (buf == bufend)
                    Serial.println("****** LZW imageData buffer overrun *******");
#endif
            }
            if (l == 0) {
                sp = sp_l; // store them back in the member vars
                slot = slot_l;
                top_slot = top_slot_l;
                bbuf = bbuf_l;
                return len;
            }
        }
       // about 15% of the time is spent here
        while (bbits < lzwMaxBits) {
            if (bcnt == bs) {
                if (bs == 0) { // first time through, we don't know the next block size
                    readIntoBuffer(temp_buffer, 1);
                    bs = temp_buffer[0];
                }
                else { // the next block size has already been read the last time through
                    bs = temp_buffer[bs];
                }
                readIntoBuffer(temp_buffer, bs+1); // the the current data + next block size
                bcnt = 0;
            }
            // It's safe to do this because we're assured the start of temp_buffer is
            // on an even address and we always increment by 2
            // and the fact that GIF data is encoded as little-endian
            bbuf_l |= ( *(uint16_t *)&temp_buffer[bcnt] ) << bbits;
            bbits += 16;
            bcnt += 2;
            if (bcnt > bs) {// check for an odd byte at the end of the buffer
                bcnt--;
                bbits -= 8; // last odd byte
                bbuf_l &= (0xffffffff >> (32 - bbits));
            }
        }
        c = bbuf_l;
        bbuf_l >>= cursize;
        bbits -= cursize;
        c &= curmask;
        if (c == end_code) {
            break;
        }
        else if (c == clear_code) {
            cursize = codesize + 1;
            curmask = mask[cursize];
            slot_l = newcodes;
            top_slot_l = 1 << cursize;
            fc = oc = -1;
        }
        else    {

            code = c;
            if ((code == slot_l) && (fc >= 0)) {
                *sp_l++ = fc;
                code = oc;
            }
            else if (code >= slot_l) {
                break;
            }
            // Most of the time is spent in this loop
            // It's dfficult to optimize because loads
            // are used/tested immediately - which causes pipeline stalls
            while (code >= newcode_l) {
                *sp_l++ = suf_pre[code];
                code = *(uint16_t *)&suf_pre[code*2 + LZW_SIZTABLE]; // code = prefix[code]
            }
            *sp_l++ = code;
            if ((slot_l < top_slot_l) && (oc >= 0)) {
                suf_pre[slot_l] = code;
                *(uint16_t *)&suf_pre[(slot_l++)*2 + LZW_SIZTABLE] = oc; // prefix[slot_l] = oc
            }
            fc = code;
            oc = c;
            if (slot_l >= top_slot_l) {
                if (cursize < lzwMaxBits) {
                    top_slot_l <<= 1;
                    curmask = mask[++cursize];
                } else {
#if LZWDEBUG == 1
                    if (!debugMessagePrinted) {
                        debugMessagePrinted = 1;
                        Serial.println("****** cursize >= lzwMaxBits *******");
                    }
#endif
                }
            }
        }
    }
    end_code = -1;
    sp = sp_l; // save local copies back in member vars
    slot = slot_l;
    top_slot = top_slot_l;
    bbuf = bbuf_l;
    return len - l;
}
