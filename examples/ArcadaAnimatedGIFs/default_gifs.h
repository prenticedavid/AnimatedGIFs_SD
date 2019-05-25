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
