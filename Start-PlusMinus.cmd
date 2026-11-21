@echo off
setlocal EnableExtensions
title PlusMinus Studio
cd /d "%~dp0"
if errorlevel 1 (
    echo [PlusMinus] Cannot find the project directory.
    pause
    exit /b 1
)

where py >nul 2>nul
if not errorlevel 1 (
    py -3 "%~dp0launch.py" %*
    goto done
)
where python >nul 2>nul
if not errorlevel 1 (
    python "%~dp0launch.py" %*
    goto done
)
where python3 >nul 2>nul
if not errorlevel 1 (
    python3 "%~dp0launch.py" %*
    goto done
)

echo [PlusMinus] Python 3.9+ was not found.
echo [PlusMinus] Install Python with Tkinter, or use a prebuilt Windows PlusMinusStudio.exe release.
pause
exit /b 127

:done
set "status=%errorlevel%"
if not "%status%"=="0" (
    echo.
    echo [PlusMinus] Startup failed with exit code %status%.
    pause
)
exit /b %status%
