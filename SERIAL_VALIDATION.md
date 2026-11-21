# Serial solver validation

The Release build passed Part 1 validation. The full Checkpoint C1 also requires cluster and written evidence beyond this serial validation.

## Part 1 checklist

| Requirement | Status | Test or evidence | Remaining issue |
| --- | --- | --- | --- |
| C++17 CMake build with optimization and `-ffp-contract=off` | Pass | Release configuration and build | None |
| One contiguous array per grid and one indexing function | Pass | `Grid::index` and grid storage/layout test | None |
| Two time levels swapped without copying | Pass | Pointer swap in `run_serial` and zero-, one-, and multi-step tests | None |
| Fixed boundaries and correct Case 1, 2, and V initial data | Pass | Initialization unit tests and end-to-end field checks | None |
| Stable explicit scheme and unstable-input rejection | Pass | Stencil tests, valid runs at 0.2 and 0.25, CLI rejection of zero, negative, non-finite and above-limit values | None |
| Section 3 serial CLI and requested final time | Pass | CLI tests and independent final-time check at `T = 0.019` | None |
| Complete raw little-endian row-major field | Pass | Exact 200-byte reference field test and byte-for-byte repeat checks | None |
| Time-stepping-only timing and machine-readable summary | Pass | `steady_clock` brackets only the loop in `run_serial`. The CLI writes the field and evaluates norms after it returns. Summary checks confirm `P=1`, `t_comm=0` and equal serial computation and total times. | None |
| Case V reports both norms at the actual final time | Pass | Known-error unit test, CLI checks for all three cases, and independent NumPy calculation | None |
| Task 7.2 convergence table and log-log figure | Pass | [CSV](results/convergence.csv) and [plot](plots/convergence.png). Observed L2 order approaches 2. | Include the evidence in the final report |
| Task 6.7 instability figure | Pass | [Heatmap](plots/instability.png) and explanation below | Include the evidence in the final report |
| Serial baseline retained for parallel checks | Pass | Explicit `--decomp serial` and deterministic field output | Parallel comparisons belong to later parts |

The field checks also verified reflection symmetry, the maximum principle and approach to steady state for both hot-edge cases. The insulated-edge conservation check in §7.5 requires Neumann boundaries, outside the three specified Dirichlet cases.

## Task 6.7 experiment

The test-build executable seeds a zero-boundary grid with a perturbation of peak amplitude `1e-6` in the highest-frequency Dirichlet sine mode on both axes. Its mode indices are `(31,31)` on a 32-interval grid. The production Case V starts with the lowest sine mode, so it does not directly seed the mode that grows fastest. The separate experiment uses the same stencil at `r = 0.2625`, 5% above the stability bound. The normal CLI rejects this value.

The chosen mode alternates sign between neighboring nodes in each coordinate. Its amplification factor is `G = 1 - 4r + 4r cos(31π/32) ≈ -1.094944`. Each step reverses the sign and grows the magnitude by about 9.49%. The worst Fourier mode at `(π,π)` has `G = 1 - 8r = -1.1`. After 200 steps the measured peak magnitude was approximately 75.575093, matching the predicted finite-grid growth. The diverging color scale shows the resulting positive and negative checkerboard extrema near the center.

## Checkpoint and later work

- C1 serial convergence passed. VM hostname and hello-world runs, same-VM and cross-VM latency and bandwidth with direct or DERP routing, and the written numerical exercises were outside this validation.
- Parts 2 and 3 require MPI strip and block decomposition, halo exchanges, and byte-for-byte agreement with the serial field. Part 4 covers performance and scaling. Part 5 is optional CUDA acceleration.

Reproduce the evidence with `python3 scripts/validate_serial.py` after a CMake build with tests enabled. The [validation transcript](results/serial-validation.txt) records the automated results.
