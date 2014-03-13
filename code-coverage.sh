#!/bin/sh
lcov -b . -d . -c -o tmp.info
lcov -e tmp.info \*/codec/\* -o gcov.info
mkdir -p code-coverage
genhtml gcov.info -o ./code-coverage
rm -f tmp.info gcov.info
