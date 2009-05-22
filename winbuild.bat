echo off
echo If you have problems finding zlib.h then you need to add it to Qt's
echo MingW installation under mingw/include
echo 
echo on
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=. -G "MinGW Makefiles"
mingw32-make clean && mingw32-make && mingw32-make package
