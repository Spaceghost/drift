"""Canonical content/path/link identity for the pinned Genode SDK source."""
from pathlib import Path
import hashlib
import os


def digest(root):
    """Hash sorted paths/types/payloads, with 64-bit length-prefixed fields.

    File payloads are SHA-256 digests; symlink payloads are their exact targets;
    directory payloads are empty. Git metadata is excluded without traversing it.
    Ownership, timestamps and permissions do not participate.
    """
    root = Path(root)
    entries = []
    for directory, directories, files in os.walk(root, followlinks=False):
        directories[:] = [name for name in directories if name != '.git']
        for name in directories + files:
            if name == '.git':
                continue
            path = Path(directory) / name
            relative = path.relative_to(root).as_posix().encode('utf-8')
            if path.is_symlink():
                kind, payload = b'L', os.fsencode(os.readlink(path))
            elif path.is_dir():
                kind, payload = b'D', b''
            elif path.is_file():
                kind, payload = b'F', hashlib.sha256(path.read_bytes()).digest()
            else:
                raise ValueError('Unsupported SDK file type: ' + str(path))
            entries.append((relative, kind, payload))
    result = hashlib.sha256(b'drift-genode-source-tree-v1\0')
    for path, kind, payload in sorted(entries, key=lambda entry: entry[0]):
        for field in [kind, path, payload]:
            result.update(len(field).to_bytes(8, 'big'))
            result.update(field)
    return result.hexdigest()
