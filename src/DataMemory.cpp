#include "DataMemory.hpp"

#include "HexFile.hpp"

#include <stdexcept>

namespace riscv {

DataMemory::DataMemory(std::size_t bytes) : bytes_(bytes, 0) {
    if (bytes == 0) throw std::invalid_argument("data memory must contain bytes");
}

void DataMemory::load_hex(const std::string& path) {
    read_hex_file(path, bytes_.size(), 0xffu,
                  [this](std::size_t index, u32 value) {
                      bytes_[index] = static_cast<u8>(value);
                  });
}

std::size_t DataMemory::size() const { return bytes_.size(); }

u8 DataMemory::read_byte(std::size_t address) const { return bytes_.at(address); }

void DataMemory::write_byte(std::size_t address, u8 value) {
    bytes_.at(address) = value;
}

u32 DataMemory::read_word_aligned(std::size_t address) const {
    const std::size_t base = address & ~std::size_t{3};
    u32 word = 0;
    for (unsigned byte = 0; byte < 4 && base + byte < bytes_.size(); ++byte)
        word |= static_cast<u32>(bytes_[base + byte]) << (byte * 8);
    return word;
}

const std::vector<u8>& DataMemory::bytes() const { return bytes_; }

} // namespace riscv
