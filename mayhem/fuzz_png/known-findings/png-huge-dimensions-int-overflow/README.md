# PNG with huge dimensions: signed overflow computing the screen size

`huge-dimensions.png` has a width/height whose product with the pixel size overflows `int`
(`engine/source/gamelib/bitmap.c:24:14`, UBSan `signed integer overflow`); the engine then sizes the
allocation from the wrapped value.

Reproduce: `/mayhem/fuzz_png-standalone mayhem/fuzz_png/known-findings/png-huge-dimensions-int-overflow/huge-dimensions.png`
