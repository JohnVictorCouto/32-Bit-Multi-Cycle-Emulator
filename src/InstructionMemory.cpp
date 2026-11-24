#include "InstructionMemory.hpp"

#include "HexFile.hpp"

#include <stdexcept>

namespace riscv {

InstructionMemory::InstructionMemory(std::size_t words) : words_(words, 0x00000013) {
    if (words == 0) throw std::invalid_argument("instruction memory must contain words");
}

void InstructionMemory::load_hex(const std::string& path) {
    read_hex_file(path, words_.size(), 0xffffffffu,
                  [this](std::size_t index, u32 value) { words_[index] = value; });
}

std::size_t InstructionMemory::size() const { return words_.size(); }

u32 InstructionMemory::read_word(std::size_t index) const {
    return words_.at(index);
}

const std::vector<u32>& InstructionMemory::words() const { return words_; }

} // namespace riscv
