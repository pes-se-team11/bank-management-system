#!/usr/bin/env bash
# Part 3: line and branch coverage for the modules outside Account/Auth/Validation
# (Transaction, Persistence, Platform, Core, CLI). Needs g++, cmake, gcovr.
#   pip install gcovr
#   bash tools/coverage_other_modules.sh
# Writes coverage-report/index.html and prints the per-file tables.
# Branch numbers exclude compiler-generated exception branches
# (--exclude-throw-branches) and unreachable ones (--exclude-unreachable-branches).
set -euo pipefail
cd "$(dirname "$0")/.."
rm -rf build-cov
cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage -O0" -DBUILD_TESTING=ON
cmake --build build-cov --parallel 4
(cd build-cov/tests && ./bms_tests)
FILTERS=(--filter 'src/' --exclude 'src/platform/platform_win.cpp'
         --exclude-throw-branches --exclude-unreachable-branches)
mkdir -p coverage-report
echo "== LINE coverage =="
gcovr -r . build-cov "${FILTERS[@]}" --txt-metric line --txt
echo "== BRANCH coverage =="
gcovr -r . build-cov "${FILTERS[@]}" --txt-metric branch --txt
gcovr -r . build-cov "${FILTERS[@]}" --html-details coverage-report/index.html
echo "HTML report: coverage-report/index.html"
