# Weak scaling at fixed physical time

2026-11-25T00:10:49+02:00 · Student 1 · Codex (GPT-6)

## User

Continue Part 4 — Performance and Scalability Analysis.\
\
Implement ONLY Step 4.3: weak-scaling analysis.\
\
Before implementation, inspect scripts/benchmark.py, the existing strong-scaling calculations, tests, and project PDFs.\
\
Requirements:\
\
1\. Extend the benchmarking framework to support weak-scaling experiments for MPI 1D and MPI 2D.\
\
2\. Use a baseline grid resolution N0 for P=1.\
\
3\. For each process count P, choose N approximately proportional to N0 × sqrt(P), so each rank owns approximately the same number of interior grid cells.\
\
4\. Use the actual interior-cell count (N-1)² when calculating and reporting work per rank.\
\
5\. Support P=1,2,3,4.\
\
6\. Preserve the same physical domain and the same numerical parameters where appropriate.\
\
7\. Important numerical constraint: because dt = r/(N²) for a unit square and fixed r, keeping the same number of timesteps does NOT represent the same physical simulation duration as N changes.\
\
8\. For meaningful weak-scaling comparisons, keep the physical final time T fixed, and calculate the required number of timesteps separately for each N using the existing solver's timestep-selection convention.\
\
9\. Clearly document that increasing N at fixed T also increases the number of timesteps. Therefore, these results include temporal-work growth and are not pure constant-total-work-per-rank measurements.\
\
10\. Report:\
&#x20;   \- decomposition\
&#x20;   \- P\
&#x20;   \- N\
&#x20;   \- steps\
&#x20;   \- actual simulated T\
&#x20;   \- owned interior cells per rank (average)\
&#x20;   \- median t_total\
&#x20;   \- weak-scaling efficiency relative to the corresponding MPI decomposition at P=1\
\
11\. Define weak-scaling efficiency as T(P=1) / T(P), and document the interpretation and limitations.\
\
12\. Add a --weak-scaling option and an --N0 baseline resolution option.\
\
13\. Generate a separate weak-scaling CSV without changing the existing raw, aggregate, or strong-scaling CSV schemas.\
\
14\. Run warm-up and repeated measurements using the existing framework.\
\
15\. Handle invalid configurations and missing reference measurements explicitly.\
\
16\. Add unit tests for grid-size selection, timestep selection, and weak-scaling calculations.\
\
17\. Update README.md and llm_log/.\
\
Important constraints:\
\- Do not modify the heat solver or numerical algorithms.\
\- Do not implement plotting yet.\
\- Do not perform large benchmark sweeps.\
\- Do not implement cross-machine benchmarking yet.\
\- Do not proceed to Step 4.4.\
\
After implementation:\
\- Run a small weak-scaling smoke test.\
\- Run all CTest tests.\
\- Report modified files, test results, and the exact benchmark command.\
\- Stop.


## Assistant

I’ll check how the benchmark runner handles fixed final time, then add weak scaling with a separate one-rank baseline for each MPI mode.

## Assistant

All eight Python tests passed. The smoke run is checking the real solver’s step selection at the same final time on four different grid sizes.

## Assistant final response

Implemented Step 4.3 in `scripts/benchmark.py` and updated `tests/benchmark_test.py` and `README.md`. Weak scaling uses a fixed physical final time and a separate one-rank MPI baseline for each decomposition. Efficiency therefore includes the extra timestep work required on finer grids.

The Release build passed all **23 CTest tests**. The smoke run produced eight weak-scaling rows across both MPI modes and 1–4 ranks. I independently verified the grid sizes, step counts, final time, average owned cells, and efficiency ratios.

```sh
python3 scripts/benchmark.py --heat build-mpi-strips/heat --weak-scaling --N0 17 --r 0.2 --T 0.01 --repetitions 2 --out results/weak-smoke.csv
```

[Raw CSV](/home/moyounes/heat-mpi/results/weak-smoke.csv) · [Aggregate CSV](/home/moyounes/heat-mpi/results/weak-smoke_aggregate.csv) · [Weak-scaling CSV](/home/moyounes/heat-mpi/results/weak-smoke_weak_scaling.csv) · [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-25-31-weak-scaling.md)

## Files and verification

Updated scripts/benchmark.py (31–34, 122–156, 167–281), tests/benchmark_test.py (6–7, 11, 21–53), and README.md (138–147). Created this log, results/weak-smoke.csv (1–17), results/weak-smoke_aggregate.csv (1–9), and results/weak-smoke_weak_scaling.csv (1–9). CTest refreshed binary fixtures under build-mpi-strips/. Release build and all 23 CTest tests passed. The fixed-step weak-scaling request was rejected and all eight output rows were independently verified.
