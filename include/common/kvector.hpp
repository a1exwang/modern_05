#pragma once
#include <vector>
#include <common/kallocator.hpp>

template <typename T>
using kvector = std::vector<T, kallocator<T>>;