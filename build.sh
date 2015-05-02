#!/bin/bash
declare -i JFLAG
if [ -e /proc/cpuinfo ]; then
    # Linux
    PROCESSORS=`grep '^processor\s*:' /proc/cpuinfo | wc -l`
    JFLAG=$PROCESSORS+1
elif [ -e /usr/sbin/system_profiler ]; then
    # Mac OS X
    PROCESSORS=`system_profiler SPHardwareDataType -detailLevel mini | grep "Total Number Of Cores:" | cut -d : -f 2`
    JFLAG=$PROCESSORS+1
else
    JFLAG=2
fi

#[ -d build ] && rm -r build

mkdir -p build && cd build || exit 1

cmake ../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=.
make -j$JFLAG && make install

if [ -e MMapper/mmapper.app/Contents/Info.plist ]; then
    # Mac OS X Retina support
    #perl -pi -e 's#<dict>#<dict>\n\t<key>NSPrincipalClass</key>\n\t<string>NSApplication</string>\n\t<key>NSHighResolutionCapable</key>\n\t<string>True</string>#' MMapper/mmapper.app/Contents/Info.plist
    make package
fi
