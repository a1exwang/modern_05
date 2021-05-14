#pragma once
#include <map>
#include <common/kallocator.hpp>

template <typename Key, typename Value, typename Compare = std::less<Key>, typename Alloc = kallocator<std::pair<const Key, Value>>>
using kmap = std::map<Key, Value, Compare, Alloc>;
