#include "SignalExtensionUnit.hpp"

namespace riscv {

Immediates SignalExtensionUnit::decode(u32 instruction) {
    const u32 imm_i = sign_extend(instruction >> 20, 12);
    const u32 imm_s = sign_extend(((instruction >> 25) << 5) |
                                  ((instruction >> 7) & 0x1f), 12);
    const u32 imm_b = sign_extend((((instruction >> 31) & 1) << 12) |
                                  (((instruction >> 7) & 1) << 11) |
                                  (((instruction >> 25) & 0x3f) << 5) |
                                  (((instruction >> 8) & 0xf) << 1), 13);
    const u32 imm_u = instruction & 0xfffff000u;
    const u32 imm_j = sign_extend((((instruction >> 31) & 1) << 20) |
                                  (((instruction >> 12) & 0xff) << 12) |
                                  (((instruction >> 20) & 1) << 11) |
                                  (((instruction >> 21) & 0x3ff) << 1), 21);
    return {imm_i, imm_s, imm_b, imm_u, imm_j};
}

} // namespace riscv
