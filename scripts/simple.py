#!/usr/bin/env python3
"""Minimal helper to run all CRC-8 generators and place outputs into <repo-root>/dist/crc/*

This runs the C99 and Arduino generators and prints where outputs were written.
"""
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PY = sys.executable

print("Running C99 generator...")
subprocess.run([PY, str(SCRIPT_DIR / "generate_crc8_c99.py")], check=True)

print("Running Arduino generator...")
subprocess.run([PY, str(SCRIPT_DIR / "generate_crc8_arduino.py")], check=True)

print("All generators finished. Outputs under", SCRIPT_DIR.parent / "dist" / "crc")
