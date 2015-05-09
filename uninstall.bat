@echo off
cd %~dp0
cls
set file=XmlLite.dll
taskkill /f /im "spotify.exe" >nul 2>&1
del "%appdata%\Spotify\%file%" >nul 2>&1
if errorlevel == 0 goto end
echo failed to delete, try again
pause
:end
