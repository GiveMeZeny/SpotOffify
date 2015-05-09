@echo off
cd %~dp0
cls
set file=XmlLite.dll
taskkill /f /im "spotify.exe" >nul 2>&1
copy "src\%file%" "%appdata%\Spotify\" >nul 2>&1
if errorlevel == 1 goto err
start %appdata%\Spotify\spotify.exe
goto end
:err
echo failed to copy, try again
pause
:end
