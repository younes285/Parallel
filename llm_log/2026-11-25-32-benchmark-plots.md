# Benchmark visualization

2026-11-25T00:15:14+02:00 · Student 1 · Codex (GPT-6)

## User

Continue Part 4 — Performance and Scalability Analysis.\
\
Implement ONLY Step 4.4: performance visualization and analysis.\
\
Before implementation, inspect:\
\- scripts/benchmark.py.\
\- Raw, aggregate, strong-scaling, and weak-scaling CSV schemas.\
\- Existing benchmark tests and README.\
\- Project PDFs if available.\
\
Requirements:\
\
1\. Create scripts/plot_benchmarks.py using Python, pandas, and matplotlib.\
\
2\. Read the benchmark CSV files produced by the existing framework. Do not invent or hardcode measurement values.\
\
3\. Generate the following graphs:\
\
&#x20;  A. Execution time vs. MPI process count:\
&#x20;     \- Serial reference as a horizontal line.\
&#x20;     \- MPI 1D and MPI 2D curves.\
&#x20;     \- Use median execution times.\
\
&#x20;  B. Speedup vs. MPI process count:\
&#x20;     \- MPI 1D and MPI 2D curves.\
&#x20;     \- Ideal linear speedup reference S(P)=P.\
\
&#x20;  C. Parallel efficiency vs. MPI process count:\
&#x20;     \- MPI 1D and MPI 2D curves.\
&#x20;     \- Ideal 100% reference.\
\
&#x20;  D. Computation vs. communication time:\
&#x20;     \- Compare MPI 1D and MPI 2D.\
&#x20;     \- Use clearly labeled grouped or stacked bars.\
&#x20;     \- Explain that independently reduced maximum computation and communication timings may not sum exactly to maximum total time.\
\
&#x20;  E. Weak-scaling efficiency vs. MPI process count:\
&#x20;     \- Separate curves for MPI 1D and MPI 2D.\
&#x20;     \- Ideal 100% reference.\
&#x20;     \- Label the fixed-physical-time interpretation and its limitations.\
\
4\. Save all graphs as PNG files under plots/benchmarks/.\
\
5\. Use descriptive titles, labeled axes, units, legends, and readable formatting.\
\
6\. Support multiple problem sizes if the CSV contains more than one N, without incorrectly mixing workloads.\
\
7\. Add configurable input CSV paths and output directory.\
\
8\. Handle missing or malformed CSV files with clear error messages.\
\
9\. Add a short automatically generated Markdown summary under results/part4/ containing:\
&#x20;  \- Configurations tested.\
&#x20;  \- Main timing observations.\
&#x20;  \- Strong-scaling observations.\
&#x20;  \- Weak-scaling observations.\
&#x20;  \- Communication overhead observations.\
&#x20;  \- Methodological limitations.\
\
10\. Use only actual benchmark data in the report. Avoid unsupported conclusions from small smoke tests.\
\
11\. Add focused tests for plotting data preparation and validation.\
\
12\. Update README.md and llm_log/.\
\
Important constraints:\
\- Do not modify the heat solver, MPI implementations, or timing definitions.\
\- Do not run large benchmark sweeps yet.\
\- Do not perform cross-machine benchmarks yet.\
\- Do not proceed to Step 4.5.\
\
After implementation:\
\- Generate plots using existing smoke-test CSVs.\
\- Run all CTest tests.\
\- Report files created, test results, and the exact plotting command.\
\- Stop.


## Assistant

I’ll inspect the smoke-test CSVs and their workload fields before adding the plots and generated report.

## Assistant final response

Implemented Step 4.4. Added `scripts/plot_benchmarks.py`, added focused tests in `tests/plot_benchmarks_test.py`, and updated `README.md`. The existing smoke-test CSVs produced five PNGs and a Markdown report. The report describes the measured runs and explains the limits of comparing independently reduced timings and fixed-physical-time weak scaling.

All **23 CTest tests** and **3 plotting tests** passed. Pandas was installed in the project virtual environment for verification.

```sh
MPLCONFIGDIR=/tmp/heat-mpl-review .venv/bin/python scripts/plot_benchmarks.py --aggregate results/scaling-smoke_aggregate.csv --strong results/scaling-smoke_strong_scaling.csv --weak results/weak-smoke_weak_scaling.csv
```

[Plots](/home/moyounes/heat-mpi/plots/benchmarks) · [Benchmark report](/home/moyounes/heat-mpi/results/part4/benchmark-summary.md) · [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-25-32-benchmark-plots.md)

## Files and verification

Added scripts/plot_benchmarks.py (1–237), tests/plot_benchmarks_test.py (1–58), results/part4/benchmark-summary.md (1–43), and this log. Updated README.md (148–157). Installed pandas in .venv/. Generated these binary artifacts:
- plots/benchmarks/computation-communication-N32-steps20.png
- plots/benchmarks/execution-time-N32-steps20.png
- plots/benchmarks/parallel-efficiency-N32-steps20.png
- plots/benchmarks/speedup-N32-steps20.png
- plots/benchmarks/weak-efficiency-N17-1.png

All 23 CTest tests and three plotting tests passed. All five PNG files were recognized as valid images.
