#!/usr/bin/env python3
"""Collect raw and aggregate timing measurements from the heat solver."""

import argparse
import csv
import math
from pathlib import Path
import shlex
import statistics
import subprocess


ROOT = Path(__file__).resolve().parents[1]
MODES = ("serial", "1d", "2d")
MPI_RANKS = range(1, 5)
SCALING_CONFIGS = [(mode, ranks) for mode in MODES[1:] for ranks in MPI_RANKS]
CONFIG_FIELDS = (
    "decomp", "P", "case", "N", "steps", "r", "dt", "T",
    "requested_r", "requested_steps", "requested_T", "command",
)
TIMING_FIELDS = ("t_total", "t_comp", "t_comm")
RAW_FIELDS = CONFIG_FIELDS + ("repetition",) + TIMING_FIELDS + ("summary",)
AGGREGATE_FIELDS = CONFIG_FIELDS + (
    "repetitions", "t_total_mean", "t_total_median", "t_total_stddev",
    "t_total_min", "t_comp_mean", "t_comm_mean",
)
SCALING_FIELDS = (
    "decomposition", "P", "N", "steps", "median_time", "serial_baseline",
    "speedup", "efficiency", "efficiency_percent",
)
WEAK_FIELDS = (
    "decomposition", "P", "N", "steps", "T", "owned_interior_cells_per_rank",
    "median_time", "weak_scaling_efficiency",
)


def parse_summary(output):
    lines = output.strip().splitlines()
    if len(lines) != 1:
        raise ValueError("expected exactly one solver summary line")
    fields = {}
    for token in lines[0].split():
        key, separator, value = token.partition("=")
        if not separator or not key or not value or key in fields:
            raise ValueError(f"invalid summary token: {token}")
        fields[key] = value
    try:
        result = {key: fields[key] for key in ("decomp", "case")}
        result.update({key: int(fields[key]) for key in ("P", "N", "steps")})
        result.update({key: float(fields[key]) for key in ("r", "dt", "T") + TIMING_FIELDS})
    except (ValueError, KeyError) as error:
        raise ValueError(f"invalid or missing solver summary field: {error}") from error
    if (
        result["decomp"] not in MODES or result["case"] not in ("1", "2", "V")
        or result["P"] < 1 or result["N"] < 2 or result["steps"] < 0
        or not 0 < result["r"] <= 0.25
        or not all(math.isfinite(result[key]) and result[key] >= 0
                   for key in ("dt", "T") + TIMING_FIELDS)
        or result["dt"] == 0
    ):
        raise ValueError("solver summary contains out-of-range values")
    return result


def aggregate_runs(rows):
    config = {key: rows[0][key] for key in CONFIG_FIELDS}
    if any(any(row[key] != value for key, value in config.items()) for row in rows):
        raise ValueError("solver reported inconsistent configuration across repetitions")
    totals = [row["t_total"] for row in rows]
    return {
        **config,
        "repetitions": len(rows),
        "t_total_mean": statistics.mean(totals),
        "t_total_median": statistics.median(totals),
        "t_total_stddev": statistics.stdev(totals) if len(totals) > 1 else 0.0,
        "t_total_min": min(totals),
        "t_comp_mean": statistics.mean(row["t_comp"] for row in rows),
        "t_comm_mean": statistics.mean(row["t_comm"] for row in rows),
    }


def strong_scaling(aggregates):
    by_config = {}
    for row in aggregates:
        key = (row["decomp"], row["P"])
        if key in by_config:
            raise ValueError(f"duplicate strong-scaling measurement for {key[0]}, P={key[1]}")
        by_config[key] = row

    baseline = by_config.get(("serial", 1))
    if baseline is None:
        raise ValueError("strong scaling requires a serial baseline")
    serial_time = baseline["t_total_median"]
    if not math.isfinite(serial_time) or serial_time <= 0:
        raise ValueError("serial baseline median must be finite and positive")
    if baseline["steps"] <= 0 or baseline["repetitions"] < 1:
        raise ValueError("strong scaling requires positive steps and recorded repetitions")

    comparison_fields = ("case", "N", "r", "steps", "dt", "T", "repetitions")
    results = []
    for mode, ranks in SCALING_CONFIGS:
        row = by_config.get((mode, ranks))
        label = f"{mode}, P={ranks}"
        if row is None:
            raise ValueError(f"missing strong-scaling measurement for {label}")
        for key in comparison_fields:
            if row[key] != baseline[key]:
                raise ValueError(f"{label} differs from the serial baseline in {key}")
        median = row["t_total_median"]
        if not math.isfinite(median) or median <= 0:
            raise ValueError(f"{label} median must be finite and positive")
        speedup = serial_time / median
        efficiency = speedup / ranks
        results.append({
            "decomposition": mode, "P": ranks, "N": row["N"], "steps": row["steps"],
            "median_time": median, "serial_baseline": serial_time, "speedup": speedup,
            "efficiency": efficiency, "efficiency_percent": 100 * efficiency,
        })
    return results


def weak_grid_size(N0, ranks):
    return 1 + round((N0 - 1) * math.sqrt(ranks))


def weak_scaling(aggregates, N0, final_time):
    by_config = {}
    for row in aggregates:
        key = row["decomp"], row["P"]
        if key in by_config:
            raise ValueError(f"duplicate weak-scaling measurement for {key}")
        by_config[key] = row

    results = []
    for mode, ranks in SCALING_CONFIGS:
        reference = by_config.get((mode, 1))
        if reference is None:
            raise ValueError(f"missing weak-scaling reference for {mode}, P=1")
        row = by_config.get((mode, ranks))
        if row is None:
            raise ValueError(f"missing weak-scaling measurement for {mode}, P={ranks}")
        N = weak_grid_size(N0, ranks)
        if (row["N"] != N or row["steps"] <= 0 or row["repetitions"] < 1 or
            not math.isclose(row["T"], final_time, rel_tol=1e-14) or
            row["r"] > row["requested_r"] or
            any(row[key] != reference[key] for key in ("case", "requested_r", "repetitions"))):
            raise ValueError(f"incomparable weak-scaling measurement for {mode}, P={ranks}")
        median = row["t_total_median"]
        if not math.isfinite(median) or median <= 0:
            raise ValueError(f"{mode}, P={ranks} median must be finite and positive")
        results.append({
            "decomposition": mode, "P": ranks, "N": N, "steps": row["steps"], "T": row["T"],
            "owned_interior_cells_per_rank": (N - 1)**2 / ranks,
            "median_time": median, "weak_scaling_efficiency": reference["t_total_median"] / median,
        })
    return results


def write_csv(path, columns, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=columns)
        writer.writeheader()
        writer.writerows(rows)


def run_configuration(args, mode, ranks, N):
    launcher = [] if mode == "serial" else [args.mpirun, *args.mpi_arg, "-np", str(ranks)]
    time_args = ["--steps", str(args.steps)] if args.steps is not None else ["--T", str(args.T)]
    command = [
        *launcher, str(args.heat), "--decomp", mode, "--case", args.case,
        "--N", str(N), "--r", str(args.r), *time_args,
    ]
    rows = []
    for repetition in range(args.repetitions + 1):
        phase = f"repetition {repetition}" if repetition else "warm-up"
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout)
            if result.returncode != 0:
                detail = result.stderr.strip() or result.stdout.strip()
                raise ValueError(f"exit {result.returncode}: {detail}")
            summary = parse_summary(result.stdout)
            if (summary["decomp"], summary["P"], summary["case"], summary["N"]) != (
                mode, ranks, args.case, N
            ):
                raise ValueError("solver summary does not match the requested configuration")
        except (OSError, ValueError, subprocess.TimeoutExpired) as error:
            raise ValueError(f"{mode}, P={ranks}, {phase}: {error}") from error
        if repetition:
            rows.append({
                **summary, "requested_r": args.r, "requested_steps": args.steps,
                "requested_T": args.T, "command": shlex.join(command),
                "repetition": repetition, "summary": result.stdout.strip(),
            })
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--heat", type=Path, default=ROOT / "build" / "heat")
    parser.add_argument("--decomp", nargs="+", choices=MODES)
    parser.add_argument("--ranks", nargs="+", type=int, choices=MPI_RANKS, default=list(MPI_RANKS))
    parser.add_argument("--case", choices=("1", "2", "V"), default="1")
    grid = parser.add_mutually_exclusive_group(required=True)
    grid.add_argument("--N", type=int)
    grid.add_argument("--N0", type=int, help="baseline resolution for weak scaling")
    parser.add_argument("--r", type=float, default=0.24)
    time = parser.add_mutually_exclusive_group(required=True)
    time.add_argument("--steps", type=int)
    time.add_argument("--T", type=float)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--out", type=Path, default=ROOT / "results" / "benchmark.csv")
    scaling_mode = parser.add_mutually_exclusive_group()
    scaling_mode.add_argument("--strong-scaling", action="store_true",
                        help="save speedup and efficiency for both MPI modes at ranks 1–4")
    scaling_mode.add_argument("--weak-scaling", action="store_true",
                              help="scale interior cells at fixed final time for MPI ranks 1–4")
    parser.add_argument("--mpirun", default="mpirun")
    parser.add_argument("--mpi-arg", action="append", default=[], help="repeat for each launcher token")
    parser.add_argument("--timeout", type=float, default=120, help="seconds per solver launch")
    args = parser.parse_args()
    args.heat = args.heat.resolve()
    if not args.heat.is_file():
        parser.error(f"solver executable does not exist: {args.heat}")
    if args.weak_scaling != (args.N0 is not None):
        parser.error("--weak-scaling and --N0 must be used together")
    if args.weak_scaling and (args.T is None or args.T <= 0):
        parser.error("--weak-scaling requires a positive --T")
    if (args.N0 if args.weak_scaling else args.N) < 2 or not 0 < args.r <= 0.25 or args.repetitions < 1:
        parser.error("require N >= 2, 0 < r <= 0.25, and at least one repetition")
    if args.steps is not None and args.steps < 0:
        parser.error("--steps must be non-negative")
    if args.T is not None and (not math.isfinite(args.T) or args.T < 0):
        parser.error("--T must be finite and non-negative")
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("--timeout must be finite and positive")

    modes = args.decomp or (MODES[1:] if args.weak_scaling else MODES)
    configurations = [
        (mode, ranks) for mode in dict.fromkeys(modes)
        for ranks in ([1] if mode == "serial" else dict.fromkeys(args.ranks))
    ]
    if args.weak_scaling and set(configurations) != set(SCALING_CONFIGS):
        parser.error("--weak-scaling requires 1d and 2d with MPI ranks 1–4")
    if args.strong_scaling:
        if set(configurations) != {("serial", 1), *SCALING_CONFIGS}:
            parser.error("--strong-scaling requires serial, 1d, and 2d with MPI ranks 1–4")
        if args.steps == 0 or args.T == 0:
            parser.error("--strong-scaling requires a positive step count or final time")
    workloads = [(mode, ranks, weak_grid_size(args.N0, ranks) if args.weak_scaling else args.N)
                 for mode, ranks in configurations]
    for mode, ranks, N in workloads:
        required_interior = 2 if mode == "2d" and ranks == 4 else ranks
        if required_interior > N - 1:
            parser.error(f"{mode}, P={ranks} requires at least {required_interior} interior cells per axis")

    aggregate_path = args.out.with_name(f"{args.out.stem}_aggregate.csv")
    scaling_path = args.out.with_name(f"{args.out.stem}_strong_scaling.csv")
    weak_path = args.out.with_name(f"{args.out.stem}_weak_scaling.csv")
    raw = []
    aggregate = []
    try:
        for mode, ranks, N in workloads:
            rows = run_configuration(args, mode, ranks, N)
            aggregate.append(aggregate_runs(rows))
            raw.extend(rows)
        scaling = strong_scaling(aggregate) if args.strong_scaling else None
        weak = weak_scaling(aggregate, args.N0, args.T) if args.weak_scaling else None
        write_csv(args.out, RAW_FIELDS, raw)
        write_csv(aggregate_path, AGGREGATE_FIELDS, aggregate)
        if scaling is not None:
            write_csv(scaling_path, SCALING_FIELDS, scaling)
        if weak is not None:
            write_csv(weak_path, WEAK_FIELDS, weak)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Error: {error}\n")
    print(f"Saved {len(raw)} measurements to {args.out} and {len(aggregate)} aggregates to {aggregate_path}")
    if args.strong_scaling:
        print(f"Saved {len(scaling)} strong-scaling results to {scaling_path}")
    if args.weak_scaling:
        print(f"Saved {len(weak)} weak-scaling results to {weak_path}")


if __name__ == "__main__":
    main()
