#pragma once

#include "Definitions.hpp"

namespace riscv {

class BranchUnit {
public:
    static bool taken(u32 funct3, u32 operand_a, u32 operand_b);
};

} // namespace riscv
