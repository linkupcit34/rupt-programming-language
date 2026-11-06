@echo off
chcp 65001 >nul 2>&1
set NASM=C:\Users\lugov\AppData\Local\bin\NASM\nasm.exe
set GOLINK=C:\Users\lugov\Videos\aline\rupt\tools\golink\GoLink.exe
"%NASM%" -f win64 rupt.asm -o rupt.obj
if errorlevel 1 exit /b 1
"%GOLINK%" /console /entry main rupt.obj kernel32.dll
if errorlevel 1 exit /b 1
echo Собрано: rupt.exe
