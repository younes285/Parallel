# Strong-scaling analysis

2026-11-25T00:04:16+02:00 · Student 1 · Codex (GPT-6)

## User

Continue Part 4 — Performance and Scalability Analysis.\
\
Implement ONLY Step 4.2: strong-scaling analysis, speedup, and parallel efficiency.\
\
First inspect scripts/benchmark.py, its generated CSV schema, existing tests, and the project PDFs.\
\
Requirements:\
\
1\. Extend the benchmarking framework to calculate strong-scaling metrics.\
\
2\. Use a fixed global problem size N and fixed simulation duration for every process count.\
\
3\. Use the serial solver's median t_total as the baseline.\
\
4\. Calculate for each MPI 1D and MPI 2D configuration:\
&#x20;  \- Median execution time T(P).\
&#x20;  \- Speedup S(P) = T_serial / T(P).\
&#x20;  \- Parallel efficiency E(P) = S(P) / P.\
&#x20;  \- Efficiency percentage = 100 × E(P).\
\
5\. Include P=1,2,3,4 for both MPI decompositions.\
\
6\. Use only comparable measurements with identical case, N, r, and timestep count.\
\
7\. Preserve the raw and aggregate CSV outputs from Step 4.1.\
\
8\. Create a separate strong-scaling CSV containing:\
&#x20;  \- decomposition\
&#x20;  \- P\
&#x20;  \- N\
&#x20;  \- steps\
&#x20;  \- median_time\
&#x20;  \- serial_baseline\
&#x20;  \- speedup\
&#x20;  \- efficiency\
&#x20;  \- efficiency_percent\
\
9\. Add a clear command-line option to generate strong-scaling results.\
\
10\. Handle missing baselines and incomplete measurements with explicit errors.\
\
11\. Add unit tests verifying speedup and efficiency calculations using known numerical examples.\
\
12\. Update documentation and llm_log/.\
\
Important constraints:\
\- Do not modify the solver's numerical algorithms.\
\- Do not implement weak scaling yet.\
\- Do not implement plots yet.\
\- Do not run large benchmark sweeps yet.\
\- Do not proceed to Step 4.3.\
\
After implementation:\
\- Run a small smoke test.\
\- Run all existing CTest tests.\
\- Report modified files, test results, and the command needed to generate strong-scaling results.\
\- Stop.


## Assistant

I’ll check the existing CSV schema and the project’s strong-scaling definition, then add the analysis and verify it with a small run.

## Assistant

The unit tests passed for known speedup and efficiency values and for rejecting incomplete or mismatched measurements. The local smoke run is underway.

## Assistant final response

Implemented Step 4.2. The new `--strong-scaling` option uses the serial median `t_total` to calculate speedup and efficiency for both MPI modes at 1–4 ranks. It checks that the workloads match and writes a separate CSV alongside the raw and aggregate results.

Updated `scripts/benchmark.py`, `tests/benchmark_test.py`, and `README.md`. All **23 CTest tests passed**. The smoke run produced 18 raw measurements, 9 aggregate rows, and 8 strong-scaling rows. I independently checked all eight rows against the aggregate medians.

```sh
python3 scripts/benchmark.py --heat build-mpi-strips/heat --N 32 --r 0.24 --steps 20 --repetitions 2 --strong-scaling --out results/scaling-smoke.csv
```

[Raw CSV](/home/moyounes/heat-mpi/results/scaling-smoke.csv) · [Aggregate CSV](/home/moyounes/heat-mpi/results/scaling-smoke_aggregate.csv) · [Strong-scaling CSV](/home/moyounes/heat-mpi/results/scaling-smoke_strong_scaling.csv) · [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-25-30-strong-scaling.md)

## Files and verification

Updated scripts/benchmark.py (15–16, 27–30, 78–117, 161, 170–171, 192–196, 203–220), tests/benchmark_test.py (9, 19–47), and README.md (128–137). Created this log, results/scaling-smoke.csv (1–19), results/scaling-smoke_aggregate.csv (1–10), and results/scaling-smoke_strong_scaling.csv (1–9). CTest refreshed binary fixtures under build-mpi-strips/. Release build and all 23 CTest tests passed. The incomplete CLI selection returned an error and the smoke CSV ratios were independently verified.
