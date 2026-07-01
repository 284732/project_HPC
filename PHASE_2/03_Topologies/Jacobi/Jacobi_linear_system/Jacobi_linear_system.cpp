#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>    
#include <mpi.h>

using namespace std;
using namespace std::chrono;

#define epsilon 0.01

int main(int argc, char* argv[]) {

    int size, rank, iters = 0;
    double x[4] = {0.0, 0.0, 0.0, 0.0}, x_new, diff, max_diff = 100.0;

    // MPI initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Check number of processes
    if (rank == 0) {
        if (size != 4) {
            cout << "Size must be equal to 4!" << endl;
        }
    }

    if (size != 4) {
        MPI_Finalize();
        return 0;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Iterative Jacobi method
    while (max_diff >= epsilon) {

        switch (rank)
        {
            case 0:
                x_new = (6.0 + x[1] - (2.0 * x[2])) / 10.0;
                break;
            case 1:
                x_new = (25.0 + x[0] + x[2] - (3.0 * x[3])) / 11.0;
                break;
            case 2:
                x_new = (-11.0 - (2.0 * x[0]) + x[1] + x[3]) / 10.0;
                break;
            case 3:
                x_new = (15.0 - (3.0 * x[1]) + x[2]) / 8.0;
                break;
        }

        diff = std::fabs(x_new - x[rank]);

        MPI_Allreduce(&diff, &max_diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        MPI_Allgather(&x_new, 1, MPI_DOUBLE, x, 1, MPI_DOUBLE, MPI_COMM_WORLD);

        iters++;
    }

    // Output finale
    if (rank == 0) {
        ofstream file("output.dat"); 

        if (!file) {
            cerr << "Errore apertura file!" << endl;
        } else {
            file << "x = " << x[0] << "\n";
            file << "y = " << x[1] << "\n";
            file << "z = " << x[2] << "\n";
            file << "t = " << x[3] << "\n";
            file << "N of iterations : " << iters << "\n";
            file.close();
        }
    }

    MPI_Finalize();
    return 0;
}
