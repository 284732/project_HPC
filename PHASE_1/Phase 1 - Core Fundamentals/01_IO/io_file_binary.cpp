/*
 * io_file_binary.cpp
 * ------------------
 * Binary file I/O in C++: ofstream/ifstream with std::ios::binary and the
 * write()/read() methods. The program writes a std::vector<double> in binary
 * format and then reads it back, checking that the data matches. Binary I/O is
 * typical in HPC because it is more compact and faster than text I/O.
 *
 * Build: g++ -Wall -Wextra -std=c++17 io_file_binary.cpp -o io_file_binary
 * Run:   ./io_file_binary
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

int main()
{
    const std::string file_name = "data.bin";
    const std::size_t N = 8;

    std::vector<double> original(N);
    for (std::size_t i = 0; i < N; ++i) {
        original[i] = i * 1.5;         // 0.0, 1.5, 3.0, ...
    }

    // --- Binary writing ---
    {
        std::ofstream out(file_name, std::ios::binary);
        if (!out) {
            std::cerr << "Open for writing failed\n";
            return 1;
        }
        // write() works on a pointer to char: the bytes of the doubles are reinterpreted
        out.write(reinterpret_cast<const char*>(original.data()),
                  static_cast<std::streamsize>(N * sizeof(double)));
    }
    std::cout << "Wrote " << N << " doubles (" << N * sizeof(double)
              << " bytes) to '" << file_name << "'.\n";

    // --- Binary reading ---
    std::vector<double> reread(N);
    {
        std::ifstream in(file_name, std::ios::binary);
        if (!in) {
            std::cerr << "Open for reading failed\n";
            return 1;
        }
        in.read(reinterpret_cast<char*>(reread.data()),
                static_cast<std::streamsize>(N * sizeof(double)));
        if (in.gcount() != static_cast<std::streamsize>(N * sizeof(double))) {
            std::cerr << "Incomplete read\n";
            return 1;
        }
    }

    // --- Check ---
    bool equal = (original == reread);

    std::cout << "Values read back: ";
    for (double v : reread) std::cout << v << " ";
    std::cout << "\nCheck: the data read back "
              << (equal ? "matches" : "does NOT match")
              << " the data written.\n";

    return equal ? 0 : 1;
}
