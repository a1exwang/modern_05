#pragma once

namespace {

void hex01(u8 value) {
  Kernel::sp() << IntRadix::Hex << (value >> 4) << (value & 0xf);
}

void hex04(u32 value) {
  for (int i = 0; i < 8; i++) {
    hex01((value >> (64 - 8 - i * 8)) & 0xff);
  }
}

template <typename T>
void print_hex(T value) {
  static_assert(std::is_integral_v<T>);
  using UnsignedT = typename std::make_unsigned_t<T>;
  for (int i = 0; i < sizeof(T); i++) {
    hex01(((UnsignedT)value >> (8 * (sizeof(T) - 1 - i))) & 0xff);
  }
}

bool isprint(char c) {
  return 0x20 <= c && c < 0x7f;
}

}

// size <= cols
template <typename T>
void hexdump_line(const T *ptr, size_t size, size_t cols, size_t addr, bool compact, int indent) {
  if (!compact) {
    for (int ind = 0; ind < indent; ind++) {
      Kernel::sp() << ' ';
    }
    print_hex(addr);
    Kernel::sp() << " | ";
  }

  for (u64 col = 0; col < size; col++) {
    hex01(ptr[col]);
    Kernel::sp() << " ";
  }

  if (!compact) {
    for (u64 col = size; col < cols; col++) {
      Kernel::sp() << "   ";
    }
    Kernel::sp() << " | ";
    for (u64 col = 0; col < size; col++) {
      auto c = (char)ptr[col];
      if (isprint(c)) {
        Kernel::sp() << c;
      } else {
        Kernel::sp() << '.';
      }
    }
    Kernel::sp() << "\n";
  }
}

// compat: true: no indent, no space, no newline, no address, no ascii
template <typename T>
void hexdump(const T *ptr, size_t size, bool compact = false, int indent = 0) {
  static_assert(std::is_integral_v<T>);

  u64 cols = 16;
  u64 whole_lines = size / cols;
  for (u64 line = 0; line < whole_lines; line++) {
    hexdump_line<T>(ptr + line*cols, cols, cols, line*cols*sizeof(T), compact, indent);
  }

  if (whole_lines*cols < size) {
    auto cols_remain = size - whole_lines*cols;
    hexdump_line<T>(ptr + whole_lines*cols, cols_remain, cols, whole_lines*cols*sizeof(T), compact, indent);
  }
}
