import sys
import subprocess
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
GENERATE = SCRIPT_DIR / "generate.py"
OUT = SCRIPT_DIR / "output_crc8"

OUT.mkdir(exist_ok=True)

MODEL = "crc-8"
ALGOS = ["bit", "nibble", "nibblem", "byte"]


def run(cmd):
    print(">>", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main():
    print("Generating CRC-8 variants only...")
    for algo in ALGOS:
        fileroot = f"crc8_{algo}"

        # Header
        run(
            [
                sys.executable,
                str(GENERATE),
                "--model",
                MODEL,
                "--algotag",
                algo,
                "--header",
            ]
        )
        hpp = SCRIPT_DIR / f"{fileroot}.hpp"
        if hpp.exists():
            hpp.rename(OUT / hpp.name)

        # Source
        run(
            [
                sys.executable,
                str(GENERATE),
                "--model",
                MODEL,
                "--algotag",
                algo,
                "--source",
            ]
        )
        cpp = SCRIPT_DIR / f"{fileroot}.cpp"
        if cpp.exists():
            cpp.rename(OUT / cpp.name)

    print("\nDone. CRC-8 files stored in:", OUT)


if __name__ == "__main__":
    main()
