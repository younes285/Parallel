#!/usr/bin/env python3
"""Validate a built serial solver and regenerate its numerical evidence."""

import argparse
import csv
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

import matplotlib
import numpy as np

from visualize import load_field

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]


def run(*command, cwd=None):
    return subprocess.run(command, cwd=cwd, check=True, text=True,
                          stdout=subprocess.PIPE).stdout


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_fields(heat, directory):
    N = 8
    path = directory / "field.bin"

    def field(case, *time_options, r="0.2"):
        output = run(heat, "--case", case, "--N", str(N), "--r", r,
                     "--decomp", "serial", *time_options, "--out", path)
        require(len(output.splitlines()) == 1, "Expected one summary line")
        summary = dict(token.split("=", 1) for token in output.split())
        require(summary["decomp"] == "serial" and summary["P"] == "1" and
                float(summary["t_comm"]) == 0 and
                float(summary["t_total"]) == float(summary["t_comp"]) >= 0,
                "Incorrect serial timing summary")
        return load_field(path, N), summary

    for case in ("1", "2"):
        previous_center = 0
        for step in range(13):
            grid, _ = field(case, "--steps", str(step))
            np.testing.assert_array_equal(grid[[0, -1], :], 1)
            np.testing.assert_array_equal(grid[1:-1, [0, -1]], int(case == "1"))
            require(np.all((grid >= 0) & (grid <= 1)), f"Case {case}: maximum principle failed")
            if step == 0:
                np.testing.assert_array_equal(grid[1:-1, 1:-1], 0)
            center = grid[N // 2, N // 2]
            require(center >= previous_center, f"Case {case}: center cooled")
            previous_center = center
            np.testing.assert_allclose(grid, grid[::-1, :], atol=1e-14, rtol=0)
            np.testing.assert_allclose(grid, grid[:, ::-1], atol=1e-14, rtol=0)
            if case == "1":
                np.testing.assert_allclose(grid, grid.T, atol=1e-14, rtol=0)
        require(previous_center > 0, f"Case {case}: heat did not reach the center")
        steady, _ = field(case, "--steps", "2000", r="0.25")
        next_field, _ = field(case, "--steps", "2001", r="0.25")
        np.testing.assert_allclose(steady, next_field, atol=1e-13, rtol=0)

    coordinates = np.arange(N + 1) / N
    mode = np.sin(np.pi * coordinates)
    mode[[0, -1]] = 0
    initial = np.outer(mode, mode)
    grid, _ = field("V", "--steps", "0")
    np.testing.assert_allclose(grid, initial, atol=1e-14, rtol=0)
    grid, summary = field("V", "--T", "0.019")
    final_time = float(summary["T"])
    require(abs(final_time - 0.019) < 1e-15 and 0 < float(summary["r"]) <= 0.2,
            "Requested time or stability limit was missed")
    np.testing.assert_array_equal(grid[[0, -1], :], 0)
    np.testing.assert_array_equal(grid[:, [0, -1]], 0)
    error = grid - initial * np.exp(-2 * np.pi**2 * final_time)
    np.testing.assert_allclose(
        [float(summary["err_inf"]), float(summary["err_l2"])],
        [np.max(np.abs(error)), np.linalg.norm(error) / N], atol=1e-14, rtol=0,
    )
    first = path.read_bytes()
    field("V", "--T", "0.019")
    require(first == path.read_bytes(), "Repeated output differed byte for byte")


def check_instability(executable):
    path = ROOT / "results" / "instability.bin"
    run(executable, path)
    grid = load_field(path, 32)
    angle = np.pi * 31 / 32
    amplification = 1 - 4 * 0.2625 + 4 * 0.2625 * np.cos(angle)
    mode = np.sin(angle * np.arange(33))
    mode[[0, -1]] = 0
    np.testing.assert_allclose(grid, (1e-6 * amplification**200) * np.outer(mode, mode),
                               atol=1e-10, rtol=1e-10)
    peak = float(np.max(np.abs(grid)))
    require(peak > 1, "Unstable mode did not grow beyond unit amplitude")
    fig, ax = plt.subplots(figsize=(7, 6), layout="constrained")
    try:
        image = ax.imshow(grid, origin="lower", extent=(0, 1, 0, 1), aspect="equal",
                          interpolation="nearest", cmap="RdBu_r", vmin=-peak, vmax=peak)
        ax.set(xlabel="x", ylabel="y", title="Unstable checkerboard after 200 steps (r = 0.2625)")
        fig.colorbar(image, ax=ax, label="Temperature")
        fig.savefig(ROOT / "plots" / "instability.png", dpi=300)
    finally:
        plt.close(fig)
    return f"Instability: PASS (mode amplification {amplification:.6f}, peak {peak:.6f})"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--ctest", default="ctest", help="CTest command or executable path")
    args = parser.parse_args()
    build = args.build_dir.resolve()
    heat = build / "heat"
    try:
        ctest_output = run(args.ctest, "--output-on-failure", cwd=build)
        require("100% tests passed" in ctest_output, "CTest did not report a passing suite")
        print(ctest_output, end="")
        with TemporaryDirectory() as directory:
            check_fields(heat, Path(directory))
        fields = "Serial fields, CLI summary, and binary reproducibility: PASS"
        print(fields, flush=True)
        convergence = run(sys.executable, ROOT / "scripts" / "convergence.py", "--heat", heat)
        print(convergence, end="")
        with (ROOT / "results" / "convergence.csv").open(newline="") as stream:
            rows = list(csv.DictReader(stream))
        require([int(row["N"]) for row in rows] == [16, 32, 64, 128, 256] and
                all(abs(float(row["p_obs"]) - 2) < 0.02 for row in rows[1:]),
                "Convergence study did not show second-order behavior")
        instability = check_instability(build / "instability_experiment")
        print(instability)
        (ROOT / "results" / "serial-validation.txt").write_text(
            ctest_output + "\n" + fields + "\n\n" + convergence + instability + "\n",
            encoding="utf-8",
        )
    except subprocess.CalledProcessError as error:
        parser.exit(1, f"{error}\n{error.stdout or ''}")
    except (OSError, ValueError, AssertionError) as error:
        parser.exit(1, f"Validation failed: {error}\n")


if __name__ == "__main__":
    main()
