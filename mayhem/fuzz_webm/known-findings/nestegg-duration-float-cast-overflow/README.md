# nestegg: segment duration cast to uint64 out of range

`duration-overflow.webm` carries a Duration element that, multiplied by the timecode scale, is a float
outside the `uint64_t` range; `engine/source/webmlib/nestegg/nestegg.c:2021:15`
(`(uint64_t)(unscaled_duration * tc_scale)`) is UBSan `float-cast-overflow`.

Reproduce: `/mayhem/fuzz_webm-standalone mayhem/fuzz_webm/known-findings/nestegg-duration-float-cast-overflow/duration-overflow.webm`
