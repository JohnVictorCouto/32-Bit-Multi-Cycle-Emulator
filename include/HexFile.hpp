#pragma once

#include "Definitions.hpp"

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>

namespace riscv {

template <typename WriteValue>
void read_hex_file(const std::string& path, std::size_t capacity,
                   u32 max_value, WriteValue write_value) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open hex file: " + path);

    std::string line;
    std::size_t index = 0;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const std::size_t comment = line.find_first_of("#");
        if (comment != std::string::npos) line.erase(comment);
        const std::size_t slash_comment = line.find("//");
        if (slash_comment != std::string::npos) line.erase(slash_comment);

        std::size_t start = 0;
        while (start < line.size()) {
            const std::size_t end = line.find_first_of(" \t\r,", start);
            const std::string token = line.substr(
                start, end == std::string::npos ? std::string::npos : end - start);
            start = end == std::string::npos ? line.size() : end + 1;
            if (token.empty()) continue;

            try {
                if (token[0] == '@') {
                    if (token.size() == 1) throw std::invalid_argument("empty address");
                    std::size_t parsed_chars = 0;
                    const unsigned long long parsed =
                        std::stoull(token.substr(1), &parsed_chars, 16);
                    if (parsed_chars != token.size() - 1)
                        throw std::invalid_argument("address");
                    if (parsed >= capacity) throw std::out_of_range("address");
                    index = static_cast<std::size_t>(parsed);
                    continue;
                }
                std::string value_token;
                for (char character : token)
                    if (character != '_') value_token.push_back(character);
                std::size_t parsed_chars = 0;
                const unsigned long long parsed =
                    std::stoull(value_token, &parsed_chars, 16);
                if (parsed_chars != value_token.size() || parsed > max_value)
                    throw std::invalid_argument("value");
                if (index >= capacity) throw std::out_of_range("capacity");
                write_value(index++, static_cast<u32>(parsed));
            } catch (const std::exception&) {
                throw std::runtime_error("invalid hex data at " + path + ":" +
                                         std::to_string(line_number) + ": " + token);
            }
        }
    }
}

} // namespace riscv
