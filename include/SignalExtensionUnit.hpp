#pragma once

#include "Definitions.hpp"

namespace riscv {

struct Immediates {
    u32 i;
    u32 s;
    u32 b;
    u32 u;
    u32 j;
};

class SignalExtensionUnit {
public:
    static Immediates decode(u32 instruction);
};

} // namespace riscv
