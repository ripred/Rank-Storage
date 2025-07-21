// rank_storage_cleanup.cpp
// Enhancement: Adds cleanup of temporary files created during analysis

#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

const std::string SYMBOL_DIGITS = "0123456789";
const std::string SYMBOL_HEX = "0123456789ABCDEF";
const std::string SYMBOL_BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string generate_random_text(size_t length, const std::string& symbol_set) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, symbol_set.size() - 1);
    std::string output;
    output.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        output += symbol_set[dis(gen)];
    }
    return output;
}

size_t bits_required(size_t base, size_t length) {
    double bits_per_char = std::log2(static_cast<double>(base));
    return static_cast<size_t>(length * bits_per_char);
}

size_t simulated_rank_storage_bytes(const std::string& text, size_t base) {
    return (bits_required(base, text.length()) + 7) / 8;
}

size_t run_compression(const std::string& compressor, const std::string& filename) {
    std::string command = compressor + " -k -f -q " + filename;
    system(command.c_str());
    std::string out_file = filename + ((compressor == "gzip") ? ".gz" : (compressor == "bzip2") ? ".bz2" : ".zst");
    std::ifstream in(out_file, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

void cleanup_files(const std::string& base) {
    try {
        fs::remove(base);
        fs::remove(base + ".gz");
        fs::remove(base + ".bz2");
        fs::remove(base + ".zst");
    } catch (...) {
        std::cerr << "Warning: Cleanup failed for one or more files.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./rank_storage_sim <symbolset: digit|hex|base64> <size_in_kb>" << std::endl;
        return 1;
    }

    std::string symbolset = argv[1];
    size_t kb = static_cast<size_t>(std::stoul(argv[2]));
    size_t total_bytes = kb * 1024;

    std::string SYMBOLS;
    if (symbolset == "digit") SYMBOLS = SYMBOL_DIGITS;
    else if (symbolset == "hex") SYMBOLS = SYMBOL_HEX;
    else if (symbolset == "base64") SYMBOLS = SYMBOL_BASE64;
    else {
        std::cerr << "Unknown symbolset. Use: digit, hex, or base64." << std::endl;
        return 1;
    }

    const std::string filename = "input.txt";

    std::string input = generate_random_text(total_bytes, SYMBOLS);
    std::ofstream file(filename);
    file << input;
    file.close();

    size_t original_size = total_bytes;
    size_t rank_size = simulated_rank_storage_bytes(input, SYMBOLS.size());
    size_t gzip_size = run_compression("gzip", filename);
    size_t bzip2_size = run_compression("bzip2", filename);
    size_t zstd_size = run_compression("zstd", filename);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n--- Compression Ratio Comparison on " << kb << "KB Random Text (" << symbolset << ") ---\n";
    std::cout << "Original Size:      " << original_size << " bytes\n";
    std::cout << "Rank Storage Size:  " << rank_size << " bytes (" << (100.0 * rank_size / original_size) << "%)\n";
    std::cout << "Gzip Compressed:    " << gzip_size << " bytes (" << (100.0 * gzip_size / original_size) << "%)\n";
    std::cout << "Bzip2 Compressed:   " << bzip2_size << " bytes (" << (100.0 * bzip2_size / original_size) << "%)\n";
    std::cout << "Zstd Compressed:    " << zstd_size << " bytes (" << (100.0 * zstd_size / original_size) << "%)\n";

    cleanup_files(filename);
    return 0;
}

