import sys
import tempfile
import unittest
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from benchmark import SCALING_CONFIGS, strong_scaling, weak_grid_size
from plot_benchmarks import prepare_strong, prepare_weak, read_csv


class PlotBenchmarkTest(unittest.TestCase):
    def test_distinct_strong_workloads_and_mismatched_case(self):
        aggregates = []
        scaling = []
        for N in (16, 32):
            rows = [
                dict(decomp=mode, P=p, case="1", N=N, steps=20, r=0.2,
                     dt=0.2 / N**2, T=4 / N**2, repetitions=2, t_total_median=1 / p)
                for mode, p in [("serial", 1), *SCALING_CONFIGS]
            ]
            aggregates.extend(rows)
            scaling.extend(strong_scaling(rows))
        aggregate = pd.DataFrame(aggregates)
        strong = pd.DataFrame(scaling)
        self.assertEqual(len(prepare_strong(aggregate, strong)), 2)
        aggregate.loc[1, "case"] = "2"
        with self.assertRaisesRegex(ValueError, "differs.*case"):
            prepare_strong(aggregate, strong)

    def test_weak_workloads_and_ambiguity(self):
        rows = []
        for N0 in (17, 33):
            for mode, p in SCALING_CONFIGS:
                N = weak_grid_size(N0, p)
                rows.append(dict(decomposition=mode, P=p, N=N, steps=N, T=0.01,
                                 owned_interior_cells_per_rank=(N - 1)**2 / p,
                                 median_time=float(p), weak_scaling_efficiency=1 / p))
        data = pd.DataFrame(rows)
        self.assertEqual([N0 for N0, _, _ in prepare_weak(data)], [17, 33])
        with self.assertRaisesRegex(ValueError, "missing or ambiguous"):
            prepare_weak(pd.concat([data, data.iloc[:1]], ignore_index=True))

    def test_missing_and_malformed_csv(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "input.csv"
            with self.assertRaisesRegex(ValueError, "input.csv"):
                read_csv(path, ("N",))
            path.write_text("N\ninvalid\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "input.csv"):
                read_csv(path, ("N",))
            with self.assertRaisesRegex(ValueError, "missing columns"):
                read_csv(path, ("P",))


if __name__ == "__main__":
    unittest.main()
