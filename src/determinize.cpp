#include <determinize.h>

// TODO: Windows
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <map>
#include <optional>
#include <vector>

#include <Zydis.h>

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

bool GenerateThunk(void* thunk, void* jump_target, void* relocation_begin, size_t relocation_size) {
  char* p = static_cast<char*>(relocation_begin);

  // Emit the following thunk:
  //   call *jump_target(%rip)
  //   <relocated instructions>
  //   jmp *return_address
  //   .quad jump_target
  //   .quad return_address

  char* thunk_cur = static_cast<char*>(thunk);
  std::vector<char> data;

  char* thunk_call = thunk_cur;
  uint32_t thunk_call_data = Append<void*>(data, static_cast<void*>(jump_target));
  thunk_cur += 6;

  ZydisDisassembledInstruction instruction;
  ZyanUSize offset = 0;
  while (offset < relocation_size) {
    char* current_instruction = p + offset;
    auto rc = ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64,
                                    reinterpret_cast<ZyanU64>(current_instruction),
                                    current_instruction, 4096, &instruction);
    if (!ZYAN_SUCCESS(rc)) return false;

    printf("%p  %s\n", current_instruction, instruction.text);
    offset += instruction.info.length;

    // TODO: Actually relocate the instruction instead of blindly copying it.
    memcpy(thunk_cur, current_instruction, instruction.info.length);
    thunk_cur += instruction.info.length;
  }

  void* return_address = p + offset;
  char* thunk_return = thunk_cur;
  uint32_t thunk_return_data = Append<void*>(data, static_cast<void*>(return_address));
  thunk_cur += 6;

  char* thunk_data = thunk_cur;
  memcpy(thunk_data, data.data(), data.size());

  thunk_call[0] = 0xff;
  thunk_call[1] = 0x15;
  thunk_call_data += (thunk_data - thunk_call) - 6;
  memcpy(&thunk_call[2], &thunk_call_data, sizeof(thunk_call_data));

  thunk_return[0] = 0xff;
  thunk_return[1] = 0x25;
  thunk_return_data += (thunk_data - thunk_return) - 6;
  memcpy(&thunk_return[2], &thunk_return_data, sizeof(thunk_return_data));

  return true;
}

bool Replace(void* replacement, void* target, size_t target_length) {
  // TODO: Handle the case where the overwritten instructions cross a page boundary.
  if (!Unprotect(target)) return false;

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

}  // namespace determinize
