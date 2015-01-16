@echo off
cls
pdp11asm.exe bk0010_miner.asm
if errorlevel 1 goto error
start C:\emu\EMU.exe bk0010_miner.bin
exit
:error
pause
