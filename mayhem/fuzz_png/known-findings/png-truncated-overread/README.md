# Truncated PNG over-reads the file buffer

`loadscreen32()` reads the file into a buffer of exactly the file's size and hands the bare pointer to
`pngToScreen()` (`engine/source/pnglib/pngdec.c`), which has no length parameter. `png_read_fn()` then
`memcpy()`s whatever libpng asks for, so a PNG whose chunks run past the end of the file reads beyond the
heap buffer (ASan `heap-buffer-overflow`, READ in `png_read_fn` at `pngdec.c:32`, 0 bytes after the 45-byte
region).

`truncated.png` is a valid 4x4 RGBA PNG cut inside its IDAT chunk (45 bytes; the width is a multiple of 4,
so the `png-width-not-multiple-of-4` bug does not apply).

`fuzz_png.c` masks this: `seal_png()` rejects any input whose chunk walk does not reach IEND with every chunk
in bounds, so the fuzzer cannot hit this over-read and spend the run on it. It does not hide any other path;
everything that passes `seal_png()` is loaded unchanged (with its CRCs recomputed).

`repro.c` is the harness's `loadscreen32()` call without `seal_png()`. Build it against the sanitized engine
objects of the image (`build/mayhem-fuzz/engine_*.o`, `$LIBS` as in `mayhem/build.sh`), then run
`repro mayhem/fuzz_png/known-findings/png-truncated-overread/truncated.png`.
