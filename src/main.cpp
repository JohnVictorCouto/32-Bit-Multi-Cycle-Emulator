#include "CPU.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace {

using riscv::Interrupts;
using riscv::u32;
using riscv::u64;

struct Options {
    std::string instruction_file;
    std::string data_file;
    std::string memory_dump_file;
    std::size_t instruction_words = 256;
    std::size_t data_bytes = 1024;
    u32 reset_pc = 0;
    u64 max_cycles = 100000;
    std::map<u64, Interrupts> interrupt_schedule;
};

u64 parse_number(const std::string& value, const std::string& option) {
    if (value.empty() || value[0] == '-')
        throw std::invalid_argument("invalid value for " + option + ": " + value);
    std::size_t parsed = 0;
    try {
        const int base = value.size() > 2 && value[0] == '0' &&
                         (value[1] == 'x' || value[1] == 'X') ? 16 : 10;
        const unsigned long long result = std::stoull(value, &parsed, base);
        if (parsed != value.size()) throw std::invalid_argument("trailing characters");
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument("invalid value for " + option + ": " + value);
    }
}

std::string value_after(int& index, int argc, char** argv, const std::string& option) {
    if (index + 1 >= argc)
        throw std::invalid_argument("missing value for " + option);
    return argv[++index];
}

void schedule_interrupt(Options& options, u64 cycle, int kind) {
    Interrupts& interrupt = options.interrupt_schedule[cycle];
    if (kind == 0) interrupt.software = true;
    else if (kind == 1) interrupt.timer = true;
    else interrupt.external = true;
}

void print_help(const char* executable) {
    std::cout
        << "RISC-V RV32IM+B emulator (cycle-stepped multicycle CPU)\n\n"
        << "Usage: " << executable << " [program.hex] [options]\n\n"
        << "Options:\n"
        << "  --imem FILE                 instruction image (32-bit hex words)\n"
        << "  --dmem FILE                 data image (8-bit hex bytes)\n"
        << "  --imem-words N              instruction memory words (default 256)\n"
        << "  --dmem-bytes N              data memory bytes (default 1024)\n"
        << "  --reset-pc N                reset address (decimal or 0x-prefixed)\n"
        << "  --max-cycles N              stop after N simulated clock cycles\n"
        << "  --dump-memory FILE          write data memory as hex bytes\n"
        << "  --irq-software-at N         pulse software interrupt at cycle N\n"
        << "  --irq-timer-at N            pulse timer interrupt at cycle N\n"
        << "  --irq-external-at N         pulse external interrupt at cycle N\n"
        << "  --help                      show this help\n";
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            print_help(argv[0]);
            std::exit(0);
        } else if (argument == "--imem") {
            options.instruction_file = value_after(i, argc, argv, argument);
        } else if (argument == "--dmem") {
            options.data_file = value_after(i, argc, argv, argument);
        } else if (argument == "--imem-words") {
            const u64 value = parse_number(value_after(i, argc, argv, argument), argument);
            if (value == 0 ||
                value > std::numeric_limits<std::size_t>::max() / sizeof(u32))
                throw std::invalid_argument("instruction memory size is out of range");
            options.instruction_words = static_cast<std::size_t>(value);
        } else if (argument == "--dmem-bytes") {
            const u64 value = parse_number(value_after(i, argc, argv, argument), argument);
            if (value == 0 ||
                value > std::numeric_limits<std::size_t>::max() / sizeof(u32))
                throw std::invalid_argument("data memory size is out of range");
            options.data_bytes = static_cast<std::size_t>(value);
        } else if (argument == "--reset-pc") {
            const u64 value = parse_number(value_after(i, argc, argv, argument), argument);
            if (value > 0xffffffffull) throw std::invalid_argument("reset PC exceeds 32 bits");
            options.reset_pc = static_cast<u32>(value);
        } else if (argument == "--max-cycles") {
            options.max_cycles = parse_number(value_after(i, argc, argv, argument), argument);
            if (options.max_cycles == 0)
                throw std::invalid_argument("--max-cycles must be greater than zero");
        } else if (argument == "--dump-memory") {
            options.memory_dump_file = value_after(i, argc, argv, argument);
        } else if (argument == "--irq-software-at" ||
                   argument == "--irq-timer-at" ||
                   argument == "--irq-external-at") {
            const u64 cycle = parse_number(value_after(i, argc, argv, argument), argument);
            const int kind = argument == "--irq-software-at" ? 0 :
                             (argument == "--irq-timer-at" ? 1 : 2);
            schedule_interrupt(options, cycle, kind);
        } else if (!argument.empty() && argument[0] != '-') {
            if (!options.instruction_file.empty())
                throw std::invalid_argument("only one instruction image may be specified");
            options.instruction_file = argument;
        } else {
            throw std::invalid_argument("unknown option: " + argument);
        }
    }
    if (options.instruction_file.empty())
        throw std::invalid_argument("provide a program image with --imem FILE or as the first argument");
    return options;
}

void dump_memory(const std::string& path, const riscv::DataMemory& memory) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot create memory dump: " + path);
    output << std::hex << std::setfill('0');
    for (riscv::u8 byte : memory.bytes())
        output << std::setw(2) << static_cast<unsigned>(byte) << '\n';
    if (!output) throw std::runtime_error("failed writing memory dump: " + path);
}

void print_state(const riscv::CPU& cpu) {
    std::cout << std::hex << std::setfill('0')
              << "PC=0x" << std::setw(8) << cpu.pc()
              << "  cycles=" << std::dec << cpu.cycles()
              << "  instret=" << cpu.retired_instructions()
              << "  last_trap=0x" << std::hex << std::setw(8)
              << cpu.last_trap_cause() << '\n';
    const auto& registers = cpu.registers().registers();
    for (unsigned row = 0; row < 8; ++row) {
        for (unsigned column = 0; column < 4; ++column) {
            const unsigned index = row * 4 + column;
            std::cout << 'x' << std::dec << std::setw(2) << std::setfill('0')
                      << index << "=0x" << std::hex << std::setfill('0')
                      << std::setw(8) << registers[index]
                      << (column == 3 ? '\n' : ' ');
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);
        riscv::CPUConfig config;
        config.instruction_words = options.instruction_words;
        config.data_bytes = options.data_bytes;
        config.reset_pc = options.reset_pc;
        riscv::CPU cpu(config);
        cpu.instruction_memory().load_hex(options.instruction_file);
        if (!options.data_file.empty())
            cpu.data_memory().load_hex(options.data_file);

        for (u64 cycle = 0; cycle < options.max_cycles; ++cycle) {
            const auto interrupt = options.interrupt_schedule.find(cycle);
            cpu.tick(interrupt == options.interrupt_schedule.end()
                         ? Interrupts{} : interrupt->second);
        }

        print_state(cpu);
        std::cout << "Stopped at the configured cycle limit.\n";
        if (!options.memory_dump_file.empty())
            dump_memory(options.memory_dump_file, cpu.data_memory());
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "emulator: " << error.what() << '\n';
        return 1;
    }
}
