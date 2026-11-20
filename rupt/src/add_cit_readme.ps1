$c = Get-Content ../../README.md -Raw
$c = $c -replace 'callable, object', 'callable, object, cit'
$c | Set-Content ../../README.md