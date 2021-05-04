#pragma once

// when n = 0, it returns (u64)-1
static u64 log2(u64 n) {
  u64 ret = 0;
  do {
    n >>= 1;
    ret++;
  } while (n > 0);
  return ret - 1;
}