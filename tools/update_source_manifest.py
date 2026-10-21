#!/usr/bin/env python3
"""Refresh hashes for already reviewed source paths after a maintainer review.

This does not approve new files or alter licence/provenance fields. Additions
need a reviewed manifest entry first, then the source audit and build checks.
"""
import hashlib
import json
from pathlib import Path


def main():
    root = Path(__file__).resolve().parents[1]
    file = root / "source-manifest.json"
    manifest = json.loads(file.read_text(encoding="utf-8"))
    for row in manifest["files"]:
        path = (root / row["path"]).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise ValueError(f"Invalid reviewed path: {row['path']}")
        raw = path.read_bytes()
        row["size"] = len(raw)
        row["sha256"] = hashlib.sha256(raw).hexdigest()
    file.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"Updated {len(manifest['files'])} existing manifest entries; no paths or licences added.")


if __name__ == "__main__":
    main()
