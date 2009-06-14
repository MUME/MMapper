#!/bin/bash
PROCESSORS=`grep processor /proc/cpuinfo | wc -l`
echo Found $PROCESSORS processors
declare -i JFLAG
JFLAG=$PROCESSORS+1
echo Going to run with $JFLAG jobs
echo If you have problems make sure you have all the required dependencies
echo \(i.e. libqt4-dev, libqt4-opengl-dev, zlib1g-dev\)
echo 

if [ -d build ]; then
    cd build
    make clean
else
    mkdir build
    cd build
fi

cmake ../ -DCMAKE_INSTALL_REFIX=. && make -j$JFLAG && make package
