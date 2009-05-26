::
:: PROJECT:     RosBE - ReactOS Build Environment for Windows
:: LICENSE:     GNU General Public License v2. (see LICENSE.txt)
:: FILE:        Root/update.cmd
:: PURPOSE:     RosBE Updater.
:: COPYRIGHT:   Copyright 2009 Daniel Reimer <reimer.daniel@freenet.de>
::

@echo off

if not defined _ROSBE_DEBUG set _ROSBE_DEBUG=0
if %_ROSBE_DEBUG% == 1 (
    @echo on
)

setlocal enabledelayedexpansion
title Updating...

:: The Update Server.
set _ROSBE_URL=http://dreimer.dr.funpic.org/rosbe

:: Save the recent dir to cd back there at the end.
set _ROSBE_OPATH=%CD%

if not exist "%_ROSBE_BASEDIR%\Tools\7z.exe" (
    cd /d "%_ROSBE_BASEDIR%\Tools"
    wget.exe -N --ignore-length --no-verbose %_ROSBE_URL%/7z.exe 1> NUL 2> NUL
    cd /d %_ROSBE_OPATH%
)

cd /d %_ROSBE_BASEDIR%

:: First check for a new Updater
for %%F in (update.cmd) do set _ROSBE_UPDDATE=%%~tF
rem "Tools\wget.exe" -N --ignore-length --no-verbose %_ROSBE_URL%/update.cmd 1> NUL 2> NUL
for %%F in (update.cmd) do set _ROSBE_UPDDATE2=%%~tF
if !_ROSBE_UPDDATE! NEQ !_ROSBE_UPDDATE2! (
    cls
    echo Updater got updated and needs to be restarted.
    goto :EOC
)

:: Get to the Updates Subfolder.
if not exist "%APPDATA%\RosBE\Updates" mkdir "%APPDATA%\RosBE\Updates" 1> NUL 2> NUL
cd /d "%APPDATA%\RosBE\Updates"

:: Parse the args.
if "%1" == "" (
    set _ROSBE_MULTIUPD=1
    set _ROSBE_STATCOUNT=1
    call :WHILE
) else if /i "%1" == "reset" (
    del /F /Q "%APPDATA%\RosBE\Updates\*.*" 1> NUL 2> NUL
    del /F /Q "%APPDATA%\RosBE\Updates\tmp\*.*" 1> NUL 2> NUL
) else if /i "%1" == "nr" (
    set _ROSBE_STATCOUNT=%2
    call :UPDCHECK
) else if /i "%1" == "delete" (
    set _ROSBE_STATCOUNT=%2
    del /F /Q "%APPDATA%\RosBE\Updates\%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.*" 1> NUL 2> NUL
    del /F /Q "%APPDATA%\RosBE\Updates\tmp\%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.*" 1> NUL 2> NUL
) else if /i "%1" == "info" (
    set _ROSBE_STATCOUNT=%2
    cd tmp
    if not exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
        "%_ROSBE_BASEDIR%\Tools\wget.exe" -N --ignore-length --no-verbose %_ROSBE_URL%/%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt 1> NUL 2> NUL
        if exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
            type "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt"
        ) else (
            echo ERROR: This Update does not seem to exist or the Internet connection is not working correctly.
            goto :EOC
        )
    )
    cd..
del /F /Q tmp\*.* 1> NUL 2> NUL
) else if /i "%1" == "status" (
    set _ROSBE_STATCOUNT=1
    if not exist "tmp" mkdir tmp 1> NUL 2> NUL
    copy *.txt .\tmp\. 1> NUL 2> NUL
    call :WHILE2
    del /F /Q tmp\*.* 1> NUL 2> NUL
    if not "%_ROSBE_UPDATES%" == "" (
        echo Following Updates available: %_ROSBE_UPDATES%
    ) else (
        echo RosBE is up to Date.
    )
) else (
    echo Unknown parameter specified. Try 'help update'.
)
goto :EOC

:UPDCHECK
cd /d "%APPDATA%\RosBE\Updates"

if exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
    goto :EOF
)

if not exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
    "%_ROSBE_BASEDIR%\Tools\wget.exe" -N --ignore-length --no-verbose %_ROSBE_URL%/%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt 1> NUL 2> NUL
)

if exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
    type "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt"
    echo.
    echo Install?
    set /p YESNO="(yes), (no)"
    if /i "!YESNO!"=="yes" goto :updyes
    if /i "!YESNO!"=="y" goto :updyes
    goto :no
    :updyes
        if not exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.7z" (
            "%_ROSBE_BASEDIR%\Tools\wget.exe" -N --ignore-length --no-verbose %_ROSBE_URL%/%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.7z 1> NUL 2> NUL
        )
        if exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.7z" (
            del /F /Q "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%\*.*" 1>NUL 2>NUL
            "%_ROSBE_BASEDIR%\Tools\7z.exe" x "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.7z"
            cd "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%"
            call "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.cmd"
            goto :EOF
        ) else (
            echo ERROR: This Update does not seem to exist or the Internet connection is not working correctly.
            goto :EOF
        )
        goto :EOF
    :no
        echo Do you want to be asked again to install this update?
        set /p YESNO="(yes), (no)"
        if /i "!YESNO!"=="yes" goto :yesagain
        if /i "!YESNO!"=="y" goto :yesagain
        goto :EOF
        :yesagain
        del "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" 1> NUL 2> NUL
        goto :EOF
    )
) else (
    if not "%_ROSBE_MULTIUPD%" == "1" (
        echo ERROR: This Update does not seem to exist or the Internet connection is not working correctly.
        goto :EOF
    )
)
goto :EOF

:WHILE
    if "!_ROSBE_STATCOUNT!" == "10" GOTO :OUT
    call :UPDCHECK
    set /a _ROSBE_STATCOUNT+=1
    GOTO :WHILE

:WHILE2
    if "!_ROSBE_STATCOUNT!" == "10" GOTO :OUT
    cd tmp
    if not exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
        "%_ROSBE_BASEDIR%\Tools\wget.exe" -N --ignore-length --no-verbose %_ROSBE_URL%/%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt 1> NUL 2> NUL
        if exist "%_ROSBE_VERSION%-%_ROSBE_STATCOUNT%.txt" (
            set _ROSBE_UPDATES=%_ROSBE_UPDATES% %_ROSBE_STATCOUNT% 
        )
    )
    cd..
    set /a _ROSBE_STATCOUNT+=1
    GOTO :WHILE2
:OUT

:EOC
cd /d "%_ROSBE_OPATH%"
title ReactOS Build Environment %_ROSBE_VERSION%
endlocal
