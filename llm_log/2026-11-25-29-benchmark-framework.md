# Benchmark framework

2026-11-25T00:00:49+02:00 · Student 1 · Codex (GPT-6)

## User

Start Part 4 — Performance and Scalability Analysis of the Parallel 2D Heat Diffusion project.\
\
Implement ONLY Step 4.1: a reproducible benchmarking framework.\
\
Before implementation:\
\- Read heat_mpi_project.pdf and heat_numerics_background.pdf if available in the repository.\
\- Inspect the serial, MPI 1D, and MPI 2D implementations.\
\- Inspect the existing timing output and project structure.\
\
Requirements:\
\
1\. Create a Python benchmarking script under scripts/, such as benchmark.py.\
\
2\. Support all three implementations:\
&#x20;  \- serial\
&#x20;  \- 1d\
&#x20;  \- 2d\
\
3\. For MPI implementations, support P = 1, 2, 3, and 4 ranks.\
\
4\. Allow configuration of:\
&#x20;  \- Grid size N.\
&#x20;  \- Stability parameter r.\
&#x20;  \- Simulation steps or final time T.\
&#x20;  \- Number of repetitions.\
&#x20;  \- Output CSV path.\
&#x20;  \- Executable path.\
\
5\. Run the executable using subprocess:\
&#x20;  \- Serial: ./build/heat\
&#x20;  \- MPI: mpirun -np P ./build/heat\
\
6\. Parse the existing summary output:\
&#x20;  \- decomp\
&#x20;  \- P\
&#x20;  \- case\
&#x20;  \- N\
&#x20;  \- steps\
&#x20;  \- r\
&#x20;  \- dt\
&#x20;  \- T\
&#x20;  \- t_total\
&#x20;  \- t_comp\
&#x20;  \- t_comm\
\
7\. Use the solver-reported t_total for performance measurements, not Python subprocess wall-clock time.\
\
8\. Perform one unrecorded warm-up run for each configuration before collecting measurements.\
\
9\. Repeat each configuration multiple times, with five repetitions by default.\
\
10\. Save raw measurements in CSV format, including configuration details and repetition number.\
\
11\. Calculate and save aggregate statistics for each configuration:\
&#x20;   \- Mean execution time.\
&#x20;   \- Median execution time.\
&#x20;   \- Standard deviation.\
&#x20;   \- Minimum execution time.\
&#x20;   \- Mean computation time.\
&#x20;   \- Mean communication time.\
\
12\. Keep the raw measurements and aggregate statistics in separate CSV files.\
\
13\. Handle execution failures, invalid MPI configurations, and missing executable paths clearly. Do not silently treat failed runs as successful measurements.\
\
14\. Make MPI launcher options configurable so the same framework can later run across VMs using Tailscale.\
\
15\. Do not change existing numerical algorithms or timing definitions.\
\
16\. Add clear usage instructions to the README.\
\
17\. Add focused tests for the benchmark parser and CSV aggregation logic.\
\
18\. Follow the project's llm_log/ requirements.\
\
Important constraints:\
\- Do not run large or time-consuming benchmark sweeps yet.\
\- Do not implement speedup or efficiency calculations yet.\
\- Do not implement plotting yet.\
\- Do not implement cross-machine benchmarks yet.\
\- Do not proceed to Step 4.2.\
\
After implementation:\
\- Run a small smoke test using N=32, steps=20, and two repetitions.\
\- Build and run the existing CTest suite.\
\- Report modified files, test results, and the exact command for running the benchmark.\
\- Stop.


## Assistant

I’ll check the project’s measurement rules and solver summary, then build the benchmark runner and run the requested small smoke test.

## Assistant

The parser and aggregation tests passed. The smoke test is running all nine local configurations, with one warm-up and two recorded runs each.

## Assistant final response

Implemented Step 4.1. Added `scripts/benchmark.py` and `tests/benchmark_test.py`, and updated `CMakeLists.txt` and `README.md`. The runner uses solver-reported timings, performs one unrecorded warm-up per configuration, and writes separate raw and aggregate CSVs.

The Release build passed all **23 CTest tests**. The smoke test covered serial and both MPI modes on 1–4 ranks, producing 18 measurements across nine configurations. I also verified that a failed launch, an invalid MPI configuration, and a missing executable each produce an error.

Exact smoke-test command:

```sh
python3 scripts/benchmark.py --heat build-mpi-strips/heat --N 32 --r 0.24 --steps 20 --repetitions 2 --out results/benchmark-smoke.csv
```

[Raw CSV](/home/moyounes/heat-mpi/results/benchmark-smoke.csv) · [Aggregate CSV](/home/moyounes/heat-mpi/results/benchmark-smoke_aggregate.csv) · [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-25-29-benchmark-framework.md)

## Files and verification

Added scripts/benchmark.py (1–165), tests/benchmark_test.py (1–56), and this log. Updated CMakeLists.txt (37–41) and README.md (114–127). Generated results/benchmark-smoke.csv (1–19) and results/benchmark-smoke_aggregate.csv (1–10). CTest regenerated binary fixtures under build-mpi-strips/. All 23 tests passed, CSV data was independently checked, and the three negative smoke checks returned errors.
