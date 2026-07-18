#include <iostream>
#include <fstream>
#include <cstdint>
#include <random>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace std :: chrono;

int main(int argc, char *argv[]) {

// MONTECARLO PROCEDURE FOR EVALUATING INTEGRAL WITH PARALLELIZATION USING MPI.

int size, rank;
int64_t n_of_points, local_n_of_points, remainder;
double bounds[2], x_random, f_x, total_sum, local_sum = 0.0;
string label;

// Initialize MPI environment.
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &size);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);

// Process 0 reads the input file and broadcasts the values to all processes.
if (rank == 0) {
    ifstream in_file("input_MC_MPI.txt");
    in_file >> label >> n_of_points;
    in_file >> label >> bounds[0];
    in_file >> label >> bounds[1];
    in_file.close();
}

// Process 0 broadcasts the number of points and bounds to all processes.
MPI_Bcast(&n_of_points, 1, MPI_INT64_T, 0, MPI_COMM_WORLD);
MPI_Bcast(bounds, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

// If the number of points is lower than the number of processes, print an error message, 
// finalize MPI and terminate.
if (n_of_points < size) {
    if (rank == 0) {
        cout << "ERROR : Impossible to solve with a number of iterations lower than the number of tasks!\n";
    }
    MPI_Finalize();
    return 0;
}

// Each process calculates its local number of points to generate.
local_n_of_points = int(n_of_points / size);
remainder = n_of_points % size;
for (int i = 0; i < remainder; i++) {
    if (rank == i) {
        local_n_of_points++;
    }
}

// Create a random number generator for each process.
random_device rd; // Random seed.
mt19937 generator(rd()); // Random number generator.
uniform_real_distribution <double> distribution(bounds[0], bounds[1]);

// Start time.
double start_time = MPI_Wtime();

for (int i = 0; i < local_n_of_points; i++) {
    // Each process executes the same cycle, but with different random values.
    x_random = distribution(generator);
    f_x = 4.0 / (1.0 + (x_random * x_random));
    local_sum += f_x;
}

// Each process sends its local sum to process 0.
MPI_Reduce(&local_sum, &total_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

// End time.
double end_time = MPI_Wtime();

// Process 0 writes the result on an output file.
if (rank == 0) {
    ofstream out_file("output_MC_MPI.txt");
    out_file << "Integral estimation : " << ((bounds[1] - bounds[0]) * (total_sum / n_of_points)) << "\n";
    out_file << "Computational time : " << (end_time - start_time) * 1000 << " ms.";
    out_file.close();
}

// Finalize MPI environment.
MPI_Finalize();
return 0;

}