@echo off
timeout /t 2 >nul
del updater.exe >nul 2>&1
rename "updater2.exe" "updater.exe"
start updater.exe
del %0
