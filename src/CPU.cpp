#include "CPU.hpp"

#include "ALU.hpp"
#include "BranchUnit.hpp"
#include "Decoder.hpp"
#include "SignalExtensionUnit.hpp"

#include <stdexcept>

namespace riscv {
namespace {

constexpr u32 kCauseInstructionMisaligned = 0;
constexpr u32 kCauseInstructionAccess = 1;
constexpr u32 kCauseIllegalInstruction = 2;
constexpr u32 kCauseBreakpoint = 3;
constexpr u32 kCauseLoadMisaligned = 4;
constexpr u32 kCauseLoadAccess = 5;
constexpr u32 kCauseStoreMisaligned = 6;
constexpr u32 kCauseStoreAccess = 7;
constexpr u32 kCauseMachineEcall = 11;
constexpr u32 kCauseSoftwareInterrupt = 0x80000003u;
constexpr u32 kCauseTimerInterrupt = 0x80000007u;
constexpr u32 kCauseExternalInterrupt = 0x8000000bu;

u32 opcode(u32 instruction) { return instruction & 0x7f; }
u32 funct3(u32 instruction) { return (instruction >> 12) & 7; }
u32 funct7(u32 instruction) { return instruction >> 25; }
unsigned rd(u32 instruction) { return (instruction >> 7) & 0x1f; }
unsigned rs1(u32 instruction) { return (instruction >> 15) & 0x1f; }
unsigned rs2(u32 instruction) { return (instruction >> 20) & 0x1f; }

bool valid_data_range(u32 address, std::size_t size, std::size_t capacity) {
    return static_cast<u64>(address) + size <= capacity;
}

} // namespace

CPU::CPU(const CPUConfig& config)
    : config_(config),
      instruction_memory_(config.instruction_words),
      data_memory_(config.data_bytes) {
    reset();
}

void CPU::reset() {
    state_ = State::Fetch;
    pc_ = config_.reset_pc;
    instruction_ = 0;
    instruction_pc_ = config_.reset_pc;
    operand_a_ = 0;
    operand_b_ = 0;
    address_ = 0;
    writeback_data_ = 0;
    writeback_register_ = 0;
    writeback_valid_ = false;
    pending_cause_ = 0;
    pending_tval_ = 0;
    pending_epc_ = 0;
    last_trap_cause_ = 0;
    trap_taken_ = false;
    mstatus_ = 0x00001800;
    mie_ = 0;
    mtvec_ = 0;
    mscratch_ = 0;
    mepc_ = 0;
    mcause_ = 0;
    mtval_ = 0;
    mcycle_ = 0;
    minstret_ = 0;
    registers_.reset();
}

u32 CPU::pc() const { return pc_; }
u64 CPU::cycles() const { return mcycle_; }
u64 CPU::retired_instructions() const { return minstret_; }
u32 CPU::last_trap_cause() const { return last_trap_cause_; }
bool CPU::trap_taken() const { return trap_taken_; }
const RegisterFile& CPU::registers() const { return registers_; }
const InstructionMemory& CPU::instruction_memory() const { return instruction_memory_; }
InstructionMemory& CPU::instruction_memory() { return instruction_memory_; }
const DataMemory& CPU::data_memory() const { return data_memory_; }
DataMemory& CPU::data_memory() { return data_memory_; }

u32 CPU::interrupt_pending(const Interrupts& interrupts) const {
    return (interrupts.external ? 1u << 11 : 0) |
           (interrupts.timer ? 1u << 7 : 0) |
           (interrupts.software ? 1u << 3 : 0);
}

bool CPU::csr_exists(u32 address) {
    switch (address) {
    case kCsrMstatus: case kCsrMisa: case kCsrMie: case kCsrMtvec:
    case kCsrMscratch: case kCsrMepc: case kCsrMcause: case kCsrMtval:
    case kCsrMip: case kCsrMcycle: case kCsrMinstret: case kCsrMcycleh:
    case kCsrMinstreth: case kCsrCycle: case kCsrTime: case kCsrInstret:
    case kCsrCycleh: case kCsrTimeh: case kCsrInstreth: case kCsrMvendorid:
    case kCsrMarchid: case kCsrMimpid: case kCsrMhartid:
        return true;
    default:
        return false;
    }
}

bool CPU::csr_is_writable(u32 address) {
    switch (address) {
    case kCsrMstatus: case kCsrMie: case kCsrMtvec: case kCsrMscratch:
    case kCsrMepc: case kCsrMcause: case kCsrMtval: case kCsrMcycle:
    case kCsrMinstret: case kCsrMcycleh: case kCsrMinstreth:
        return true;
    default:
        return false;
    }
}

u32 CPU::read_csr(u32 address, const Interrupts& interrupts,
                  u64 cycle_value, u64 instret_value) const {
    switch (address) {
    case kCsrMstatus: return mstatus_;
    case kCsrMisa: return 0x40001102;
    case kCsrMie: return mie_;
    case kCsrMtvec: return mtvec_;
    case kCsrMscratch: return mscratch_;
    case kCsrMepc: return mepc_;
    case kCsrMcause: return mcause_;
    case kCsrMtval: return mtval_;
    case kCsrMip: return interrupt_pending(interrupts);
    case kCsrMcycle: case kCsrCycle: return static_cast<u32>(cycle_value);
    case kCsrMcycleh: case kCsrCycleh: return static_cast<u32>(cycle_value >> 32);
    case kCsrMinstret: case kCsrInstret: return static_cast<u32>(instret_value);
    case kCsrMinstreth: case kCsrInstreth: return static_cast<u32>(instret_value >> 32);
    case kCsrTime: case kCsrTimeh: case kCsrMvendorid: case kCsrMarchid:
    case kCsrMimpid: case kCsrMhartid:
        return 0;
    default:
        return 0;
    }
}

u32 CPU::csr(u32 address, const Interrupts& interrupts) const {
    if (!csr_exists(address)) throw std::invalid_argument("unimplemented CSR");
    return read_csr(address, interrupts, mcycle_, minstret_);
}

void CPU::write_csr(u32 address, u32 value) {
    switch (address) {
    case kCsrMstatus:
        mstatus_ = (value & 0x00000088u) | 0x00001800u;
        break;
    case kCsrMie:
        mie_ = value & 0x00000888u;
        break;
    case kCsrMtvec:
        mtvec_ = (value & 0xfffffffcu) | ((value & 3) == 1 ? 1u : 0u);
        break;
    case kCsrMscratch: mscratch_ = value; break;
    case kCsrMepc: mepc_ = value & 0xfffffffcu; break;
    case kCsrMcause: mcause_ = value; break;
    case kCsrMtval: mtval_ = value; break;
    case kCsrMcycle:
        mcycle_ = (mcycle_ & 0xffffffff00000000ull) | value;
        break;
    case kCsrMcycleh:
        mcycle_ = (mcycle_ & 0xffffffffull) | (static_cast<u64>(value) << 32);
        break;
    case kCsrMinstret:
        minstret_ = (minstret_ & 0xffffffff00000000ull) | value;
        break;
    case kCsrMinstreth:
        minstret_ = (minstret_ & 0xffffffffull) | (static_cast<u64>(value) << 32);
        break;
    default:
        break;
    }
}

void CPU::request_trap(u32 cause, u32 value, u32 exception_pc) {
    pending_cause_ = cause;
    pending_tval_ = value;
    pending_epc_ = exception_pc;
    state_ = State::Trap;
}

void CPU::begin_writeback(unsigned destination, u32 value) {
    writeback_register_ = destination;
    writeback_data_ = value;
    writeback_valid_ = true;
    state_ = State::Writeback;
}

AluOperation CPU::alu_operation(bool immediate) const {
    const u32 f3 = funct3(instruction_);
    const u32 f7 = funct7(instruction_);
    const u32 imm12 = instruction_ >> 20;

    if (!immediate && f7 == 1) {
        switch (f3) {
        case 0: return AluOperation::Multiply;
        case 1: return AluOperation::MultiplyHigh;
        case 2: return AluOperation::MultiplyHighSignedUnsigned;
        case 3: return AluOperation::MultiplyHighUnsigned;
        case 4: return AluOperation::Divide;
        case 5: return AluOperation::DivideUnsigned;
        case 6: return AluOperation::Remainder;
        case 7: return AluOperation::RemainderUnsigned;
        }
    }
    if (!immediate && f7 == 0x10) {
        if (f3 == 2) return AluOperation::Shift1Add;
        if (f3 == 4) return AluOperation::Shift2Add;
        if (f3 == 6) return AluOperation::Shift3Add;
    }
    if (!immediate && f7 == 0x20 &&
        (f3 == 4 || f3 == 6 || f3 == 7)) {
        if (f3 == 4) return AluOperation::Xnor;
        if (f3 == 6) return AluOperation::OrNot;
        return AluOperation::AndNot;
    }
    if (!immediate && f7 == 5 && (f3 & 4)) {
        if (f3 == 4) return AluOperation::Minimum;
        if (f3 == 5) return AluOperation::MinimumUnsigned;
        if (f3 == 6) return AluOperation::Maximum;
        return AluOperation::MaximumUnsigned;
    }
    if (!immediate && f7 == 0x30)
        return f3 == 1 ? AluOperation::RotateLeft : AluOperation::RotateRight;
    if (f7 == 0x24)
        return f3 == 1 ? AluOperation::BitClear : AluOperation::BitExtract;
    if (!immediate && f7 == 0x34) return AluOperation::BitInvert;
    if (f7 == 4) {
        if (!immediate && f3 == 4) return AluOperation::ZeroExtendHalf;
        if (f3 == 1) return AluOperation::BitSet;
    }
    if (immediate) {
        switch (imm12) {
        case 0x600: return AluOperation::CountLeadingZeros;
        case 0x601: return AluOperation::CountTrailingZeros;
        case 0x602: return AluOperation::PopulationCount;
        case 0x604: return AluOperation::SignExtendByte;
        case 0x605: return AluOperation::SignExtendHalf;
        case 0x287: return AluOperation::OrCombineBytes;
        case 0x698: return AluOperation::ReverseBytes;
        default: break;
        }
        if (f7 == 0x04 && f3 == 1) return AluOperation::BitSet;
        if (f7 == 0x24)
            return f3 == 1 ? AluOperation::BitClear : AluOperation::BitExtract;
        if (f7 == 0x34 && f3 == 1) return AluOperation::BitInvert;
        if (f7 == 0x30 && f3 == 5) return AluOperation::RotateRight;
    }

    switch (f3) {
    case 0: return !immediate && (instruction_ & (1u << 30))
                         ? AluOperation::Subtract : AluOperation::Add;
    case 1: return AluOperation::ShiftLeft;
    case 2: return AluOperation::SetLessThan;
    case 3: return AluOperation::SetLessThanUnsigned;
    case 4: return AluOperation::Xor;
    case 5: return (instruction_ & (1u << 30))
                         ? AluOperation::ShiftRightArithmetic : AluOperation::ShiftRight;
    case 6: return AluOperation::Or;
    case 7: return AluOperation::And;
    default: return AluOperation::Nop;
    }
}

u32 CPU::alu_result(AluOperation operation, u32 a, u32 b) const {
    return ALU::execute(operation, a, b);
}

void CPU::tick(const Interrupts& interrupts) {
    const u64 previous_cycle = mcycle_;
    const u64 previous_instret = minstret_;
    ++mcycle_;
    trap_taken_ = false;

    switch (state_) {
    case State::Fetch: {
        const u32 pending = interrupt_pending(interrupts);
        if ((mstatus_ & 8) && (mie_ & pending)) {
            if (mie_ & pending & 0x800)
                request_trap(kCauseExternalInterrupt, 0, pc_);
            else if (mie_ & pending & 0x80)
                request_trap(kCauseTimerInterrupt, 0, pc_);
            else
                request_trap(kCauseSoftwareInterrupt, 0, pc_);
        } else if (pc_ & 3) {
            request_trap(kCauseInstructionMisaligned, pc_, pc_);
        } else if ((pc_ >> 2) >= instruction_memory_.size()) {
            request_trap(kCauseInstructionAccess, pc_, pc_);
        } else {
            instruction_ = instruction_memory_.read_word(pc_ >> 2);
            instruction_pc_ = pc_;
            pc_ += 4;
            state_ = State::Decode;
        }
        break;
    }

    case State::Decode: {
        const DecodedInstruction decoded = Decoder::decode(instruction_);
        operand_a_ = registers_.read(rs1(instruction_));
        operand_b_ = registers_.read(rs2(instruction_));
        if (!decoded.legal) {
            request_trap(kCauseIllegalInstruction, instruction_, instruction_pc_);
            break;
        }
        switch (decoded.kind) {
        case InstructionKind::RType: state_ = State::ExecuteR; break;
        case InstructionKind::IType: state_ = State::ExecuteI; break;
        case InstructionKind::UType: state_ = State::ExecuteU; break;
        case InstructionKind::Load:
        case InstructionKind::Store: state_ = State::Address; break;
        case InstructionKind::Branch: state_ = State::Branch; break;
        case InstructionKind::Jal:
        case InstructionKind::Jalr: state_ = State::Jump; break;
        case InstructionKind::Fence:
            ++minstret_;
            state_ = State::Fetch;
            break;
        case InstructionKind::Ecall:
            request_trap(kCauseMachineEcall, 0, instruction_pc_);
            break;
        case InstructionKind::Ebreak:
            request_trap(kCauseBreakpoint, 0, instruction_pc_);
            break;
        case InstructionKind::Mret: state_ = State::Mret; break;
        case InstructionKind::Csr: state_ = State::Csr; break;
        case InstructionKind::Illegal:
            request_trap(kCauseIllegalInstruction, instruction_, instruction_pc_);
            break;
        }
        break;
    }

    case State::ExecuteR:
        begin_writeback(rd(instruction_),
                        alu_result(alu_operation(false), operand_a_, operand_b_));
        break;

    case State::ExecuteI: {
        const Immediates immediates = SignalExtensionUnit::decode(instruction_);
        begin_writeback(rd(instruction_),
                        alu_result(alu_operation(true), operand_a_, immediates.i));
        break;
    }

    case State::ExecuteU: {
        const Immediates immediates = SignalExtensionUnit::decode(instruction_);
        const u32 result = opcode(instruction_) == kOpcodeLui
            ? immediates.u : instruction_pc_ + immediates.u;
        begin_writeback(rd(instruction_), result);
        break;
    }

    case State::Address: {
        const Immediates immediates = SignalExtensionUnit::decode(instruction_);
        address_ = operand_a_ + (opcode(instruction_) == kOpcodeLoad
                                     ? immediates.i : immediates.s);
        state_ = opcode(instruction_) == kOpcodeLoad ? State::Load : State::Store;
        break;
    }

    case State::Load: {
        const u32 f3 = funct3(instruction_);
        std::size_t access_size = 0;
        if (f3 == 0 || f3 == 4) access_size = 1;
        else if (f3 == 1 || f3 == 5) access_size = 2;
        else if (f3 == 2) access_size = 4;
        if (access_size == 0) {
            request_trap(kCauseIllegalInstruction, instruction_, instruction_pc_);
            break;
        }
        if ((access_size == 2 && (address_ & 1)) ||
            (access_size == 4 && (address_ & 3))) {
            request_trap(kCauseLoadMisaligned, address_, instruction_pc_);
            break;
        }
        if (!valid_data_range(address_, access_size, data_memory_.size())) {
            request_trap(kCauseLoadAccess, address_, instruction_pc_);
            break;
        }
        u32 value = data_memory_.read_word_aligned(address_) >> ((address_ & 3) * 8);
        if (access_size == 1) {
            value &= 0xff;
            if (f3 == 0) value = sign_extend(value, 8);
        } else if (access_size == 2) {
            value &= 0xffff;
            if (f3 == 1) value = sign_extend(value, 16);
        }
        begin_writeback(rd(instruction_), value);
        break;
    }

    case State::Store: {
        const u32 f3 = funct3(instruction_);
        const std::size_t access_size = f3 == 0 ? 1 : (f3 == 1 ? 2 : (f3 == 2 ? 4 : 0));
        if (access_size == 0) {
            request_trap(kCauseIllegalInstruction, instruction_, instruction_pc_);
            break;
        }
        if ((access_size == 2 && (address_ & 1)) ||
            (access_size == 4 && (address_ & 3))) {
            request_trap(kCauseStoreMisaligned, address_, instruction_pc_);
            break;
        }
        if (!valid_data_range(address_, access_size, data_memory_.size())) {
            request_trap(kCauseStoreAccess, address_, instruction_pc_);
            break;
        }
        for (std::size_t byte = 0; byte < access_size; ++byte)
            data_memory_.write_byte(address_ + byte,
                static_cast<u8>(operand_b_ >> (byte * 8)));
        ++minstret_;
        state_ = State::Fetch;
        break;
    }

    case State::Branch: {
        if (BranchUnit::taken(funct3(instruction_), operand_a_, operand_b_)) {
            const Immediates immediates = SignalExtensionUnit::decode(instruction_);
            const u32 target = instruction_pc_ + immediates.b;
            if (target & 3) {
                request_trap(kCauseInstructionMisaligned, target, instruction_pc_);
                break;
            }
            pc_ = target;
        }
        ++minstret_;
        state_ = State::Fetch;
        break;
    }

    case State::Jump: {
        const Immediates immediates = SignalExtensionUnit::decode(instruction_);
        u32 target = opcode(instruction_) == kOpcodeJal
            ? instruction_pc_ + immediates.j : operand_a_ + immediates.i;
        if (opcode(instruction_) == kOpcodeJalr) target &= 0xfffffffeu;
        if (target & 3) {
            request_trap(kCauseInstructionMisaligned, target, instruction_pc_);
            break;
        }
        pc_ = target;
        begin_writeback(rd(instruction_), instruction_pc_ + 4);
        break;
    }

    case State::Csr: {
        const u32 address = instruction_ >> 20;
        if (!csr_exists(address)) {
            request_trap(kCauseIllegalInstruction, instruction_, instruction_pc_);
            break;
        }
        const u32 old_value = read_csr(address, interrupts, previous_cycle, previous_instret);
        const u32 f3 = funct3(instruction_);
        const bool immediate = f3 >= 5;
        const u32 source = immediate ? rs1(instruction_) : registers_.read(rs1(instruction_));
        u32 new_value = old_value;
        bool write_requested = false;
        switch (f3) {
        case 1: case 5:
            new_value = source;
            write_requested = true;
            break;
        case 2: case 6:
            new_value = old_value | source;
            write_requested = rs1(instruction_) != 0;
            break;
        case 3: case 7:
            new_value = old_value & ~source;
            write_requested = rs1(instruction_) != 0;
            break;
        default:
            break;
        }
        if (write_requested && !csr_is_writable(address)) {
            request_trap(kCauseIllegalInstruction, instruction_, instruction_pc_);
            break;
        }
        if (write_requested) write_csr(address, new_value);
        begin_writeback(rd(instruction_), old_value);
        break;
    }

    case State::Writeback:
        if (writeback_valid_) registers_.write(writeback_register_, writeback_data_);
        writeback_valid_ = false;
        ++minstret_;
        state_ = State::Fetch;
        break;

    case State::Trap: {
        mepc_ = pending_epc_ & 0xfffffffcu;
        mcause_ = pending_cause_;
        mtval_ = pending_tval_;
        mstatus_ = (mstatus_ & ~0x00001888u) | 0x00001880u;
        if ((pending_cause_ & 0x80000000u) && (mtvec_ & 3) == 1)
            pc_ = (mtvec_ & 0xfffffffcu) + ((pending_cause_ & 0x7fffffffu) << 2);
        else
            pc_ = mtvec_ & 0xfffffffcu;
        last_trap_cause_ = pending_cause_;
        trap_taken_ = true;
        state_ = State::Fetch;
        break;
    }

    case State::Mret:
        pc_ = mepc_;
        mstatus_ = (mstatus_ & ~0x00001888u) |
                   ((mstatus_ & 0x80) ? 0x08u : 0) | 0x00001880u;
        ++minstret_;
        state_ = State::Fetch;
        break;
    }
}

} // namespace riscv
