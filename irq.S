.global _de_irq_handler
.global _nmi_irq_handler
.global _db_irq_handler
.global _bp_irq_handler
.global _timer_irq_handler
.global _default_irq_handler
.global irq_handler

_irq_handler:
    push %rax
    push %rcx
    push %rdx
    push %rbx
    push %rdi
    push %rsi
    push %rbp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    movq 120(%rsp), %rdi
    movq 128(%rsp), %rsi
    call irq_handler

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rbp
    pop %rsi
    pop %rdi
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax

    add $16, %rsp
    iretq

# divide by 0
_de_irq_handler:
    pushq $0
    pushq $0

    jmp _irq_handler

    iretq

# debugger
_db_irq_handler:
    pushq $0
    pushq $1
    jmp _irq_handler
    iretq

# NMI
_nmi_irq_handler:
    pushq $0
    pushq $2
    jmp _irq_handler
    iretq

# breakpoint
_bp_irq_handler:
    pushq $0
    pushq $3
    jmp _irq_handler
    iretq

_timer_irq_handler:
    pushq $0
    pushq $32

    jmp _irq_handler

    iretq

_default_irq_handler:
    pushq $-1
    pushq $0xff

    jmp _irq_handler