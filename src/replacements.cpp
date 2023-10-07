#include <determinize/replacements.h>

#include <stdio.h>

#include <string>

#include <Zydis.h>

#include <determinize/thunk.h>

#include "trampoline.h"
#include "util.h"

namespace determinize {

static ZydisRegister XmmScratch(ZydisRegister dst, ZydisRegister src) {
  if (ZYDIS_REGISTER_XMM0 != dst && ZYDIS_REGISTER_XMM0 != src) return ZYDIS_REGISTER_XMM0;
  if (ZYDIS_REGISTER_XMM1 != dst && ZYDIS_REGISTER_XMM1 != src) return ZYDIS_REGISTER_XMM1;
  return ZYDIS_REGISTER_XMM2;
}

static ZydisRegister XmmScratch(ZydisRegister dst, ZydisDecodedOperand src) {
  if (src.type == ZYDIS_OPERAND_TYPE_REGISTER) {
    return XmmScratch(dst, src.reg.value);
  } else {
    if (ZYDIS_REGISTER_XMM0 != dst) return ZYDIS_REGISTER_XMM0;
    return ZYDIS_REGISTER_XMM1;
  }
}

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

static ZydisEncoderOperand TranslateOperand(ZydisDecodedOperand dec, size_t operand_size) {
  ZydisEncoderOperand operand = {};
  operand.type = dec.type;

  switch (dec.type) {
    case ZYDIS_OPERAND_TYPE_REGISTER:
      // HACK: We need to know the instruction and operand index to properly set is4.
      // We're only handling 2-operand instructions, though, so it's always false.
      operand.reg.value = dec.reg.value;
      break;

    case ZYDIS_OPERAND_TYPE_MEMORY:
      operand.mem = {
          .base = dec.mem.base,
          .index = dec.mem.index,
          .scale = dec.mem.scale,
          .displacement = dec.mem.disp.has_displacement ? dec.mem.disp.value : 0,
          .size = operand_size,
      };
      break;

    default:
      abort();
  }

  return operand;
}

template <typename Fn>
static bool GenerateXmmReplacement(Thunk* thunk, const char* name, ZydisRegister dst,
                                   ZydisDecodedOperand src, Fn fn) {
  ZydisRegister scratch = XmmScratch(dst, src);
  printf("%s(%s, %s): # scratch = %s\n", name, ZydisRegisterGetString(dst),
         FormatOperand(src).c_str(), ZydisRegisterGetString(scratch));

  thunk->Append(ZYDIS_MNEMONIC_SUB, Register(ZYDIS_REGISTER_RSP), Immediate(16));
  thunk->Append(ZYDIS_MNEMONIC_MOVUPS, Memory(ZYDIS_REGISTER_RSP, 0, 16), Register(scratch));

  if (!fn(thunk, dst, src, scratch)) {
    return false;
  }

  thunk->Append(ZYDIS_MNEMONIC_MOVUPS, Register(scratch), Memory(ZYDIS_REGISTER_RSP, 0, 16));
  thunk->Append(ZYDIS_MNEMONIC_ADD, Register(ZYDIS_REGISTER_RSP), Immediate(16));
  thunk->Dump("  ");

  return true;
}

bool GenerateRcpps(Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src) {
  return GenerateXmmReplacement(
      thunk, "rcpps", dst, src,
      [](Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src, ZydisRegister scratch) {
        float ones[4] = {1.0, 1.0, 1.0, 1.0};
        std::vector<char> ones_data;
        ones_data.resize(sizeof(ones));
        memcpy(ones_data.data(), &ones, sizeof(ones));
        thunk->Append(ZYDIS_MNEMONIC_MOVAPS, Register(scratch), TranslateOperand(src, 16));
        thunk->Append(ZYDIS_MNEMONIC_MOVAPS, Register(dst), RelocatedData(std::move(ones_data)));
        thunk->Append(ZYDIS_MNEMONIC_DIVPS, Register(dst), Register(scratch));
        return true;
      });
}

bool GenerateRsqrtps(Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src) {
  return GenerateXmmReplacement(
      thunk, "rsqrtps", dst, src,
      [](Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src, ZydisRegister scratch) {
        float ones[4] = {1.0, 1.0, 1.0, 1.0};
        std::vector<char> ones_data;
        ones_data.resize(sizeof(ones));
        memcpy(ones_data.data(), &ones, sizeof(ones));
        thunk->Append(ZYDIS_MNEMONIC_SQRTPS, Register(scratch), TranslateOperand(src, 16));
        thunk->Append(ZYDIS_MNEMONIC_MOVAPS, Register(dst), RelocatedData(std::move(ones_data)));
        thunk->Append(ZYDIS_MNEMONIC_DIVPS, Register(dst), Register(scratch));
        return true;
      });
}

bool GenerateRsqrtss(Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src) {
  return GenerateXmmReplacement(
      thunk, "rsqrtss", dst, src,
      [](Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src, ZydisRegister scratch) {
        double one = 1.0;
        std::vector<char> one_data;
        one_data.resize(sizeof(one));
        memcpy(one_data.data(), &one, sizeof(one));

        thunk->Append(ZYDIS_MNEMONIC_MOVAPS, Register(scratch), Register(dst));
        thunk->Append(ZYDIS_MNEMONIC_SQRTSS, Register(scratch), TranslateOperand(src, 4));
        thunk->Append(ZYDIS_MNEMONIC_CVTSD2SS, Register(dst), RelocatedData(std::move(one_data)));
        thunk->Append(ZYDIS_MNEMONIC_DIVSS, Register(dst), Register(scratch));

        return true;
      });
}

}  // namespace determinize
