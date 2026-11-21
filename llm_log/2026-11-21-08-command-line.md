# Step 2.6 interaction

- Date: 2026-11-21 (Asia/Beirut)
- Team member: Student 1
- Tool and model: Codex (GPT-6)

## User


# Files pasted by the user:

## "# Task 2.6 — Implement Command-Line Arguments We have completed: - Step 2.1: Pr…": /home/moyounes/.codex/attachments/6d678e6d-e259-48ea-b6fa-4bd4647fe1fe/Pasted text.txt

Pasted text contains the user's request.

## My request:



### Pasted text.txt

# Task 2.6 — Implement Command-Line Arguments

We have completed:
- Step 2.1: Project structure
- Step 2.2: 2D temperature grid
- Step 2.3: Initial and boundary conditions
- Step 2.4: Heat equation update algorithm
- Step 2.5: Serial simulation loop

Now implement **Step 2.6: Command-Line Interface (CLI)**.

## 1. Read the reference documents

The reference PDFs are available in the repository.

Review:
- `heat_mpi_project.pdf` — Section 3: Program Interface
- `heat_mpi_project.pdf` — Part 1: Serial Solver
- `heat_numerics_background.pdf` — Sections 6.3 and 8.3

Inspect the existing code before making changes. Follow the reference documents if any specifications differ from this prompt.

## 2. Objective

Implement command-line argument parsing for the existing `heat` executable.

Users should be able to configure the simulation without modifying the source code.

Example:

```bash
./heat --case 1 --N 128 --r 0.2 --steps 500 --decomp serial
```

## 3. Required command-line options

| Option | Description | Example |
|---|---|---|
| `--case` | Simulation case: 1, 2, or V | `--case 2` |
| `--N` | Grid intervals per direction | `--N 128` |
| `--r` | Stability parameter | `--r 0.2` |
| `--steps` | Number of time steps | `--steps 500` |
| `--T` | Requested final simulation time | `--T 0.05` |
| `--decomp` | Decomposition type | `--decomp serial` |
| `--out` | Optional binary output filename | `--out results/u.bin` |
| `--help` | Display usage instructions | `--help` |

For this step, only `serial` is supported as a decomposition type.

Reject `strips` and `blocks` with a clear message explaining that they are not implemented yet.

## 4. Time-step calculation

The project uses:

\[
h=\frac{1}{N},\qquad \Delta t=rh^2
\]

Support two modes.

### Mode A — Fixed number of steps

When `--steps` is provided:

- Use the requested value of `r`.
- Calculate `dt = r / (N*N)`.
- Calculate the final time as `T = steps * dt`.

### Mode B — Requested final time

When `--T` is provided:

- Interpret `r` as the maximum permitted stability parameter.
- Choose an integer number of steps using `ceil(T / dt_max)`.
- Calculate `dt = T / steps`.
- Calculate the effective stability parameter `r_effective = dt / (h*h)`.
- Ensure `r_effective <= r_requested`, allowing only appropriate floating-point rounding tolerance.
- Use `r_effective` for the numerical updates.
- Ensure the simulation reaches the requested final time.

The implementation must handle floating-point rounding carefully and must not silently violate the stability condition.

Require exactly one of `--steps` or `--T`.

## 5. Validation

Reject invalid inputs, including:

- Missing required arguments.
- Unknown arguments.
- Unsupported simulation cases.
- `N <= 1`.
- `r <= 0` or `r > 0.25`.
- NaN or infinite numerical values.
- Negative step counts.
- Negative or non-finite final times.
- Both `--steps` and `--T` supplied together.
- Neither `--steps` nor `--T` supplied.
- Unsupported decomposition modes.
- Invalid numerical formats or integer overflow.

Display a helpful error message and return a non-zero exit code.

Use sensible validation for grid allocation sizes and avoid integer overflow when computing `N*N`.

## 6. Implementation

- Implement a simple parser using standard C++17.
- Avoid introducing unnecessary third-party libraries.
- Keep parsing separate from numerical computation.
- Reuse the existing `run_serial()` function.
- Modify `main.cpp` to parse arguments and run the requested simulation.
- Add a `--help` option documenting the interface.

For now, `--out` should be accepted and its filename retained in the configuration, but actual binary file writing will be implemented in the next step.

Do not pretend an output file has been created.

## 7. Program output

After successful execution, print a summary in `key=value` format.

Include:

- `decomp=serial`
- `P=1`
- `case=...`
- `N=...`
- `steps=...`
- `r=...`
- `dt=...`
- `T=...`
- `t_total=...`
- `t_comp=...`
- `t_comm=0`

Use the actual measured time-stepping duration. Do not include initialization, argument parsing, or output writing in the measured solver time.

For Case V, numerical error reporting will be added in a later verification step.

## 8. Testing

Add tests covering:

1. A valid execution using `--steps`.
2. A valid execution using `--T`.
3. Correct time-step calculation.
4. Correct adjustment of `r` when `--T` is specified.
5. Rejection of an unstable `r`.
6. Rejection of invalid N values.
7. Rejection of unknown arguments.
8. Rejection of conflicting `--steps` and `--T`.
9. Rejection of unsupported decomposition types.
10. Correct behavior of `--help`.
11. Successful execution of all three simulation cases.
12. Existing tests continue passing.

## 9. Build and verification

After implementing:

1. Compile using CMake.
2. Run all tests.
3. Execute several valid CLI commands.
4. Execute several invalid CLI commands.
5. Verify error messages and exit codes.
6. Update README.md with the available arguments and examples.

## 10. Restrictions

- Implement **Step 2.6 only**.
- Do not implement binary file writing yet.
- Do not implement heatmap visualization.
- Do not implement convergence studies.
- Do not implement MPI or domain decomposition.
- Do not introduce unnecessary dependencies.
- Preserve all existing working functionality.
- Do not start Step 2.7.

Follow the mandatory AI logging requirements in `AGENTS.md` and `llm_log/`. Record this complete interaction.

## 11. Final response

Briefly report:

1. Files created or modified.
2. Supported command-line arguments.
3. How `--steps` and `--T` differ.
4. Build and test results.
5. Example commands to run the program.
6. Any remaining limitations.

**Stop after completing Step 2.6 and wait for my next instruction.**

## Assistant

I’ll read the Step 2.6 attachment before making changes.

## Assistant

I’ll keep parsing separate from the solver, default `--decomp` to `serial`, and treat `--T 0` as a zero-step run. Positive final times will use a rounded-up step count and a stability parameter capped at the requested value.

## Assistant final response

Step 2.6 is complete. `heat` supports `--case`, `--N`, `--r`, `--steps`, `--T`, `--decomp`, `--out`, and `--help`.

`--steps` uses the supplied `r` for a fixed step count. `--T` rounds the step count up as needed and uses an effective `r` at or below the requested limit. The summary reports the simulated time and measured loop duration.

```sh
./build/heat --case 1 --N 128 --r 0.2 --steps 500
./build/heat --case V --N 64 --r 0.25 --T 0.05
```

The Release build and all five CTest tests passed. Manual runs succeeded for Cases 1, 2, and V. Unstable `r` and unsupported decomposition modes returned exit code 1 with clear errors.

Added `src/cli.hpp`, `src/cli.cpp`, and `tests/cli_test.cmake`. Updated `src/main.cpp`, `CMakeLists.txt`, and `README.md). [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-21-08-command-line.md)

Serial is the only supported decomposition. `--out` retains the filename for a later step. Binary output and Case V error reporting are pending.

## Files created or modified

- Created `src/cli.hpp`, `src/cli.cpp`, `tests/cli_test.cmake`, and this log.
- Modified `src/main.cpp`, `CMakeLists.txt`, and `README.md`.
- Regenerated ignored build artifacts in `build/`.
