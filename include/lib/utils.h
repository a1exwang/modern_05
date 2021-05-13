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

// when n = 0, it returns (u64)-1
static u64 log2_ceil(u64 n) {
  u64 ones = 0;
  u64 ret = 0;
  do {
    if ((n & 1) != 0) {
      ones++;
    }
    n >>= 1;
    ret++;
  } while (n > 0);
  return ones > 1 ? ret : ret - 1;
}
