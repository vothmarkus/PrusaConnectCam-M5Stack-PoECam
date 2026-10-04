#!/usr/bin/env bash
set -euo pipefail
repo=$(cd "$(dirname "$0")/.." && pwd)
build="$repo/.build/tests"
mkdir -p "$build"
cxx=${CXX:-g++}
cc=${CC:-gcc}
flags=(-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie)
"$cxx" -std=c++17 -Wall -Wextra -Werror "${flags[@]}" -no-pie "$repo/tests/protocol_test.cpp" -o "$build/protocol_test"
"$build/protocol_test"
objects=()
for source in quirc decode identify version_db collections; do
  "$cc" -std=gnu11 "${flags[@]}" -Dmalloc=test_malloc -Dfree=test_free \
    -include "$repo/tests/alloc_hooks.h" -c "$repo/ESP32_PrusaConnectCam_web/$source.c" -o "$build/$source.o"
  objects+=("$build/$source.o")
done
"$cxx" -std=c++17 -Wall -Wextra -Werror "${flags[@]}" -no-pie \
  "$repo/tests/quirc_test.cpp" "${objects[@]}" -lm -o "$build/quirc_test"
"$build/quirc_test"
