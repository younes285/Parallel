## User

\# Task 2.3 — Implement Initial and Boundary Conditions\
\
We have completed:\
\- Step 2.1: Project Structure Setup\
\- Step 2.2: 2D Temperature Grid\
\
Now implement \*\*Step 2.3: Initial and Boundary Conditions\*\*.\
\
Follow the specifications in \`heat_mpi_project.pdf\`, especially Section 3, and reuse the existing \`Grid\` class.\
\
\## 1. Objective\
\
Implement the initialization of the temperature grid for three simulation cases.\
\
Do not implement heat diffusion or time stepping yet.\
\
\## 2. Case 1 — Four Hot Edges\
\
The entire plate initially has temperature 0 in the interior.\
\
All four boundaries are fixed at temperature 1.\
\
\- Interior: \`u = 0\`\
\- Top edge: \`u = 1\`\
\- Bottom edge: \`u = 1\`\
\- Left edge: \`u = 1\`\
\- Right edge: \`u = 1\`\
\- All corners: \`u = 1\`\
\
\## 3. Case 2 — Hot Top and Bottom\
\
The plate initially has temperature 0 in the interior.\
\
\- Interior: \`u = 0\`\
\- Top edge: \`u = 1\`\
\- Bottom edge: \`u = 1\`\
\- Left edge: \`u = 0\`\
\- Right edge: \`u = 0\`\
\- All four corners: \`u = 1\`\
\
Be careful: corners must remain hot even though they also belong to the left and right boundaries.\
\
\## 4. Case V — Verification\
\
Use the analytical initial condition:\
\
\`\`\`cpp\
u(x, y, 0) = sin(pi \* x) \* sin(pi \* y);\
\`\`\`\
\
With:\
\
\`\`\`text\
x = i / N\
y = j / N\
\`\`\`\
\
All four boundary edges must be exactly zero.\
\
The exact solution for later verification is:\
\
\`\`\`text\
u(x,y,t) = sin(pi\*x) \* sin(pi\*y) \* exp(-2\*pi\*pi\*t)\
\`\`\`\
\
Do not implement error calculations yet.\
\
\## 5. Implementation\
\
\- Add a simple reusable initialization function, such as \`initialize_grid(Grid& grid, SimulationCase simulation_case)\`.\
\- Introduce a clear enum for the three simulation cases.\
\- Implement boundary initialization cleanly.\
\- Ensure the initialization works for different values of N.\
\- Prefer a separate header/source file for simulation cases if appropriate.\
\- Avoid unnecessary abstractions.\
\
The implementation should eventually be reusable in the MPI solver.\
\
\## 6. Testing\
\
Add tests for:\
\
1\. Case 1: all boundary nodes are 1, interior nodes are 0.\
2\. Case 2: top/bottom nodes are 1, left/right non-corner nodes are 0, and corners are 1.\
3\. Case V: all boundary nodes are exactly 0.\
4\. Case V: the center node has temperature approximately 1 for even N.\
5\. Grid initialization for N = 4, N = 8, and N = 16.\
6\. Reinitialization correctly replaces previous grid values.\
\
\## 7. Build and Verification\
\
\- Update CMake if necessary.\
\- Compile the project.\
\- Execute all tests.\
\- Ensure the existing executable still works.\
\- Briefly explain how each simulation case initializes the grid.\
\
\## Restrictions\
\
\- Implement \*\*Step 2.3 only\*\*.\
\- Do not implement the heat update formula.\
\- Do not implement time stepping.\
\- Do not implement MPI.\
\- Do not implement ghost cells.\
\- Do not implement command-line parsing.\
\- Do not start Step 2.4.\
\- Preserve existing project code.\
\
Follow the AI logging requirements in \`AGENTS.md\` and \`llm_log/\`.\
\
At the end, report:\
1\. Files created or modified.\
2\. How the three cases are initialized.\
3\. Test results.\
4\. Any remaining issues.\
\
\*\*Stop after Step 2.3 and wait for my next instruction.\*\*


## Assistant

I’ll look for the project PDF and review the grid API before adding the three initialization cases.

## Assistant final response

Step 2.3 is complete. The reusable `initialize_grid` function follows Section 3 of the project PDF and resets the grid on every call.

- **Case 1** sets all four edges to 1 and the interior to 0.
- **Case 2** sets the top and bottom edges, including all corners, to 1. The rest of the grid is 0.
- **Case V** sets the interior to `sin(πx) * sin(πy)` and all boundary nodes to exactly 0.

Added `src/simulation_case.hpp`, `src/simulation_case.cpp`, and `tests/simulation_case_test.cpp`. Updated `CMakeLists.txt` and saved this exchange in the [AI interaction log](/home/moyounes/heat-mpi/llm_log/2026-11-21-05-initial-boundary-conditions.md).

The build succeeded, both CTest tests passed, and `heat` exited with code 0. The initialization tests covered `N = 4, 8, 16` and reuse of a populated grid. No remaining issues surfaced.

## Files created or modified

- Created `src/simulation_case.hpp`, `src/simulation_case.cpp`, `tests/simulation_case_test.cpp`, and this log.
- Modified `CMakeLists.txt`.
- Regenerated local build artifacts under the ignored `build/` directory.
- Extracted the project PDF text to `/tmp/heat-mpi-project-spec.txt` to verify Section 3.
