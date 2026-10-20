#!/usr/bin/env python3
"""Compare frozen payload manifests without opening game files or containers.

Input: {"files": [{"path": "relative/file", "size": 123, "sha256": "..."}]}.
The result estimates whole-file replacement bytes, never binary-delta size.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path, PurePosixPath
import re


def index(document: dict) -> dict[str, dict]:
    result = {}
    for row in document["files"]:
        name = row["path"].replace("\\", "/")
        path = PurePosixPath(name)
        if not name or path.is_absolute() or ".." in path.parts or ":" in name:
            raise ValueError(f"Expected relative payload path: {name}")
        if name in result:
            raise ValueError(f"Duplicate payload path: {name}")
        if type(row["size"]) is not int or row["size"] < 0:
            raise ValueError(f"Invalid byte count: {name}")
        digest = row["sha256"].lower()
        if not re.fullmatch(r"[0-9a-f]{64}", digest):
            raise ValueError(f"Invalid SHA-256: {name}")
        result[name] = {"size": row["size"], "sha256": digest}
    return result


def compare(before: dict, after: dict) -> dict:
    old, new = index(before), index(after)
    unchanged = sorted(p for p in old.keys() & new.keys() if old[p] == new[p])
    changed = sorted(p for p in old.keys() & new.keys() if old[p] != new[p])
    added = sorted(new.keys() - old.keys())
    removed = sorted(old.keys() - new.keys())
    total = sum(row["size"] for row in new.values())
    replacement = sum(new[p]["size"] for p in changed + added)
    def group(names, source):
        return {"count": len(names), "bytes": sum(source[p]["size"] for p in names), "paths": names}
    return {
        "unchanged": group(unchanged, new), "changed": group(changed, new),
        "added": group(added, new), "removed": group(removed, old),
        "newPayloadBytes": total, "wholeFileReplacementBytes": replacement,
        "wholeFileReplacementFraction": replacement / total if total else None,
        "scope": "Manifest comparison only. Not a binary-delta estimate, dependency audit or runtime acceptance.",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("old", type=Path)
    parser.add_argument("new", type=Path)
    args = parser.parse_args()
    try:
        result = compare(json.loads(args.old.read_text(encoding="utf-8-sig")),
                         json.loads(args.new.read_text(encoding="utf-8-sig")))
    except (OSError, KeyError, TypeError, ValueError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
