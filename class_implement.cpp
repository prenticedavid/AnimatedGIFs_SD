//.kbv I really, really do not like this.   All the defines from the xxx_Impl.h files are read before setup() and loop()
//.kbv I suppose that you could include at the end of AnimatedGIFs_SD.ino
//.kbv I am happier with adding a statement to CPP file e.g. template class GifDecoder<480, 320, 12>;   // .kbv tell the world.

#include "GifDecoder_Impl.h"
#include "LzwDecoder_Impl.h"

template class GifDecoder<480, 320, 12>;   // .kbv tell the world.
