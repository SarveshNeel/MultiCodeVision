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
# Read pipeline configuration
# -------------------------------------------------

def read_config_file() -> str:

    repo_root = Path(__file__).resolve().parents[1]
    config_path = repo_root / "include" / "mcv" / "core" / "config.hpp"

    if not config_path.exists():
        return "[CONFIG] config.hpp not found\n"

    variables = []

    inside_block = False

    with open(config_path, "r") as f:
        for line in f:
            stripped = line.strip()

            # Detect start of struct/enum/class blocks
            if stripped.startswith("struct ") or stripped.startswith("enum ") or stripped.startswith("class "):
                inside_block = True

            if inside_block:
                if "};" in stripped or "}" in stripped:
                    inside_block = False
                continue

            # Capture only initialized variables
            if "=" in stripped and stripped.endswith(";"):
                variables.append(stripped)

    header = "\n================ CONFIGURATION VARIABLES ================\n"
    footer = "\n========================================================\n\n"

    return header + "\n".join(variables) + footer

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

    config_text = read_config_file()

    console_outputs = [output]

    # -----------------------------
    # Write exact console report
    # -----------------------------

    with open(console_report_path, "w") as f:

        # Write configuration used for the experiment
        f.write(config_text)

        # Write console output from decoder
        for block in console_outputs:
            f.write(block)

            if not block.endswith("\n"):
                f.write("\n")

    print("\nReport generated:")
    print(console_report_path)


if __name__ == "__main__":
    main()