MMapper
============================

## Setup
1.  Download the [latest version of MMapper](https://github.com/MUME/MMapper/releases)

2.  Uncompress the archive you downloaded into any directory.
    e.g. `C:\mmapper\` or `~/mmapper`

## Features
1.  Automatic room creation during mapping
2.  Automatic connection of new rooms
3.  Terrain detection (forest, road, mountain, etc)
4.  Exits detections
5.  Fast OpenGL rendering
6.  Pseudo 3D layers and drag and drop mouse operations
7.  Multi platform support
8.  Group manager support to see other people on your map

## Usage
Set up your client according to the wiki instructions at: http://mume.org/wiki/index.php/Guide_to_install_mmapper2_on_Windows

Mapping advice: https://github.com/MUME/MMapper/blob/master/doc/mapping_advice.txt

## Building
You need to have a C++ compiler, CMake, and Qt and zlib development packages installed.

Change the directory to the unpackage source tree, and run:
```
	mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make -j4
	sudo make install
```

Alternatively, you can use one of the wrappers:
```
	winbuild.bat
	./build.sh
```

### Ubuntu 16.04
```
	sudo apt-get install build-essential qt5-qmake qt5-default qttools5-dev-tools libqt5opengl5-dev libqt5svg5-dev qtcreator cmake
```

### Apple
```
	brew install qt5 qt-creator cmake
	brew link qt-creator
	brew link qt5 --force
```
