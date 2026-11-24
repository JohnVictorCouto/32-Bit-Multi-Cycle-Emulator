#pragma once

#include "Definitions.hpp"

#include <string>
#include <vector>

namespace riscv {

class InstructionMemory {
public:
    explicit InstructionMemory(std::size_t words = 256);
    void load_hex(const std::string& path);
    std::size_t size() const;
    u32 read_word(std::size_t index) const;
    const std::vector<u32>& words() const;

private:
    std::vector<u32> words_;
};

} // namespace riscv
