import importlib.util
from pathlib import Path
import unittest

TOOL = Path(__file__).resolve().parents[1] / "tools/compare_payload_manifests.py"
SPEC = importlib.util.spec_from_file_location("payload_comparison", TOOL)
PAYLOAD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PAYLOAD)


def manifest(*rows):
    return {"files": [{"path": name, "size": size, "sha256": char * 64}
                      for name, size, char in rows]}


class PayloadComparison(unittest.TestCase):
    def test_all_four_categories_and_changed_container_cost(self):
        old = manifest(("same", 10, "a"), ("main.ucas", 90, "b"), ("removed", 5, "c"))
        new = manifest(("same", 10, "a"), ("main.ucas", 90, "c"), ("added", 5, "d"))
        result = PAYLOAD.compare(old, new)
        self.assertEqual([result[k]["count"] for k in ("unchanged", "changed", "added", "removed")], [1, 1, 1, 1])
        self.assertEqual(result["wholeFileReplacementBytes"], 95)
        self.assertAlmostEqual(result["wholeFileReplacementFraction"], 95 / 105)
        self.assertIn("Not a binary-delta estimate", result["scope"])

    def test_invalid_manifest_paths_duplicates_and_hashes_are_rejected(self):
        for document in (manifest(("../escape", 1, "a")), manifest(("/absolute", 1, "a")),
                         manifest(("C:/absolute", 1, "a")), manifest(("duplicate", 1, "a"), ("duplicate", 1, "a")),
                         manifest(("bad-sha", 1, "z")), manifest(("bad-size", -1, "a"))):
            with self.subTest(document=document), self.assertRaises(ValueError):
                PAYLOAD.index(document)

    def test_empty_payload_has_no_misleading_savings_percentage(self):
        result = PAYLOAD.compare(manifest(), manifest())
        self.assertIsNone(result["wholeFileReplacementFraction"])
        self.assertEqual(result["wholeFileReplacementBytes"], 0)


if __name__ == "__main__":
    unittest.main()
