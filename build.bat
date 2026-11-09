@echo off
rem Atalho para Windows: so chama o make (w64devkit no PATH). Ex.: build.bat run
cd /d "%~dp0"
make %*
