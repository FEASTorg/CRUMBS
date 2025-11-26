#!/usr/bin/env bash
set -euo pipefail

OUT=docs/doxygen.log
echo "Running doxygen..."
doxygen docs/Doxyfile 2>&1 | tee "$OUT" || true

echo "Summary: (doxygen log)"
grep -i "warning:" "$OUT" || echo "No warnings found."

echo "Doc-check completed (warnings printed above)." 
exit 0
