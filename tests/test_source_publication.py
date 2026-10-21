import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("source_audit", ROOT / "tools/audit_source_export.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class SourcePublication(unittest.TestCase):
    def test_reviewed_export_is_intact_and_has_no_common_secret_markers(self):
        result = AUDIT.verify()
        self.assertTrue(result["passed"], result["findings"])
        self.assertGreater(result["reviewedFiles"], 700)

    def test_scanner_flags_secret_shapes_without_echoing_them(self):
        self.assertIn("github-token", AUDIT.scan_text("ghp_" + "a" * 36))
        self.assertIn("android-debug-token", AUDIT.scan_text("SecurityToken=" + "a" * 32))
        self.assertIn("private-key", AUDIT.scan_text("-----BEGIN " + "PRIVATE KEY-----"))
        self.assertEqual(AUDIT.scan_text('ProjectID=EC682F074C214C8CB2486C39D08283C1'), [])

    def test_omitted_asset_interfaces_remain_explicitly_unavailable(self):
        private = ROOT / "Source/WarfareFront/Private"
        for name in ("FRRU/FRRUVauxTripoReleaseV87.generated.inl", "FRRU/FRRUVauxHQ27ReleaseV87.generated.inl"):
            self.assertIn("SourceDataAvailable=false", (private / name).read_text(encoding="utf-8"))
        self.assertIn("SandbagGeometryAvailable=false", (private / "WarfareFrontlineSandbagContactV87.inl").read_text(encoding="utf-8"))
        self.assertFalse((ROOT / "Source/WarfareFront/Docs/V8221/Art/blast-composition-board.png").exists())


if __name__ == "__main__":
    unittest.main()
