#include "RegisterFile.hpp"

namespace riscv {

RegisterFile::RegisterFile() { reset(); }

void RegisterFile::reset() { registers_.fill(0); }

u32 RegisterFile::read(unsigned address) const {
    return address == 0 ? 0 : registers_.at(address);
}

void RegisterFile::write(unsigned address, u32 value) {
    if (address != 0) registers_.at(address) = value;
    registers_[0] = 0;
}

const std::array<u32, 32>& RegisterFile::registers() const {
    return registers_;
}

} // namespace riscv
