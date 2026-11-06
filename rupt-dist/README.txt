# Rupt — установка (без зависимостей)

Этот пакет содержит только готовый `rupt.exe` (Windows x64) и установщики.
**NASM, GoLink, Python и другие инструменты скачивать не нужно.**

## Установка
1. Распакуйте папку `rupt-dist` (или этот zip) в любое место.
2. Запустите **`install.bat`** (двойной клик) или выполните:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\install.ps1
   ```
Установщик скопирует `rupt.exe` в `%LOCALAPPDATA%\Rupt\bin` и добавит этот путь в пользовательский `PATH`.

3. Откройте **новое** окно терминала и проверьте:
   ```bat
   rupt examples\..\ваш_файл.rpt
   ```

## Запуск скриптов
```bat
rupt название.rpt
rupt C:\путь\к\файлу.rpt
```

## Пример (`.rpt`)
```rpt
rupt
cit citCough = "Kha-kha! *cough* Khe-khe..."
print citCough
print 44
```
Правила: файл начинается с `rupt`; переменные объявляются как `cit citName = ...`.

## Удаление
Запустите `uninstall.bat` (удаляет файлы из `%LOCALAPPDATA%\Rupt` и убирает путь из `PATH`).

## Значки файлов
Установщик назначает значок `rupt.ico` для `.rpt` и `.rtp`. Если значки не обновились, нажмите F5 в Проводнике или перезапустите его.
