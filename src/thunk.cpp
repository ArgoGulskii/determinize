#include <determinize/thunk.h>

#include <stdio.h>

#include "util.h"

namespace determinize {

bool Thunk::Relocate(ZydisDisassembledInstruction* insn) {
  ZydisEncoderRequest req;
  ZydisEncoderDecodedInstructionToEncoderRequest(&insn->info, insn->operands,
                                                 insn->info.operand_count_visible, &req);

  if (insn->info.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE) {
    // Conditional branches are annoying because they only support a relative jump, and we don't
    // know where we're going to be written so replace:
    //
    //   jcc offset
    //
    // with:
    //   jcc 2
    //   1:
    //   jmp 3
    //   2:
    //   jmp [rip + absolute_target]
    //   3:
    //
    // Unconditional branches are handled identically because I'm lazy, and the branch predictor
    // should magically make it basically free.
    //
    // Additionally, we need to handle the case where we're jumping into patched instructions.
    // Do this as a fixup during Emit.
    if (insn->info.operand_count_visible != 1) {
      for (int i = 0; i < insn->info.operand_count_visible; ++i) {
        fprintf(stderr, "operand %d = %s\n", i, FormatOperand(insn->operands[i]).c_str());
      }

      fprintf(stderr, "unexpected operand count %d\n", insn->info.operand_count);
      fprintf(stderr, "  0x%016zx  %s\n", insn->runtime_address, insn->text);
      return false;
    }

    auto& operand = insn->operands[0];
    if (operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) {
      ZyanU64 jump_target = insn->runtime_address + insn->info.length + operand.imm.value.s;
      fprintf(stderr, "  0x%016zx  %s\n", insn->runtime_address, insn->text);
      printf("jump target = 0x%016zx\n", jump_target);
      Append(insn->info.mnemonic, Immediate(2));
      Append(ZYDIS_MNEMONIC_JMP, Immediate(6));
      Append(ZYDIS_MNEMONIC_JMP, RelocatedData(&jump_target, sizeof(jump_target)));
      return true;
    } else {
      fprintf(stderr, "unhandled branch operand type: %d\n", operand.type);
      fprintf(stderr, "  0x%016zx  %s\n", insn->runtime_address, insn->text);
    }
    return false;
  }

  // We need to fixup instructions that reference the instruction pointer, since we're executing
  // at a different address.
  for (size_t i = 0; i < insn->info.operand_count_visible; ++i) {
    const auto& operand = insn->operands[i];
    switch (operand.type) {
      case ZYDIS_OPERAND_TYPE_REGISTER:
        if (operand.reg.value == ZYDIS_REGISTER_RIP) {
          fprintf(stderr, "unhandled instruction operand: register\n");
          return false;
        }
        break;

      case ZYDIS_OPERAND_TYPE_MEMORY: {
        const auto& mem = operand.mem;
        if (mem.base == ZYDIS_REGISTER_RIP) {
          if (i == 0) {
            // This should never happen?
            fprintf(stderr, "unhandled case: rip-relative memory operand as destination\n");
            return false;
          }
        }

        if (mem.index == ZYDIS_REGISTER_RIP) {
          fprintf(stderr, "rip in index register?\n");
          return false;
        }

        if (mem.base != ZYDIS_REGISTER_RIP) continue;

        ZyanU64 data_address;
        if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&insn->info, &operand, insn->runtime_address,
                                                   &data_address))) {
          fprintf(stderr, "failed to calculate data address\n");
          return false;
        }

        // TODO: Appropriately align data.
        uint64_t data_offset = data_.size();
        auto p = reinterpret_cast<char*>(data_address);
        data_.insert(data_.end(), p, p + operand.size / 8);

        relocations_[instructions_.size()].emplace_back(Relocation{
            .operand = i,
            .data_offset = data_offset,
        });
        break;
      }

      case ZYDIS_OPERAND_TYPE_POINTER:
      case ZYDIS_OPERAND_TYPE_UNUSED:
      case ZYDIS_OPERAND_TYPE_IMMEDIATE:
        continue;
    }
  }

  instructions_.push_back(req);
  return true;
}

std::vector<char> Thunk::Emit() {
  std::vector<char> result = EmitInstructions();
  EmitData(&result);
  return result;
}

std::vector<char> Thunk::EmitInstructions() {
  std::vector<ZydisEncoderRequest> instructions = instructions_;
  std::vector<char> result;

  // Do two passes: one to figure out the lengths of our instructions, and one to actually emit
  // our relocated instructions.
  size_t instructions_length = 0;
  for (size_t i = 0; i < instructions.size(); ++i) {
    ZyanU8 encoded[ZYDIS_MAX_INSTRUCTION_LENGTH];
    ZyanUSize encoded_length = sizeof(encoded);
    ZydisEncoderEncodeInstruction(&instructions[i], encoded, &encoded_length);
    instructions_length += encoded_length;
  }

  // Make sure data is aligned.
  size_t nops_length = 0;
  if (instructions_length % 16 != 0) {
    nops_length = 16 - instructions_length % 16;
  }

  instructions_length += nops_length;

  size_t instruction_offset = 0;
  for (size_t i = 0; i < instructions.size(); ++i) {
    ZyanU8 encoded[ZYDIS_MAX_INSTRUCTION_LENGTH];
    ZyanUSize encoded_length = sizeof(encoded);

    auto relocations = relocations_.find(i);
    if (relocations != relocations_.end()) {
      // There has to be a better way of doing this...
      ZydisEncoderEncodeInstruction(&instructions[i], encoded, &encoded_length);
      size_t next_instruction = instruction_offset + encoded_length;

      for (auto [operand, offset] : relocations->second) {
        size_t relocated_offset = instructions_length - next_instruction + offset;
        instructions[i].operands[operand].mem.displacement = relocated_offset;
      }
    }

    ZydisEncoderEncodeInstruction(&instructions[i], encoded, &encoded_length);
    result.insert(result.end(), encoded, encoded + encoded_length);
    instruction_offset += encoded_length;
  }

  result.resize(result.size() + nops_length);
  ZydisEncoderNopFill(&*(result.end() - nops_length), nops_length);
  return result;
}

void Thunk::EmitData(std::vector<char>* out) {
  out->insert(out->end(), data_.begin(), data_.end());
}

void Thunk::Dump(const char* prefix) {
  std::vector<char> bytes = EmitInstructions();

  ZydisDisassembledInstruction instruction;
  ZyanUSize offset = 0;
  const char* p = bytes.data();
  while (offset < bytes.size()) {
    const char* current_instruction = p + offset;
    auto rc = ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, offset, current_instruction,
                                    bytes.size() - offset, &instruction);
    if (!ZYAN_SUCCESS(rc)) {
      printf("%s0x%016zx  <DECODING FAILED>", prefix, offset);
      offset += 4;
      continue;
    }

    printf("%s0x%016zx  %s\n", prefix, offset, instruction.text);
    offset += instruction.info.length;
  }

  printf("%s0x%016zx  [%zu bytes of data]\n", prefix, offset, data_.size());
}

}  // namespace determinize
