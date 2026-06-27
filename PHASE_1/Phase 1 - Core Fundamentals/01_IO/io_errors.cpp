/*
 * io_errors.cpp
 * -------------
 * I/O error handling in C++:
 *   - checking the stream state (operator bool, fail(), eof());
 *   - reading "while (in >> x)" as long as extraction succeeds;
 *   - using exceptions enabled with exceptions().
 *
 * Build: g++ -Wall -Wextra -std=c++17 io_errors.cpp -o io_errors
 * Run:   ./io_errors
 */

#include <iostream>
#include <fstream>
#include <string>

int main()
{
    // --- 1. Attempt to open a non-existent file ---
    const std::string nonexistent = "this_file_does_not_exist.txt";
    std::ifstream f(nonexistent);
    if (!f) {   // the stream converted to bool is false if the open fails
        std::cerr << "Opening '" << nonexistent << "' failed.\n";
        std::cout << "-> Handled correctly: the program continues.\n\n";
    }

    // --- 2. Create a file and read it back, checking the state ---
    {
        std::ofstream out("numbers.txt");
        if (!out) {
            std::cerr << "File creation failed\n";
            return 1;
        }
        for (int i = 1; i <= 5; ++i) {
            out << i * i << "\n";   // squares: 1 4 9 16 25
        }
    }

    std::ifstream in("numbers.txt");
    if (!in) {
        std::cerr << "Opening numbers.txt failed\n";
        return 1;
    }

    int value;
    long sum = 0;
    int count = 0;
    // Extraction returns the stream: the loop ends at end of file or on error.
    while (in >> value) {
        sum += value;
        ++count;
    }
    if (in.eof()) {
        std::cout << "Reached the end of the file (normal condition).\n";
    }
    std::cout << "Read " << count << " values, sum = " << sum << ".\n\n";

    // --- 3. Same read but with exceptions enabled ---
    try {
        std::ifstream in2("numbers.txt");
        // Ask the stream to throw an exception on badbit/failbit.
        in2.exceptions(std::ifstream::badbit);
        long s = 0;
        int v;
        while (in2 >> v) {
            s += v;
        }
        std::cout << "With exceptions enabled, recomputed sum = " << s << ".\n";
    } catch (const std::ios_base::failure& e) {
        std::cerr << "I/O exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
