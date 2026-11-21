#!/usr/bin/env python3
"""Run the Case V grid refinement study and save its table and plot."""

import argparse
import csv
import math
from pathlib import Path
import subprocess

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parents[1]
GRID_SIZES = (16, 32, 64, 128, 256)
COLUMNS = ("N", "h", "steps", "err_inf", "err_l2", "p_obs")


def run_case(heat, intervals):
    command = [
        str(heat), "--case", "V", "--N", str(intervals),
        "--r", "0.2", "--T", "0.05", "--decomp", "serial",
    ]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"heat failed for N={intervals} (exit {result.returncode}): {detail}")
    try:
        summary = dict(field.split("=", 1) for field in result.stdout.split())
        steps = int(summary["steps"])
        errors = (float(summary["err_inf"]), float(summary["err_l2"]))
        if (
            summary["case"] != "V"
            or int(summary["N"]) != intervals
            or not math.isclose(float(summary["T"]), 0.05, rel_tol=1e-14)
            or steps <= 0
            or not all(math.isfinite(error) and error > 0 for error in errors)
        ):
            raise ValueError("unexpected simulation results")
    except (KeyError, ValueError) as error:
        raise ValueError(f"Invalid heat summary for N={intervals}: {result.stdout.strip()}") from error
    return steps, *errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--heat", type=Path, default=ROOT / "build" / "heat",
                        help="path to the compiled solver (default: build/heat)")
    args = parser.parse_args()

    try:
        rows = []
        for intervals in GRID_SIZES:
            steps, err_inf, err_l2 = run_case(args.heat.resolve(), intervals)
            order = math.log2(rows[-1][4] / err_l2) if rows else None
            rows.append((intervals, 1 / intervals, steps, err_inf, err_l2, order))

        csv_path = ROOT / "results" / "convergence.csv"
        with csv_path.open("w", newline="", encoding="utf-8") as output:
            writer = csv.writer(output)
            writer.writerow(COLUMNS)
            writer.writerows(rows)

        plot_path = ROOT / "plots" / "convergence.png"
        fig, ax = plt.subplots(figsize=(7, 5), layout="constrained")
        try:
            spacing = [row[1] for row in rows]
            errors = [row[4] for row in rows]
            ax.loglog(spacing, errors, "o-", label="Case V")
            ax.loglog(spacing, [errors[0] * (h / spacing[0]) ** 2 for h in spacing],
                      "--", label="Second-order reference")
            ax.set(xlabel="Grid spacing h", ylabel="Discrete L2 error",
                   title="Serial heat solver convergence at T = 0.05")
            ax.grid(True, which="both", alpha=0.25)
            ax.legend()
            fig.savefig(plot_path, dpi=300)
        finally:
            plt.close(fig)
    except (OSError, RuntimeError, ValueError) as error:
        parser.exit(1, f"Error: {error}\n")

    print(f"{'N':>4} {'h':>12} {'steps':>7} {'err_inf':>14} {'err_l2':>14} {'p_obs':>8}")
    for intervals, h, steps, err_inf, err_l2, order in rows:
        order_text = "—" if order is None else f"{order:.4f}"
        print(f"{intervals:4d} {h:12.8f} {steps:7d} {err_inf:14.7e} {err_l2:14.7e} {order_text:>8}")
    print(f"CSV: {csv_path}\nPlot: {plot_path}")


if __name__ == "__main__":
    main()
