#!/bin/sh
# DEV MODE (macOS / Linux): build the previewer and render every scene to dev/out/
cd "$(dirname "$0")/.." || exit 1
CXX=${CXX:-$(command -v g++ || command -v clang++)}
[ -z "$CXX" ] && { echo "Need a C++ compiler (macOS: xcode-select --install, Linux: apt install g++)"; exit 1; }
"$CXX" -O2 -std=c++17 dev/preview.cpp -o dev/preview || exit 1
./dev/preview "$@" || exit 1
OUT=dev/out/index.html
command -v open >/dev/null && open "$OUT" || (command -v xdg-open >/dev/null && xdg-open "$OUT") || true
