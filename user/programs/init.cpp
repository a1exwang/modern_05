void print(const char *str) {
  asm volatile(
  "movq $3, %%rax\t\n"
  "movq %0, %%rdi\t\n"
  "int $42"
  :
  :"r"(str)
  :"%rax"
  );
}

extern "C" int _start() {
  const char *hello = "hello from uSeR5p@ze";
  print(hello);
  while (1) {
//    print(hello);
    for (int i = 0; i < 10000; i++) { }
  }
  return 0;
}