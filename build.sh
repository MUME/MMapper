#!/bin/bash
echo If you have problems make sure you have all the required dependencies
echo \(i.e. libqt4-dev, libqt4-opengl-dev, zlib1g-dev\)
echo 

declare -i JFLAG
if [ -e /proc/cpuinfo ]; then
    PROCESSORS=`grep '^processor\s*:' /proc/cpuinfo | wc -l`
    JFLAG=$PROCESSORS+1
else
    JFLAG=2
fi

[ -d build ] && rm -r build

mkdir -p build && cd build || exit 1

cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=. \
-DMCLIENT_BIN_DIR=. -DMCLIENT_PLUGINS_DIR=plugins &&        \
make -j$JFLAG && make install
ln -sf ../config config
