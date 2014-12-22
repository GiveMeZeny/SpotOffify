@echo off
cd %~dp0
cls
taskkill /f /im "spotify.exe" >nul 2>&1
del "%appdata%\Spotify\d3d9.dll" >nul 2>&1
if errorlevel == 0 goto end
echo failed to delete, try again
pause
:end
