#!/bin/bash
mkdir -p build && cd build || exit 1

cmake ../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=. \
	&& make -j$(getconf _NPROCESSORS_ONLN) \
	&& make test \
	&& make install
