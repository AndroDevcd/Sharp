@echo off
rem Removes Sharp from this Windows user: the folder %LOCALAPPDATA%\Sharp and its entry in your PATH.
rem Programs you compiled are yours and are not touched.
rem
rem     double-click this file          (win-uninstall.cmd /quiet  does not wait at the end)
setlocal
set "DEST=%LOCALAPPDATA%\Sharp"
set "QUIET=%~1"

powershell -NoProfile -ExecutionPolicy Bypass -Command "$bin = Join-Path $env:LOCALAPPDATA 'Sharp\bin'; $path = [Environment]::GetEnvironmentVariable('Path', 'User'); if ($path) { $parts = $path.Split(';') | Where-Object { $_ -and ($_.TrimEnd('\') -ne $bin) }; [Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User') }"

if not exist "%DEST%\bin" if not exist "%DEST%\compiler" goto nothing
rem (this file may be the copy inside that folder: the folder is removed by a second command line
rem  that starts after this script has ended)
cd /d "%TEMP%"
start "" /b cmd /c "ping -n 2 127.0.0.1 >nul & rmdir /s /q "%DEST%""
echo Sharp was removed from %DEST%. Open a new terminal: 'sharp' and 'sharpc' are gone from it.
if /i not "%QUIET%"=="/quiet" pause
exit /b 0

:nothing
echo Sharp is not installed in %DEST% (nothing to remove).
if /i not "%QUIET%"=="/quiet" pause
exit /b 0
