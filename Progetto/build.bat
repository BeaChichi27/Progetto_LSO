@echo off
set MINGW_PATH=C:\Users\PC\Downloads\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin
set PATH=%MINGW_PATH%;%PATH%

echo Compilando il progetto...

cd client
"%MINGW_PATH%\mingw32-make" clean
"%MINGW_PATH%\mingw32-make"
if %ERRORLEVEL% neq 0 (
    echo Errore nella compilazione del client!
    exit /b 1
)
cd ..

cd server
"%MINGW_PATH%\mingw32-make" clean
"%MINGW_PATH%\mingw32-make"
if %ERRORLEVEL% neq 0 (
    echo Errore nella compilazione del server!
    exit /b 1
)
cd ..

echo Compilazione completata con successo!