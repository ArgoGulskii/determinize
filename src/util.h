#pragma once

#include <stddef.h>
#include <stdlib.h>

#include <string>

#include <Zydis.h>

static void* PageAlign(void* p) {
  uintptr_t value = reinterpret_cast<uintptr_t>(p);
  return reinterpret_cast<void*>(value & ~4095ULL);
}

static void* NextPage(void* p) {
  uintptr_t value = reinterpret_cast<uintptr_t>(PageAlign(p));
  return reinterpret_cast<void*>(value + 4096);
}

static size_t PointerRange(void* p, void* q) {
  auto x = reinterpret_cast<intptr_t>(p);
  auto y = reinterpret_cast<intptr_t>(q);
  auto result = x - y;
  if (result > 0) return result;
  return -result;
}

#if !defined(_WIN32)

#include <sys/mman.h>

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

// TODO: Make this take a pointer and length.
static bool Unprotect(void* p) {
  return mprotect(PageAlign(p), 4096, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}

#else

#include <Windows.h>

static void* MapPage(void* page) {
  return VirtualAlloc(page, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

static void* MapRandomPage() {
  void* result = MapPage(nullptr);
  if (!result) abort();
  return result;
}

// TODO: Make this take a pointer and length.
static bool Unprotect(void* p) {
  DWORD dummy;
  return VirtualProtect(PageAlign(p), 4096, PAGE_EXECUTE_READWRITE, &dummy);
}

#endif

static std::string FormatOperand(ZydisDecodedOperand operand) {
  if (operand.type == ZYDIS_OPERAND_TYPE_REGISTER) {
    return ZydisRegisterGetString(operand.reg.value);
  } else if (operand.type == ZYDIS_OPERAND_TYPE_MEMORY) {
    auto& mem = operand.mem;
    std::string result = "[";
    result += ZydisRegisterGetString(mem.base);
    if (mem.index != ZYDIS_REGISTER_NONE) {
      result += " + ";
      result += ZydisRegisterGetString(mem.index);
      if (mem.scale != 1) {
        result += " * ";
        result += std::to_string(mem.scale);
      }
    }
    if (mem.disp.has_displacement) {
      result += " + ";
      result += std::to_string(mem.disp.value);
    }
    result += "]";
    return result;
  }
  return "<unhandled operand type>";
}
