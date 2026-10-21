#!/usr/bin/env python3
"""Check the reviewed source allowlist, hashes and common secret markers.

This is a bounded publication guard, not a proof of authorship or a general
security scanner. The licence/provenance review is recorded in LICENSING.md.
Findings identify files and rules without printing suspected secret values.
"""
from __future__ import annotations
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

ROOT = Path(__file__).resolve().parents[1]
ALLOWED = {"MIT", "MIT AND CC0-1.0"}
PATTERNS = {
    "github-token": re.compile(r"\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{35,})\b"),
    "aws-access-key": re.compile(r"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b"),
    "openai-key": re.compile(r"\bsk-(?:proj-)?[A-Za-z0-9_-]{40,}\b"),
    "private-key": re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    "android-debug-token": re.compile(r"(?im)^\s*SecurityToken\s*="),
    "personal-windows-path": re.compile(r"(?i)[A-Z]:[/\\]Users[/\\](?!Public[/\\]|Default[/\\])[^/\\\s\"'<>]+"),
}


def scan_text(text):
    return sorted(name for name, expression in PATTERNS.items() if expression.search(text))


def verify(root=ROOT):
    manifest = json.loads((root / "source-manifest.json").read_text(encoding="utf-8"))
    findings, seen = [], set()
    for row in manifest["files"]:
        name = row["path"]
        path = PurePosixPath(name)
        if path.is_absolute() or ".." in path.parts or ":" in name or name in seen:
            findings.append({"path": name, "rule": "invalid-or-duplicate-path"})
            continue
        seen.add(name)
        if row["license"] not in ALLOWED:
            findings.append({"path": name, "rule": "unreviewed-licence"})
        actual = root / name
        if not actual.is_file() or actual.is_symlink():
            findings.append({"path": name, "rule": "missing-or-linked-file"})
            continue
        raw = actual.read_bytes()
        if len(raw) != row["size"] or hashlib.sha256(raw).hexdigest() != row["sha256"]:
            findings.append({"path": name, "rule": "reviewed-bytes-changed"})
        try:
            text = raw.decode("utf-8-sig")
        except UnicodeError:
            findings.append({"path": name, "rule": "non-text-source"})
            continue
        findings.extend({"path": name, "rule": rule} for rule in scan_text(text))
    actual_sources = {p.relative_to(root).as_posix() for p in (root / "Source").rglob("*") if p.is_file()}
    for name in sorted(actual_sources - seen):
        findings.append({"path": name, "rule": "source-not-in-reviewed-manifest"})
    if any(p.suffix.lower() not in {".h", ".cpp", ".inl", ".cs"} for p in (root / "Source").rglob("*") if p.is_file()):
        findings.append({"path": "Source/", "rule": "non-source-asset"})
    return {"passed": not findings, "reviewedFiles": len(seen), "findings": findings,
            "scope": "Allowlist/hash/common-secret checks; not a legal opinion or complete security audit."}


def main():
    result = verify()
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
