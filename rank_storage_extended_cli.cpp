// rank_storage_extended_cli.cpp
// Author: Trent Wyatt
// Description: Extended CLI with support for symbol sets, persistent storage, and benchmarks

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;

// Default: numeric base-10
std::string SYMBOLS = "0123456789";
std::unordered_map<char, size_t> symbol_index;
size_t BASE;

void initialize_symbol_index() {
    symbol_index.clear();
    BASE = SYMBOLS.size();
    for (size_t i = 0; i < BASE; ++i) {
        symbol_index[SYMBOLS[i]] = i;
    }
}

uint64_t rank(const std::string& data) {
    uint64_t offset = 0;
    for (char c : data) {
        if (symbol_index.find(c) == symbol_index.end()) {
            throw std::invalid_argument("Invalid character in input string: " + std::string(1, c));
        }
        offset = offset * BASE + symbol_index[c];
    }
    return offset;
}

std::string unrank(uint64_t offset, size_t length) {
    std::string result(length, SYMBOLS[0]);
    for (size_t i = 0; i < length; ++i) {
        size_t pos = length - i - 1;
        result[pos] = SYMBOLS[offset % BASE];
        offset /= BASE;
    }
    if (offset != 0) {
        throw std::overflow_error("Offset too large for specified length.");
    }
    return result;
}

void print_help(const std::string& progname) {
    std::cout << "\nUsage: " << progname << " <command> [args]\n\n"
              << "Commands:\n"
              << "  rank <string>                Convert a string to offset\n"
              << "  unrank <offset> <length>     Convert offset back to string\n"
              << "  store <string> <file>        Store string at its offset in file\n"
              << "  retrieve <offset> <file>     Retrieve stored string at offset\n"
              << "  symbolset <id>               Choose symbol set: digit | hex | base64\n"
              << "  benchmark <len> <count>      Time rank/unrank cycles\n"
              << "  help                         Show this message\n\n"
              << "Examples:\n"
              << "  " << progname << " symbolset base64\n"
              << "  " << progname << " rank B3Z\n"
              << "  " << progname << " store B3Z ./store.txt\n"
              << "  " << progname << " retrieve 12345 ./store.txt\n" << std::endl;
}

void set_symbol_set(const std::string& id) {
    if (id == "digit") {
        SYMBOLS = "0123456789";
    } else if (id == "hex") {
        SYMBOLS = "0123456789ABCDEF";
    } else if (id == "base64") {
        SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    } else {
        throw std::invalid_argument("Unsupported symbol set: " + id);
    }
    initialize_symbol_index();
    std::cout << "Symbol set set to [" << id << "] with base = " << BASE << "\n";
}

void store_to_file(const std::string& input, const std::string& filepath) {
    uint64_t offset = rank(input);
    std::ofstream out(filepath, std::ios::app);
    if (!out) throw std::runtime_error("Unable to open file for writing.");
    out << offset << "," << input << "\n";
    std::cout << "Stored: offset=" << offset << " string='" << input << "'\n";
}

void retrieve_from_file(uint64_t offset, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in) throw std::runtime_error("Unable to open file for reading.");
    std::string line;
    while (std::getline(in, line)) {
        size_t sep = line.find(',');
        if (sep == std::string::npos) continue;
        uint64_t found = std::stoull(line.substr(0, sep));
        if (found == offset) {
            std::cout << "Retrieved: '" << line.substr(sep + 1) << "'\n";
            return;
        }
    }
    throw std::runtime_error("Offset not found in file.");
}

void benchmark(size_t length, size_t count) {
    std::string test(length, SYMBOLS[1]);
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        uint64_t r = rank(test);
        std::string s = unrank(r, length);
        assert(s == test);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Benchmark completed: " << count << " cycles in " << ms.count() << " ms\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    try {
        std::string cmd = argv[1];

        if (cmd == "symbolset") {
            if (argc != 3) throw std::runtime_error("Usage: symbolset <id>");
            set_symbol_set(argv[2]);
        } else {
            set_symbol_set("digit"); // default base-10

            if (cmd == "rank") {
                if (argc != 3) throw std::runtime_error("Usage: rank <string>");
                std::cout << rank(argv[2]) << std::endl;
            } else if (cmd == "unrank") {
                if (argc != 4) throw std::runtime_error("Usage: unrank <offset> <length>");
                std::cout << unrank(std::stoull(argv[2]), std::stoul(argv[3])) << std::endl;
            } else if (cmd == "store") {
                if (argc != 4) throw std::runtime_error("Usage: store <string> <file>");
                store_to_file(argv[2], argv[3]);
            } else if (cmd == "retrieve") {
                if (argc != 4) throw std::runtime_error("Usage: retrieve <offset> <file>");
                retrieve_from_file(std::stoull(argv[2]), argv[3]);
            } else if (cmd == "benchmark") {
                if (argc != 4) throw std::runtime_error("Usage: benchmark <length> <count>");
                benchmark(std::stoul(argv[2]), std::stoul(argv[3]));
            } else {
                print_help(argv[0]);
                return 1;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
