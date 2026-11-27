@echo off
rem sharpc: the Sharp compiler.
rem
rem     sharpc main.sharp -o app          compile          (then:  sharp app)
rem     sharpc --unoptimized ...          use the compiler that was built without the optimizer
rem                                       (the same compiler; for when the optimized one misbehaves)
rem     sharpc --debug ...                the debug compiler: also prints how the compile went, stage by
rem                                       stage, and has -Oreport, -Otrace, -ir, -listing, -timings ...
rem
rem The compiler is itself a Sharp program: this starts the VM on it, and adds the standard
rem library that was installed beside it (-stdlib; one of your own on the line comes first and wins).
setlocal
set "BIN=%~dp0"
for %%I in ("%BIN%..") do set "ROOT=%%~fI"
set "COMPILER=%ROOT%\compiler\sharpc-v@VERSION@"
set ARGS=
:next
if "%~1"=="" goto run
if "%~1"=="--unoptimized" (
    set "COMPILER=%ROOT%\compiler\sharpc-unoptimized-v@VERSION@"
) else if "%~1"=="--debug" (
    set "COMPILER=%ROOT%\compiler\sharpc-debug-v@VERSION@"
) else (
    set ARGS=%ARGS% %1
)
shift
goto next
:run
"%BIN%sharp.exe" "%COMPILER%" %ARGS% -stdlib "%ROOT%\lib"
exit /b %ERRORLEVEL%
