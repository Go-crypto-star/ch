section .text
    global asm_u256_add
    global asm_u256_sub
    global asm_u256_mul
    global asm_u256_div
    global asm_u256_mod
    global asm_u256_sqrt
    global asm_u256_cmp
    global asm_u256_shl
    global asm_u256_shr

; uint256_t asm_u256_add(const uint256_t* a, const uint256_t* b, uint256_t* result)
; Returns carry flag (0 - no overflow, 1 - overflow)
asm_u256_add:
    push rbp
    mov rbp, rsp
    push rbx
    
    mov rbx, rdx        ; rbx = result pointer
    
    ; Сложение младших 128 бит
    mov rax, [rdi]      ; a[0]
    add rax, [rsi]      ; + b[0]
    mov [rbx], rax
    
    mov rax, [rdi + 8]  ; a[1]
    adc rax, [rsi + 8]  ; + b[1] with carry
    mov [rbx + 8], rax
    
    ; Сложение старших 128 бит
    mov rax, [rdi + 16] ; a[2]
    adc rax, [rsi + 16] ; + b[2] with carry
    mov [rbx + 16], rax
    
    mov rax, [rdi + 24] ; a[3]
    adc rax, [rsi + 24] ; + b[3] with carry
    mov [rbx + 24], rax
    
    ; Возвращаем флаг переноса
    setc al
    movzx eax, al
    
    pop rbx
    pop rbp
    ret

; uint256_t asm_u256_sub(const uint256_t* a, const uint256_t* b, uint256_t* result)
; Returns borrow flag (0 - no underflow, 1 - underflow)
asm_u256_sub:
    push rbp
    mov rbp, rsp
    push rbx
    
    mov rbx, rdx        ; rbx = result pointer
    
    ; Вычитание младших 128 бит
    mov rax, [rdi]      ; a[0]
    sub rax, [rsi]      ; - b[0]
    mov [rbx], rax
    
    mov rax, [rdi + 8]  ; a[1]
    sbb rax, [rsi + 8]  ; - b[1] with borrow
    mov [rbx + 8], rax
    
    ; Вычитание старших 128 бит
    mov rax, [rdi + 16] ; a[2]
    sbb rax, [rsi + 16] ; - b[2] with borrow
    mov [rbx + 16], rax
    
    mov rax, [rdi + 24] ; a[3]
    sbb rax, [rsi + 24] ; - b[3] with borrow
    mov [rbx + 24], rax
    
    ; Возвращаем флаг заёма
    setc al
    movzx eax, al
    
    pop rbx
    pop rbp
    ret

; void asm_u256_mul(const uint256_t* a, const uint256_t* b, uint256_t* result)
; 256-bit multiplication using schoolbook algorithm
asm_u256_mul:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    
    ; Обнуляем результат (512 бит для промежуточного хранения)
    sub rsp, 64
    mov qword [rsp], 0
    mov qword [rsp + 8], 0
    mov qword [rsp + 16], 0
    mov qword [rsp + 24], 0
    mov qword [rsp + 32], 0
    mov qword [rsp + 40], 0
    mov qword [rsp + 48], 0
    mov qword [rsp + 56], 0
    
    ; Умножение a[0] * b
    mov rax, [rdi]        ; a[0]
    mul qword [rsi]       ; * b[0]
    mov [rsp], rax        ; result[0]
    mov [rsp + 8], rdx    ; carry
    
    mov rax, [rdi]        ; a[0]
    mul qword [rsi + 8]   ; * b[1]
    add [rsp + 8], rax
    adc rdx, 0
    mov [rsp + 16], rdx
    
    mov rax, [rdi]        ; a[0]
    mul qword [rsi + 16]  ; * b[2]
    add [rsp + 16], rax
    adc rdx, 0
    mov [rsp + 24], rdx
    
    mov rax, [rdi]        ; a[0]
    mul qword [rsi + 24]  ; * b[3]
    add [rsp + 24], rax
    adc rdx, 0
    mov [rsp + 32], rdx
    
    ; Умножение a[1] * b
    mov rax, [rdi + 8]    ; a[1]
    mul qword [rsi]       ; * b[0]
    add [rsp + 8], rax
    adc [rsp + 16], rdx
    adc qword [rsp + 24], 0
    adc qword [rsp + 32], 0
    adc qword [rsp + 40], 0
    
    mov rax, [rdi + 8]    ; a[1]
    mul qword [rsi + 8]   ; * b[1]
    add [rsp + 16], rax
    adc [rsp + 24], rdx
    adc qword [rsp + 32], 0
    adc qword [rsp + 40], 0
    
    mov rax, [rdi + 8]    ; a[1]
    mul qword [rsi + 16]  ; * b[2]
    add [rsp + 24], rax
    adc [rsp + 32], rdx
    adc qword [rsp + 40], 0
    
    mov rax, [rdi + 8]    ; a[1]
    mul qword [rsi + 24]  ; * b[3]
    add [rsp + 32], rax
    adc [rsp + 40], rdx
    
    ; Умножение a[2] * b
    mov rax, [rdi + 16]   ; a[2]
    mul qword [rsi]       ; * b[0]
    add [rsp + 16], rax
    adc [rsp + 24], rdx
    adc qword [rsp + 32], 0
    adc qword [rsp + 40], 0
    adc qword [rsp + 48], 0
    
    mov rax, [rdi + 16]   ; a[2]
    mul qword [rsi + 8]   ; * b[1]
    add [rsp + 24], rax
    adc [rsp + 32], rdx
    adc qword [rsp + 40], 0
    adc qword [rsp + 48], 0
    
    mov rax, [rdi + 16]   ; a[2]
    mul qword [rsi + 16]  ; * b[2]
    add [rsp + 32], rax
    adc [rsp + 40], rdx
    adc qword [rsp + 48], 0
    
    mov rax, [rdi + 16]   ; a[2]
    mul qword [rsi + 24]  ; * b[3]
    add [rsp + 40], rax
    adc [rsp + 48], rdx
    
    ; Умножение a[3] * b
    mov rax, [rdi + 24]   ; a[3]
    mul qword [rsi]       ; * b[0]
    add [rsp + 24], rax
    adc [rsp + 32], rdx
    adc qword [rsp + 40], 0
    adc qword [rsp + 48], 0
    adc qword [rsp + 56], 0
    
    mov rax, [rdi + 24]   ; a[3]
    mul qword [rsi + 8]   ; * b[1]
    add [rsp + 32], rax
    adc [rsp + 40], rdx
    adc qword [rsp + 48], 0
    adc qword [rsp + 56], 0
    
    mov rax, [rdi + 24]   ; a[3]
    mul qword [rsi + 16]  ; * b[2]
    add [rsp + 40], rax
    adc [rsp + 48], rdx
    adc qword [rsp + 56], 0
    
    mov rax, [rdi + 24]   ; a[3]
    mul qword [rsi + 24]  ; * b[3]
    add [rsp + 48], rax
    adc [rsp + 56], rdx
    
    ; Копируем младшие 256 бит в результат
    mov rax, [rsp]
    mov [rdx], rax
    mov rax, [rsp + 8]
    mov [rdx + 8], rax
    mov rax, [rsp + 16]
    mov [rdx + 16], rax
    mov rax, [rsp + 24]
    mov [rdx + 24], rax
    
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret

; void asm_u256_div(const uint256_t* a, const uint256_t* b, uint256_t* quotient, uint256_t* remainder)
; 256-bit division using restoring division algorithm
asm_u256_div:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    
    mov r12, rdi        ; a (dividend)
    mov r13, rsi        ; b (divisor)
    mov r14, rdx        ; quotient
    mov r15, rcx        ; remainder
    
    ; Проверка деления на ноль
    mov rax, [r13]
    or rax, [r13 + 8]
    or rax, [r13 + 16]
    or rax, [r13 + 24]
    jnz .not_zero
    ; Деление на ноль - возвращаем 0
    mov qword [r14], 0
    mov qword [r14 + 8], 0
    mov qword [r14 + 16], 0
    mov qword [r14 + 24], 0
    mov qword [r15], 0
    mov qword [r15 + 8], 0
    mov qword [r15 + 16], 0
    mov qword [r15 + 24], 0
    jmp .done
    
.not_zero:
    ; Инициализация remainder = 0
    mov qword [r15], 0
    mov qword [r15 + 8], 0
    mov qword [r15 + 16], 0
    mov qword [r15 + 24], 0
    
    ; Алгоритм деления на 256 бит
    mov rcx, 256        ; 256 бит для обработки
    
.div_loop:
    ; Сдвиг remainder:quotient влево на 1 бит
    shl qword [r15], 1
    rcl qword [r15 + 8], 1
    rcl qword [r15 + 16], 1
    rcl qword [r15 + 24], 1
    
    ; Сдвиг dividend и установка младшего бита remainder
    shl qword [r12], 1
    rcl qword [r12 + 8], 1
    rcl qword [r12 + 16], 1
    rcl qword [r12 + 24], 1
    adc qword [r15], 0
    
    ; remainder >= divisor?
    mov rax, [r15 + 24]
    cmp rax, [r13 + 24]
    jb .no_sub
    ja .do_sub
    mov rax, [r15 + 16]
    cmp rax, [r13 + 16]
    jb .no_sub
    ja .do_sub
    mov rax, [r15 + 8]
    cmp rax, [r13 + 8]
    jb .no_sub
    ja .do_sub
    mov rax, [r15]
    cmp rax, [r13]
    jb .no_sub
    
.do_sub:
    ; remainder -= divisor
    mov rax, [r13]
    sub [r15], rax
    mov rax, [r13 + 8]
    sbb [r15 + 8], rax
    mov rax, [r13 + 16]
    sbb [r15 + 16], rax
    mov rax, [r13 + 24]
    sbb [r15 + 24], rax
    
    ; Установка бита в quotient
    or qword [r14], 1
    
.no_sub:
    ; Сдвиг quotient влево
    shl qword [r14], 1
    rcl qword [r14 + 8], 1
    rcl qword [r14 + 16], 1
    rcl qword [r14 + 24], 1
    
    loop .div_loop
    
    ; Корректировка последнего сдвига
    shr qword [r14], 1
    rcr qword [r14 + 8], 1
    rcr qword [r14 + 16], 1
    rcr qword [r14 + 24], 1
    
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret

; int asm_u256_cmp(const uint256_t* a, const uint256_t* b)
; Returns: -1 if a < b, 0 if a == b, 1 if a > b
asm_u256_cmp:
    push rbp
    mov rbp, rsp
    
    ; Сравнение старших 64 бит
    mov rax, [rdi + 24]
    cmp rax, [rsi + 24]
    jb .less
    ja .greater
    
    ; Сравнение следующих 64 бит
    mov rax, [rdi + 16]
    cmp rax, [rsi + 16]
    jb .less
    ja .greater
    
    ; Сравнение следующих 64 бит
    mov rax, [rdi + 8]
    cmp rax, [rsi + 8]
    jb .less
    ja .greater
    
    ; Сравнение младших 64 бит
    mov rax, [rdi]
    cmp rax, [rsi]
    jb .less
    ja .greater
    
    ; Равны
    xor eax, eax
    jmp .done
    
.less:
    mov eax, -1
    jmp .done
    
.greater:
    mov eax, 1
    
.done:
    pop rbp
    ret

; void asm_u256_sqrt(const uint256_t* x, uint256_t* result)
; Babylonian method for square root
asm_u256_sqrt:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    push r14
    push r15
    
    mov r12, rdi        ; x
    mov r13, rsi        ; result
    
    ; Если x == 0, возвращаем 0
    mov rax, [r12]
    or rax, [r12 + 8]
    or rax, [r12 + 16]
    or rax, [r12 + 24]
    jnz .not_zero
    
    mov qword [r13], 0
    mov qword [r13 + 8], 0
    mov qword [r13 + 16], 0
    mov qword [r13 + 24], 0
    jmp .done
    
.not_zero:
    ; Начальное приближение: 2^(bit_length/2)
    mov r14, 1          ; bit = 1
    mov r15, 0          ; count bits
    
.count_bits:
    mov rax, [r12 + 24]
    test rax, rax
    jnz .count_high
    mov rax, [r12 + 16]
    test rax, rax
    jnz .count_mid
    mov rax, [r12 + 8]
    test rax, rax
    jnz .count_low
    mov rax, [r12]
    jmp .count_final
    
.count_high:
    bsr rax, rax
    add rax, 192
    jmp .set_approx
    
.count_mid:
    bsr rax, rax
    add rax, 128
    jmp .set_approx
    
.count_low:
    bsr rax, rax
    add rax, 64
    jmp .set_approx
    
.count_final:
    bsr rax, rax
    add rax, 0
    
.set_approx:
    shr rax, 1          ; bit_length / 2
    mov rcx, rax
    mov rax, 1
    shl rax, cl
    mov [r13], rax
    mov qword [r13 + 8], 0
    mov qword [r13 + 16], 0
    mov qword [r13 + 24], 0
    
    ; Итерации метода Ньютона (3 итерации для 256 бит)
    mov rcx, 3
    
.newton_loop:
    push rcx
    
    ; result = (result + x/result) / 2
    
    ; Сохраняем текущий result
    sub rsp, 32
    mov rax, [r13]
    mov [rsp], rax
    mov rax, [r13 + 8]
    mov [rsp + 8], rax
    mov rax, [r13 + 16]
    mov [rsp + 16], rax
    mov rax, [r13 + 24]
    mov [rsp + 24], rax
    
    ; Вычисляем x/result
    lea rdi, [rsp + 32]  ; temp для remainder
    mov rsi, r12         ; x
    mov rdx, rsp         ; result (делитель)
    call asm_u256_div
    
    ; (result + x/result)
    mov rdi, r13         ; result
    mov rsi, r13         ; quotient (x/result)
    mov rdx, r13         ; result
    call asm_u256_add
    
    ; / 2
    shr qword [r13 + 24], 1
    rcr qword [r13 + 16], 1
    rcr qword [r13 + 8], 1
    rcr qword [r13], 1
    
    add rsp, 32
    pop rcx
    loop .newton_loop
    
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    ret

; void asm_u256_shl(const uint256_t* a, uint32_t bits, uint256_t* result)
asm_u256_shl:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    
    mov r12, rdi        ; a
    mov r13d, esi       ; bits
    mov rdx, rdx        ; result
    
    ; bits >= 256?
    cmp r13d, 256
    jb .valid_bits
    ; Слишком много бит - обнуляем результат
    mov qword [rdx], 0
    mov qword [rdx + 8], 0
    mov qword [rdx + 16], 0
    mov qword [rdx + 24], 0
    jmp .done
    
.valid_bits:
    ; bits == 0?
    test r13d, r13d
    jnz .do_shift
    ; Просто копируем
    mov rax, [r12]
    mov [rdx], rax
    mov rax, [r12 + 8]
    mov [rdx + 8], rax
    mov rax, [r12 + 16]
    mov [rdx + 16], rax
    mov rax, [r12 + 24]
    mov [rdx + 24], rax
    jmp .done
    
.do_shift:
    mov ecx, r13d
    ; bits >= 192?
    cmp r13d, 192
    jae .shift_192
    ; bits >= 128?
    cmp r13d, 128
    jae .shift_128
    ; bits >= 64?
    cmp r13d, 64
    jae .shift_64
    
    ; Сдвиг < 64 бит
    mov rax, [r12]
    mov r8, [r12 + 8]
    mov r9, [r12 + 16]
    mov r10, [r12 + 24]
    
    shld r10, r9, cl
    shld r9, r8, cl
    shld r8, rax, cl
    shl rax, cl
    
    mov [rdx], rax
    mov [rdx + 8], r8
    mov [rdx + 16], r9
    mov [rdx + 24], r10
    jmp .done
    
.shift_64:
    mov ecx, r13d
    sub ecx, 64
    cmp r13d, 128
    jae .shift_128_plus
    
    ; Сдвиг 64-127 бит
    mov rax, [r12]
    mov r8, [r12 + 8]
    mov r9, [r12 + 16]
    mov r10, [r12 + 24]
    
    mov [rdx + 8], rax
    mov [rdx + 16], r8
    mov [rdx + 24], r9
    xor rax, rax
    mov [rdx], rax
    
    shl qword [rdx + 8], cl
    shld qword [rdx + 16], [rdx + 8], cl
    shld qword [rdx + 24], [rdx + 16], cl
    jmp .done
    
.shift_128_plus:
    mov ecx, r13d
    sub ecx, 128
    cmp r13d, 192
    jae .shift_192_plus
    
    ; Сдвиг 128-191 бит
    mov rax, [r12]
    mov r8, [r12 + 8]
    
    mov [rdx + 16], rax
    mov [rdx + 24], r8
    xor rax, rax
    mov [rdx], rax
    mov [rdx + 8], rax
    
    shl qword [rdx + 16], cl
    shld qword [rdx + 24], [rdx + 16], cl
    jmp .done
    
.shift_192_plus:
    mov ecx, r13d
    sub ecx, 192
    
    ; Сдвиг 192-255 бит
    mov rax, [r12]
    mov [rdx + 24], rax
    xor rax, rax
    mov [rdx], rax
    mov [rdx + 8], rax
    mov [rdx + 16], rax
    
    shl qword [rdx + 24], cl
    
.done:
    pop r13
    pop r12
    pop rbp
    ret

; void asm_u256_shr(const uint256_t* a, uint32_t bits, uint256_t* result)
asm_u256_shr:
    push rbp
    mov rbp, rsp
    push r12
    push r13
    
    mov r12, rdi        ; a
    mov r13d, esi       ; bits
    mov rdx, rdx        ; result
    
    ; bits >= 256?
    cmp r13d, 256
    jb .valid_bits
    ; Слишком много бит - обнуляем результат
    mov qword [rdx], 0
    mov qword [rdx + 8], 0
    mov qword [rdx + 16], 0
    mov qword [rdx + 24], 0
    jmp .done
    
.valid_bits:
    ; bits == 0?
    test r13d, r13d
    jnz .do_shift
    ; Просто копируем
    mov rax, [r12]
    mov [rdx], rax
    mov rax, [r12 + 8]
    mov [rdx + 8], rax
    mov rax, [r12 + 16]
    mov [rdx + 16], rax
    mov rax, [r12 + 24]
    mov [rdx + 24], rax
    jmp .done
    
.do_shift:
    mov ecx, r13d
    ; bits >= 192?
    cmp r13d, 192
    jae .shift_192
    ; bits >= 128?
    cmp r13d, 128
    jae .shift_128
    ; bits >= 64?
    cmp r13d, 64
    jae .shift_64
    
    ; Сдвиг < 64 бит
    mov rax, [r12]
    mov r8, [r12 + 8]
    mov r9, [r12 + 16]
    mov r10, [r12 + 24]
    
    shrd rax, r8, cl
    shrd r8, r9, cl
    shrd r9, r10, cl
    shr r10, cl
    
    mov [rdx], rax
    mov [rdx + 8], r8
    mov [rdx + 16], r9
    mov [rdx + 24], r10
    jmp .done
    
.shift_64:
    mov ecx, r13d
    sub ecx, 64
    cmp r13d, 128
    jae .shift_128_plus
    
    ; Сдвиг 64-127 бит
    mov rax, [r12 + 8]
    mov r8, [r12 + 16]
    mov r9, [r12 + 24]
    
    mov [rdx], rax
    mov [rdx + 8], r8
    mov [rdx + 16], r9
    xor rax, rax
    mov [rdx + 24], rax
    
    shr qword [rdx], cl
    shrd qword [rdx + 8], [rdx], cl
    shrd qword [rdx + 16], [rdx + 8], cl
    jmp .done
    
.shift_128_plus:
    mov ecx, r13d
    sub ecx, 128
    cmp r13d, 192
    jae .shift_192_plus
    
    ; Сдвиг 128-191 бит
    mov rax, [r12 + 16]
    mov r8, [r12 + 24]
    
    mov [rdx], rax
    mov [rdx + 8], r8
    xor rax, rax
    mov [rdx + 16], rax
    mov [rdx + 24], rax
    
    shr qword [rdx], cl
    shrd qword [rdx + 8], [rdx], cl
    jmp .done
    
.shift_192_plus:
    mov ecx, r13d
    sub ecx, 192
    
    ; Сдвиг 192-255 бит
    mov rax, [r12 + 24]
    mov [rdx], rax
    xor rax, rax
    mov [rdx + 8], rax
    mov [rdx + 16], rax
    mov [rdx + 24], rax
    
    shr qword [rdx], cl
    
.done:
    pop r13
    pop r12
    pop rbp
    ret
