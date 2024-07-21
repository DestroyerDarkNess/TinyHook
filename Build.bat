@echo off
TCC\tcc.exe "src.c" -o "HookMessageBox.exe" -luser32 -lkernel32
pause