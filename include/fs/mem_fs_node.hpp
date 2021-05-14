#pragma once

#include <common/kvector.hpp>
#include <common/kstring.hpp>
#include <common/kmap.hpp>

enum class NodeType {
  Dir,
  File,
};

class MemFsNode {
 public:
  virtual constexpr NodeType type() = 0;
};

class MemFsDirNode;
class MemFsFileNode :public MemFsNode {
 public:
  MemFsFileNode(MemFsDirNode *parent) :parent_(parent) { }
  constexpr NodeType type() override { return NodeType::Dir; }

  size_t Read(void *buffer, size_t size, size_t offset) {
    assert1(offset < data_.size());
    size_t read_size = size;
    if (offset + size > data_.size()) {
      read_size = data_.size() - offset;
    }
    memcpy(buffer, data_.data() + offset, read_size);
    return read_size;
  }

  size_t Write(const void *buffer, size_t size, size_t offset) {
    if (offset > data_.size()) {
      data_.resize(offset + size);
    }
    memcpy(data_.data() + offset, buffer, size);
    return size;
  }

  void Resize(size_t new_size) {
    data_.resize(new_size);
  }

  void Flush() { }

  size_t size() { return data_.size(); }

 private:
  kvector<char> data_;
  MemFsDirNode *parent_;
};

class MemFsDirNode :public MemFsNode {
 public:
  MemFsDirNode(MemFsDirNode *parent) : parent_(parent) { }

  constexpr NodeType type() override { return NodeType::File; }

  kmap<kstring, MemFsNode*> ListDirNode() {
    kmap<kstring, MemFsNode*> ret;
    for (const auto& [k, v] : entries_) {
      ret.emplace(k, v.get());
    }
    return ret;
  }

  MemFsDirNode *Mkdir(const kstring &name) {
    assert1(entries_.find(name) == entries_.end());
    auto p = knew<MemFsDirNode>(this);
    auto [it, _] = entries_.emplace(name, kup<MemFsNode>(p));
    p = nullptr;
    // we don't have dynamic_cast without RTTI
    return reinterpret_cast<MemFsDirNode*>(it->second.get());
  }

  MemFsFileNode *Open(const kstring &name) {
    auto it = entries_.find(name);
    if (it != entries_.end()) {
      assert1(it->second->type() == NodeType::File);
      return reinterpret_cast<MemFsFileNode*> (it->second.get());
    }

    auto p = knew<MemFsFileNode>(this);
    auto [it2, _] = entries_.emplace(name, kup<MemFsNode>(p));
    p = nullptr;
    return reinterpret_cast<MemFsFileNode*>(it2->second.get());
  }
//  void Rename(const kstring &name, const kstring &new_name) {
//
//  }
//  void Remove(const kstring &name) {
//
//  }
//
//  void LoadEntryCache(const kstring &name) {
//
//  }
 private:
  MemFsDirNode *parent_;
  kmap<kstring, kup<MemFsNode>> entries_;
};
