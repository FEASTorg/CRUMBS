#!/usr/bin/env bash
set -euo pipefail

OUT=docs/doxygen.log
echo "Running doxygen..."
if ! command -v doxygen >/dev/null 2>&1; then
	echo "doxygen not installed; skipping doxygen run" | tee "$OUT"
else
	doxygen docs/Doxyfile 2>&1 | tee "$OUT" || true
fi

echo "Summary: (doxygen log)"
grep -i "warning:" "$OUT" || echo "No warnings found."

echo "Doc-check completed (warnings printed above)." 
exit 0
