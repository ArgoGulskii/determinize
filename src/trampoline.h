#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <map>
#include <optional>

#include "util.h"

namespace determinize {

struct Trampoline {
  explicit Trampoline(char* address) : address_(address) {}

  void Write(void* target) {
    // jmp *0x0(%rip)
    // .quad <thunk>
    memset(address_, 0, 14);
    address_[0] = 0xff;
    address_[1] = 0x25;
    memcpy(&address_[6], &target, sizeof(target));
  }

  void* Address() const { return address_; }

  static constexpr size_t length = 14;

 private:
  char* address_;
};

struct Page {
  explicit Page(char* page) : base_(page) {}

  std::optional<Trampoline> Allocate(void* p, size_t range) {
    if (allocated_ + Trampoline::length > 4096) {
      return std::nullopt;
    }

    size_t diff = PointerRange(p, base_ + allocated_);
    if (diff > range) {
      return std::nullopt;
    }

    Trampoline result(base_ + allocated_);
    allocated_ += Trampoline::length;
    return result;
  }

  intptr_t Base() const { return reinterpret_cast<intptr_t>(base_); }
  void* Next() const { return base_ + allocated_; }
  size_t Available() const { return (4096 - allocated_) / Trampoline::length; }

 private:
  char* base_;
  size_t allocated_ = 0;
};

struct TrampolineAllocator {
  Page* FindPage(void* p, size_t range) {
    if (pages_.empty()) return nullptr;

    uintptr_t value = reinterpret_cast<uintptr_t>(p);

    auto it = pages_.upper_bound(value);
    if (it != pages_.end()) {
      if (PointerRange(p, it->second.Next()) < range) {
        return &it->second;
      }
    }

    if (it == pages_.begin()) {
      // Nothing before.
      return nullptr;
    }

    --it;
    if (PointerRange(p, it->second.Next()) < range) {
      return &it->second;
    }

    return nullptr;
  }

  Page* AllocatePage(void* p, size_t range) {
    for (size_t i = 0; i < range; i += 4096) {
      for (int sign : {-1, 1}) {
        void* page = PageAlign(p);

        uintptr_t page_addr = reinterpret_cast<uintptr_t>(page);
        intptr_t offset = i * sign;

        page = MapPage(reinterpret_cast<void*>(page_addr + offset));
        if (!page) {
          continue;
        }

        auto it = pages_.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(reinterpret_cast<uintptr_t>(page)),
                                 std::forward_as_tuple(static_cast<char*>(page)));
        return &it.first->second;
      }
    }

    return nullptr;
  }

  std::optional<Trampoline> AllocateTrampoline(void* p, size_t range) {
    Page* page = FindPage(p, range);
    if (!page) {
      page = AllocatePage(p, range);
      if (!page) return std::nullopt;
    }

    std::optional<Trampoline> trampoline = page->Allocate(p, range);
    if (trampoline) {
      if (page->Available() == 0) {
        pages_.erase(page->Base());
      }
    }
    return trampoline;
  }
  std::map<uintptr_t, Page> pages_;
};

}  // namespace determinize
