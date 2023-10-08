#include <determinize/determinize.h>

// TODO: Windows
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <algorithm>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include <Zydis.h>

#include <determinize/replacements.h>
#include <determinize/thunk.h>

#include "trampoline.h"
#include "util.h"

static_assert(sizeof(void*) == 8);

namespace determinize {

static TrampolineAllocator g_trampoline_allocator;

template <typename T>
size_t Append(std::vector<char>& vec, T value) {
  size_t size = vec.size();
  vec.resize(size + sizeof(value));
  memcpy(&vec[size], &value, sizeof(value));
  return size;
}

std::optional<size_t> GetInstructionLength(void* p) {
  ZydisDisassembledInstruction instruction;
  auto rc = ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, reinterpret_cast<ZyanU64>(p), p, 4096,
                                  &instruction);
  if (!ZYAN_SUCCESS(rc)) return {};
  return instruction.info.length;
}

bool GenerateThunk(void* thunk_address, void* jump_target, void* relocation_begin,
                   size_t relocation_size) {
  Thunk thunk;
  char* p = static_cast<char*>(relocation_begin);

  // Emit the following thunk:
  //   call *jump_target(%rip)
  //   <relocated instructions>
  //   jmp *return_address
  //   .quad jump_target
  //   .quad return_address
  thunk.Call(jump_target);

  ZydisDisassembledInstruction instruction;
  ZyanUSize offset = 0;
  printf("Relocated instructions:\n");
  while (offset < relocation_size) {
    char* current_instruction = p + offset;
    auto rc = ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64,
                                    reinterpret_cast<ZyanU64>(current_instruction),
                                    current_instruction, 4096, &instruction);
    if (!ZYAN_SUCCESS(rc)) return false;

    printf("  0x%016lx  %s\n", reinterpret_cast<long>(current_instruction), instruction.text);
    offset += instruction.info.length;
    if (!thunk.Relocate(&instruction)) {
      return false;
    }
  }

  void* return_address = p + offset;
  thunk.Jump(return_address);

  printf("Thunk:\n");
  thunk.Dump("  ");

  std::vector<char> thunk_data = thunk.Emit();
  memcpy(thunk_address, thunk_data.data(), thunk_data.size());

  return true;
}

bool Replace(void* replacement, void* target, size_t target_length) {
  if (!Unprotect(target)) return false;
  void* next_page = NextPage(target);
  if (!Unprotect(next_page)) return false;

  // TODO: Allocate less stupidly.
  void* thunk = MapRandomPage();

  std::optional<Trampoline> trampoline;
  std::vector<char> call_instructions;

  if ((trampoline = g_trampoline_allocator.AllocateTrampoline(target, INT32_MAX))) {
    call_instructions.resize(5);
    call_instructions[0] = 0xE9;
    int32_t trampoline_offset =
        reinterpret_cast<intptr_t>(trampoline->Address()) - reinterpret_cast<intptr_t>(target) - 5;
    memcpy(&call_instructions[1], &trampoline_offset, sizeof(trampoline_offset));
  } else {
    // jmp *0x0(%rip)
    // .quad <thunk>
    call_instructions.resize(14);
    call_instructions[0] = 0xff;
    call_instructions[1] = 0x25;
    memcpy(&call_instructions[6], &thunk, sizeof(thunk));
  }

  uintptr_t relocation_addr = reinterpret_cast<uintptr_t>(target) + target_length;
  size_t relocation_size = call_instructions.size();
  if (target_length > relocation_size) {
    relocation_size = 0;
  } else {
    relocation_size -= target_length;
  }

  if (!GenerateThunk(thunk, replacement, reinterpret_cast<void*>(relocation_addr),
                     relocation_size)) {
    return false;
  }

  trampoline->Write(thunk);

  printf("trampoline: %p\n", trampoline->Address());
  printf("thunk: %p\n", thunk);
  memcpy(target, call_instructions.data(), call_instructions.size());
  __builtin___clear_cache(target, static_cast<char*>(target) + call_instructions.size());

  return true;
}

static bool ShouldDeterminize(ZydisDisassembledInstruction* instruction) {
  auto mnemonic = instruction->info.mnemonic;
  switch (mnemonic) {
    case ZYDIS_MNEMONIC_RCPPS:
    case ZYDIS_MNEMONIC_RSQRTPS:
    case ZYDIS_MNEMONIC_RSQRTSS:
      return true;

    default:
      return false;
  }
}

static std::optional<size_t> Determinize(Thunk* thunk, char* instruction) {
  ZydisDisassembledInstruction disasm;
  auto rc =
      ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, reinterpret_cast<ZyanU64>(instruction),
                            instruction, 4096, &disasm);
  if (!ZYAN_SUCCESS(rc)) return {};

  if (!ShouldDeterminize(&disasm)) {
    if (!thunk->Relocate(&disasm)) return {};
    return disasm.info.length;
  }

  if (disasm.info.operand_count != 2) {
    fprintf(stderr, "invalid operand count %d\n", disasm.info.operand_count);
    return {};
  }

  if (disasm.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER) {
    fprintf(stderr, "invalid destination operand type: %d", disasm.operands[0].type);
    return {};
  }

  ZydisRegister dst = disasm.operands[0].reg.value;
  switch (disasm.info.mnemonic) {
    case ZYDIS_MNEMONIC_RCPPS:
      if (!GenerateRcpps(thunk, dst, disasm.operands[1])) return {};
      break;

    case ZYDIS_MNEMONIC_RSQRTPS:
      if (!GenerateRsqrtps(thunk, dst, disasm.operands[1])) return {};
      break;

    case ZYDIS_MNEMONIC_RSQRTSS:
      if (!GenerateRsqrtss(thunk, dst, disasm.operands[1])) return {};
      break;

    default:
      fprintf(stderr, "UNHANDLED INSTRUCTION:\n");
      fprintf(stderr, "  0x%016lx  %s\n", reinterpret_cast<long>(instruction), disasm.text);
      return {};
  }

  return disasm.info.length;
}

bool Determinize(std::vector<void*> instruction_addresses) {
  std::vector<char*> addrs;
  for (auto addr : instruction_addresses) {
    addrs.push_back(static_cast<char*>(addr));
  }
  std::sort(addrs.begin(), addrs.end());

  auto it = addrs.begin();
  while (it != addrs.end()) {
    char* target = reinterpret_cast<char*>(*it);
    std::optional<Trampoline> trampoline =
        g_trampoline_allocator.AllocateTrampoline(target, INT32_MAX);

    size_t jump_length = trampoline ? 5 : 14;

    ZyanUSize offset = 0;
    Thunk thunk;
    while (offset <= jump_length) {
      std::optional<size_t> length = Determinize(&thunk, target + offset);
      if (!length) return false;
      offset += *length;
    }

    // We might be overwriting an instruction that we're supposed to fix up.
    while (it != addrs.end() && *it < target + offset) ++it;

    // Keep going past the end until we hit an instruction we're not patching.
    while (it != addrs.end() && *it == target + offset) {
      std::optional<size_t> length = Determinize(&thunk, target + offset);
      if (!length) return false;
      offset += *length;
      ++it;
    }

    // Jump back to the following instruction.
    thunk.Jump(target + offset);

    if (!Unprotect(target)) return false;
    void* next_page = NextPage(target);
    if (!Unprotect(next_page)) return false;

    printf("Thunk:\n");
    thunk.Dump("  ");

    std::vector<char> thunk_data = thunk.Emit();
    void* thunk_address = MapRandomPage();
    memcpy(thunk_address, thunk_data.data(), thunk_data.size());

    trampoline->Write(thunk_address);

    auto trampoline_address = reinterpret_cast<intptr_t>(trampoline->Address());
    if (trampoline) {
      target[0] = 0xE9;
      int32_t trampoline_offset = trampoline_address - reinterpret_cast<intptr_t>(target) - 5;
      memcpy(&target[1], &trampoline_offset, sizeof(trampoline_offset));
    } else {
      // jmp *0x0(%rip)
      // .quad <thunk>
      target[0] = 0xFF;
      target[1] = 0x25;
      target[2] = 0x00;
      target[3] = 0x00;
      target[4] = 0x00;
      target[5] = 0x00;
      memcpy(&target[6], &trampoline_address, sizeof(trampoline_address));
    }
  }

  return true;
}

}  // namespace determinize
