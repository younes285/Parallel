import csv
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from benchmark import AGGREGATE_FIELDS, CONFIG_FIELDS, aggregate_runs, parse_summary, write_csv
from benchmark import SCALING_CONFIGS, run_configuration, strong_scaling, weak_grid_size, weak_scaling


SUMMARY = (
    "decomp=2d P=4 case=V N=32 steps=20 r=0.2 dt=0.0001953125 T=0.00390625 "
    "t_total=3e-4 t_comp=1e-4 t_comm=2e-4 err_inf=0.01 err_l2=0.005"
)


class BenchmarkTest(unittest.TestCase):
    def test_weak_grid_sizes(self):
        self.assertEqual([weak_grid_size(17, p) for p in range(1, 5)], [17, 24, 29, 33])

    def test_weak_timestep_selection_uses_solver(self):
        args = SimpleNamespace(mpirun="mpirun", mpi_arg=[], steps=None, T=0.05,
                               heat="heat", case="V", r=0.2, repetitions=1, timeout=10)
        for N, steps in ((17, 73), (33, 273)):
            output = (
                f"decomp=2d P=4 case=V N={N} steps={steps} r=0.19 dt={args.T / steps} "
                "T=0.05 t_total=0.1 t_comp=0.08 t_comm=0.02"
            )
            with patch("benchmark.subprocess.run", return_value=SimpleNamespace(
                returncode=0, stdout=output, stderr=""
            )) as run:
                row, = run_configuration(args, "2d", 4, N)
            self.assertEqual(run.call_count, 2)
            self.assertEqual(run.call_args.args[0][-6:], ["--N", str(N), "--r", "0.2", "--T", "0.05"])
            self.assertEqual((row["steps"], row["T"], row["requested_steps"]), (steps, 0.05, None))

    def test_weak_efficiency_and_missing_reference(self):
        rows = [
            dict(parse_summary(SUMMARY), decomp=mode, P=p, N=weak_grid_size(17, p),
                 T=0.05, requested_r=0.2, repetitions=2,
                 t_total_median=(4 if mode == "1d" else 8) * p)
            for mode, p in SCALING_CONFIGS
        ]
        output = weak_scaling(rows, 17, 0.05)
        self.assertEqual([row["weak_scaling_efficiency"] for row in output], [1, 0.5, 1/3, 0.25] * 2)
        self.assertEqual(output[-1]["owned_interior_cells_per_rank"], 256)
        with self.assertRaisesRegex(ValueError, "missing weak-scaling reference"):
            weak_scaling(rows[1:], 17, 0.05)
        with self.assertRaisesRegex(ValueError, "incomparable"):
            weak_scaling(rows[:-1] + [dict(rows[-1], T=0.04)], 17, 0.05)

    def scaling_input(self):
        config = dict(parse_summary(SUMMARY), repetitions=2)
        return [dict(config, decomp="serial", P=1, t_total_median=12.0)] + [
            dict(config, decomp=mode, P=ranks, t_total_median=6.0)
            for mode, ranks in SCALING_CONFIGS
        ]

    def test_strong_scaling(self):
        rows = strong_scaling(self.scaling_input())
        self.assertEqual([(row["decomposition"], row["P"]) for row in rows], SCALING_CONFIGS)
        self.assertEqual(rows[-1], {
            "decomposition": "2d", "P": 4, "N": 32, "steps": 20,
            "median_time": 6.0, "serial_baseline": 12.0, "speedup": 2.0,
            "efficiency": 0.5, "efficiency_percent": 50.0,
        })
        self.assertEqual(rows[0]["efficiency_percent"], 200.0)

    def test_reject_incomplete_or_incomparable_scaling(self):
        rows = self.scaling_input()
        with self.assertRaisesRegex(ValueError, "serial baseline"):
            strong_scaling(rows[1:])
        with self.assertRaisesRegex(ValueError, "missing.*2d, P=4"):
            strong_scaling(rows[:-1])
        for key, value in (("case", "2"), ("N", 64), ("r", 0.1), ("steps", 10),
                           ("dt", 0.1), ("T", 0.1), ("repetitions", 1)):
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, f"differs.*{key}"):
                strong_scaling(rows[:-1] + [dict(rows[-1], **{key: value})])
        with self.assertRaisesRegex(ValueError, "median must be finite and positive"):
            strong_scaling(rows[:-1] + [dict(rows[-1], t_total_median=0)])

    def test_parse_summary(self):
        row = parse_summary(SUMMARY + "\n")
        self.assertEqual((row["decomp"], row["P"], row["case"], row["steps"]), ("2d", 4, "V", 20))
        self.assertEqual(row["t_total"], 0.0003)
        self.assertEqual(row["t_comm"], 0.0002)

    def test_reject_invalid_summary(self):
        for output in (
            "", SUMMARY + "\n" + SUMMARY, SUMMARY + " P=4",
            SUMMARY.replace("t_total=3e-4", "t_total=nan"),
            SUMMARY.replace("t_comp=1e-4", ""),
        ):
            with self.subTest(output=output), self.assertRaises(ValueError):
                parse_summary(output)

    def test_aggregate_csv(self):
        config = parse_summary(SUMMARY)
        config.update(requested_r=0.2, requested_steps=20, requested_T=None, command="mpirun ...")
        rows = [dict(config, t_total=t, t_comp=t / 2, t_comm=t / 4) for t in (2, 4, 6)]
        result = aggregate_runs(rows)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "aggregate.csv"
            write_csv(path, AGGREGATE_FIELDS, [result])
            with path.open(newline="", encoding="utf-8") as stream:
                reader = csv.DictReader(stream)
                self.assertEqual(tuple(reader.fieldnames), AGGREGATE_FIELDS)
                row, = reader
        self.assertEqual({key: row[key] for key in CONFIG_FIELDS if config[key] is not None},
                         {key: str(config[key]) for key in CONFIG_FIELDS if config[key] is not None})
        keys = ("t_total_mean", "t_total_median", "t_total_stddev", "t_total_min",
                "t_comp_mean", "t_comm_mean", "repetitions")
        self.assertEqual(tuple(float(row[key]) for key in keys), (4, 4, 2, 2, 2, 1, 3))
        self.assertEqual(aggregate_runs(rows[:1])["t_total_stddev"], 0)
        with self.assertRaises(ValueError):
            aggregate_runs([rows[0], dict(rows[1], N=64)])


if __name__ == "__main__":
    unittest.main()
