#!/usr/bin/env python3
"""Plot a headerless temperature field from the heat solver."""

import argparse
import os
from pathlib import Path

import matplotlib
import numpy as np


CASE_TITLES = {
    "1": "Case 1 — Four hot edges",
    "2": "Case 2 — Hot top and bottom",
    "V": "Case V — Verification field",
}


def load_field(path, intervals):
    side = intervals + 1
    dtype = np.dtype("<f8")
    expected_bytes = side * side * dtype.itemsize
    with path.open("rb") as field:
        actual_bytes = os.fstat(field.fileno()).st_size
        if actual_bytes != expected_bytes:
            raise ValueError(
                f"{path}: expected {expected_bytes} bytes for N={intervals}, "
                f"found {actual_bytes}"
            )
        data = np.fromfile(field, dtype=dtype)
    if data.size != side * side:
        raise ValueError(f"{path}: field size changed while reading")
    return data.reshape((side, side))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="binary field filename")
    parser.add_argument("--N", required=True, type=int, help="grid intervals per axis")
    parser.add_argument("--case", choices=CASE_TITLES, help="optional case label")
    parser.add_argument("--save", type=Path, help="PNG output filename")
    parser.add_argument("--show", action="store_true", help="display the heatmap interactively")
    args = parser.parse_args()

    if args.N < 1:
        parser.error("--N must be positive")
    if args.save is None and not args.show:
        parser.error("specify --save, --show, or both")
    if args.save is not None and args.save.suffix.lower() != ".png":
        parser.error("--save requires a .png filename")

    if not args.show:
        matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    try:
        grid = load_field(args.input, args.N)
        fig, ax = plt.subplots(figsize=(7, 6), layout="constrained")
        try:
            image = ax.imshow(
                grid, origin="lower", extent=[0, 1, 0, 1], aspect="equal",
                interpolation="nearest", cmap="inferno", vmin=0, vmax=1,
            )
            ax.set(xlabel="x", ylabel="y",
                   title=CASE_TITLES.get(args.case, "Heat diffusion — Temperature field"))
            fig.colorbar(image, ax=ax, label="Temperature")
            if args.save is not None:
                fig.savefig(args.save, dpi=300, format="png")
            if args.show:
                plt.show()
        finally:
            plt.close(fig)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()
