@echo off
cd %~dp0
cls
taskkill /f /im "spotify.exe" >nul 2>&1
del "%appdata%\Spotify\d3d9.dll" >nul 2>&1
if errorlevel == 1 goto err
echo K, unistalled :)
goto end
:err
echo Awww, failed to delete :(
:end
pause
