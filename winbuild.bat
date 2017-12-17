@ECHO OFF
REM ####
REM #### START
REM ####
FOR /f %%j in ("qmake.exe") DO SET QMAKE_EXISTS=%%~dp$PATH:j
IF /i "%QMAKE_EXISTS%" == "" GOTO :noqmake
IF /i "%1" == "" GOTO :release
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
REM ####
REM #### COMPILER SELECTION
REM ####
FOR /f %%j in ("mingw32-make") DO SET QMAKESPEC=win32-g++
FOR /f %%j in ("mingw64-make") DO SET QMAKESPEC=win64-g++
IF /i "%QMAKESPEC%" == "win32-g++" GOTO :mingw
IF /i "%QMAKESPEC%" == "win64-g++" GOTO :mingw
IF /i "%QMAKESPEC%" == "win32-msvc" GOTO :msvc
IF /i "%QMAKESPEC%" == "win64-msvc" GOTO :msvc

IF /i "%QMAKESPEC%" == "" GOTO :noqmakespec
GOTO :end

REM ####
REM #### BUILD FOR MINGW
REM ####
:mingw
ECHO -- MingW compiler found
IF NOT EXIST winbuild MKDIR winbuild
CD winbuild
cmake ../ -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=. -G "MinGW Makefiles"
mingw32-make --jobs=%NUMBER_OF_PROCESSORS%  &&  mingw32-make install
GOTO :success


REM ####
REM #### BUILD FOR MS VISUAL C++
REM ####
:msvc
ECHO -- Microsoft Visual C++ compiler found
IF NOT EXIST winbuild MKDIR winbuild
CD winbuild
cmake ../ -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX=. -G "NMake Makefiles"
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
cpack
ECHO Done!
CD ..
@ECHO ON
GOTO :end

:end
