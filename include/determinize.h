#pragma once

#include <stddef.h>
#include <string.h>

#include <unordered_map>
#include <vector>

#include <Zydis.h>

namespace determinize {

struct Register {
  explicit Register(ZydisRegister reg) : value(reg) {}
  ZydisRegister value;
};

struct Immediate {
  explicit Immediate(uint64_t x) : value(x) {}
  uint64_t value;
};

struct RelocatedData {
  explicit RelocatedData(std::vector<char> buf) : data(std::move(buf)) {}
  std::vector<char> data;
};

struct Relocation {
  size_t operand;
  size_t data_offset;
};

struct Thunk {
  void AppendOperand(ZydisEncoderRequest&) { return; }

  template <typename... Operands>
  void AppendOperand(ZydisEncoderRequest& req, ZydisEncoderOperand raw_operand,
                     Operands... operands) {
    auto& operand = req.operands[req.operand_count++];
    operand = raw_operand;
    return AppendOperand(req, operands...);
  }

  template <typename... Operands>
  void AppendOperand(ZydisEncoderRequest& req, Register reg, Operands... operands) {
    auto& operand = req.operands[req.operand_count++];
    operand.type = ZYDIS_OPERAND_TYPE_REGISTER;
    operand.reg.value = reg.value;
    return AppendOperand(req, operands...);
  }

  template <typename... Operands>
  void AppendOperand(ZydisEncoderRequest& req, Immediate imm, Operands... operands) {
    auto& operand = req.operands[req.operand_count++];
    operand.type = ZYDIS_OPERAND_TYPE_IMMEDIATE;
    operand.imm.u = imm.value;
    return AppendOperand(req, operands...);
  }

  template <typename... Operands>
  void AppendOperand(ZydisEncoderRequest& req, const RelocatedData& data, Operands... operands) {
    size_t data_offset = data_.size();
    data_.insert(data_.end(), data.data.begin(), data.data.end());

    size_t operand_index = req.operand_count++;
    auto& operand = req.operands[operand_index];

    // This data will get rewritten by relocation, but fill in some fields anyway so that the
    // encoder won't barf if we don't run relocations.
    operand.type = ZYDIS_OPERAND_TYPE_MEMORY;
    operand.mem.base = ZYDIS_REGISTER_RIP;
    operand.mem.index = ZYDIS_REGISTER_NONE;
    operand.mem.scale = 0;
    operand.mem.displacement = 0;
    operand.mem.size = data.data.size();

    relocations_[instructions_.size()].emplace_back(Relocation{
        .operand = operand_index,
        .data_offset = data_offset,
    });

    return AppendOperand(req, operands...);
  }

  template <typename... Operands>
  bool Append(ZydisMnemonic mnemonic, Operands... operands) {
    ZydisEncoderRequest req = {};
    req.machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
    req.mnemonic = mnemonic;
    req.operand_count = 0;

    AppendOperand(req, operands...);
    instructions_.push_back(req);
    return true;
  }

  void Jump(void* address, ZydisMnemonic mnemonic = ZYDIS_MNEMONIC_JMP) {
    std::vector<char> buf;
    buf.resize(sizeof(address));
    memcpy(buf.data(), &address, sizeof(address));

    Append(mnemonic, RelocatedData(buf));
  }

  void Call(void* address) { return Jump(address, ZYDIS_MNEMONIC_CALL); }

  bool Relocate(ZydisDisassembledInstruction* insn);

  std::vector<char> Emit();
  std::vector<char> EmitInstructions();
  void EmitData(std::vector<char>* out);

  void Dump(const char* prefix = "");

 private:
  std::vector<ZydisEncoderRequest> instructions_;
  std::vector<char> data_;
  std::unordered_map<size_t, std::vector<Relocation>> relocations_;
};

bool Replace(void* replacement, void* target, size_t target_length);

template <typename T, typename... Args>
static bool Replace(T (*replacement)(Args...), T (*target)(Args...), size_t target_length) {
  return Replace(reinterpret_cast<void*>(replacement), reinterpret_cast<void*>(target),
                 target_length);
}

template <typename T, typename... Args>
static bool Replace(T (*replacement)(Args..., ...), T (*target)(Args..., ...),
                    size_t target_length) {
  return Replace(reinterpret_cast<void*>(replacement), reinterpret_cast<void*>(target),
                 target_length);
}
}  // namespace determinize
