#pragma once

#include <cpu_defs.h>

template <typename T>
T swap_endian(T v);

template <typename T>
T be(T v);

template <typename T>
T cpu2be(T v) { return be<T>(v); }

template <typename T>
T be2cpu(T v) { return be<T>(v); }

template <typename T>
T le(T v);