#pragma once

#include <Zydis.h>

namespace determinize {

void* GetDeterministicRcpps(ZydisRegister dst, ZydisRegister src);
void* GetDeterministicRsqrtps(ZydisRegister dst, ZydisRegister src);
void* GetDeterministicRsqrtss(ZydisRegister dst, ZydisRegister src);

}  // namespace determinize
