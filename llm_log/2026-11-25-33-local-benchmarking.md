# Local benchmarking

2026-11-25T00:26:08+02:00 · Student 1 · Codex (GPT-6)

## User

Continue Part 4 — Performance and Scalability Analysis.\
\
Perform ONLY Step 4.5: full local benchmarking and performance analysis.\
\
First inspect:\
\- scripts/benchmark.py and its CLI options.\
\- scripts/plot_benchmarks.py.\
\- Existing benchmark CSV files and tests.\
\- README.md and the project PDFs if available.\
\
Requirements:\
\
1\. Use the existing Release build of the heat solver. Verify it is up to date.\
\
2\. Run local strong-scaling benchmarks for:\
&#x20;  \- Case V.\
&#x20;  \- N = 64, 128, 256.\
&#x20;  \- r = 0.2.\
&#x20;  \- Fixed physical final time T = 0.05 for each grid size.\
&#x20;  \- Serial, MPI 1D, and MPI 2D.\
&#x20;  \- MPI P = 1, 2, 3, 4.\
&#x20;  \- Five recorded repetitions and one unrecorded warm-up per configuration.\
\
3\. For each N, ensure serial and MPI configurations use the same timestep count and simulated final time.\
\
4\. Save raw, aggregate, and strong-scaling CSV files under results/part4/local/.\
\
5\. Run a separate weak-scaling benchmark:\
&#x20;  \- Baseline N0 = 64.\
&#x20;  \- P = 1, 2, 3, 4.\
&#x20;  \- MPI 1D and MPI 2D.\
&#x20;  \- r = 0.2.\
&#x20;  \- T = 0.05.\
&#x20;  \- Five repetitions and one warm-up.\
&#x20;  \- Save results separately so strong-scaling data is not overwritten.\
\
6\. Generate all five performance graphs and Markdown analysis reports using scripts/plot_benchmarks.py.\
\
7\. Ensure graphs distinguish different N values and do not mix incomparable configurations.\
\
8\. Analyze:\
&#x20;  \- Serial vs. MPI execution time.\
&#x20;  \- Speedup and efficiency.\
&#x20;  \- MPI 1D vs. MPI 2D.\
&#x20;  \- Computation vs. communication time.\
&#x20;  \- Strong-scaling behavior.\
&#x20;  \- Weak-scaling behavior and its fixed-T limitations.\
\
9\. Report actual measured values only. Do not invent performance results.\
\
10\. Preserve all existing implementation files and numerical algorithms.\
\
11\. Run all CTest tests after benchmarking.\
\
12\. Record this interaction in llm_log/.\
\
Important:\
\- Run these benchmarks only on the local VM.\
\- Do not perform Tailscale or cross-machine benchmarking.\
\- Do not implement CUDA.\
\- Do not proceed beyond Step 4.5.\
\
After completion:\
\- Report generated files.\
\- Summarize key measured findings.\
\- Show the exact commands used.\
\- Stop.


## Assistant

I’ll verify the Release build and benchmark options, then run the local measurements sequentially so the benchmark jobs do not compete with one another.

## Assistant

Open MPI treated `--host localhost` as one slot. I restarted the sweep with `localhost:4`, within the VM’s eight available vCPUs.

## Assistant

The `N=64` run is complete. Both MPI implementations were already much faster than serial at one rank, so I’ll distinguish that implementation difference from the speedup due to adding ranks.

## Assistant

The `N=64` and `N=128` sweeps are complete, with 45 recorded measurements each. The `N=256` sweep is running now.

## Assistant final response

Step 4.5 is complete. The local Release sweep produced **175 recorded measurements**, 14 CSV files, 13 PNGs, and a Markdown analysis. All **23 CTest tests passed** after benchmarking. [CSV files](/home/moyounes/heat-mpi/results/part4/local) · [Plots](/home/moyounes/heat-mpi/plots/benchmarks/local) · [Analysis](/home/moyounes/heat-mpi/results/part4/local/benchmark-summary.md) · [AI log](/home/moyounes/heat-mpi/llm_log/2026-11-25-33-local-benchmarking.md)

| N | Steps | Serial median | Fastest MPI median | Speedup vs. serial |
|---:|---:|---:|---:|---:|
| 64 | 1,024 | 41.782 ms | 2.674 ms | 15.62× |
| 128 | 4,096 | 697.578 ms | 33.457 ms | 20.85× |
| 256 | 16,384 | 11.582 s | 0.419 s | 27.64× |

Four-rank 1D was fastest at all three sizes. At `N=256`, its four-rank gain relative to its own one-rank run was **2.57×**, compared with **2.12×** for 2D. The larger serial-relative ratios include differences between the serial and MPI implementations. At that size and rank count, mean communication was **0.127 s** for 1D and **0.226 s** for 2D.

At fixed `T=0.05`, weak scaling kept average owned cells near 3,960–3,969 per rank while steps rose from 1,024 to 4,033. Four-rank efficiency was **11.93% for 1D** and **10.38% for 2D**, including the extra timestep work.

The first launch used `localhost`, which Open MPI assigned one slot. The successful run explicitly used four local slots on the eight-vCPU VM.

```bash
cmake --build build-mpi-strips -j 4
for N in 64 128 256; do
    python3 -u scripts/benchmark.py --heat build-mpi-strips/heat --case V --N "$N" --r 0.2 --T 0.05 --repetitions 5 --strong-scaling --mpi-arg=--host --mpi-arg=localhost:4 --out "results/part4/local/strong-N${N}.csv"
done
python3 -u scripts/benchmark.py --heat build-mpi-strips/heat --case V --N0 64 --r 0.2 --T 0.05 --repetitions 5 --weak-scaling --mpi-arg=--host --mpi-arg=localhost:4 --out results/part4/local/weak-N64.csv

cd results/part4/local
for kind in aggregate strong_scaling; do
    awk 'FNR == 1 && NR != 1 {next} {print}' strong-N{64,128,256}_"${kind}".csv > "strong-combined_${kind}.csv"
done
cd ../../..
MPLCONFIGDIR=/tmp/heat-mpl-review .venv/bin/python scripts/plot_benchmarks.py --aggregate results/part4/local/strong-combined_aggregate.csv --strong results/part4/local/strong-combined_strong_scaling.csv --weak results/part4/local/weak-N64_weak_scaling.csv --out-dir plots/benchmarks/local --report results/part4/local/benchmark-summary.md
ctest --test-dir build-mpi-strips -j 8 --output-on-failure
```

## Files and verification

Created this log and the following text files:
- results/part4/local/benchmark-summary.md (lines 1–93)
- results/part4/local/strong-N128.csv (lines 1–46)
- results/part4/local/strong-N128_aggregate.csv (lines 1–10)
- results/part4/local/strong-N128_strong_scaling.csv (lines 1–9)
- results/part4/local/strong-N256.csv (lines 1–46)
- results/part4/local/strong-N256_aggregate.csv (lines 1–10)
- results/part4/local/strong-N256_strong_scaling.csv (lines 1–9)
- results/part4/local/strong-N64.csv (lines 1–46)
- results/part4/local/strong-N64_aggregate.csv (lines 1–10)
- results/part4/local/strong-N64_strong_scaling.csv (lines 1–9)
- results/part4/local/strong-combined_aggregate.csv (lines 1–28)
- results/part4/local/strong-combined_strong_scaling.csv (lines 1–25)
- results/part4/local/weak-N64.csv (lines 1–41)
- results/part4/local/weak-N64_aggregate.csv (lines 1–9)
- results/part4/local/weak-N64_weak_scaling.csv (lines 1–9)

Generated PNGs:
- plots/benchmarks/local/computation-communication-N128-steps4096.png
- plots/benchmarks/local/computation-communication-N256-steps16384.png
- plots/benchmarks/local/computation-communication-N64-steps1024.png
- plots/benchmarks/local/execution-time-N128-steps4096.png
- plots/benchmarks/local/execution-time-N256-steps16384.png
- plots/benchmarks/local/execution-time-N64-steps1024.png
- plots/benchmarks/local/parallel-efficiency-N128-steps4096.png
- plots/benchmarks/local/parallel-efficiency-N256-steps16384.png
- plots/benchmarks/local/parallel-efficiency-N64-steps1024.png
- plots/benchmarks/local/speedup-N128-steps4096.png
- plots/benchmarks/local/speedup-N256-steps16384.png
- plots/benchmarks/local/speedup-N64-steps1024.png
- plots/benchmarks/local/weak-efficiency-N64-1.png

CTest regenerated binary fixtures in build-mpi-strips/. All 23 tests passed. Raw counts, medians, steps, final times and image formats were checked. No implementation files were modified.
