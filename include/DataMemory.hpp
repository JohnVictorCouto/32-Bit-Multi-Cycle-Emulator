#pragma once

#include "Definitions.hpp"

#include <string>
#include <vector>

namespace riscv {

class DataMemory {
public:
    explicit DataMemory(std::size_t bytes = 1024);
    void load_hex(const std::string& path);
    std::size_t size() const;
    u8 read_byte(std::size_t address) const;
    void write_byte(std::size_t address, u8 value);
    u32 read_word_aligned(std::size_t address) const;
    const std::vector<u8>& bytes() const;

private:
    std::vector<u8> bytes_;
};

} // namespace riscv
