$ErrorActionPreference = 'Stop'
$dest = Join-Path $env:LOCALAPPDATA 'Rupt'
$bin = Join-Path $dest 'bin'
$srcBin = Join-Path $PSScriptRoot 'bin'
$srcEx = Join-Path $PSScriptRoot 'examples'

Write-Host "Установка Rupt в $dest ..."
New-Item -ItemType Directory -Force -Path $bin | Out-Null
foreach ($f in @('rupt.exe','rupt.ico','rupt-icon.png')) {
  $s = Join-Path $srcBin $f
  if (Test-Path $s) { Copy-Item $s (Join-Path $bin $f) -Force }
}
if (Test-Path $srcEx) {
  $exDest = Join-Path $dest 'examples'
  New-Item -ItemType Directory -Force -Path $exDest | Out-Null
  Copy-Item (Join-Path $srcEx '*.rpt') $exDest -Force -ErrorAction SilentlyContinue
}

$p = [Environment]::GetEnvironmentVariable('Path','User')
if ($p -notlike "*$bin*") {
  $new = ($p.TrimEnd(';') + ';' + $bin)
  [Environment]::SetEnvironmentVariable('Path', $new, 'User')
  $env:Path = $env:Path + ';' + $bin
  Write-Host 'Добавлено в PATH.'
} else { Write-Host 'Уже есть в PATH.' }

$exe = Join-Path $bin 'rupt.exe'
$ico = Join-Path $bin 'rupt.ico'
$icon = if (Test-Path $ico) { "$ico,0" } else { "$exe,0" }
New-Item -Path 'HKCU:\Software\Classes\RuptScript' -Force | Out-Null
Set-ItemProperty -Path 'HKCU:\Software\Classes\RuptScript' -Name '(Default)' -Value 'Скрипт Rupt'
New-Item -Path 'HKCU:\Software\Classes\RuptScript\DefaultIcon' -Force | Out-Null
Set-ItemProperty -Path 'HKCU:\Software\Classes\RuptScript\DefaultIcon' -Name '(Default)' -Value $icon
New-Item -Path 'HKCU:\Software\Classes\RuptScript\shell\open\command' -Force | Out-Null
Set-ItemProperty -Path 'HKCU:\Software\Classes\RuptScript\shell\open\command' -Name '(Default)' -Value "`"$exe`" `"%1`""
foreach ($ext in '.rpt','.rtp') {
  New-Item -Path ("HKCU:\Software\Classes\" + $ext) -Force | Out-Null
  Set-ItemProperty -Path ("HKCU:\Software\Classes\" + $ext) -Name '(Default)' -Value 'RuptScript'
}
Write-Host 'Назначены .rpt/.rtp и значок Rupt.'
Write-Host ""
Write-Host "Rupt установлен. Откройте НОВОЕ окно терминала: rupt вашфайл.rpt"
Write-Host "Проводнику может потребоваться обновление (F5 / перезапуск Проводника), чтобы показать новые значки."
