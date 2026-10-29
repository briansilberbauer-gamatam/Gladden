#!/usr/bin/env python3
"""Print matching lines from a build log as a GitHub Actions error annotation."""

import re
import sys
from pathlib import Path


def main() -> None:
    if len(sys.argv) < 4:
        raise SystemExit(f"usage: {sys.argv[0]} LOG TITLE PATTERN")

    log_path = Path(sys.argv[1])
    title = sys.argv[2]
    pattern = re.compile(sys.argv[3])

    if not log_path.is_file():
        print(f"::error title={title}::Log file not found: {log_path}")
        return

    lines = [
        line
        for line in log_path.read_text(errors="replace").splitlines()
        if pattern.search(line)
    ]
    body = "\n".join(lines[-15:])[:5000]
    body = body.replace("%", "%25").replace("\r", "").replace("\n", "%0A")
    print(f"::error title={title}::{body}")


if __name__ == "__main__":
    main()
