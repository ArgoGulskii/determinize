#pragma once

// TODO: Windows
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>

static void* PageAlign(void* p) {
  uintptr_t value = reinterpret_cast<uintptr_t>(p);
  return reinterpret_cast<void*>(value & ~4095UL);
}

static size_t PointerRange(void* p, void* q) {
  auto x = reinterpret_cast<intptr_t>(p);
  auto y = reinterpret_cast<intptr_t>(q);
  auto result = x - y;
  if (result > 0) return result;
  return -result;
}

static void* MapPage(void* page) {
  void* result = mmap(page, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
  if (result == MAP_FAILED) {
    return nullptr;
  }

  if (result != page) {
    munmap(page, 4096);
    return nullptr;
  }

  return result;
}

static void* MapRandomPage() {
  void* result =
      mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (result == MAP_FAILED) abort();
  return result;
}

static bool Unprotect(void* p) {
  return mprotect(PageAlign(p), 4096, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}
