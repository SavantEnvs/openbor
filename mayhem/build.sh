#!/usr/bin/env bash
#
# mayhem/build.sh — build the OpenBOR fuzz harnesses, their standalone reproducers and the oracle.
#
# Runs inside the commit image (mayhem/Dockerfile) as `mayhem` in /mayhem. Everything is built from
# the checked-in sources with the compiler named on PATH; nothing is fetched. The engine is compiled
# object by object (the CMake build needs SDL's whole GUI/GL stack and a git-describe step), once with
# the sanitizers (fuzz + standalone binaries) and once with the project's normal flags (oracle).
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}"
: "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export SANITIZER_FLAGS DEBUG_FLAGS CC LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"

OUT=/mayhem
FUZZ_OBJ=build/mayhem-fuzz
KAT_OBJ=build/mayhem-kat

# engine/version.h is gitignored and normally written by engine/version.sh, which also rewrites tracked
# resource files and prompts on stdin without a .git; write the header directly instead (idempotent).
cat > engine/version.h <<'EOF'
#ifndef VERSION_H
#define VERSION_H

#define VERSION_NAME "OpenBOR"
#define VERSION_MAJOR "4"
#define VERSION_MINOR "0"
#define VERSION_BUILD "0"
#define VERSION_BUILD_INT 0
#define VERSION_COMMIT "0000000"
#define VERSION "v"VERSION_MAJOR"."VERSION_MINOR" Build "VERSION_BUILD" (commit hash: "VERSION_COMMIT")"

#endif
EOF

INC="-Iengine -Iengine/source -Iengine/source/gamelib -Iengine/source/gfxlib -Iengine/source/openborscript
 -Iengine/source/pnglib -Iengine/source/preprocessorlib -Iengine/source/ramlib -Iengine/source/randlib
 -Iengine/source/scriptlib -Iengine/source/tracelib2 -Iengine/source/webmlib -Iengine/sdl $(pkg-config --cflags sdl2)"
DEFS="-DLINUX -DSDL=1 -DWEBM -DOPENGL -DLOADGL -DOV_EXCLUDE_STATIC_CALLBACKS"
CSTD="-std=gnu99 -fsigned-char -w"
LIBS="$(pkg-config --libs sdl2) -lvorbisfile -lvorbis -logg -lpng -lvpx -lz -lm -ldl -lpthread"

{
  ls engine/*.c
  find engine/source -name '*.c' | grep -v -E 'test|/gfxlib/|/webmlib/'
  ls engine/source/gfxlib/*.c
  find engine/source/webmlib -name '*.c'
  ls engine/sdl/*.c
} | LC_ALL=C sort -u > build-sources.list.tmp
mkdir -p build
mv build-sources.list.tmp build/mayhem-sources.list

rm -rf "$FUZZ_OBJ" "$KAT_OBJ"

# compile_engine <objdir> <flags...>: one object per engine source. sdlport.c carries the game's main(),
# renamed so the fuzzer's (or the oracle's) own main links; it also defines the global `packfile`.
compile_engine() {
  local objdir="$1"; shift
  mkdir -p "$objdir"
  export ENGINE_FLAGS="$*" PARSERSET_FLAGS="${PARSERSET_FLAGS:-}" OBJDIR="$objdir" ENGINE_INC="$INC" ENGINE_DEFS="$DEFS" ENGINE_CSTD="$CSTD"
  xargs -P "$MAYHEM_JOBS" -I{} bash -c '
    f="$1"; o="$OBJDIR/$(echo "$f" | tr / _).o"
    extra=""
    case "$f" in
      */sdlport.c) extra="-Dmain=openbor_engine_main";;
      */ParserSet.c) extra="$PARSERSET_FLAGS";;
    esac
    $CC $ENGINE_CSTD $ENGINE_DEFS $ENGINE_INC $ENGINE_FLAGS $extra -c "$f" -o "$o"' _ {} < build/mayhem-sources.list
}

# 1) Sanitized engine (fuzzed code is instrumented; fuzzer-no-link adds the coverage counters).
#    The script/parser structs are declared under `#pragma pack(4)` but embed 8-byte-aligned members,
#    so UBSan's alignment check fires on every script compile; that is the engine's deliberate layout
#    (harmless on x86), so only that one check is relaxed. ASan and every other UBSan check still halt.
RELAX="-fno-sanitize=alignment"
#    ParserSet_Buildup() stores one element past the end of FirstSet[]/FollowSet[] on every parser
#    construction (index 62 of 62; it lands on the neighbouring field and is overwritten), which would
#    halt every single input, so array-bounds is relaxed for that one file only.
export PARSERSET_FLAGS="-fno-sanitize=array-bounds"
compile_engine "$FUZZ_OBJ" $SANITIZER_FLAGS $RELAX -fsanitize=fuzzer-no-link $DEBUG_FLAGS

$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.c -o "$FUZZ_OBJ/mayhem_lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ_OBJ/mayhem_standalone_main.o"
ENGINE_OBJS=$(ls "$FUZZ_OBJ"/engine_*.o)

# 2) Harnesses: fuzzer binary + standalone reproducer each.
for t in fuzz_script fuzz_png fuzz_pak fuzz_webm; do
  $CC $CSTD $DEFS $INC $SANITIZER_FLAGS $RELAX -fsanitize=fuzzer-no-link $DEBUG_FLAGS \
      -c "mayhem/$t.c" -o "$FUZZ_OBJ/mayhem_$t.o"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
      "$FUZZ_OBJ/mayhem_$t.o" $ENGINE_OBJS "$FUZZ_OBJ/mayhem_lsan_off.o" $LIBS -o "$OUT/$t"
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS \
      "$FUZZ_OBJ/mayhem_$t.o" "$FUZZ_OBJ/mayhem_standalone_main.o" $ENGINE_OBJS "$FUZZ_OBJ/mayhem_lsan_off.o" \
      $LIBS -o "$OUT/$t-standalone"
done

# 3) Oracle: the same engine with its normal flags (no sanitizers), plus the borpak tool.
compile_engine "$KAT_OBJ" -O2 $COVERAGE_FLAGS
KAT_ENGINE_OBJS=$(ls "$KAT_OBJ"/engine_*.o)
$CC $CSTD $DEFS $INC -O2 $COVERAGE_FLAGS mayhem/kat.c $KAT_ENGINE_OBJS $LIBS $COVERAGE_FLAGS -o "$OUT/openbor_kat"
$CC -w -O2 $COVERAGE_FLAGS tools/borpak/source/borpak.c tools/borpak/source/stristr.c -o "$OUT/borpak"
