#!/usr/bin/env python3
"""Plot timing and scaling CSVs produced by benchmark.py."""
import argparse
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

from benchmark import SCALING_FIELDS, WEAK_FIELDS, strong_scaling, weak_grid_size

MODES = {"1d": "MPI 1D", "2d": "MPI 2D"}
AGGREGATE = ("decomp", "P", "case", "N", "steps", "r", "dt", "T",
             "repetitions", "t_total_median", "t_comp_mean", "t_comm_mean")


def read_csv(path, columns):
    try:
        frame = pd.read_csv(path, dtype={"case": str})
        missing = set(columns) - set(frame.columns)
        if missing or frame.empty:
            raise ValueError(f"missing columns: {', '.join(sorted(missing))}" if missing else "no rows")
        frame = frame[list(columns)].copy()
        for column in set(columns) - {"decomp", "decomposition", "case"}:
            values = pd.to_numeric(frame[column], errors="raise")
            if not values.map(math.isfinite).all() or (values < 0).any():
                raise ValueError(f"{column} must contain finite nonnegative numbers")
            if column in {"N", "P", "steps", "repetitions"} and (values % 1 != 0).any():
                raise ValueError(f"{column} must contain integers")
            frame[column] = values
        mode = "decomp" if "decomp" in columns else "decomposition"
        allowed = [*MODES, "serial"] if mode == "decomp" else list(MODES)
        if not frame[mode].isin(allowed).all() or not frame["P"].isin(range(1, 5)).all():
            raise ValueError("invalid decomposition or rank count")
        if (frame["N"] < 2).any() or (frame["steps"] < 1).any():
            raise ValueError("expected N >= 2 and positive steps")
        if "case" in frame and (not frame["case"].isin(["1", "2", "V"]).all() or
                                (frame["r"] <= 0).any() or (frame["r"] > 0.25).any()):
            raise ValueError("invalid simulation case or stability parameter")
        return frame
    except (OSError, ValueError) as error:
        raise ValueError(f"{path}: {error}") from error


def prepare_strong(aggregate, scaling):
    groups = []
    keys = ["N", "steps"]
    aggregate_keys = set(map(tuple, aggregate[keys].to_numpy()))
    scaling_keys = set(map(tuple, scaling[keys].to_numpy()))
    if aggregate_keys != scaling_keys:
        raise ValueError("aggregate and strong-scaling workload keys differ")
    for (N, steps), data in aggregate.groupby(keys, sort=True):
        expected = pd.DataFrame(strong_scaling(data.to_dict("records")))
        actual = scaling[(scaling["N"] == N) & (scaling["steps"] == steps)]
        order = ["decomposition", "P"]
        try:
            pd.testing.assert_frame_equal(
                actual.sort_values(order).reset_index(drop=True),
                expected[list(SCALING_FIELDS)].sort_values(order).reset_index(drop=True),
                check_dtype=False, rtol=1e-12, atol=0,
            )
        except AssertionError as error:
            raise ValueError(f"N={N}, steps={steps}: strong-scaling rows disagree with aggregates") from error
        groups.append((data, actual))
    return groups


def prepare_weak(scaling):
    groups = {}
    used = set()
    for (mode, time), data in scaling.groupby(["decomposition", "T"], sort=True):
        baselines = data[data["P"] == 1]
        if baselines.empty:
            raise ValueError(f"{mode}, T={time}: missing one-rank weak baseline")
        for _, baseline in baselines.iterrows():
            N0 = int(baseline["N"])
            indices = []
            for ranks in range(1, 5):
                N = weak_grid_size(N0, ranks)
                # Timesteps are solver-selected. Compare final time within rounding tolerance.
                candidates = scaling[(scaling["decomposition"] == mode) & (scaling["P"] == ranks) &
                                     (scaling["N"] == N) &
                                     scaling["T"].map(lambda t: math.isclose(t, time, rel_tol=1e-14))]
                if len(candidates) != 1:
                    raise ValueError(f"{mode}, N0={N0}, T={time}, P={ranks}: missing or ambiguous row")
                index = candidates.index[0]
                item = candidates.iloc[0]
                if (index in used or item["median_time"] <= 0 or
                    not math.isclose(item["owned_interior_cells_per_rank"], (N - 1)**2 / ranks) or
                    not math.isclose(item["weak_scaling_efficiency"],
                                     baseline["median_time"] / item["median_time"])):
                    raise ValueError(f"{mode}, N0={N0}, P={ranks}: invalid or overlapping weak workload")
                indices.append(index)
                used.add(index)
            # Match modes by baseline size and actual final time, within rounding tolerance.
            key = next((key for key in groups if key[0] == N0 and
                        math.isclose(key[1], time, rel_tol=1e-14)), (N0, time))
            groups.setdefault(key, []).extend(indices)
    if len(used) != len(scaling):
        raise ValueError("weak CSV contains unmatched measurements")
    for indices in groups.values():
        workload = scaling.loc[indices]
        if len(workload) != 8:
            raise ValueError("each weak workload requires both MPI modes at all four rank counts")
        if (workload.groupby("P")["steps"].nunique() != 1).any():
            raise ValueError("weak-scaling modes report different step counts for the same workload")
    return [(N0, time, scaling.loc[indices]) for (N0, time), indices in sorted(groups.items())]


def save_plot(out, name, title, ylabel, draw, note=None):
    fig, ax = plt.subplots(figsize=(7.5, 4.7), layout="constrained")
    try:
        draw(ax)
        ax.set(title=title, xlabel="MPI process count", ylabel=ylabel, xticks=range(1, 5))
        ax.grid(axis="y", alpha=0.22)
        ax.legend()
        if note:
            fig.supxlabel(note, fontsize=8)
        fig.savefig(out / f"{name}.png", dpi=180)
    finally:
        plt.close(fig)


def curves(ax, data, column, multiplier=1):
    for mode, label in MODES.items():
        rows = data[data["decomposition"] == mode].sort_values("P")
        ax.plot(rows["P"], rows[column] * multiplier, "o-", label=label)


def plot_strong(out, aggregate, scaling, suffix, title):
    def execution(ax):
        curves(ax, scaling, "median_time")
        ax.axhline(scaling["serial_baseline"].iloc[0], ls="--", color="black", label="Serial median")

    def speedup(ax):
        curves(ax, scaling, "speedup")
        ax.plot(range(1, 5), range(1, 5), "k--", label="Ideal S(P) = P")

    def efficiency(ax):
        curves(ax, scaling, "efficiency_percent")
        ax.axhline(100, ls="--", color="black", label="Ideal 100%")

    def components(ax):
        for index, (mode, label) in enumerate(MODES.items()):
            rows = aggregate[aggregate["decomp"] == mode].sort_values("P")
            for part, color in (("comp", f"C{2*index}"), ("comm", f"C{2*index+1}")):
                offset = (2 * index + (part == "comm") - 1.5) * 0.19
                ax.bar(rows["P"] + offset, rows[f"t_{part}_mean"], width=0.18,
                       label=f"{label} {'computation' if part == 'comp' else 'communication'}", color=color)

    for name, heading, ylabel, draw in (
        ("execution-time", "Median execution time", "Loop time (s)", execution),
        ("speedup", "Strong-scaling speedup", "Speedup (×)", speedup),
        ("parallel-efficiency", "Strong-scaling efficiency", "Efficiency (%)", efficiency),
        ("computation-communication", "Mean computation and communication", "Time (s)", components),
    ):
        note = ("Component maxima are reduced independently, so their sum can differ from maximum total time."
                if name == "computation-communication" else None)
        save_plot(out, name + suffix, f"{heading} · {title}", ylabel, draw, note)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--aggregate", type=Path, required=True)
    parser.add_argument("--strong", type=Path, required=True)
    parser.add_argument("--weak", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, default=Path("plots/benchmarks"))
    parser.add_argument("--report", type=Path, default=Path("results/part4/benchmark-summary.md"))
    args = parser.parse_args()
    try:
        strong = prepare_strong(read_csv(args.aggregate, AGGREGATE), read_csv(args.strong, SCALING_FIELDS))
        weak = prepare_weak(read_csv(args.weak, WEAK_FIELDS))
        args.out_dir.mkdir(parents=True, exist_ok=True)
        report = ["# Benchmark summary", "", "## Fixed-grid runs", ""]
        for aggregate, scaling in strong:
            baseline = aggregate[aggregate["decomp"] == "serial"].iloc[0]
            N, steps = int(baseline["N"]), int(baseline["steps"])
            title = f"Case {baseline['case']}, N={N}, {steps} steps"
            plot_strong(args.out_dir, aggregate, scaling, f"-N{N}-steps{steps}", title)
            report.extend([f"### {title}", "",
                           f"Effective r={baseline['r']:.6g}, T={baseline['T']:.6g}, "
                           f"{int(baseline['repetitions'])} repetitions per configuration. "
                           f"Serial median was {baseline['t_total_median']:.6g} s.", "",
                           "| Mode | P | Median loop (s) | Speedup | Efficiency (%) | Mean comp (s) | Mean comm (s) |",
                           "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"])
            for row in scaling.sort_values(["decomposition", "P"]).itertuples():
                timing = aggregate[(aggregate["decomp"] == row.decomposition) &
                                   (aggregate["P"] == row.P)].iloc[0]
                report.append(f"| {MODES[row.decomposition]} | {row.P} | {row.median_time:.6g} | "
                              f"{row.speedup:.4g} | {row.efficiency_percent:.4g} | "
                              f"{timing['t_comp_mean']:.6g} | {timing['t_comm_mean']:.6g} |")
            fastest = scaling.loc[scaling["median_time"].idxmin()]
            highest_comm = aggregate[aggregate["decomp"].isin(MODES)].sort_values("t_comm_mean").iloc[-1]
            report.extend(["",
                           f"The lowest MPI median was {fastest['median_time']:.6g} s "
                           f"({MODES[fastest['decomposition']]}, P={fastest['P']}), "
                           f"giving {fastest['speedup']:.4g}× speedup against serial. "
                           f"The highest mean communication time was {highest_comm['t_comm_mean']:.6g} s "
                           f"({MODES[highest_comm['decomp']]}, P={highest_comm['P']}).", ""])
        report.extend(["## Weak scaling at fixed physical time", ""])
        for index, (N0, time, data) in enumerate(weak, 1):
            def draw(ax):
                curves(ax, data, "weak_scaling_efficiency", multiplier=100)
                ax.axhline(100, ls="--", color="black", label="Equal elapsed time")
            save_plot(args.out_dir, f"weak-efficiency-N{N0}-{index}",
                      f"Weak efficiency at fixed physical time · N₀={N0}, T={time:.6g}",
                      "Efficiency (%)", draw,
                      "Finer grids take more steps at fixed T, increasing total work per rank.")
            report.extend([f"### N₀={N0}, T={time:.6g}", "",
                           "| Mode | P | N | Steps | Cells/rank (avg.) | Median loop (s) | Efficiency (%) |",
                           "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"])
            for row in data.sort_values(["decomposition", "P"]).itertuples():
                report.append(f"| {MODES[row.decomposition]} | {row.P} | {row.N} | {row.steps} | "
                              f"{row.owned_interior_cells_per_rank:.4g} | {row.median_time:.6g} | "
                              f"{100 * row.weak_scaling_efficiency:.4g} |")
            report.extend(["", f"The supplied runs used {data['steps'].min()}–{data['steps'].max()} "
                           f"steps. At four ranks, observed efficiencies were "
                           + " and ".join(f"{100 * row.weak_scaling_efficiency:.4g}% ({MODES[row.decomposition]})"
                                          for row in data[data["P"] == 4].itertuples()) + ".", ""])
        report.extend(["## Methodological limitations", "",
                       "- MPI reduces computation, communication, and total time independently. "
                       "Their maxima can come from different ranks, so the component times can sum "
                       "to a value different from the maximum total time.",
                       "- Weak efficiency includes additional timestep work as the grid is refined. "
                       "A value of 100% means equal loop time relative to the same mode at one rank.",
                       "- These observations describe only the supplied runs. Small grids and few "
                       "repetitions provide smoke-test evidence and limited evidence for scalability.", ""])
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text("\n".join(report), encoding="utf-8")
    except (OSError, ValueError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
