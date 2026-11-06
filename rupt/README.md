# Rupt (ASM) — NASM x64 / Windows

Минимальный интерпретатор языка **Rupt**, написанный на ассемблере (NASM, x64, Windows API).

## Язык
- Файлы `.rpt` (и `.rtp`) **должны начинаться** со слова `rupt`.
- Переменные объявляются через `cit`; **имя переменной должно начинаться с `cit`**.
  - `cit citName = "Алина"`
- Инструкции: `cit <имя> = <выражение>` и `print <выражение>`.
- Выражения: целые числа, строки в `"..."`, ссылки на переменные, `+ - * /`, скобки `()`.
- `+` выполняет сложение чисел или конкатенацию, если есть строка.

## Сборка (NASM + GoLink)
Требуются `nasm` и `GoLink` в `PATH` (пути заданы в `build.bat`).

```bat
build.bat
```
или вручную:
```bat
nasm -f win64 rupt.asm -o rupt.obj
GoLink /console /entry main rupt.obj kernel32.dll
```

## Запуск
```bat
rupt.exe examples\hello.rpt
```
После установки (см. `rupt-dist/` / установщик): `rupt название.rpt`

## Пример (`examples/hello.rpt`)
```rpt
rupt
cit citName = "Alina"
cit citAge = 21
cit citSum = 10 + 5 * 2
print citName
print citAge
print citSum
print "Hello, " + citName
```

> Примечание: это минимальная реализация подмножества языка (целые числа, базовые выражения/строки, вывод). Для UTF-8/кириллицы в консоли Windows может потребоваться `chcp 65001`.
