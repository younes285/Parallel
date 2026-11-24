# Instability trace

2026-11-24T02:43:06+02:00 · Student 1 · Codex (GPT-6)

## User

Implement a separate numerical instability experiment for Part 1 of the heat diffusion project.\
\
Requirements:\
\- Review the existing serial solver and the project PDFs.\
\- Keep the production solver's stability validation (\`0 < r <= 0.25\`) unchanged.\
\- Create a separate experiment executable that allows \`r = 0.2625\`.\
\- Reuse the existing grid, initialization, and heat-step logic where possible.\
\- Use an initial condition containing a small high-frequency perturbation so unstable modes can be observed.\
\- Record the maximum absolute temperature at selected iterations to demonstrate instability.\
\- Write experiment results to a CSV file under \`results/\`.\
\- Add the experiment executable to CMake.\
\- Follow the project's \`llm_log/\` requirements.\
\- Do not modify the MPI implementations or other unrelated components.\
\
Compile and test the experiment, then report the exact command used to run it and stop.


## Assistant

I’ll review the existing instability experiment against the PDFs and add the iteration-by-iteration evidence it’s missing.

## Assistant final response

Extended the existing CMake-built instability experiment to record peak absolute temperature every 20 steps at `r = 0.2625`. Updated the experiment source, serial validation script, and experiment documentation.

Exact command used:

```sh
./build-instability-review/instability_experiment results/instability.bin results/instability.csv
```

The Release build and all six CTest tests passed, including production rejection of `r = 0.2625`. All 11 CSV samples matched predicted growth. The peak rose from `1e-6` to `75.575093` by step 200 and agreed with the final binary field. [CSV](/home/moyounes/heat-mpi/results/instability.csv) · [AI log](/home/moyounes/heat-mpi/llm_log/2026-11-24-16-instability-trace.md)

## Files and verification

Updated tests/instability_experiment.cpp (lines 4–59), scripts/validate_serial.py (line 93), SERIAL_VALIDATION.md (lines 28–34). Created this log and results/instability.csv (lines 1–12). Regenerated results/instability.bin and built under build-instability-review/. All six CTest tests and the independent trace check passed.
