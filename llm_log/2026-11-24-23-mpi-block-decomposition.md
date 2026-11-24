# MPI block decomposition

2026-11-24T23:29:01+02:00 · Student 1 · Codex (GPT-6)

## User

We are starting Part 3 — MPI 2D Block Decomposition of the Parallel 2D Heat Diffusion project.\
\
Before implementing, read the project PDFs (\`heat_mpi_project.pdf\` and \`heat_numerics_background.pdf\`) if available in the repository, and inspect the existing serial and MPI 1D implementations.\
\
Implement ONLY Step 3.1: 2D Cartesian grid decomposition.\
\
Requirements:\
\
1\. Use C++17 and Open MPI.\
2\. Create a dedicated module, such as \`src/mpi_block.hpp\` and \`src/mpi_block.cpp\`.\
3\. Use \`MPI_Dims_create\` to determine the 2D MPI process grid.\
4\. Use \`MPI_Cart_create\` with non-periodic boundaries.\
5\. Use \`MPI_Cart_coords\` and \`MPI_Cart_shift\` to determine each rank's Cartesian coordinates and its top, bottom, left, and right neighbors.\
6\. Decompose the global interior grid of size \`(N-1) × (N-1)\` into rectangular blocks.\
7\. Support uneven partitions in both dimensions, distributing extra rows and columns consistently.\
8\. Each rank must know:\
&#x20;  \- Cartesian rank and coordinates.\
&#x20;  \- Process-grid dimensions.\
&#x20;  \- First global interior row and column.\
&#x20;  \- Number of owned interior rows and columns.\
&#x20;  \- Four neighboring MPI ranks.\
9\. Allocate contiguous local storage for owned cells plus a one-cell halo on all four sides.\
10\. Preserve the existing serial solver's row-major layout and bottom-to-top row convention.\
11\. Account for global physical boundaries without duplicating owned interior cells.\
12\. Support P=1,2,3,4, including a 2×2 process grid for P=4.\
13\. Reject configurations where any process would own zero interior rows or columns.\
14\. Add MPI tests verifying:\
&#x20;   \- Global coverage without gaps or overlaps.\
&#x20;   \- Correct Cartesian coordinates and neighbors.\
&#x20;   \- Even and uneven partitions.\
&#x20;   \- Correct local storage dimensions.\
&#x20;   \- Physical boundary placement.\
15\. Update CMake/CTest as necessary.\
16\. Preserve all existing serial and MPI 1D functionality.\
17\. Record this Codex interaction in \`llm_log/\` according to the project's requirements.\
\
Important constraints:\
\- Do not implement halo communication yet.\
\- Do not implement the heat stencil, timestep loop, global gathering, CLI integration, or performance benchmarks.\
\- Do not modify unrelated components.\
\- Do not proceed to Step 3.2.\
\
After implementation:\
\- Build in Release mode.\
\- Run all CTest tests, including existing serial and MPI 1D tests.\
\- Report modified files and test results.\
\- Stop.


## Assistant

I’ll review the PDFs and existing grid conventions before adding the Cartesian block module and tests.

## Assistant final response

Implemented Step 3.1. `MpiBlock` creates a non-periodic Cartesian grid with `MPI_Dims_create` and `MPI_Cart_create`, then records each rank’s coordinates, neighbors, and owned bounds. Lower coordinates receive extra rows or columns. Each block stores its owned cells and a one-cell halo in contiguous row-major storage, with rows ordered bottom to top.

Added `src/mpi_block.hpp`, `src/mpi_block.cpp`, and `tests/mpi_block_test.cpp`. Updated `CMakeLists.txt`. [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-24-23-mpi-block-decomposition.md)

The Release build passed all **18 CTest tests**, including existing serial and MPI strip tests. The new tests covered 1–4 ranks, the 2×2 grid on four ranks, even and uneven partitions, complete interior coverage, physical boundary placement, and rejection of empty blocks.

## Files and verification

Added src/mpi_block.cpp (1–45), src/mpi_block.hpp (1–53), tests/mpi_block_test.cpp (1–94), and this log. Updated CMakeLists.txt (21–23, 37–38, 45–48). Built build-mpi-strips/libmpi_block.a and build-mpi-strips/mpi_block_test in Release mode. All 18 CTest tests passed.
