#include "BranchUnit.hpp"

namespace riscv {
namespace {

std::int64_t signed_value(u32 value) {
    return (value & 0x80000000u)
        ? static_cast<std::int64_t>(value) - 0x100000000ll
        : static_cast<std::int64_t>(value);
}

} // namespace

bool BranchUnit::taken(u32 funct3, u32 operand_a, u32 operand_b) {
    switch (funct3) {
    case 0: return operand_a == operand_b;
    case 1: return operand_a != operand_b;
    case 4: return signed_value(operand_a) < signed_value(operand_b);
    case 5: return signed_value(operand_a) >= signed_value(operand_b);
    case 6: return operand_a < operand_b;
    case 7: return operand_a >= operand_b;
    default: return false;
    }
}

} // namespace riscv
