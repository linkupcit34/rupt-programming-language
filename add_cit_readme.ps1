$c = Get-Content README.md -Raw
# Add cit to Russian Features section
$c = $c -replace '\(-\) функция \(like print\)\.', '\(-\) функция \(как print\)\n- `cit()` — выводит "Made by nikita kozeratsky konstantinovich".'
# Add cit to English Features section
$c = $c -replace '\(-\) output \(like print\)\.', '\(-\) output \(like print\)\n- `cit()` — prints "Made by nikita kozeratsky konstantinovich".'
$c | Set-Content README.md