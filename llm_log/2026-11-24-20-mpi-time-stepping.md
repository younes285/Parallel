# MPI time stepping

2026-11-24T03:27:33+02:00 · Student 1 · Codex (GPT-6)

## User

Continue Part 2 — MPI 1D Horizontal Strip Decomposition.\
\
Implement ONLY Step 2.4: the complete MPI time-stepping loop.\
\
Inspect the existing serial simulation, MPI strip decomposition, ghost-row communication, local stencil computation, and relevant project PDFs before modifying code.\
\
Requirements:\
\
1\. Implement a reusable MPI 1D simulation function that executes multiple timesteps.\
2\. Initialize local temperatures correctly for Cases 1, 2, and V, using global coordinates and the same initial/boundary conditions as the serial solver.\
3\. Maintain two contiguous local temperature buffers: current and next.\
4\. For every timestep:\
&#x20;  \- Exchange the current buffer's ghost rows using the existing MPI_Sendrecv function.\
&#x20;  \- Apply the local five-point heat stencil to owned interior cells.\
&#x20;  \- Swap the current and next buffers.\
5\. Preserve physical Dirichlet boundaries throughout the simulation, including physical boundary rows stored in outer ghost slots.\
6\. Ensure that communication happens before computation at every timestep, including the first.\
7\. Work correctly for P=1,2,3,4, uneven partitions, and single-row strips.\
8\. Avoid global gathering or unnecessary synchronization inside the timestep loop.\
9\. Measure:\
&#x20;  \- t_total: elapsed time for the timestep loop only.\
&#x20;  \- t_comp: time spent computing the local stencil.\
&#x20;  \- t_comm: time spent exchanging ghost rows.\
&#x20;  Use MPI_Wtime and calculate process-level timing locally. Do not add global timing reductions yet.\
10\. Keep the numerical scheme and timestep selection identical to the serial solver.\
11\. Add tests that compare each rank's final owned temperatures to the corresponding serial reference values after multiple timesteps for Cases 1, 2, and V.\
12\. Verify both even and uneven partitions, including P=1.\
13\. Preserve all existing serial behavior and previous MPI tests.\
14\. Update CMake/CTest and llm_log/ as appropriate.\
\
Important constraints:\
\- Do not implement MPI_Gatherv, final binary output, CLI integration, or performance benchmarks yet.\
\- Do not implement 2D block decomposition.\
\- Do not modify unrelated components.\
\- Do not proceed to Step 2.5.\
\
After implementation:\
\- Build in Release mode.\
\- Run all CTest tests.\
\- Report modified files and test results.\
\- Stop.


## Assistant

I’ll review the serial and strip APIs, then build the MPI loop from the existing ghost exchange and local stencil functions.

## Assistant final response

Implemented Step 2.4. `run_mpi_strip` initializes each strip from global row coordinates, exchanges ghosts before every stencil update, and swaps the two local buffers. It returns the final local strip and per-rank `MPI_Wtime` measurements for the loop, computation, and communication.

Updated `src/mpi_strip.hpp`, `src/mpi_strip.cpp`, `src/simulation_case.hpp`, `src/simulation_case.cpp`, and `tests/mpi_strip_test.cpp`. Serial and MPI initialization share the same row routine.

The Release build passed all 10 CTest tests. Cases 1, 2, and V matched the serial reference exactly on 1–4 ranks, including even and uneven partitions, single-row strips, and zero-step runs. [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-24-20-mpi-time-stepping.md)

## Files and verification

Updated src/mpi_strip.hpp (result and API), src/mpi_strip.cpp (simulation loop), src/simulation_case.hpp and src/simulation_case.cpp (shared initialization), and tests/mpi_strip_test.cpp (simulation checks). Created this log and regenerated build-mpi-strips/ Release artifacts. All 10 CTest tests passed.
