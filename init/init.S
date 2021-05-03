.global kernel_init_stack_bottom
.global _setup_init_stack

_setup_init_stack:
    popq %rdi
    movq kernel_init_stack_bottom, %rsp
    pushq %rdi
    ret
