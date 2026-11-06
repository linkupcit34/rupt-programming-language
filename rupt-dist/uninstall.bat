@echo off
chcp 65001 >nul 2>&1
setlocal
set "DEST=%LOCALAPPDATA%\Rupt"
set "BIN=%DEST%\bin"

echo Удаление Rupt из %DEST% ...
if exist "%BIN%\rupt.exe" del /F /Q "%BIN%\rupt.exe"
if exist "%BIN%\rupt.ico" del /F /Q "%BIN%\rupt.ico"
if exist "%BIN%\rupt-icon.png" del /F /Q "%BIN%\rupt-icon.png"
if exist "%DEST%\examples" rmdir /S /Q "%DEST%\examples"
if exist "%BIN%" rmdir /Q "%BIN%" 2>nul
if exist "%DEST%" rmdir /Q "%DEST%" 2>nul

powershell -NoProfile -ExecutionPolicy Bypass -Command "$p=[Environment]::GetEnvironmentVariable('Path','User'); $b='%BIN%'; $parts=$p -split ';' | Where-Object { $_ -and $_ -ne $b }; [Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User'); ^
  foreach($k in @('HKCU:\Software\Classes\RuptScript','HKCU:\Software\Classes\.rpt','HKCU:\Software\Classes\.rtp')){ if(Test-Path $k){ Remove-Item $k -Recurse -Force } }; ^
  Write-Host 'Удалены PATH и ассоциации файлов.'"

echo Готово.
pause
