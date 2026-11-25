@echo off
setlocal

echo [ETDAmbilight] Build starting...

set ROOT=%~dp0
set SRC_DIR=%ROOT%src
set INST_DIR=%ROOT%installer
set OUT_DIR=%ROOT%dist
set OBJ_DIR=%ROOT%build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

for /f "delims=" %%i in ('where g++ 2^>nul') do set GPP=%%i
if "%GPP%"=="" (
    echo ERROR: g++ not found in PATH
    exit /b 1
)
echo [ETDAmbilight] Compiler: %GPP%

for %%i in ("%GPP%") do set BINDIR=%%~dpi
set WINDRES=%BINDIR%windres.exe
if not exist "%WINDRES%" (
    echo ERROR: windres.exe not found at %WINDRES%
    exit /b 1
)

echo [ETDAmbilight] Compiling app resources...
"%WINDRES%" "%SRC_DIR%\resources.rc" -O coff -o "%OBJ_DIR%\resources.o" --include-dir="%SRC_DIR%"
if %ERRORLEVEL% neq 0 (
    echo ERROR: Resource compilation failed
    exit /b 1
)

echo [ETDAmbilight] Compiling ETDAmbilight.exe (DXGI + GDI, 1Mbaud, Adalight, TR/EN)...
"%GPP%" ^
    -std=c++20 ^
    -Wall ^
    -O3 -mtune=generic -DNDEBUG ^
    -mwindows -municode ^
    -static -static-libgcc -static-libstdc++ ^
    "%SRC_DIR%\main.cpp" ^
    "%OBJ_DIR%\resources.o" ^
    -o "%OUT_DIR%\ETDAmbilight.exe" ^
    -lcomctl32 -lsetupapi -lwinmm ^
    -lgdi32 -luser32 -ladvapi32 -lcomdlg32 ^
    -lole32 -luuid -lshell32 -lshlwapi ^
    -ld3d11 -ldxgi -ldwmapi -luxtheme -lwinhttp ^
    -Wl,-s

if %ERRORLEVEL% neq 0 (
    echo ERROR: App compilation failed
    exit /b 1
)

echo [ETDAmbilight] Compiling Setup Wizard resources...
"%WINDRES%" "%INST_DIR%\setup.rc" -O coff -o "%OBJ_DIR%\setup_res.o" --include-dir="%INST_DIR%"
if %ERRORLEVEL% neq 0 (
    echo ERROR: Setup resource compilation failed
    exit /b 1
)

echo [ETDAmbilight] Compiling ETDAmbilight-Setup.exe (Bilingual Installer and Updater)...
"%GPP%" ^
    -std=c++20 ^
    -Wall ^
    -O3 -mtune=generic -DNDEBUG ^
    -mwindows -municode ^
    -static -static-libgcc -static-libstdc++ ^
    "%INST_DIR%\setup.cpp" ^
    "%OBJ_DIR%\setup_res.o" ^
    -o "%OUT_DIR%\ETDAmbilight-Setup.exe" ^
    -lcomctl32 -lsetupapi ^
    -lgdi32 -luser32 -ladvapi32 ^
    -lole32 -luuid -lshell32 -lshlwapi ^
    -ldwmapi -luxtheme -lwinhttp ^
    -Wl,-s

if %ERRORLEVEL% neq 0 (
    echo ERROR: Setup Wizard compilation failed
    exit /b 1
)

echo.
echo ========================================================
echo [ETDAmbilight] Build successful!
for %%F in ("%OUT_DIR%\ETDAmbilight.exe") do echo   App EXE:   %%~nxF (%%~zF bytes)
for %%F in ("%OUT_DIR%\ETDAmbilight-Setup.exe") do echo   Setup EXE: %%~nxF (%%~zF bytes)
echo ========================================================
