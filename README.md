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
| `--decomp` | Optional. Defaults to `serial`, the currently supported mode. |
| `--out` | Optional path for the final binary temperature field. |
| `--help` | Show usage and exit. |

Supply exactly one of `--steps` and `--T`. The summary reports the effective `r`, simulated time, and measured time spent in the time-stepping loop.

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

## Planned stages

1. Implement and verify the serial heat diffusion solver.
2. Add MPI strip and block decompositions, then test locally and across the Tailscale cluster.
3. Compare the parallel results with the serial solver and measure scaling.
