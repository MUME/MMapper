#!/bin/bash
DIR=$(dirname $0)
FILE=odr.cpp
cd "$DIR"
echo '// This generated file exists to help detect ODR violations.' > "$FILE"
echo '// It will catch duplicate class names in header files,' >> "$FILE"
echo '// but it will NOT catch duplicate names in source files.' >> "$FILE"
echo '' >> "$FILE"
find * -type f -iname '*.h' | sort | sed -e 's/^/#include "/' -e s'/$/"/' >> "$FILE"
