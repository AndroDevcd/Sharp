@echo off
rem Installs Sharp @VERSION@ for the current Windows user. No administrator rights needed.
rem
rem     double-click this file          (or run it from a terminal;  win-install.cmd /quiet  does not wait at the end)
rem
rem What it does: copies Sharp into %LOCALAPPDATA%\Sharp, adds %LOCALAPPDATA%\Sharp\bin to your
rem PATH (the user's PATH, not the system's), and then compiles and runs a small program to
rem prove that everything works.
setlocal
set "HERE=%~dp0"
for %%I in ("%HERE%..\shared") do set "SHARED=%%~fI"
set "DEST=%LOCALAPPDATA%\Sharp"
set "QUIET=%~1"

if not exist "%HERE%sharp.exe" goto broken
if not exist "%SHARED%\lib" goto broken
if not exist "%SHARED%\compiler" goto broken

echo Installing Sharp @VERSION@ into %DEST% ...
if exist "%DEST%\bin" rmdir /s /q "%DEST%\bin"
if exist "%DEST%\compiler" rmdir /s /q "%DEST%\compiler"
if exist "%DEST%\lib" rmdir /s /q "%DEST%\lib"
if exist "%DEST%\examples" rmdir /s /q "%DEST%\examples"
mkdir "%DEST%\bin" 2>nul
copy /y "%HERE%sharp.exe" "%DEST%\bin\" >nul
copy /y "%HERE%sharpc.cmd" "%DEST%\bin\" >nul
copy /y "%HERE%win-uninstall.cmd" "%DEST%\" >nul
copy /y "%HERE%..\VERSION.txt" "%DEST%\" >nul 2>nul
xcopy /e /i /q /y "%SHARED%\compiler" "%DEST%\compiler" >nul
xcopy /e /i /q /y "%SHARED%\lib" "%DEST%\lib" >nul
xcopy /e /i /q /y "%SHARED%\examples" "%DEST%\examples" >nul
if not exist "%DEST%\bin\sharp.exe" goto nocopy

rem ---- the PATH of this user: add %DEST%\bin once (PowerShell: no length limit, and running programs are told)
powershell -NoProfile -ExecutionPolicy Bypass -Command "$bin = Join-Path $env:LOCALAPPDATA 'Sharp\bin'; $path = [Environment]::GetEnvironmentVariable('Path', 'User'); $parts = @(); if ($path) { $parts = $path.Split(';') | Where-Object { $_ -and ($_.TrimEnd('\') -ne $bin) } }; $parts += $bin; [Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User')"
if errorlevel 1 echo (the PATH could not be changed: add %DEST%\bin to your PATH by hand)

rem ---- the proof: compile and run the example
set "WORK=%TEMP%\sharp-install-test"
if exist "%WORK%" rmdir /s /q "%WORK%"
mkdir "%WORK%"
call "%DEST%\bin\sharpc.cmd" "%DEST%\examples\hello.sharp" -o "%WORK%\hello" > "%WORK%\compile.txt" 2>&1
"%DEST%\bin\sharp.exe" "%WORK%\hello" > "%WORK%\run.txt" 2>&1
findstr /c:"Hello from Sharp" "%WORK%\run.txt" >nul
if errorlevel 1 goto failed

echo.
findstr /c:"Hello from Sharp" "%WORK%\run.txt"
echo.
echo Sharp @VERSION@ is installed.
echo Open a NEW terminal (so it sees the new PATH), then from any folder:
echo     sharpc myprogram.sharp -o myprogram
echo     sharp myprogram
echo An example to start from: %DEST%\examples\hello.sharp
rmdir /s /q "%WORK%"
if /i not "%QUIET%"=="/quiet" pause
exit /b 0

:failed
echo.
echo Sharp was copied to %DEST%, but the test program did not run. What it said:
type "%WORK%\compile.txt"
type "%WORK%\run.txt"
echo (the full output is in %WORK%)
if /i not "%QUIET%"=="/quiet" pause
exit /b 1

:nocopy
echo The files could not be copied to %DEST%.
if /i not "%QUIET%"=="/quiet" pause
exit /b 1

:broken
echo This script must stay inside the unzipped sharp-@VERSION@ folder: it needs the
echo 'shared' folder next to the 'windows' folder. Unzip the WHOLE archive first
echo (right-click the zip, "Extract All"), then run win-install.cmd from the windows folder.
if /i not "%QUIET%"=="/quiet" pause
exit /b 1
