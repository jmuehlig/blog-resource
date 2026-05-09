#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

for csv in "$ROOT_DIR"/result*.csv; do
    [ -f "$csv" ] || continue
    echo "Plotting $csv..."
    python3 "$SCRIPT_DIR/plot.py" "$csv" "$@"
done

python3 "$SCRIPT_DIR/compare.py" result-skylake.csv  result-sapphire-rapids.csv result-zen4.csv --metric cycles --kb 1
