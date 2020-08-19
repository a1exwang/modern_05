extern "C" void kernel_start();

extern "C"
void init() {
  kernel_start();
}