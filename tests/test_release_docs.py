"""Offline regression checks for public release metadata and generated guidance."""

import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("render_release_docs", ROOT / "tools/render_release_docs.py")
DOCS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DOCS)


class ReleaseDocuments(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.current = DOCS.read_json(ROOT / "release/current.json")
        cls.history = DOCS.read_json(ROOT / "release/history.json")
        cls.v86 = DOCS.read_json(ROOT / "tests/fixtures/v86.json")
        cls.outputs = DOCS.render(cls.current, cls.history)

    def test_reviewed_v86_metadata_and_all_nine_sha256_values(self):
        expected_names = [f"Warfare-Front-v86-Data.7z.{i:03d}" for i in range(1, 9)]
        expected_names.append("Warfare-Front-v86-Setup.exe")
        self.assertEqual([a["name"] for a in self.v86["assets"]], expected_names)
        self.assertTrue(all(DOCS.SHA256.fullmatch(a["sha256"]) for a in self.v86["assets"]))
        self.assertEqual(sum(a["size"] for a in self.v86["assets"][:-1]), 13_943_971_064)
        self.assertEqual(self.v86["installedBytes"], 14_968_829_789)
        self.assertEqual(self.v86["oldSingleDriveComponentSumBytes"], 28_912_800_853)
        self.assertEqual(self.v86["recommendedFreeBytes"], 35_000_000_000)
        # A later current release must not overwrite the V86 evidence fixture.
        if self.current["tag"] == "v86":
            for key in ("assets", "downloadBytes", "installedBytes", "recommendedFreeBytes",
                        "oldSingleDriveComponentSumBytes"):
                self.assertEqual(self.current[key], self.v86[key])

    def test_inconsistent_metadata_is_rejected(self):
        for mutate in (
            lambda c: c.update(downloadBytes=c["downloadBytes"] + 1),
            lambda c: c["assets"][0].update(sha256="bad"),
            lambda c: c["assets"][0].update(url="https://example.invalid/replaced"),
            lambda c: c["installer"].update(signed=True),
            lambda c: c.update(minimumPerformanceValidated=True),
        ):
            with self.subTest(mutation=mutate):
                candidate = copy.deepcopy(self.current)
                mutate(candidate)
                with self.assertRaises(ValueError):
                    DOCS.render(candidate, self.history)

    def test_all_player_documents_retain_scope_security_and_storage(self):
        formatted = DOCS.context(self.current)
        for name, text in self.outputs.items():
            if name == "CHANGELOG.md":
                continue
            with self.subTest(document=name):
                if name != "README.zh-CN.md":
                    self.assertIn(self.current["validation"]["scope"], text)
                if name != "README.md":
                    self.assertIn(self.current["validation"]["scopeZh"], text)
                for key in ("data_gb", "installed_gb", "component_gb", "free_gb"):
                    self.assertIn(formatted[key] + " GB", text)
                self.assertIn(self.current["assets"][-1]["sha256"], text)
                self.assertIn("Get-FileHash -LiteralPath", text)
                self.assertIn("SHA256SUMS.txt", text)
                if name != "README.zh-CN.md":
                    self.assertIn("not digitally signed", text)
                    self.assertIn("not independent proof of publisher identity", text)
                    self.assertIn("not a measured peak for every installation scenario", text)
                if name != "README.md":
                    self.assertIn("尚未进行数字签名", text)
                    self.assertIn("不能独立证明发布者身份", text)
                    self.assertIn("不是所有安装情境下实测的峰值占用", text)

    def test_no_bom_or_crlf_and_all_outputs_are_utf8(self):
        for name, text in self.outputs.items():
            with self.subTest(document=name):
                encoded = text.encode("utf-8")
                self.assertFalse(encoded.startswith(b"\xef\xbb\xbf"))
                self.assertNotIn(b"\r", encoded)
                self.assertTrue(encoded.endswith(b"\n"))
                self.assertEqual(encoded.decode("utf-8"), text)

    def test_public_history_preserves_all_original_notes_and_dates(self):
        changelog = self.outputs["CHANGELOG.md"]
        by_tag = {item["tag"]: item for item in self.history}
        for tag, digest in self.v86["historicalNotesSha256"].items():
            self.assertIn(tag, by_tag)
            self.assertEqual(hashlib.sha256(by_tag[tag]["notes"].encode("utf-8")).hexdigest(), digest)
        indexes = []
        for item in sorted(self.history, key=lambda h: h["published_at"], reverse=True):
            header = f"## [{item['tag']}]({item['url']}) — {item['published_at'][:10]}"
            self.assertEqual(changelog.count(header), 1)
            self.assertIn(item["title"], changelog)
            self.assertIn(item["notes"].rstrip(), changelog)
            indexes.append(changelog.index(header))
        self.assertEqual(indexes, sorted(indexes))
        self.assertTrue(all(not item.get("draft") for item in self.history))

    def test_latest_link_and_source_code_boundary(self):
        for name in ("README.md", "README.zh-CN.md"):
            text = self.outputs[name]
            self.assertIn("/releases/latest)", text)
            self.assertNotIn("latest/download/Warfare-Front-Setup.exe", text)
            self.assertIn("Source code", text)
        self.assertIn("The game source is not published", self.outputs["README.md"])
        self.assertIn("未公开游戏源码", self.outputs["README.zh-CN.md"])

    def test_installed_notes_are_an_identical_generated_copy(self):
        base = f"docs/releases/{self.current['tag']}/"
        self.assertEqual(self.outputs[base + "ReleaseNotes.md"],
                         self.outputs[base + "install/ReleaseNotes.txt"])
        installed = self.outputs[base + "README-INSTALLED.txt"]
        self.assertIn("Setup intentionally copies", installed)
        self.assertIn("ReleaseNotes.md", installed)
        self.assertIn("ReleaseNotes.txt", installed)

    def test_screenshot_link_requires_a_real_local_asset(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            absent = DOCS.render(self.current, self.history, root)["README.md"]
            self.assertNotIn("![", absent)
            target = root / "docs/media/battlefield.webp"
            target.parent.mkdir(parents=True)
            target.write_bytes(b"test fixture only")
            present = DOCS.render(self.current, self.history, root)["README.md"]
            self.assertIn("](docs/media/battlefield.webp)", present)
            self.assertIn("](docs/media/README.md)", present)
            self.assertIn("synthetic review data", present)

    def test_later_release_can_be_rendered_without_discarding_history(self):
        future = copy.deepcopy(self.current)
        future["version"] = "87"
        future["tag"] = "v87"
        future["releaseUrl"] = future["releaseUrl"].replace("/v86", "/v87")
        future["installer"]["name"] = future["installer"]["name"].replace("v86", "v87")
        for asset in future["assets"]:
            asset["name"] = asset["name"].replace("v86", "v87")
            asset["url"] = asset["url"].replace("v86", "v87")
        future["originalNotes"] = "# V87\n\nA later release fixture.\n"
        history = self.history + [{
            "tag": "v87", "title": "V87", "published_at": "2026-10-22T00:00:00Z",
            "url": future["releaseUrl"], "notes": future["originalNotes"],
        }]
        rendered = DOCS.render(future, history)
        self.assertIn("docs/releases/v87/RELEASE-BODY.md", rendered)
        self.assertIn("Warfare-Front-v87-Setup.exe", rendered["README.md"])
        self.assertIn("## [v86]", rendered["CHANGELOG.md"])
        self.assertIn("## [v87]", rendered["CHANGELOG.md"])

    def test_check_detects_drift_without_repairing_it(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "release").mkdir()
            for name, data in (("current.json", self.current), ("history.json", self.history)):
                (root / "release" / name).write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(DOCS.main(["--root", str(root)]), 0)
                self.assertEqual(DOCS.main(["--root", str(root), "--check"]), 0)
            target = root / "README.md"
            target.write_bytes(target.read_bytes() + b"unreviewed drift\n")
            before = target.read_bytes()
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                self.assertEqual(DOCS.main(["--root", str(root), "--check"]), 1)
            self.assertIn("README.md", stderr.getvalue())
            self.assertEqual(target.read_bytes(), before)

    def test_generated_working_tree_is_current(self):
        self.assertEqual(DOCS.check_outputs(ROOT, self.outputs), [])


if __name__ == "__main__":
    unittest.main()
