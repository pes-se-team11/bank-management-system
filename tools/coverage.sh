#!/bin/bash
# Regenerates the code and branch coverage reports in docs/test-reports/.
# Needs a Linux shell with g++, cmake and gcovr (for example the gcc:14
# Docker image, or WSL/Ubuntu with: apt install g++ cmake gcovr).
# Run from the repository root:  bash tools/coverage.sh
set -euo pipefail

repo="$(pwd)"
build="${COVERAGE_BUILD_DIR:-/tmp/bms-coverage}"
out="$repo/docs/test-reports"

cmake -S "$repo" -B "$build" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
      -DCMAKE_CXX_FLAGS="--coverage -O0" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
cmake --build "$build" -j"$(nproc)"

# Reset counters from any earlier run, then run the whole suite once.
find "$build" -name '*.gcda' -delete
(cd "$build/tests" && ./bms_tests)

# Application sources only; compiler-generated exception and unreachable
# branches are left out so the branch figure counts real decisions.
common=("$build" --root "$repo" --filter "$repo/src/"
        --exclude-throw-branches --exclude-unreachable-branches)
mkdir -p "$out/coverage-html"
gcovr "${common[@]}" --txt "$out/code_coverage.txt"
gcovr "${common[@]}" --txt-metric branch --txt "$out/branch_coverage.txt"
gcovr "${common[@]}" --json-summary-pretty --json-summary "$out/coverage_summary.json"
gcovr "${common[@]}" --html-details "$out/coverage-html/index.html" \
      --html-title "BMS Code and Branch Coverage"
echo "Coverage reports written to $out"
