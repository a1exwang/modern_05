#pragma once
#include <list>
#include <common/kallocator.hpp>

template <typename T, typename Alloc = kallocator<T>>
using klist = std::list<T, Alloc>;
