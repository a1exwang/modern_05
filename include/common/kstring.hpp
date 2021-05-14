#pragma once
#include <string>
#include <common/kallocator.hpp>

template<typename Char>
using kbasic_string = std::basic_string<Char, std::char_traits<Char>, kallocator<Char>>;

using kstring = kbasic_string<char>;
