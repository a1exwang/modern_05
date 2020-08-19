
#include <init/efi_info.h>

extern "C"
void kernel_start() {
  EFIServicesInfo *ServicesInfo = (EFIServicesInfo*)EFIServiceInfoAddress;
  ServicesInfo->func();

  while (1) {
    // spin
  }
}