@echo off
chcp 65001 >nul 2>&1
setlocal
set "DEST=%LOCALAPPDATA%\Rupt"
set "BIN=%DEST%\bin"

echo Установка Rupt в %DEST% ...
if not exist "%BIN%" mkdir "%BIN%"
copy /Y "%~dp0bin\rupt.exe" "%BIN%\rupt.exe" >nul
if errorlevel 1 ( echo Не удалось скопировать rupt.exe & exit /b 1 )
if exist "%~dp0bin\rupt.ico" copy /Y "%~dp0bin\rupt.ico" "%BIN%\rupt.ico" >nul
if exist "%~dp0bin\rupt-icon.png" copy /Y "%~dp0bin\rupt-icon.png" "%BIN%\rupt-icon.png" >nul
if not exist "%DEST%\examples" mkdir "%DEST%\examples"
if exist "%~dp0examples\*.rpt" copy /Y "%~dp0examples\*.rpt" "%DEST%\examples\" >nul

REM Пользовательский PATH
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p=[Environment]::GetEnvironmentVariable('Path','User'); $b='%BIN%'; if($p -notlike ('*'+$b+'*')){ [Environment]::SetEnvironmentVariable('Path', ($p.TrimEnd(';')+';'+$b), 'User'); Write-Host 'Добавлено в PATH.' } else { Write-Host 'Уже есть в PATH.' }"

REM Ассоциации файлов .rpt и .rtp (без дополнительных загрузок)
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$exe='%BIN%\rupt.exe'; $ico='%BIN%\rupt.ico'; ^
   if(Test-Path $ico){ $icon=\"$ico,0\" } else { $icon=\"$exe,0\" }; ^
   New-Item -Path 'HKCU:\Software\Classes\RuptScript' -Force | Out-Null; ^
   Set-ItemProperty -Path 'HKCU:\Software\Classes\RuptScript' -Name '(Default)' -Value 'Скрипт Rupt'; ^
   New-Item -Path 'HKCU:\Software\Classes\RuptScript\DefaultIcon' -Force | Out-Null; ^
   Set-ItemProperty -Path 'HKCU:\Software\Classes\RuptScript\DefaultIcon' -Name '(Default)' -Value $icon; ^
   New-Item -Path 'HKCU:\Software\Classes\RuptScript\shell\open\command' -Force | Out-Null; ^
   Set-ItemProperty -Path 'HKCU:\Software\Classes\RuptScript\shell\open\command' -Name '(Default)' -Value ('\"'+$exe+'\" \"%1\"'); ^
   foreach($ext in '.rpt','.rtp'){ ^
     New-Item -Path ('HKCU:\Software\Classes\'+$ext) -Force | Out-Null; ^
     Set-ItemProperty -Path ('HKCU:\Software\Classes\'+$ext) -Name '(Default)' -Value 'RuptScript'; ^
   }; ^
   Write-Host 'Назначены .rpt/.rtp и значок Rupt.'"

echo.
echo Rupt установлен: %BIN%\rupt.exe
echo Откройте НОВОЕ окно терминала и выполните:
echo   rupt вашфайл.rpt
echo Значки .rpt/.rtp назначены (Проводнику может потребоваться обновление).
echo.
pause
