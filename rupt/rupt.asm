; rupt.asm — интерпретатор языка Rupt (минимальная версия)
; NASM x64 / Windows, kernel32.dll
; nasm -f win64 rupt.asm -o rupt.obj
; GoLink /console /entry main rupt.obj kernel32.dll

bits 64
default rel

%define NAME_MAX  64
%define VAR_SZ    96
%define MAX_VARS  64
%define HEAP_MAX  65536
%define FILE_MAX  131072

extern GetStdHandle
extern WriteFile
extern ExitProcess
extern CreateFileA
extern ReadFile
extern CloseHandle
extern GetCommandLineA

STD_OUT equ -11

section .bss
hOut:    resq 1
hFile:   resq 1
nRead:   resq 1
nWr:     resq 1
cur:     resq 1
srcEnd:  resq 1
tokP:    resq 1
tokL:    resq 1
vars:    resb VAR_SZ*MAX_VARS
vcount:  resq 1
heap:    resb HEAP_MAX
hused:   resq 1
fbuf:    resb FILE_MAX
nbuf:    resb 48
; результат: тип 1=целое, 2=строка
rType:   resq 1
rI:      resq 1
rS:      resq 1
rLen:    resq 1
lType:   resq 1
lI:      resq 1
lS:      resq 1
lLen:    resq 1
nameTmp: resb NAME_MAX

section .data
crlf:    db 13,10
e_ru:    db "Ошибка: файл должен начинаться со слова 'rupt'",13,10,0
e_cit:   db "Ошибка: имя переменной должно начинаться с 'cit'",13,10,0
e_syn:   db "Ошибка синтаксиса",13,10,0
e_undef: db "Ошибка: неизвестная переменная",13,10,0
e_file:  db "Ошибка: не удалось прочитать файл",13,10,0
e_use:   db "Использование: rupt <файл.rpt>",13,10,0
s_rupt:  db "rupt",0
s_cit:   db "cit",0
s_print: db "print",0

section .text
global main

; wout: rcx=указатель, rdx=длина
wout:
    sub rsp, 40
    mov r8, rdx          ; длина
    mov rdx, rcx         ; буфер
    mov rcx, [hOut]
    lea r9, [nWr]
    mov qword [rsp+32], 0
    call WriteFile
    add rsp, 40
    ret

; wz: rcx=строка, завершающаяся нулём
wz:
    sub rsp, 40
    mov r11, rcx
    xor edx, edx
.l: cmp byte [r11+rdx], 0
    je .d
    inc rdx
    jmp .l
.d: mov rcx, r11
    call wout
    add rsp, 40
    ret

die:
    sub rsp, 40
    call wz
    mov ecx, 1
    call ExitProcess

skipws:
.l: mov rax,[cur]
    cmp rax,[srcEnd]
    jae .r
    movzx eax, byte [rax]
    cmp al,' '
    je .n
    cmp al,9
    je .n
    cmp al,13
    je .n
    cmp al,10
    je .n
    cmp al,'#'
    je .c
.r: ret
.n: inc qword [cur]
    jmp .l
.c: inc qword [cur]
.c2:mov rax,[cur]
    cmp rax,[srcEnd]
    jae .r
    movzx eax, byte [rax]
    inc qword [cur]
    cmp al,10
    jne .c2
    jmp .l

read_ident: ; eax=1 — успешно
    call skipws
    mov rax,[cur]
    mov [tokP],rax
    cmp rax,[srcEnd]
    jae .no
    movzx ecx, byte [rax]
    cmp cl,'_'
    je .ok
    cmp cl,'A'
    jb .c1
    cmp cl,'Z'
    jbe .ok
.c1:cmp cl,'a'
    jb .no
    cmp cl,'z'
    ja .no
.ok:
.l: mov rax,[cur]
    cmp rax,[srcEnd]
    jae .d
    movzx ecx, byte [rax]
    cmp cl,'_'
    je .a
    cmp cl,'0'
    jb .c2
    cmp cl,'9'
    jbe .a
.c2:cmp cl,'A'
    jb .c3
    cmp cl,'Z'
    jbe .a
.c3:cmp cl,'a'
    jb .d
    cmp cl,'z'
    ja .d
.a: inc qword [cur]
    jmp .l
.d: mov rax,[cur]
    sub rax,[tokP]
    mov [tokL],rax
    mov eax,1
    ret
.no:xor eax,eax
    mov qword [tokL],0
    ret

; tok_eq_ci: rcx = строка (нуль-терм.) -> eax (1 при совпадении)
tok_eq_ci:
    sub rsp, 8
    mov r8, rcx          ; шаблон
    mov r9, [tokP]
    mov r10, [tokL]
.l: movzx eax, byte [r8]
    test al,al
    jz .e
    test r10,r10
    jz .no
    movzx edx, byte [r9]
    ; в нижний регистр оба символа
    cmp al,'A'
    jb .a2
    cmp al,'Z'
    ja .a2
    add al,32
.a2:cmp dl,'A'
    jb .d2
    cmp dl,'Z'
    ja .d2
    add dl,32
.d2:cmp al,dl
    jne .no
    inc r8
    inc r9
    dec r10
    jmp .l
.e: test r10,r10
    jnz .no
    mov eax,1
    jmp .x
.no:xor eax,eax
.x: add rsp,8
    ret

; tok_starts_ci_n: rcx=шаблон, edx=n -> eax
tok_starts_ci_n:
    mov r8, rcx
    mov r9, [tokP]
    mov r10d, edx
    cmp [tokL], r10
    jb .no
.l: test r10d,r10d
    jz .y
    movzx eax, byte [r8]
    movzx ecx, byte [r9]
    cmp al,'A'
    jb .a2
    cmp al,'Z'
    ja .a2
    add al,32
.a2:cmp cl,'A'
    jb .c2
    cmp cl,'Z'
    ja .c2
    add cl,32
.c2:cmp al,cl
    jne .no
    inc r8
    inc r9
    dec r10d
    jmp .l
.y: mov eax,1
    ret
.no:xor eax,eax
    ret

find_var: ; -> rax (указатель на запись или 0)
    xor r11d,r11d
.l: cmp r11,[vcount]
    jae .nf
    mov rax,r11
    imul rax,VAR_SZ
    lea r8,[vars+rax]
    mov r9,[tokP]
    mov r10,[tokL]
    lea rcx,[r8]
.c: test r10,r10
    jz .e
    mov al,[r9]
    cmp al,[rcx]
    jne .n
    inc r9
    inc rcx
    dec r10
    jmp .c
.e: cmp byte [rcx],0
    jne .n
    mov rax,r8
    ret
.n: inc r11
    jmp .l
.nf:xor eax,eax
    ret

store_var:
    sub rsp, 40
    call find_var
    test rax,rax
    jnz .wr
    mov rcx,[vcount]
    cmp rcx,MAX_VARS
    jae .d
    imul rcx,VAR_SZ
    lea rax,[vars+rcx]
    ; копирование имени
    mov r8,[tokP]
    mov r9,[tokL]
    lea rdx,[rax]
    cmp r9,NAME_MAX-1
    jbe .cn
    mov r9,NAME_MAX-1
.cn:test r9,r9
    jz .cz
    mov cl,[r8]
    mov [rdx],cl
    inc r8
    inc rdx
    dec r9
    jmp .cn
.cz:mov byte [rdx],0
    inc qword [vcount]
.wr:mov rcx,[rType]
    mov [rax+NAME_MAX],rcx
    mov rcx,[rI]
    mov [rax+NAME_MAX+8],rcx
    mov rcx,[rS]
    mov [rax+NAME_MAX+16],rcx
    mov rcx,[rLen]
    mov [rax+NAME_MAX+24],rcx
.d: add rsp,40
    ret

int_to_heap: ; rax=int -> sets rS/rLen (as temp) returns rdx=ptr,rcx=len
    push rbx
    lea r11,[nbuf+47]
    mov byte [r11],0
    mov r10,10
    xor ebx,ebx
    test rax,rax
    jns .p
    neg rax
    mov bl,1
.p: test rax,rax
    jnz .l
    dec r11
    mov byte [r11],'0'
    jmp .s
.l: xor edx,edx
    div r10
    add dl,'0'
    dec r11
    mov [r11],dl
    test rax,rax
    jnz .l
.s: test bl,bl
    jz .cp
    dec r11
    mov byte [r11],'-'
.cp:lea rcx,[nbuf+47]
    sub rcx,r11
    mov r8,[hused]
    lea rdx,[heap+r8]
    mov r9,rcx
    mov rsi,r11
    mov rdi,rdx
    push rcx
    push rdx
    mov rcx,r9
    rep movsb
    pop rdx
    pop rcx
    add [hused],rcx
    pop rbx
    ret

r_to_str_res:
    cmp qword [rType],2
    je .r
    mov rax,[rI]
    call int_to_heap
    mov [rS],rdx
    mov [rLen],rcx
    mov qword [rType],2
.r: ret

l_to_str_res:
    cmp qword [lType],2
    je .r
    mov rax,[lI]
    call int_to_heap
    mov [lS],rdx
    mov [lLen],rcx
    mov qword [lType],2
.r: ret

parse_expr:
    sub rsp,40
    call parse_term
.l: call skipws
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .d
    movzx eax, byte [rax]
    cmp al,'+'
    je .op
    cmp al,'-'
    je .op
.d: add rsp,40
    ret
.op:mov r12b,al
    ; сохранить левый операнд
    mov rax,[rType]; mov [lType],rax
    mov rax,[rI];   mov [lI],rax
    mov rax,[rS];   mov [lS],rax
    mov rax,[rLen]; mov [lLen],rax
    inc qword [cur]
    call parse_term
    cmp r12b,'+'
    je .plus
    mov rax,[lI]
    sub rax,[rI]
    mov qword [rType],1
    mov [rI],rax
    jmp .l
.plus:
    cmp qword [lType],2
    je .cat
    cmp qword [rType],2
    je .cat
    mov rax,[lI]
    add rax,[rI]
    mov qword [rType],1
    mov [rI],rax
    jmp .l
.cat:
    call l_to_str_res
    call r_to_str_res
    mov r8,[hused]
    lea rdi,[heap+r8]
    mov rbx,rdi
    mov rsi,[lS]
    mov rcx,[lLen]
    rep movsb
    mov rsi,[rS]
    mov rcx,[rLen]
    rep movsb
    mov [rS],rbx
    sub rdi,rbx
    mov [rLen],rdi
    mov qword [rType],2
    add [hused],rdi
    jmp .l

parse_term:
    sub rsp,40
    call parse_factor
.l: call skipws
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .d
    movzx eax, byte [rax]
    cmp al,'*'
    je .m
    cmp al,'/'
    je .dv
.d: add rsp,40
    ret
.m: mov rbx,[rI]
    inc qword [cur]
    call parse_factor
    imul rbx,[rI]
    mov [rI],rbx
    mov qword [rType],1
    jmp .l
.dv:mov rbx,[rI]
    inc qword [cur]
    call parse_factor
    mov rax,rbx
    cqo
    idiv qword [rI]
    mov [rI],rax
    mov qword [rType],1
    jmp .l

parse_factor:
    sub rsp,40
    call skipws
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .err
    movzx ecx, byte [rax]
    cmp cl,'"'
    je .str
    cmp cl,'('
    je .par
    cmp cl,'-'
    je .neg
    cmp cl,'0'
    jb .id
    cmp cl,'9'
    jbe .num
.id:call read_ident
    test eax,eax
    jz .err
    call find_var
    test rax,rax
    jz .undef
    mov r8,[rax+NAME_MAX]
    mov [rType],r8
    mov r8,[rax+NAME_MAX+8]
    mov [rI],r8
    mov r8,[rax+NAME_MAX+16]
    mov [rS],r8
    mov r8,[rax+NAME_MAX+24]
    mov [rLen],r8
    add rsp,40
    ret
.neg:
    inc qword [cur]
    call parse_factor
    neg qword [rI]
    add rsp,40
    ret
.num:
    xor ebx,ebx
.nl:mov rax,[cur]
    cmp rax,[srcEnd]
    jae .nd
    movzx eax, byte [rax]
    cmp al,'0'
    jb .nd
    cmp al,'9'
    ja .nd
    sub al,'0'
    imul rbx,10
    movzx eax,al
    add rbx,rax
    inc qword [cur]
    jmp .nl
.nd:mov qword [rType],1
    mov [rI],rbx
    add rsp,40
    ret
.str:
    inc qword [cur]
    mov r8,[hused]
    lea rdi,[heap+r8]
    mov rbx,rdi
.sl:mov rax,[cur]
    cmp rax,[srcEnd]
    jae .sd
    movzx ecx, byte [rax]
    inc qword [cur]
    cmp cl,'"'
    je .sd
    cmp cl,92
    jne .sc
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .sd
    movzx ecx, byte [rax]
    inc qword [cur]
    cmp cl,'n'
    jne .e2
    mov cl,10
    jmp .sc
.e2:cmp cl,'t'
    jne .sc
    mov cl,9
.sc:mov [rdi],cl
    inc rdi
    jmp .sl
.sd:mov [rS],rbx
    sub rdi,rbx
    mov [rLen],rdi
    mov qword [rType],2
    add [hused],rdi
    add rsp,40
    ret
.par:
    inc qword [cur]
    call parse_expr
    call skipws
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .err
    cmp byte [rax],')'
    jne .err
    inc qword [cur]
    add rsp,40
    ret
.err:
    lea rcx,[e_syn]
    call die
.undef:
    lea rcx,[e_undef]
    call die

do_print:
    sub rsp,40
    cmp qword [rType],2
    je .s
    mov rax,[rI]
    call int_to_heap
    mov rcx,rdx
    mov rdx,rcx
    ; int_to_heap возвращает rdx=указатель, rcx=длина
    ; исправление: обмен значений
    add rsp,40
    sub rsp,40
    mov rax,[rI]
    call int_to_heap
    ; rdx=указатель, rcx=длина
    mov r8,rcx
    mov rcx,rdx
    mov rdx,r8
    call wout
    lea rcx,[crlf]
    mov edx,2
    call wout
    add rsp,40
    ret
.s: mov rcx,[rS]
    mov rdx,[rLen]
    call wout
    lea rcx,[crlf]
    mov edx,2
    call wout
    add rsp,40
    ret

get_arg1:
    sub rsp,40
    call GetCommandLineA
    add rsp,40
    mov rcx,rax
    mov al,[rcx]
    cmp al,'"'
    je .q
.a: mov al,[rcx]
    test al,al
    jz .nf
    cmp al,' '
    je .ws
    cmp al,9
    je .ws
    inc rcx
    jmp .a
.q: inc rcx
    mov rax,rcx       ; начало
.q2:mov dl,[rcx]
    test dl,dl
    jz .nf
    cmp dl,'"'
    je .qe
    inc rcx
    jmp .q2
.qe:mov byte [rcx],0
    ret
.ws:mov al,[rcx]
    cmp al,' '
    je .w2
    cmp al,9
    je .w2
    test al,al
    jz .nf
    cmp al,'"'
    je .q
    ; без кавычек: завершить на пробеле
    mov rax,rcx
.u: mov dl,[rcx]
    test dl,dl
    jz .rt
    cmp dl,' '
    je .ut
    cmp dl,9
    je .ut
    inc rcx
    jmp .u
.ut:mov byte [rcx],0
.rt:ret
.w2:inc rcx
    jmp .ws
.nf:xor eax,eax
    ret

main:
    sub rsp, 56
    mov ecx, STD_OUT
    call GetStdHandle
    mov [hOut], rax

    call get_arg1
    test rax,rax
    jz .usage
    mov r12,rax
    mov rcx,r12
    mov edx,0x80000000
    mov r8d,1
    xor r9d,r9d
    mov qword [rsp+32],3
    mov qword [rsp+40],0
    mov qword [rsp+48],0
    call CreateFileA
    cmp rax,-1
    je .ferr
    mov [hFile],rax

    mov rcx,[hFile]
    lea rdx,[fbuf]
    mov r8d,FILE_MAX-1
    lea r9,[nRead]
    mov qword [rsp+32],0
    call ReadFile

    mov rcx,[hFile]
    call CloseHandle

    lea rax,[fbuf]
    mov [cur],rax
    add rax,[nRead]
    mov [srcEnd],rax

    call read_ident
    test eax,eax
    jz .eru
    lea rcx,[s_rupt]
    call tok_eq_ci
    test eax,eax
    jz .eru

.loop:
    call skipws
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .ok
    call read_ident
    test eax,eax
    jz .ok
    lea rcx,[s_cit]
    call tok_eq_ci
    test eax,eax
    jnz .dcit
    lea rcx,[s_print]
    call tok_eq_ci
    test eax,eax
    jnz .dprint
    lea rcx,[e_syn]
    call die

.dcit:
    call read_ident
    test eax,eax
    jz .syn
    lea rcx,[s_cit]
    mov edx,3
    call tok_starts_ci_n
    test eax,eax
    jz .ecit
    ; копирование имени
    mov rsi,[tokP]
    mov rcx,[tokL]
    lea rdi,[nameTmp]
    mov r13,rcx
    cmp rcx,NAME_MAX-1
    jbe .c1
    mov rcx,NAME_MAX-1
    mov r13,rcx
.c1:test rcx,rcx
    jz .c2
    mov al,[rsi]
    mov [rdi],al
    inc rsi
    inc rdi
    dec rcx
    jmp .c1
.c2:mov byte [rdi],0
    call skipws
    mov rax,[cur]
    cmp rax,[srcEnd]
    jae .syn
    cmp byte [rax],'='
    jne .syn
    inc qword [cur]
    call parse_expr
    lea rax,[nameTmp]
    mov [tokP],rax
    mov [tokL],r13
    call store_var
    jmp .loop

.dprint:
    call parse_expr
    call do_print
    jmp .loop

.ok:
    xor ecx,ecx
    call ExitProcess

.usage:
    lea rcx,[e_use]
    call die
.ferr:
    lea rcx,[e_file]
    call die
.eru:
    lea rcx,[e_ru]
    call die
.ecit:
    lea rcx,[e_cit]
    call die
.syn:
    lea rcx,[e_syn]
    call die
