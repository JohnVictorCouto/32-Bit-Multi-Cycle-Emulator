#include "ALU.hpp"

#include <limits>

namespace riscv {
namespace {

std::int64_t signed_value(u32 value) {
    return (value & 0x80000000u)
        ? static_cast<std::int64_t>(value) - 0x100000000ll
        : static_cast<std::int64_t>(value);
}

u32 arithmetic_shift_right(u32 value, unsigned shift) {
    if (shift == 0) return value;
    u32 result = value >> shift;
    if (value & 0x80000000u)
        result |= 0xffffffffu << (32 - shift);
    return result;
}

} // namespace

u32 ALU::execute(AluOperation operation, u32 a, u32 b) {
    const unsigned shift = b & 31;
    switch (operation) {
    case AluOperation::Add: return a + b;
    case AluOperation::Subtract: return a - b;
    case AluOperation::And: return a & b;
    case AluOperation::Or: return a | b;
    case AluOperation::Xor: return a ^ b;
    case AluOperation::SetLessThan: return signed_value(a) < signed_value(b);
    case AluOperation::SetLessThanUnsigned: return a < b;
    case AluOperation::ShiftLeft: return a << shift;
    case AluOperation::ShiftRight: return a >> shift;
    case AluOperation::ShiftRightArithmetic: return arithmetic_shift_right(a, shift);
    case AluOperation::Multiply: return static_cast<u32>(static_cast<u64>(a) * b);
    case AluOperation::MultiplyHigh:
        return static_cast<u32>((static_cast<u64>(signed_value(a) * signed_value(b))) >> 32);
    case AluOperation::MultiplyHighSignedUnsigned:
        return static_cast<u32>((static_cast<u64>(signed_value(a) *
            static_cast<std::int64_t>(b))) >> 32);
    case AluOperation::MultiplyHighUnsigned:
        return static_cast<u32>((static_cast<u64>(a) * b) >> 32);
    case AluOperation::Divide:
        if (b == 0) return 0xffffffffu;
        if (a == 0x80000000u && b == 0xffffffffu) return a;
        return static_cast<u32>(signed_value(a) / signed_value(b));
    case AluOperation::DivideUnsigned:
        return b == 0 ? 0xffffffffu : a / b;
    case AluOperation::Remainder:
        if (b == 0) return a;
        if (a == 0x80000000u && b == 0xffffffffu) return 0;
        return static_cast<u32>(signed_value(a) % signed_value(b));
    case AluOperation::RemainderUnsigned:
        return b == 0 ? a : a % b;
    case AluOperation::PassB: return b;
    case AluOperation::AndNot: return a & ~b;
    case AluOperation::OrNot: return a | ~b;
    case AluOperation::Xnor: return ~(a ^ b);
    case AluOperation::CountLeadingZeros:
        for (int bit = 31; bit >= 0; --bit)
            if ((a >> bit) & 1) return static_cast<u32>(31 - bit);
        return 32;
    case AluOperation::CountTrailingZeros:
        for (unsigned bit = 0; bit < 32; ++bit)
            if ((a >> bit) & 1) return bit;
        return 32;
    case AluOperation::PopulationCount: {
        u32 count = 0;
        for (unsigned bit = 0; bit < 32; ++bit) count += (a >> bit) & 1;
        return count;
    }
    case AluOperation::Minimum: return signed_value(a) < signed_value(b) ? a : b;
    case AluOperation::MinimumUnsigned: return a < b ? a : b;
    case AluOperation::Maximum: return signed_value(a) > signed_value(b) ? a : b;
    case AluOperation::MaximumUnsigned: return a > b ? a : b;
    case AluOperation::RotateLeft:
        return shift == 0 ? a : (a << shift) | (a >> (32 - shift));
    case AluOperation::RotateRight:
        return shift == 0 ? a : (a >> shift) | (a << (32 - shift));
    case AluOperation::OrCombineBytes: {
        u32 result = 0;
        for (unsigned byte = 0; byte < 4; ++byte) {
            const u32 part = (a >> (byte * 8)) & 0xff;
            if (part != 0) result |= 0xffu << (byte * 8);
        }
        return result;
    }
    case AluOperation::ReverseBytes:
        return ((a & 0x000000ffu) << 24) | ((a & 0x0000ff00u) << 8) |
               ((a & 0x00ff0000u) >> 8) | ((a & 0xff000000u) >> 24);
    case AluOperation::SignExtendByte:
        return (a & 0x80) ? (a | 0xffffff00u) : (a & 0xff);
    case AluOperation::SignExtendHalf:
        return (a & 0x8000) ? (a | 0xffff0000u) : (a & 0xffff);
    case AluOperation::ZeroExtendHalf: return a & 0xffff;
    case AluOperation::BitClear: return a & ~(u32{1} << shift);
    case AluOperation::BitExtract: return (a >> shift) & 1;
    case AluOperation::BitInvert: return a ^ (u32{1} << shift);
    case AluOperation::BitSet: return a | (u32{1} << shift);
    case AluOperation::Shift1Add: return (a << 1) + b;
    case AluOperation::Shift2Add: return (a << 2) + b;
    case AluOperation::Shift3Add: return (a << 3) + b;
    case AluOperation::Nop: return 0;
    }
    return 0;
}

} // namespace riscv
