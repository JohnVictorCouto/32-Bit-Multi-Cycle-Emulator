#pragma once

#include <cstdint>

namespace riscv {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

constexpr u32 kOpcodeLui = 0x37;
constexpr u32 kOpcodeAuipc = 0x17;
constexpr u32 kOpcodeJal = 0x6f;
constexpr u32 kOpcodeJalr = 0x67;
constexpr u32 kOpcodeBranch = 0x63;
constexpr u32 kOpcodeLoad = 0x03;
constexpr u32 kOpcodeStore = 0x23;
constexpr u32 kOpcodeAlu = 0x33;
constexpr u32 kOpcodeAluImmediate = 0x13;
constexpr u32 kOpcodeFence = 0x0f;
constexpr u32 kOpcodeSystem = 0x73;

constexpr u32 kCsrMstatus = 0x300;
constexpr u32 kCsrMisa = 0x301;
constexpr u32 kCsrMie = 0x304;
constexpr u32 kCsrMtvec = 0x305;
constexpr u32 kCsrMscratch = 0x340;
constexpr u32 kCsrMepc = 0x341;
constexpr u32 kCsrMcause = 0x342;
constexpr u32 kCsrMtval = 0x343;
constexpr u32 kCsrMip = 0x344;
constexpr u32 kCsrMcycle = 0xb00;
constexpr u32 kCsrMinstret = 0xb02;
constexpr u32 kCsrMcycleh = 0xb80;
constexpr u32 kCsrMinstreth = 0xb82;
constexpr u32 kCsrCycle = 0xc00;
constexpr u32 kCsrTime = 0xc01;
constexpr u32 kCsrInstret = 0xc02;
constexpr u32 kCsrCycleh = 0xc80;
constexpr u32 kCsrTimeh = 0xc81;
constexpr u32 kCsrInstreth = 0xc82;
constexpr u32 kCsrMvendorid = 0xf11;
constexpr u32 kCsrMarchid = 0xf12;
constexpr u32 kCsrMimpid = 0xf13;
constexpr u32 kCsrMhartid = 0xf14;

enum class AluOperation {
    Nop,
    Add, Subtract, And, Or, Xor, SetLessThan, SetLessThanUnsigned,
    ShiftLeft, ShiftRight, ShiftRightArithmetic,
    Multiply, MultiplyHigh, MultiplyHighSignedUnsigned, MultiplyHighUnsigned,
    Divide, DivideUnsigned, Remainder, RemainderUnsigned, PassB,
    AndNot, OrNot, Xnor, CountLeadingZeros, CountTrailingZeros, PopulationCount,
    Minimum, MinimumUnsigned, Maximum, MaximumUnsigned, RotateLeft, RotateRight,
    OrCombineBytes, ReverseBytes, SignExtendByte, SignExtendHalf,
    ZeroExtendHalf, BitClear, BitExtract, BitInvert, BitSet,
    Shift1Add, Shift2Add, Shift3Add
};

inline u32 sign_extend(u32 value, unsigned width) {
    const u32 sign = u32{1} << (width - 1);
    return (value ^ sign) - sign;
}

} // namespace riscv
