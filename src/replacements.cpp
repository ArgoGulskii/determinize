#include <determinize/replacements.h>

#include <stdio.h>

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

template <typename Fn>
static void* GenerateXmmReplacement(const char* name, ZydisRegister dst, ZydisRegister src, Fn fn) {
  Thunk thunk;
  ZydisRegister scratch = XmmScratch(dst, src);
  printf("%s(%s, %s): # scratch = %s\n", name, ZydisRegisterGetString(dst),
         ZydisRegisterGetString(src), ZydisRegisterGetString(scratch));

  thunk.Append(ZYDIS_MNEMONIC_SUB, Register(ZYDIS_REGISTER_RSP), Immediate(16));
  thunk.Append(ZYDIS_MNEMONIC_MOVUPS, Memory(ZYDIS_REGISTER_RSP, 0, 16), Register(scratch));

  fn(thunk, dst, src, scratch);

  thunk.Append(ZYDIS_MNEMONIC_MOVUPS, Register(scratch), Memory(ZYDIS_REGISTER_RSP, 0, 16));
  thunk.Append(ZYDIS_MNEMONIC_ADD, Register(ZYDIS_REGISTER_RSP), Immediate(16));
  thunk.Append(ZYDIS_MNEMONIC_RET);
  thunk.Dump("  ");

  std::vector<char> bytes = thunk.Emit();
  void* page = MapRandomPage();
  memcpy(page, bytes.data(), bytes.size());
  return page;
}

static void* GenerateRcpps(ZydisRegister dst, ZydisRegister src) {
  return GenerateXmmReplacement(
      "rcpps", dst, src,
      [](Thunk& thunk, ZydisRegister dst, ZydisRegister src, ZydisRegister scratch) {
        float ones[4] = {1.0, 1.0, 1.0, 1.0};
        std::vector<char> ones_data;
        ones_data.resize(sizeof(ones));
        memcpy(ones_data.data(), &ones, sizeof(ones));
        thunk.Append(ZYDIS_MNEMONIC_MOVAPS, Register(scratch), Register(src));
        thunk.Append(ZYDIS_MNEMONIC_MOVAPS, Register(dst), RelocatedData(std::move(ones_data)));
        thunk.Append(ZYDIS_MNEMONIC_DIVPS, Register(dst), Register(scratch));
      });
}

static void* GenerateRsqrtps(ZydisRegister dst, ZydisRegister src) {
  return GenerateXmmReplacement(
      "rsqrtps", dst, src,
      [](Thunk& thunk, ZydisRegister dst, ZydisRegister src, ZydisRegister scratch) {
        float ones[4] = {1.0, 1.0, 1.0, 1.0};
        std::vector<char> ones_data;
        ones_data.resize(sizeof(ones));
        memcpy(ones_data.data(), &ones, sizeof(ones));
        thunk.Append(ZYDIS_MNEMONIC_SQRTPS, Register(scratch), Register(src));
        thunk.Append(ZYDIS_MNEMONIC_MOVAPS, Register(dst), RelocatedData(std::move(ones_data)));
        thunk.Append(ZYDIS_MNEMONIC_DIVPS, Register(dst), Register(scratch));
      });
}

static void* GenerateRsqrtss(ZydisRegister dst, ZydisRegister src) {
  return GenerateXmmReplacement(
      "rsqrtss", dst, src,
      [](Thunk& thunk, ZydisRegister dst, ZydisRegister src, ZydisRegister scratch) {
        float one = 1.0;
        std::vector<char> one_data;
        one_data.resize(sizeof(one));
        memcpy(one_data.data(), &one, sizeof(one));

        // There doesn't seem to be a way to mov into just the bottom dword of an xmm register without AVX?
        // TODO: Is this actually true?
        ZydisRegister scratch2 = XmmScratch(dst, scratch);
        if (dst == src) {
          // If src == dst, we need another register, because movss will overwrite the entire register.
          // TODO: Is there a way to avoid this?
          thunk.Append(ZYDIS_MNEMONIC_SUB, Register(ZYDIS_REGISTER_RSP), Immediate(16));
          thunk.Append(ZYDIS_MNEMONIC_MOVUPS, Memory(ZYDIS_REGISTER_RSP, 0, 16), Register(scratch2));
          thunk.Append(ZYDIS_MNEMONIC_MOVSS, Register(scratch2), Register(src));
        }

        thunk.Append(ZYDIS_MNEMONIC_XORPS, Register(scratch), Register(scratch));
        thunk.Append(ZYDIS_MNEMONIC_MULSS, Register(dst), Register(scratch));
        thunk.Append(ZYDIS_MNEMONIC_MOVSS, Register(scratch), RelocatedData(std::move(one_data)));
        thunk.Append(ZYDIS_MNEMONIC_ADDSS, Register(dst), Register(scratch));

        if (dst == src) {
          thunk.Append(ZYDIS_MNEMONIC_SQRTSS, Register(scratch), Register(scratch2));
        } else {
          thunk.Append(ZYDIS_MNEMONIC_SQRTSS, Register(scratch), Register(src));
        }

        thunk.Append(ZYDIS_MNEMONIC_DIVSS, Register(dst), Register(scratch));

        if (dst == src) {
          thunk.Append(ZYDIS_MNEMONIC_MOVUPS, Register(scratch2), Memory(ZYDIS_REGISTER_RSP, 0, 16));
          thunk.Append(ZYDIS_MNEMONIC_ADD, Register(ZYDIS_REGISTER_RSP), Immediate(16));
        }
      });
}

// TODO: Cache these instead of mapping a page every single time.
void* GetDeterministicRcpps(ZydisRegister dst, ZydisRegister src) {
  return GenerateRcpps(dst, src);
}

void* GetDeterministicRsqrtps(ZydisRegister dst, ZydisRegister src) {
  return GenerateRsqrtps(dst, src);
}

void* GetDeterministicRsqrtss(ZydisRegister dst, ZydisRegister src) {
  return GenerateRsqrtss(dst, src);
}

}  // namespace determinize
