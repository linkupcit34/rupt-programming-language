$c = Get-Content eval.c -Raw
$c = $c -replace [regex]::Escape('strcmp(node->sval,"jwt_decode")==0'), 'strcmp(node->sval,"jwt_decode")==0 || strcmp(node->sval,"cit")==0'
$c | Set-Content eval.c