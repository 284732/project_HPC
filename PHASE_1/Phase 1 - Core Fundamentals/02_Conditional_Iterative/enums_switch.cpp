/*
 * enums_switch.cpp
 * ----------------
 * switch combined with an enumerated type (enum class, C++11).
 * Enumerations make the code more readable than "magic numbers" and pair
 * naturally with switch, which dispatches on discrete values.
 *
 * Build: g++ -Wall -Wextra -std=c++17 enums_switch.cpp -o enums_switch
 * Run:   ./enums_switch
 */

#include <iostream>

// Strongly typed enumeration: the values live in the State:: scope.
enum class State { Idle, Running, Paused, Stopped };

// Maps a state to its textual name with a switch.
const char* to_string(State s)
{
    switch (s) {
        case State::Idle:    return "Idle";
        case State::Running: return "Running";
        case State::Paused:  return "Paused";
        case State::Stopped: return "Stopped";
    }
    return "Unknown";   // defensive: should never be reached
}

// Returns the next state in a simple state machine.
State next(State s)
{
    switch (s) {
        case State::Idle:    return State::Running;
        case State::Running: return State::Paused;
        case State::Paused:  return State::Running;
        case State::Stopped: return State::Stopped;
    }
    return State::Stopped;
}

int main()
{
    std::cout << "All states:\n";
    State all[] = {State::Idle, State::Running, State::Paused, State::Stopped};
    for (State s : all) {
        std::cout << "  " << to_string(s) << "\n";
    }

    std::cout << "\nState machine transitions starting from Idle:\n";
    State s = State::Idle;
    for (int i = 0; i < 4; ++i) {
        State n = next(s);
        std::cout << "  " << to_string(s) << " -> " << to_string(n) << "\n";
        s = n;
    }

    return 0;
}
