#!/bin/bash
PROCESSORS=`grep processor /proc/cpuinfo | wc -l`
echo Found $PROCESSORS processors
declare -i JFLAG
JFLAG=$PROCESSORS+1
echo Going to run with $JFLAG jobs
echo If you have problems make sure you have all the required dependencies
echo \(i.e. libqt4-dev, libqt4-opengl-dev\)
echo 
cd src && qmake && make clean && make -j$JFLAG