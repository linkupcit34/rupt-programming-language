@echo off
if exist "..\tools\tcc\tcc\tcc.exe" (
  ..\tools\tcc\tcc\tcc.exe -I src -o rupt.exe src\*.c
) else (
  tcc -I src -o rupt.exe src\*.c
)
