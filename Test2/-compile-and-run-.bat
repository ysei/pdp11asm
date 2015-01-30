@echo off
cls
pdp11asm.exe SCALE144.MAC
if errorlevel 1 goto error
start C:\emu\EMU.exe SCALE144.BIN
exit
:error
pause
