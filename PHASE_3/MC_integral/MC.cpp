#include <iostream>
#include <fstream>
#include <cstdint>
#include <random>
#include <chrono>

using namespace std;
using namespace std :: chrono;

int main() {

// MONTECARLO PROCEDURE FOR EVALUATING AN INTEGRAL 
// WITHOUT PARALLELIZATION.

int64_t n_of_points;
double a, b, x_random, f_x, total_sum = 0.0;
string label;

// From the input file you must read:
// - Number of iterations.
// - Bounds of the interval (a, b).
ifstream in_file("input.txt");
in_file >> label >> n_of_points;
in_file >> label >> a;
in_file >> label >> b;
in_file.close();

random_device rd; // Random seed.
mt19937 generator(rd()); // Random number generator.
uniform_real_distribution <double> distribution(a, b);

// Start time.
auto start = high_resolution_clock :: now();

for (int i = 0; i < n_of_points; i++) {
    
    // Generation of a random value into the interval.
    x_random = distribution(generator);

    // Calculation of the function in x.
    f_x = 4.0 / (1.0 + (x_random*x_random));

    // Add the value of f_x to the total sum.
    total_sum += f_x;
}

// End time.
auto stop = high_resolution_clock :: now();
// Duration is an obj, we want to take the numerical value (count).
auto duration = duration_cast<milliseconds>(stop - start);

// Write the result on an output file.
ofstream out_file("output.txt");
out_file << "Integral estimation : " << ((b - a) * (total_sum / n_of_points)) << "\n";
out_file << "Computational time : " << duration.count() << " ms.";
out_file.close();

return 0;

}

