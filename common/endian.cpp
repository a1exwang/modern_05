#include <common/endian.hpp>


template <>
u32 swap_endian<u32>(u32 v) {
  return ((v>>0) & 0xff) << 24 |
      ((v>>8) & 0xff) << 16 |
      ((v>>16) & 0xff) << 8 |
      ((v>>24) & 0xff) << 0;
}


template <>
u16 swap_endian<u16>(u16 v) {
  return ((v>>0) & 0xff) << 8 |
      ((v>>8) & 0xff) << 0;
}

template <>
u8 swap_endian<u8>(u8 v) {
  return v;
}
