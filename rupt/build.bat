@echo off
if exist "..\tools\tcc\tcc\tcc.exe" (
  ..\tools\tcc\tcc\tcc.exe -I src -o rupt.exe src\eval.c src\env.c src\lexer.c src\main.c src\parser.c src\value.c
) else (
  tcc -I src -o rupt.exe src\eval.c src\env.c src\lexer.c src\main.c src\parser.c src\value.c
)
