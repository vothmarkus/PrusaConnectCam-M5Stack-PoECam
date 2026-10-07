import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("release_publication", Path(__file__).resolve().parent.parent / "tools/release_publication.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
requested = module.publication_requested


class PublicationTest(unittest.TestCase):
    def test_only_explicit_matching_bytes_are_published(self):
        manifest = {"version": "1.4.1", "sha256": "a" * 64}
        self.assertFalse(requested(None, manifest))
        self.assertFalse(requested({**manifest, "version": "1.4.0"}, manifest))
        self.assertFalse(requested(manifest, {**manifest, "version": "1.4.2"}))
        self.assertTrue(requested(dict(manifest), manifest))
        for request in ({**manifest, "sha256": "b" * 64}, {**manifest, "sha256": ""},
                        {**manifest, "version": "1.4.1-rc1"}, {}, [], {**manifest, "extra": True}):
            with self.assertRaises(ValueError):
                requested(request, manifest)


if __name__ == "__main__":
    unittest.main()
