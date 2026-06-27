#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace std :: chrono;

#define epsilon 0.01

// THIS PROGRAM EVALUATES THE SOLUTION OF A LINEAR SYSTEM WITH
// THE JACOBI METHOD.
// THE  OUTPUT DATA ARE READ FROM THE INPUT FILE OUTPUT FILE 
// (output.dat).
// THE LINEAR SYSTEM IS THE FOLLOWING:
// 1) 10x - y + 2z = 6
// 2) -x + 11y - z + 3t = 25
// 3) 2x - y + 10z - t = -11
// 4) 3y - z + 8t = 15

int main(int argc, char* argv[]) {

int size, rank, iters = 0;
double x[4] = {0.0, 0.0, 0.0, 0.0}, x_new, diff, max_diff = 100.0;

// MPI environment initialization
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &size);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);

// Task 0 controls that size is equal to 4.
if (rank == 0) {
    if (size != 4) {
        cout << "Size must be equal to 4!";
        return 0;
    }
}

// Start the iteration until all the error between the previous value 
// and the new value is lower than epsilon.
MPI_Barrier(MPI_COMM_WORLD);
while (max_diff >= epsilon) {
    // Each rank calculate the referred new variable value.
    switch (rank)
    {
    case 0:
        x_new = (6.0 + x[1] - (2.0 * x[2])) / 10.0;
        break;
    case 1:
        x_new = (25.0 + x[0] + x[2] - (3 * x[3])) / 11.0;
        break;
    case 2:
        x_new = (-11.0 - (2 * x[0]) + x[1] + x[3]) / 10.0;
        break;
    case 3:
        x_new = (15.0 - (3 * x[1]) + x[2]) / 8.0;
        break;
    }
    // Calculate the difference and find the maximum.
    diff = fabs(x_new - x[rank]);
    MPI_Allreduce(&diff, &max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    // Update the new value of the variable.
    MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);

    iters++;
}

if (rank == 0) {
    ofstream file("output.dat", "w");
    file << "x = " << x[0] << "\n";
    file << "y = " << x[1] << "\n" ;
    file << "z = " << x[2] << "\n";
    file << "t = " << x[3] << "\n";
    file << "N of iterations : " << iters;
    file.close();
}

MPI_Finalize();
return 0;

}