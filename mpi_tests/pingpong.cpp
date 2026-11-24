#include <mpi.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank;
    int ranks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &ranks);
    if (ranks != 2) {
        if (rank == 0) {
            std::cerr << "pingpong requires exactly 2 MPI ranks.\n";
        }
        MPI_Finalize();
        return 1;
    }

    constexpr int max_bytes = 16 * 1024 * 1024;
    constexpr int warmups = 5;
    std::vector<unsigned char> buffer(max_bytes, 0);

    if (rank == 0) {
        std::cout << "message_bytes,iterations,avg_round_trip_seconds,one_way_seconds\n"
                  << std::scientific << std::setprecision(9);
    }
    for (int bytes = 8; bytes <= max_bytes; bytes *= 2) {
        const int iterations = std::clamp(64 * 1024 * 1024 / bytes, 16, 1000);
        const int peer = 1 - rank;
        const auto exchange = [&] {
            if (rank == 0) {
                MPI_Send(buffer.data(), bytes, MPI_BYTE, peer, 0, MPI_COMM_WORLD);
                MPI_Recv(buffer.data(), bytes, MPI_BYTE, peer, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(buffer.data(), bytes, MPI_BYTE, peer, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
                MPI_Send(buffer.data(), bytes, MPI_BYTE, peer, 0, MPI_COMM_WORLD);
            }
        };

        for (int i = 0; i < warmups; ++i) {
            exchange();
        }
        MPI_Barrier(MPI_COMM_WORLD);
        const double start = MPI_Wtime();
        for (int i = 0; i < iterations; ++i) {
            exchange();
        }
        if (rank == 0) {
            const double round_trip = (MPI_Wtime() - start) / iterations;
            std::cout << bytes << ',' << iterations << ',' << round_trip << ','
                      << round_trip / 2.0 << '\n';
        }
    }

    MPI_Finalize();
}
