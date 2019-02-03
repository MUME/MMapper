@ECHO OFF
REM ####
REM #### START
REM ####
FOR /f %%j in ("qmake.exe") DO SET QMAKE_EXISTS=%%~dp$PATH:j
IF /i "%QMAKE_EXISTS%" == "" GOTO :noqmake
IF /i "%1" == "" SET BUILD_TYPE=RelWithDebInfo && GOTO :start
IF /i "%1" == "Debug" SET BUILD_TYPE=Debug && GOTO :start
IF /i "%1" == "Release" SET BUILD_TYPE=Release && GOTO :start
IF /i "%1" == "RelWithDebInfo" SET BUILD_TYPE=RelWithDebInfo && GOTO :start
GOTO :help

:start
REM ####
REM #### COMPILER SELECTION
REM ####
FOR /f %%j in ("cl.exe") DO SET CL_EXISTS=%%~dp$PATH:j
IF /i "%CL_EXISTS%" NEQ "" GOTO :msvc
FOR /f %%j in ("mingw32-make.exe") DO SET MINGW32_EXISTS=%%~dp$PATH:j
IF /i "%MINGW32_EXISTS%" NEQ "" GOTO :mingw
ECHO -- No compiler found
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
SET CL=/MP
nmake /NOLOGO && nmake /NOLOGO install
GOTO :success


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
ECHO Usage: winbuild [RelWithDebInfo/Release/Debug]
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
