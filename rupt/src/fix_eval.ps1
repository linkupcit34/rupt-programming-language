$lines = Get-Content 'C:\Users\lugov\Videos\aline\rupt\src\eval.c'
$newLines = @()
$skip = $false
foreach ($line in $lines) {
    if ($line -match '^\s*map_set\(m, "val", val_str\(s\)\);$') {
        $skip = $true
        continue
    }
    if ($skip -and $line -match '^\s*\}$') {
        $skip = $false
        continue
    }
    if ($skip) {
        continue
    }
    $newLines += $line
}
$newLines | Set-Content 'C:\Users\lugov\Videos\aline\rupt\src\eval.c'