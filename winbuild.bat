@ECHO OFF
REM ####
REM #### START
REM ####
IF /i "%QMAKESPEC%" == "" GOTO :noqmakespec
FOR /f %%j in ("qmake.exe") DO SET QMAKE_EXISTS=%%~dp$PATH:j
IF /i "%QMAKE_EXISTS%" == "" GOTO :noqmake
IF /i "%1" == "" GOTO :debug
IF /i "%1" == "Debug" GOTO :debug
IF /i "%1" == "Release" GOTO :release
GOTO :help

:debug
SET BUILD_TYPE=Debug
GOTO :start

:release
SET BUILD_TYPE=Release
GOTO :start

:start
IF NOT EXIST winbuild MKDIR winbuild
CD winbuild
SET DFLAGS=-DZLIB_LIBRARY=%ZLIB_LIBRARY% -DZLIB_INCLUDE_DIR=%ZLIB_INCLUDE_DIR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=. -DMCLIENT_BIN_DIR=. -DMCLIENT_PLUGINS_DIR=plugins

REM ####
REM #### COMPILER SELECTION
REM ####
IF /i "%QMAKESPEC%" == "win32-g++" GOTO :mingw
ECHO %QMAKESPEC% | findstr /i "win32-msvc" > nul
IF %ERRORLEVEL% EQU 0 GOTO :msvc

ECHO -- No compiler found
GOTO :end



REM ####
REM #### BUILD FOR MINGW
REM ####
:mingw
ECHO -- MingW compiler found
cmake ../ %DFLAGS% -G "MinGW Makefiles"
mingw32-make --jobs=%NUMBER_OF_PROCESSORS%  &&  mingw32-make install
GOTO :success


REM ####
REM #### BUILD FOR MS VISUAL C++
REM ####
:msvc
ECHO -- Microsoft Visual C++ compiler found
cmake ../ %DFLAGS% -G "NMake Makefiles"
FOR /f %%j in ("jom.exe") DO SET JOM_EXISTS=%%~dp$PATH:j
IF /i "%JOM_EXISTS%" == "" GOTO :nojom
jom && jom install
GOTO :success
:nojom
nmake /NOLOGO && nmake /NOLOGO install
GOTO :success


REM ####
REM #### NO QMAKESPEC
REM ####
:noqmakespec
ECHO No QMAKESPEC environmental variable found. (e.g. win32-g++, win32-msvc)
goto :end


REM ####
REM #### NO QMAKE
REM ####
:noqmake
ECHO qmake.exe was not found within your PATH environmental variable's directories.
goto :end


REM ####
REM #### HELP
REM ####
:help
ECHO Usage: winbuild [Release/Debug]
goto :end


REM ####
REM #### SUCCESS
REM ####
:success
CD ..
@ECHO ON


:end