.global kernel_init_stack_bottom
.global _setup_init_stack

_setup_init_stack:
    popq %rdi
    movabsq (kernel_init_stack_bottom), %rax
    movq %rax, %rsp
    pushq %rdi
    ret
