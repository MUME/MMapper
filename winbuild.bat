@echo off
echo.
echo If you have problems finding zlib.h, zconf.h then you need to add it to the
echo MingW installation under include/
echo.
@echo on
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=. -G "MinGW Makefiles"
mingw32-make clean && mingw32-make && mingw32-make package
