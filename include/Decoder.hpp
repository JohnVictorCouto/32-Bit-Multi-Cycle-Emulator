#pragma once

#include "Definitions.hpp"

namespace riscv {

enum class InstructionKind {
    Illegal, RType, IType, UType, Load, Store, Branch, Jal, Jalr,
    Fence, Csr, Ecall, Ebreak, Mret
};

struct DecodedInstruction {
    InstructionKind kind;
    bool legal;
};

class Decoder {
public:
    static DecodedInstruction decode(u32 instruction);
};

} // namespace riscv
