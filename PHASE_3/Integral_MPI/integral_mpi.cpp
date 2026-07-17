#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace std :: chrono;

// THIS PROGRAM CALCULATES AN INTEGRAL IN A GENERIC INTERVAL.
// THE DATA ARE TAKEN FROM AN INPUT FILE 'input.txt'.
// THE OUTPUT DATA ARE WRITED ON 'output.txt'.
// THE PROCEDURE IS SHEDULED IN A PARALLELIZED WAY BY USING THE MPI LIBRARY.

double calculate_function(double x, double step) {
    return (double(4) * step) / (double(1) + (x*x));
}


int main(int argc, char* argv[]) {

int rank, size, local_tasks, remainder;
int32_t iterations;
double bounds[2], interval_amp, estimate, x_start, step, step_per_task, local_sum = 0.0;
string line;

// Initialize the MPI environment.
// I use & because MPI needs to modify the variables, then it needs the memory register, not the stored value into them.
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &size);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);

// Task 0 reads the data from the input file.
if (rank == 0) {
    ifstream in_file("input.txt");

    // Control if the file is open correctly.
    // ! before means the state 'not'.
    if (!in_file.is_open()) {
        cout << "ERROR : The file is not correctly open!\n";
        return 0;
    }

    getline(in_file, line); // Save into 'line' the current line of the file.
    // Take the position of the ':' and takes the string after (+1).
    // stoi (or stod) converts the substring in an integer (or double).
    iterations = stoi(line.substr(line.find(":") + 1));

    // Control that the number of iterations is higher than the number of tasks.
    if (size > iterations) {
        cout << "ERROR : Impossible to solve with a number of iterations lower than the number of tasks!\n";
    }

    // Reads the 2 bounds of the interval.
    for (int i = 0; i < 2; i++) {
        getline(in_file, line);
        bounds[i] = stod(line.substr(line.find(":") + 1));
    }
    
    in_file.close();
}

// Now, the process 0 must broadcast the number of iterations with the other processes.
// Arguments : variable register, data count, data type, root process, communicator. 
MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(&bounds, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

if (size > iterations) {
    MPI_Finalize();
    return 0;
}

MPI_Barrier(MPI_COMM_WORLD);
auto start = high_resolution_clock :: now();

// Find the number of steps that every tasks have to elaborate.
local_tasks = int(iterations / size);
remainder = iterations % size;
// Distribute the remainder to the processes.
for (int i = 0; i < remainder; i++) {
    if (rank == i) {
        local_tasks++;
    }
}

// Each process calculates its starting point, the step and the step for the next x.
interval_amp = bounds[1] - bounds[0];
x_start = bounds[0] + (interval_amp / double(iterations)) * double(rank);
step = double(interval_amp / iterations);
step_per_task = step * double(size);

// Each task calculates its local sum.
for (int i = 0; i < local_tasks; i++) {
    local_sum += calculate_function(x_start, step);
    x_start += step_per_task;
}

// Use a MPI_Reduce for calculate the global sum.
// MPI_Reduce is blocking, then it waits that all the process sends their value until going on.
MPI_Reduce(&local_sum, &estimate, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

MPI_Barrier(MPI_COMM_WORLD);
auto stop = high_resolution_clock :: now();
auto duration = duration_cast<milliseconds>(stop - start);

// Process 0 have saved into estimate the value of the integral, now writes the results in the output file.
if (rank == 0) {
    ofstream out_file("output.dat");
    out_file << "Resolution with " << size << " processes:\n";
    out_file << "Estimated value = " << estimate << "\n";
    out_file << "Computational time = " << duration.count() << " ms.";
    out_file.close();
}

MPI_Finalize();
return 0;

}
