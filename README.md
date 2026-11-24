# Parallel 2D Heat Diffusion with MPI over Tailscale

This university project will simulate heat diffusion across a square plate with C++ and run the parallel solver across machines connected by Tailscale.

## Requirements

- A C++17 compiler that supports `-O2` and `-ffp-contract=off`, such as GCC or Clang
- CMake 3.16 or newer

## Build and run

From the project root:

```sh
cmake -S . -B build
cmake --build build
./build/heat --help
./build/heat --case 1 --N 128 --r 0.2 --steps 500 --decomp serial
./build/heat --case V --N 64 --r 0.25 --T 0.05
```

| Option | Meaning |
| --- | --- |
| `--case` | Required. Choose `1`, `2`, or `V`. |
| `--N` | Required. Number of grid intervals per axis, greater than 1. |
| `--r` | Required. Stability parameter in `(0, 0.25]`. |
| `--steps` | Non-negative integer step count. Uses the supplied `r`. |
| `--T` | Non-negative final time. Uses enough steps to keep effective `r` at or below the supplied limit. Zero runs no steps. |
| `--decomp` | Optional. Defaults to `serial`. Use `1d` for MPI horizontal strips or `2d` for MPI Cartesian blocks. |
| `--out` | Optional path for the final binary temperature field. |
| `--help` | Show usage and exit. |

Supply exactly one of `--steps` and `--T`. The summary reports the effective `r`, simulated time, and measured time spent in the time-stepping loop.

CMake enables both MPI modes when it finds MPI. For `1d`, use at most `N-1` ranks. For `2d`, each Cartesian process-grid dimension must be at most `N-1`.

```sh
mpirun -np 3 ./build/heat --case V --N 8 --r 0.2 --T 0.03 --decomp 1d --out results/strip-v.bin
mpirun -np 4 ./build/heat --case V --N 8 --r 0.2 --T 0.03 --decomp 2d --out results/block-v.bin
```

The gather root writes the field and prints the summary. For `2d`, this is Cartesian rank 0. Each reported MPI timing is the maximum of that measurement across the ranks.

## Binary output

```sh
./build/heat --case 1 --N 128 --r 0.2 --steps 500 --out results/case1.bin
```

The parent directory must already exist. An existing file at the specified path is replaced. The file contains all `(N+1)²` nodes, including boundaries, as raw little-endian 64-bit doubles in row-major order. The x-index varies fastest. There is no header, so the file is exactly `8 * (N+1)²` bytes and the reader needs `N` separately. Writing occurs after the measured time-stepping loop.

```python
import numpy as np

N = 128
u = np.fromfile("results/case1.bin", dtype="<f8").reshape(N + 1, N + 1)
# u[j, i] holds the temperature at grid node (i, j).
```

## Visualize temperature fields

The visualizer requires Python 3, NumPy, and Matplotlib. Run these commands from the project root after building:

```sh
for case in 1 2 V; do
    ./build/heat --case "$case" --N 128 --r 0.2 --steps 500 --out "results/case${case}.bin"
    python3 scripts/visualize.py --input "results/case${case}.bin" --N 128 \
        --case "$case" --save "plots/case${case}.png"
done
```

Use `--save` for a 300 dpi PNG without a graphical display, `--show` to open an interactive window, or both. The plot places `y = 0` at the bottom and uses a fixed color scale from 0 to 1 for every case, including Case V, so the decay in peak temperature remains visible across runs. Case 1 warms from all four edges. Case 2 warms from the top and bottom, with hot corners and cold side edges. Case V has a warm center that cools while its edges stay at zero.

## Verify the serial solver

Case V starts with `sin(πx) sin(πy)` and holds every edge at zero. At the actual final time `T`, the exact field is `sin(πx) sin(πy) exp(-2π²T)`. For the nodal error `e = numerical − exact`, the Case V summary reports `err_inf = max |e|` and `err_l2 = sqrt(h² sum(e²))` over all nodes. This comparison runs outside the solver timer.

```sh
python3 scripts/convergence.py
# Or specify a different build:
python3 scripts/convergence.py --heat /path/to/heat
```

The study runs Case V to `T = 0.05` at `N = 16, 32, 64, 128, 256` with requested `r = 0.2`. It prints a table and writes `results/convergence.csv` and `plots/convergence.png`. The observed order is `log2(E_coarse / E_fine)` for consecutive L2 errors. Order 2 means halving the grid spacing reduces the error by about four.

## Status

Step 2.10 completes serial validation. Run `python3 scripts/validate_serial.py` after building with tests enabled to regenerate the evidence. See the [Part 1 checklist and instability experiment](SERIAL_VALIDATION.md).

`Grid(N)` represents the unit square with `N` intervals per axis and spacing `h = 1.0 / N`. It owns one contiguous vector of `(N+1)²` double values initialized to zero. Access temperatures with `grid(i, j)` for coordinates from 0 through `N`. The x-index `i` varies fastest, so row-major storage puts each value at offset `j * (N + 1) + i`. Use `data()` for contiguous access and `fill(temperature)` to set the whole grid.

Run the tests after building with `(cd build && ctest --output-on-failure)`.

## Part 0 MPI ping-pong benchmark

Build the standalone benchmark with an MPI C++ compiler wrapper. Run two ranks on one VM and save the CSV output:

```sh
mkdir -p build
mpicxx -std=c++17 -O2 mpi_tests/pingpong.cpp -o build/pingpong
mpirun -np 2 ./build/pingpong > results/pingpong-local.csv
```

For a cross-VM measurement, build the executable at the same path on both VMs. With Open MPI, list the two hosts in `hosts-pair` using one slot per host and run:

```sh
mpirun --hostfile hosts-pair --map-by node -np 2 \
    --mca oob_tcp_if_include tailscale0 --mca btl_tcp_if_include tailscale0 \
    --mca btl tcp,self ./build/pingpong > results/pingpong-pair.csv
```

Rank 0 reports message bytes, timed iterations, average round-trip seconds, and estimated one-way seconds. Each size uses five untimed warm-up exchanges and 16–1,000 timed exchanges. For Section 4.7, repeat for each VM pair, record direct or DERP routing, and fit the one-way times to `t = t_s + m*t_w` to estimate latency `t_s` and bandwidth `1/t_w`.

## Run timing benchmarks

Build `heat` in Release mode with MPI available, then run the Python 3 benchmark:

```sh
python3 scripts/benchmark.py --heat build/heat --N 32 --r 0.24 --steps 20 --repetitions 2 --out results/benchmark-smoke.csv
```

By default this runs serial once per repetition and both MPI modes at 1–4 ranks. Use `--decomp serial 1d 2d` and `--ranks 1 2 3 4` to select subsets. Choose `--steps` or `--T`. The default is five repetitions, with one unrecorded warm-up per configuration. For measured comparisons, use a fixed step count and at least three repetitions. The two-repetition command above is a smoke test.

The raw CSV retains each recorded solver summary, the launch command, and requested and effective parameters. The sibling file ending in `_aggregate.csv` records mean, median, sample standard deviation, and minimum for solver-reported `t_total`, plus mean `t_comp` and `t_comm`. A single repetition records standard deviation as zero. Timings are in seconds.

Use `--mpirun` to select a launcher and repeat `--mpi-arg` for each launcher token, such as `--mpi-arg=--hostfile --mpi-arg=hosts-pair`. Use `--timeout` to change the limit per launch.

### Strong scaling

Add `--strong-scaling` to benchmark one fixed case, grid size, stability parameter, and simulation duration across serial and both MPI modes at 1–4 ranks:

```sh
python3 scripts/benchmark.py --heat build/heat --N 32 --r 0.24 --steps 20 --repetitions 2 --strong-scaling --out results/scaling-smoke.csv
```

The runner keeps the raw and aggregate CSVs and adds `results/scaling-smoke_strong_scaling.csv`. Each MPI row uses the serial median loop time divided by the MPI median loop time for speedup, then divides speedup by rank count for efficiency. Comparisons require matching case, grid, effective stability parameter, timestep count, timestep size, final simulated time, and repetition count. The runner rejects missing configurations and nonpositive median timings.

### Weak scaling at fixed physical time

```sh
python3 scripts/benchmark.py --heat build/heat --weak-scaling --N0 17 --r 0.2 --T 0.01 --repetitions 2 --out results/weak-smoke.csv
```

Both MPI modes run at 1–4 ranks. The grid uses `N = 1 + round((N0 - 1) * sqrt(P))`, keeping the average `(N-1)^2 / P` owned interior cells roughly constant. The solver receives the same positive `--T` and requested `r` for every run. It chooses the integer step count and effective stability parameter for each grid.

The additional `_weak_scaling.csv` reports actual final time and steps, average owned cells, median loop time, and efficiency. Efficiency is the median loop time at one MPI rank divided by the median at P ranks for the same decomposition. A value of 1 means equal elapsed time. Finer grids require smaller timesteps and more steps at fixed physical time, so the ratio includes temporal-work growth alongside communication and computation costs. It does not measure pure constant-total-work-per-rank scaling.

### Plot benchmark results

Install pandas and Matplotlib in your Python environment. Pass the fixed-grid aggregate and strong-scaling CSVs from the same run, plus the weak-scaling CSV:

```sh
python3 scripts/plot_benchmarks.py --aggregate results/scaling-smoke_aggregate.csv --strong results/scaling-smoke_strong_scaling.csv --weak results/weak-smoke_weak_scaling.csv
```

The script saves PNGs in `plots/benchmarks/` and a Markdown summary in `results/part4/benchmark-summary.md`. Override these destinations with `--out-dir` and `--report`. Raw repetitions remain available in the runner's raw CSV. Distinct fixed-grid sizes receive separate plots. If combining weak CSVs, use the same case and requested stability parameter throughout. The weak schema identifies workloads by baseline grid and final time and does not carry case or stability metadata. Ambiguous rows are rejected. Component bars show mean independently reduced rank maxima, whose sum can differ from the maximum total loop time.

## Planned stages

1. Implement and verify the serial heat diffusion solver.
2. Add MPI strip and block decompositions, then test locally and across the Tailscale cluster.
3. Compare the parallel results with the serial solver and measure scaling.
