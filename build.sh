#!/bin/bash
mkdir -p build && cd build || exit 1
export QT_QPA_PLATFORM=offscreen
cmake ../ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=. \
	&& make -j$(getconf _NPROCESSORS_ONLN) \
	&& make test \
	&& make install \
	&& cpack
