#!/usr/bin/env python3

"""
MultiCodeVision Batch Report Generator

This script runs the QR decoding application on every image in a folder
and generates experiment reports.

Outputs:

1) console_report.txt  -> Exact console output from the decoder

Example:

python scripts/export_stats.py \
    --app ../build/app \
    --images ../datasets/mixed \
    --out reports
"""

import argparse
import subprocess
from pathlib import Path
from datetime import datetime

# -------------------------------------------------
# Run decoder
# -------------------------------------------------

def run_app_batch(app_path: str, image_dir: Path) -> str:

    result = subprocess.run(
        [app_path, str(image_dir), "--batch"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )

    return result.stdout

# -------------------------------------------------
# Main
# -------------------------------------------------

def main():

    parser = argparse.ArgumentParser()

    parser.add_argument("--app", required=True, help="Path to compiled decoder app")
    parser.add_argument("--images", required=True, help="Directory containing images")
    parser.add_argument("--out", required=True, help="Output directory for reports")

    args = parser.parse_args()

    app_path = args.app
    image_dir = Path(args.images)
    out_dir = Path(args.out)

    out_dir.mkdir(parents=True, exist_ok=True)

    # More readable timestamp
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    dataset_name = image_dir.name
    console_report_path = out_dir / f"report_{dataset_name}_{timestamp}.txt"

    print("......Running Batch ......")

    output = run_app_batch(app_path, image_dir)

    console_outputs = [output]

    # -----------------------------
    # Write exact console report
    # -----------------------------

    with open(console_report_path, "w") as f:

        for block in console_outputs:
            f.write(block)

            if not block.endswith("\n"):
                f.write("\n")

    print("\nReport generated:")
    print(console_report_path)


if __name__ == "__main__":
    main()