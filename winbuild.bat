mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=. -G "MinGW Makefiles"
mingw32-make clean && mingw32-make && mingw32-make install
