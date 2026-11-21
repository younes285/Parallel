# Step 2.10 interaction

- Date: 2026-11-21 (Asia/Beirut)
- Team member: Student 1
- Tool and model: Codex (GPT-6)

## User


# Files pasted by the user:

## "# Task 2.10 — Final Serial Solver Validation We have completed Steps 2.1 throug…": /home/moyounes/.codex/attachments/8f1bb62e-ae79-437e-8076-c95a5632513d/Pasted text.txt

Pasted text contains the user's request.

## My request:



### Pasted text.txt

# Task 2.10 — Final Serial Solver Validation

We have completed Steps 2.1 through 2.9, including the serial heat solver, initialization, time stepping, CLI, binary output, visualization, and convergence study.

Now perform **Step 2.10: Final Serial Solver Validation**.

## 1. Read the reference documents

Both PDFs are available in the repository.

Review:
- `heat_mpi_project.pdf` — Section 3, Part 1, and Checkpoint C1
- `heat_numerics_background.pdf` — Tasks 6.3, 6.7, 7.2 and Section 7.5

Use these PDFs as the authoritative specifications.

Inspect the existing project code and tests before making changes.

## 2. Objective

Perform a comprehensive validation of the serial solver to confirm that it is numerically correct, stable, reproducible, and ready to serve as the reference for the future MPI implementations.

Do not implement MPI in this step.

## 3. Validate all simulation cases

### Case 1 — Four Hot Edges

Verify:
- All four boundaries remain at temperature 1.
- The interior starts at temperature 0.
- Heat gradually spreads toward the center.
- For stable time steps, all temperatures remain between 0 and 1.
- The temperature field remains symmetric.

### Case 2 — Hot Top and Bottom

Verify:
- Top and bottom boundaries remain at temperature 1.
- Left and right boundaries remain at temperature 0, except the hot corners.
- The interior starts at temperature 0.
- Heat diffuses from the hot edges toward the colder interior.
- Boundary conditions are preserved after every time step.

### Case V — Analytical Verification

Verify:
- The initial condition is correct.
- All boundary temperatures remain exactly zero.
- The calculated error norms are correct.
- The numerical solution approaches the analytical solution as the grid is refined.
- The observed convergence order approaches 2.

## 4. Stability testing

The explicit 2D heat scheme requires:

\[
0<r\leq0.25
\]

Verify that:

1. `r = 0.2` runs successfully.
2. `r = 0.25` is accepted.
3. `r > 0.25` is rejected by the normal CLI.
4. Zero, negative, NaN and infinite values are rejected.

### Required instability experiment — Task 6.7

The background notes also require deliberately exceeding the stability limit by 5%.

Use:

\[
r=1.05\times0.25=0.2625
\]

Because the production solver must reject unstable values, implement this experiment as a separate, clearly isolated test or experimental program.

Requirements:
- Do not weaken the normal CLI stability validation.
- Do not expose unstable execution through ordinary production options.
- Use the existing numerical update rule.
- Run enough steps to observe numerical instability.
- Generate a heatmap illustrating the unstable behavior.
- Explain the observed oscillations or growth and their relationship to the high-frequency modes of the scheme.
- If one test case does not clearly exhibit instability, document this and choose an appropriate initial perturbation or test configuration.

Save the resulting figure and a short explanation for the final report.

## 5. Validate binary output

Verify:

- The file contains exactly `(N+1)²` doubles.
- The file is raw little-endian binary.
- Row-major order is respected.
- All boundary nodes are included.
- Running the same simulation twice produces bit-for-bit identical files.

Use `cmp` or an equivalent binary comparison.

## 6. Validate command-line behavior

Test all supported options:

- `--case`
- `--N`
- `--r`
- `--steps`
- `--T`
- `--out`
- `--decomp serial`
- `--help`

Check invalid inputs, conflicting options, exit codes, and error messages.

Ensure that `--T` reaches the requested final time without violating the stability limit.

## 7. Validate convergence

Repeat the required convergence study:

- Case V
- `T = 0.05`
- `N = 16, 32, 64, 128, 256`
- Requested `r = 0.2`

Generate:
- Convergence results table.
- CSV results file.
- Log-log convergence plot.
- Observed numerical orders.

Confirm that the observed convergence order approaches 2.

## 8. Validate timing

Confirm:

- Only the time-stepping loop is measured.
- Grid initialization is excluded.
- Binary output is excluded.
- Error calculations are excluded.
- The summary uses machine-readable `key=value` formatting.
- Serial execution reports `P=1` and `t_comm=0`.

## 9. Automate the validation

Create a script such as:

`scripts/validate_serial.py`

The script should run the relevant checks automatically and produce a concise validation summary.

Reuse existing tests and scripts whenever possible instead of duplicating their logic.

Save relevant evidence under `results/` and `plots/`.

## 10. Final requirements audit

Compare the current implementation against every mandatory requirement of Part 1 in the project PDF.

Create a checklist showing:

- Requirement
- Implementation status
- Test or evidence
- Remaining issue, if any

Also identify requirements belonging to later project parts without implementing them.

## 11. Build and verification

After validation:

1. Build the complete project using CMake.
2. Run all existing unit tests.
3. Run the serial validation script.
4. Run the convergence study.
5. Perform the instability experiment.
6. Verify all expected output files.
7. Report any failures honestly.

Fix issues directly related to the serial solver if necessary, but avoid unrelated refactoring.

## 12. Restrictions

- Complete **Step 2.10 only**.
- Do not implement MPI.
- Do not implement 1D strips.
- Do not implement 2D blocks.
- Do not implement halo exchanges.
- Do not implement parallel performance experiments.
- Do not start the next project phase.

Follow the AI interaction logging requirements in `AGENTS.md` and `llm_log/`. Record the complete interaction.

## 13. Final response

Provide:

1. Files created or modified.
2. Results of all validation tests.
3. The convergence results table.
4. Results of the instability experiment.
5. Binary output verification results.
6. Any remaining issues.
7. A clear PASS/FAIL assessment of Part 1 readiness.

**Stop after completing Step 2.10 and wait for my next instruction.**

## Assistant

I’ll read the final serial validation checklist and compare it with the project PDFs before running the checks.

## Assistant

The isolated experiment now uses the same stencil as the production solver. The validation run is checking the normal CLI separately, including that it rejects the experiment’s unstable `r = 0.2625`.

## Assistant final response

**PASS — Part 1 serial solver is ready to serve as the MPI reference.** The Release build, all six CTest tests, and the end-to-end validation passed.

- **Cases 1, 2, and V:** Verified initial conditions, fixed boundaries, symmetry, and heating behavior. Independently recalculated both Case V error norms.
- **CLI and timing:** Verified valid and invalid inputs, including rejection of `r = 0.2625`. The requested-time run met its target without exceeding the stability limit. Timing covers only the update loop, with `P=1` and `t_comm=0`.
- **Binary output:** Verified the complete little-endian, row-major field against an exact 200-byte fixture. Repeated runs produced identical bytes.

At `T = 0.05` with requested `r = 0.2`, the observed L2 order approached 2.

| N | h | Steps | Maximum error | L2 error | L2 order |
|---:|---:|---:|---:|---:|---:|
| 16 | 0.06250000 | 64 | 1.6633705e-03 | 8.3168525e-04 | — |
| 32 | 0.03125000 | 256 | 4.1418243e-04 | 2.0709121e-04 | 2.0058 |
| 64 | 0.01562500 | 1,024 | 1.0344249e-04 | 5.1721244e-05 | 2.0014 |
| 128 | 0.00781250 | 4,096 | 2.5854187e-05 | 1.2927094e-05 | 2.0004 |
| 256 | 0.00390625 | 16,384 | 6.4631447e-06 | 3.2315724e-06 | 2.0001 |

The isolated instability experiment amplified a checkerboard perturbation from `1e-6` to **75.575093** in 200 steps. The measured field matched the predicted mode, which reverses sign and grows by about 9.49% each step. [Instability heatmap](/home/moyounes/heat-mpi/plots/instability.png)

Added the validation script, experiment, shared stencil header, and audit. Updated the serial wrapper, CMake, CLI tests, and README. [Audit and explanation](/home/moyounes/heat-mpi/SERIAL_VALIDATION.md) · [Validation results](/home/moyounes/heat-mpi/results/serial-validation.txt) · [Convergence CSV](/home/moyounes/heat-mpi/results/convergence.csv) · [Convergence plot](/home/moyounes/heat-mpi/plots/convergence.png) · [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-21-12-serial-validation.md)

No serial issues surfaced. Full Checkpoint C1 still requires separate cluster evidence and written numerical exercises.

## Files created or modified

- Added `src/heat_stencil.hpp`, `tests/instability_experiment.cpp`, `scripts/validate_serial.py`, `SERIAL_VALIDATION.md`, and this log.
- Updated `src/serial.cpp`, `CMakeLists.txt`, `tests/cli_test.cmake`, and `README.md`.
- Generated or refreshed `results/instability.bin`, `results/serial-validation.txt`, `results/convergence.csv`, `plots/instability.png`, `plots/convergence.png`, and ignored build and Python cache artifacts.
