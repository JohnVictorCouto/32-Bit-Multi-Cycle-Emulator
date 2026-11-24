#pragma once

#include "Definitions.hpp"

namespace riscv {

class ALU {
public:
    static u32 execute(AluOperation operation, u32 operand_a, u32 operand_b);
};

} // namespace riscv
