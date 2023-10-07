#pragma once

#include <Zydis.h>

#include "thunk.h"

namespace determinize {

bool GenerateRcpps(Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src);
bool GenerateRsqrtps(Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src);
bool GenerateRsqrtss(Thunk* thunk, ZydisRegister dst, ZydisDecodedOperand src);

}  // namespace determinize
