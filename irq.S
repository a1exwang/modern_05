.global _de_irq_handler
.global _nmi_irq_handler
.global _db_irq_handler
.global _bp_irq_handler

.global _of_irq_handler
.global _br_irq_handler
.global _ud_irq_handler
.global _nm_irq_handler

.global _df_irq_handler
.global _ts_irq_handler
.global _np_irq_handler

.global _ss_irq_handler
.global _gp_irq_handler

.global _timer_irq_handler
.global _syscall_irq_handler
.global _default_irq_handler
.global irq_handler
.global return_from_syscall
.global _interrupt_stack_bottom
.global schedule

# context from %rdi
return_from_syscall:
    mov %rdi, %rsp

    pop %rax
    mov %ax, %ds
    pop %rax
    mov %ax, %es
    pop %rax
    mov %ax, %fs
    pop %rax
    mov %ax, %gs

    pop %rax
    mov %rax, %cr3

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

    mov %cr3, %rax
    push %rax

    xor %rax, %rax
    mov %gs, %ax
    push %rax
    mov %fs, %ax
    push %rax
    mov %es, %ax
    push %rax
    mov %ds, %ax
    push %rax

    movq %rsp, %rdi
    call store_current_thread_context

    movq 160(%rsp), %rdi
    movq 168(%rsp), %rsi
    call irq_handler

    call schedule

    call current_context
    mov %rax, %rdi
    jmp return_from_syscall

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

# overflow
_of_irq_handler:
    pushq $0
    pushq $4
    jmp _irq_handler
    iretq

# bound-range
_br_irq_handler:
    pushq $0
    pushq $5
    jmp _irq_handler
    iretq

# invalid opcode
_ud_irq_handler:
    pushq $0
    pushq $6
    jmp _irq_handler
    iretq

# device not available
_nm_irq_handler:
    pushq $0
    pushq $7
    jmp _irq_handler
    iretq

# double fault
_df_irq_handler:
    pushq $8
    jmp _irq_handler
    iretq

# invalid tss
_ts_irq_handler:
    pushq $10
    jmp _irq_handler
    iretq

# segment not present
_np_irq_handler:
    pushq $11
    jmp _irq_handler
    iretq

# stack exception
_ss_irq_handler:
    pushq $12
    jmp _irq_handler
    iretq

# general protection
_gp_irq_handler:
    pushq $13
    jmp _irq_handler
    iretq

_timer_irq_handler:
    pushq $0
    pushq $32

    jmp _irq_handler

    iretq

_syscall_irq_handler:
    pushq $0
    pushq $42

    jmp _irq_handler

    iretq

_default_irq_handler:
    pushq $-1
    pushq $0xff

    jmp _irq_handler

