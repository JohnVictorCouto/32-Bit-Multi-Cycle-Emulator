#include "Decoder.hpp"

namespace riscv {
namespace {

u32 opcode(u32 instruction) { return instruction & 0x7f; }
u32 funct3(u32 instruction) { return (instruction >> 12) & 7; }
u32 funct7(u32 instruction) { return instruction >> 25; }

bool valid_bitmanip_r(u32 instruction) {
    const u32 f3 = funct3(instruction);
    const u32 f7 = funct7(instruction);
    switch (f7) {
    case 0x10: return f3 == 2 || f3 == 4 || f3 == 6;
    case 0x20: return f3 == 4 || f3 == 6 || f3 == 7;
    case 0x05: return (f3 & 4) != 0;
    case 0x30: return f3 == 1 || f3 == 5;
    case 0x24: return f3 == 1 || f3 == 5;
    case 0x34: return f3 == 1;
    case 0x04:
        return f3 == 1 || (f3 == 4 && ((instruction >> 20) & 0x1f) == 0);
    default: return false;
    }
}

bool valid_bitmanip_i(u32 instruction) {
    const u32 f3 = funct3(instruction);
    const u32 imm12 = instruction >> 20;
    const u32 f7 = funct7(instruction);
    if (f3 == 1) {
        return imm12 == 0x600 || imm12 == 0x601 || imm12 == 0x602 ||
               imm12 == 0x604 || imm12 == 0x605 ||
               f7 == 0x04 || f7 == 0x24 || f7 == 0x34;
    }
    if (f3 == 5) {
        return f7 == 0x30 || f7 == 0x24 ||
               imm12 == 0x287 || imm12 == 0x698;
    }
    return false;
}

} // namespace

DecodedInstruction Decoder::decode(u32 instruction) {
    const u32 op = opcode(instruction);
    const u32 f3 = funct3(instruction);
    const u32 f7 = funct7(instruction);
    switch (op) {
    case kOpcodeAlu:
        if (f7 == 0 || (f7 == 0x20 && (f3 == 0 || f3 == 5)) ||
            (f7 == 1) || valid_bitmanip_r(instruction))
            return {InstructionKind::RType, true};
        break;
    case kOpcodeAluImmediate:
        if ((f3 == 1 && f7 != 0 && !valid_bitmanip_i(instruction)) ||
            (f3 == 5 && f7 != 0 && f7 != 0x20 &&
             !valid_bitmanip_i(instruction)))
            break;
        return {InstructionKind::IType, true};
    case kOpcodeLui:
    case kOpcodeAuipc:
        return {InstructionKind::UType, true};
    case kOpcodeLoad:
        if (f3 == 0 || f3 == 1 || f3 == 2 || f3 == 4 || f3 == 5)
            return {InstructionKind::Load, true};
        break;
    case kOpcodeStore:
        if (f3 <= 2) return {InstructionKind::Store, true};
        break;
    case kOpcodeBranch:
        if (f3 == 0 || f3 == 1 || f3 == 4 || f3 == 5 || f3 == 6 || f3 == 7)
            return {InstructionKind::Branch, true};
        break;
    case kOpcodeJal:
        return {InstructionKind::Jal, true};
    case kOpcodeJalr:
        if (f3 == 0) return {InstructionKind::Jalr, true};
        break;
    case kOpcodeFence:
        if (f3 == 0 && ((instruction >> 15) & 0x1f) == 0 &&
            ((instruction >> 7) & 0x1f) == 0 &&
            (((instruction >> 28) & 0xf) == 0 ||
             (((instruction >> 28) & 0xf) == 8 &&
              ((instruction >> 20) & 0xff) == 0x33)))
            return {InstructionKind::Fence, true};
        if (f3 == 1 && ((instruction >> 20) & 0xfff) == 0 &&
            ((instruction >> 15) & 0x1f) == 0 &&
            ((instruction >> 7) & 0x1f) == 0)
            return {InstructionKind::Fence, true};
        break;
    case kOpcodeSystem:
        if (f3 == 0) {
            if (instruction == 0x00000073) return {InstructionKind::Ecall, true};
            if (instruction == 0x00100073) return {InstructionKind::Ebreak, true};
            if (instruction == 0x30200073) return {InstructionKind::Mret, true};
            break;
        }
        if (f3 == 1 || f3 == 2 || f3 == 3 ||
            f3 == 5 || f3 == 6 || f3 == 7)
            return {InstructionKind::Csr, true};
        break;
    default:
        break;
    }
    return {InstructionKind::Illegal, false};
}

} // namespace riscv
