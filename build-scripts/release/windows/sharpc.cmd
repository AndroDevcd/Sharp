@echo off
rem sharpc: the Sharp compiler.
rem
rem     sharpc main.sharp -o app          compile          (then:  sharp app)
rem     sharpc --unoptimized ...          use the compiler that was built without the optimizer
rem                                       (the same compiler; for when the optimized one misbehaves)
rem
rem The compiler is itself a Sharp program: this starts the VM on it, and adds the standard
rem library that was installed beside it, so no -L is ever needed for it.
setlocal
set "BIN=%~dp0"
for %%I in ("%BIN%..") do set "ROOT=%%~fI"
set "COMPILER=%ROOT%\compiler\sharpc-v@VERSION@"
set ARGS=
:next
if "%~1"=="" goto run
if "%~1"=="--unoptimized" (
    set "COMPILER=%ROOT%\compiler\sharpc-unoptimized-v@VERSION@"
) else (
    set ARGS=%ARGS% %1
)
shift
goto next
:run
"%BIN%sharp.exe" "%COMPILER%" %ARGS% -L "%ROOT%\lib"
exit /b %ERRORLEVEL%
