/*
 * conditionals.cpp
 * ----------------
 * Selection constructs in C++: if/else if/else, the ternary operator, switch.
 *
 * Build: g++ -Wall -Wextra -std=c++17 conditionals.cpp -o conditionals
 * Run:   ./conditionals
 */

#include <iostream>
#include <vector>

// Returns a letter based on the grade (example of an if/else if chain).
char grade(int score)
{
    if (score >= 90) {
        return 'A';
    } else if (score >= 80) {
        return 'B';
    } else if (score >= 70) {
        return 'C';
    } else {
        return 'F';
    }
}

int main()
{
    // --- if / else if / else ---
    std::vector<int> scores = {95, 83, 72, 50};

    std::cout << "Grading the scores:\n";
    for (int s : scores) {
        std::cout << "  score " << s << " -> category " << grade(s) << "\n";
    }

    // --- Ternary operator: maximum of two numbers ---
    int a = 17, b = 42;
    int maximum = (a > b) ? a : b;
    std::cout << "\nThe maximum of " << a << " and " << b << " is " << maximum << "\n";

    // --- Short-circuit evaluation ---
    int x = 10;
    if (x != 0 && 100 / x > 5) {
        std::cout << "100/" << x
                  << " > 5 (the division is safe thanks to short-circuiting)\n";
    }

    // --- switch over discrete integer values ---
    std::cout << "\nDays of the weekend (switch):\n";
    for (int day = 1; day <= 7; ++day) {
        switch (day) {
            case 6:
            case 7:
                // intentional fall-through: 6 and 7 share the same code
                std::cout << "  day " << day << ": weekend\n";
                break;
            default:
                std::cout << "  day " << day << ": weekday\n";
                break;
        }
    }

    return 0;
}
