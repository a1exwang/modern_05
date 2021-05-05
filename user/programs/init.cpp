extern "C" int _start() {
  const char *hello = "hello from uSeR5p@ze";
  asm volatile(
      "movq $3, %%rax\t\n"
      "movq %0, %%rdi\t\n"
      "int $42"
      :
      :"r"(hello)
      :"%rax"
      );
  while (1) ;
  return 0;
}