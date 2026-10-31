"""Check source pinning against tampering, path changes and metadata exclusion."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
from source_tree import digest


class SourceTreeTests(unittest.TestCase):
    def setUp(self):
        build = Path(__file__).resolve().parents[2] / 'build'
        build.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix='genode-source-test-', dir=build)
        self.root = Path(self.temp.name)
        (self.root / 'VERSION').write_text('26.08\n')
        (self.root / 'entry').write_bytes(b'original')

    def tearDown(self):
        self.temp.cleanup()

    def test_changed_file_and_path(self):
        before = digest(self.root)
        (self.root / 'entry').write_bytes(b'modified')
        self.assertNotEqual(before, digest(self.root))
        (self.root / 'entry').write_bytes(b'original')
        (self.root / 'entry').rename(self.root / 'renamed')
        self.assertNotEqual(before, digest(self.root))

    def test_added_directory_and_file(self):
        before = digest(self.root)
        (self.root / 'empty').mkdir()
        self.assertNotEqual(before, digest(self.root))
        directory_digest = digest(self.root)
        (self.root / 'empty' / 'file').write_bytes(b'')
        self.assertNotEqual(directory_digest, digest(self.root))

    def test_symlink_identity_and_no_external_traversal(self):
        (self.root / 'link').symlink_to('first')
        first = digest(self.root)
        (self.root / 'link').unlink()
        (self.root / 'link').symlink_to('second')
        self.assertNotEqual(first, digest(self.root))
        (self.root / 'outside').symlink_to('/path/that/does/not/exist')
        self.assertEqual(digest(self.root), digest(self.root))

    def test_git_metadata_and_timestamps_are_excluded(self):
        before = digest(self.root)
        (self.root / '.git').mkdir()
        (self.root / '.git' / 'objects').write_bytes(b'not source')
        os.utime(self.root / 'entry', (1, 1))
        self.assertEqual(before, digest(self.root))

    def test_sdk_version_alone_does_not_authorize_build(self):
        build = self.root / 'must-not-be-created'
        result = subprocess.run([sys.executable, str(Path(__file__).with_name('build.py')),
                                 '--genode-dir', str(self.root), '--build-dir', str(build),
                                 '--toolchain-prefix', '/unavailable/compiler-'], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn('SDK source tree differs', result.stderr)
        self.assertFalse(build.exists())


if __name__ == '__main__':
    unittest.main()
