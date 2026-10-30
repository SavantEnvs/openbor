#!/usr/bin/env bash
#
# mayhem/test.sh — run the OpenBOR known-answer oracle that mayhem/build.sh built (no compiling here).
#
# /mayhem/openbor_kat links the real engine (normal flags, no sanitizers) and asserts known inputs ->
# known outputs through it: pak32/pak64 archive reads (names, sizes, contents, missing entries), PNG
# decode (RGBA pixel values and a palette), and the script compiler/interpreter (computed results).
# borpak, the packer built from tools/borpak, is also checked against the fixture archive.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

if [ ! -x "$SRC/openbor_kat" ] || [ ! -x "$SRC/borpak" ]; then
  echo "test.sh: openbor_kat/borpak missing — mayhem/build.sh did not produce them" >&2
  emit_ctrf "openbor-kat" 0 1
  exit 1
fi

out="$("$SRC/openbor_kat" 2>&1)"; rc=$?
printf '%s\n' "$out"
pass=$(printf '%s\n' "$out" | grep -c '^KAT PASS ')
fail=$(printf '%s\n' "$out" | grep -c '^KAT FAIL ')
[ "$rc" -eq 0 ] || fail=$(( fail + 1 ))
[ "$pass" -gt 0 ] || fail=$(( fail + 1 ))

# borpak -l must list exactly the entries the fixture archive was built with.
lst="$("$SRC/borpak" -l mayhem/kat/sample32.pak 2>&1)"
for name in 'hero.txt' 'blob.bin' 'test.c' 'tile.png'; do
  if printf '%s\n' "$lst" | grep -qi "$name"; then pass=$(( pass + 1 )); echo "KAT PASS borpak-list-$name"
  else fail=$(( fail + 1 )); echo "KAT FAIL borpak-list-$name"; fi
done

if printf '%s\n' "$lst" | grep -q 'finished: 4 files'; then pass=$(( pass + 1 )); echo "KAT PASS borpak-list-count"
else fail=$(( fail + 1 )); echo "KAT FAIL borpak-list-count"; fi

emit_ctrf "openbor-kat" "$pass" "$fail"
