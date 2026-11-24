#pragma once

#include "DataMemory.hpp"
#include "InstructionMemory.hpp"
#include "RegisterFile.hpp"

#include <cstddef>

namespace riscv {

struct CPUConfig {
    std::size_t instruction_words = 256;
    std::size_t data_bytes = 1024;
    u32 reset_pc = 0;
};

struct Interrupts {
    bool software = false;
    bool timer = false;
    bool external = false;
};

class CPU {
public:
    explicit CPU(const CPUConfig& config = CPUConfig{});

    void reset();
    void tick(const Interrupts& interrupts = Interrupts{});

    u32 pc() const;
    u64 cycles() const;
    u64 retired_instructions() const;
    u32 last_trap_cause() const;
    bool trap_taken() const;
    const RegisterFile& registers() const;
    const InstructionMemory& instruction_memory() const;
    InstructionMemory& instruction_memory();
    const DataMemory& data_memory() const;
    DataMemory& data_memory();
    u32 csr(u32 address, const Interrupts& interrupts = Interrupts{}) const;

private:
    enum class State {
        Fetch, Decode, ExecuteR, ExecuteI, ExecuteU, Address,
        Load, Store, Branch, Jump, Csr, Writeback, Trap, Mret
    };

    CPUConfig config_;
    InstructionMemory instruction_memory_;
    DataMemory data_memory_;
    RegisterFile registers_;

    State state_;
    u32 pc_;
    u32 instruction_;
    u32 instruction_pc_;
    u32 operand_a_;
    u32 operand_b_;
    u32 address_;
    u32 writeback_data_;
    unsigned writeback_register_;
    bool writeback_valid_;

    u32 pending_cause_;
    u32 pending_tval_;
    u32 pending_epc_;
    u32 last_trap_cause_;
    bool trap_taken_;

    u32 mstatus_;
    u32 mie_;
    u32 mtvec_;
    u32 mscratch_;
    u32 mepc_;
    u32 mcause_;
    u32 mtval_;
    u64 mcycle_;
    u64 minstret_;

    void request_trap(u32 cause, u32 value, u32 exception_pc);
    void begin_writeback(unsigned destination, u32 value);
    u32 interrupt_pending(const Interrupts& interrupts) const;
    u32 read_csr(u32 address, const Interrupts& interrupts,
                 u64 cycle_value, u64 instret_value) const;
    static bool csr_exists(u32 address);
    static bool csr_is_writable(u32 address);
    void write_csr(u32 address, u32 value);
    AluOperation alu_operation(bool immediate) const;
    u32 alu_result(AluOperation operation, u32 a, u32 b) const;
};

} // namespace riscv
