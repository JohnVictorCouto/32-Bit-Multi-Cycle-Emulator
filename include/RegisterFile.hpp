#pragma once

#include "Definitions.hpp"

#include <array>

namespace riscv {

class RegisterFile {
public:
    RegisterFile();
    void reset();
    u32 read(unsigned address) const;
    void write(unsigned address, u32 value);
    const std::array<u32, 32>& registers() const;

private:
    std::array<u32, 32> registers_;
};

} // namespace riscv
