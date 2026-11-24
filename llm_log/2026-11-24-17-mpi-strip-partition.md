# MPI strip partition

2026-11-24T02:52:41+02:00 · Student 1 · Codex (GPT-6)

## User

We are starting Part 2 — MPI 1D Horizontal Strip Decomposition of the Parallel 2D Heat Diffusion project.\
\
Read \`heat_mpi_project.pdf\` and \`heat_numerics_background.pdf\` if they are available in the repository. Inspect the existing serial implementation before modifying anything.\
\
Implement ONLY Step 2.1: partitioning the global 2D grid across MPI ranks.\
\
Requirements:\
\
1\. Use C++17 and Open MPI.\
2\. Create a dedicated module for MPI 1D horizontal strip decomposition.\
3\. Partition the global interior rows among P MPI ranks.\
4\. Support uneven partitions when the number of interior rows is not divisible by P.\
5\. Each MPI rank must know:\
&#x20;  \- Its rank and the total number of ranks.\
&#x20;  \- Its first global interior row.\
&#x20;  \- Its number of owned interior rows.\
&#x20;  \- Its upper and lower neighbor ranks, using MPI_PROC_NULL at the domain boundaries.\
6\. Allocate contiguous local storage with one ghost row above and one ghost row below the owned rows.\
7\. Preserve the global grid convention of the existing serial solver: (N+1) × (N+1).\
8\. Do not duplicate or incorrectly assign physical boundary rows.\
9\. Add focused tests to verify correct decomposition for P = 1, 2, 3, and 4, including uneven partitions.\
10\. Follow the existing project structure and document this Codex interaction in \`llm_log/\`.\
\
Important constraints:\
\- Do not implement MPI_Sendrecv yet.\
\- Do not implement heat updates, time stepping, MPI output gathering, or benchmarks yet.\
\- Do not modify the existing serial solver's behavior.\
\- Do not break existing CMake or CTest tests.\
\- Keep the implementation reusable for the next steps.\
\
After implementation:\
\- Build the project.\
\- Run the relevant tests.\
\- Report modified files and test results.\
\- Stop. Do not proceed to Step 2.2.


## Assistant

I’ll check the strip-decomposition spec and serial indexing, then add and test the MPI partition module.

## Assistant final response

Implemented Part 2, Step 2.1. Added `src/mpi_strip.hpp`, `src/mpi_strip.cpp`, and `tests/mpi_strip_test.cpp`. Updated `CMakeLists.txt` to build the module and register its tests when MPI is available.

The first ranks receive any extra interior rows. Each rank stores its owned rows and two ghost rows in one contiguous array. The end ranks use their outer ghost slots for physical boundary rows. Configurations with fewer interior rows than ranks are rejected.

The Release build succeeded. All 10 CTest tests passed, including local Open MPI runs with 1, 2, 3, and 4 ranks and all six existing serial tests. [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-24-17-mpi-strip-partition.md)

## Files and verification

Added src/mpi_strip.hpp, src/mpi_strip.cpp, tests/mpi_strip_test.cpp and this log in full. Updated CMakeLists.txt (MPI target and MPI CTest blocks). Generated build-mpi-strips/ artifacts. Open MPI discovery needed sandbox escalation. All 10 tests passed.
