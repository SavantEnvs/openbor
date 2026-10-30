# PNG width that is not a multiple of 4 overflows the screen buffer

`width3.png` is a valid 3x2 RGBA PNG. `loadscreen32()` -> `pngToScreen()` allocates the destination with
`allocscreen()`, which rounds the width down (`width &= ~3`), then `png_read_row()` writes the full
3-pixel row, so the heap buffer is overrun (ASan `heap-buffer-overflow` in `memcpy`).

Reproduce: `/mayhem/fuzz_png-standalone mayhem/fuzz_png/known-findings/png-width-not-multiple-of-4/width3.png`

Kept out of `testsuite/` so the seeds stay crash-free. A second, related upstream issue is deliberately
masked by the harness; see `../png-truncated-overread`.
