from __future__ import annotations
import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('runtime_cache_check', ROOT / 'verify_runtime_artifacts.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


class RuntimeCacheTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.p = Path(self.tmp.name)
        self.src = self.p / 'runtime'
        self.out = self.p / 'build'
        self.manifest = self.p / 'cache.json'
        self.src.mkdir()
        (self.src / 'dc_runtime.cpp').write_text('// before\n')
        for name in mod.REQUIRED_DLLS:
            target = self.out / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b'binary bytes ' + name.encode('ascii'))

    def test_requires_initial_rebuild(self):
        self.assertEqual(mod.run('check', self.src, self.out, self.manifest), 1)

    def test_source_and_binary_protection(self):
        self.assertEqual(mod.run('write', self.src, self.out, self.manifest), 0)
        self.assertEqual(mod.run('check', self.src, self.out, self.manifest), 0)
        (self.src / 'dc_runtime.cpp').write_text('// after\n')
        self.assertEqual(mod.run('check', self.src, self.out, self.manifest), 1)
        self.assertEqual(mod.run('write', self.src, self.out, self.manifest), 0)
        path = self.out / mod.REQUIRED_DLLS[0]
        path.write_bytes(b'old DLL unexpectedly reinstalled')
        self.assertEqual(mod.run('check', self.src, self.out, self.manifest), 1)

    def test_missing_dll_rejected(self):
        (self.out / mod.REQUIRED_DLLS[2]).unlink()
        self.assertEqual(mod.run('check', self.src, self.out, self.manifest), 1)
        self.assertEqual(mod.run('write', self.src, self.out, self.manifest), 1)

    def test_cmake_source_validation(self):
        other_spec = importlib.util.spec_from_file_location('home', ROOT / 'check_cmake_home.py')
        home = importlib.util.module_from_spec(other_spec)
        other_spec.loader.exec_module(home)
        cache = self.p / 'CMakeCache.txt'
        cache.write_text('CMAKE_HOME_DIRECTORY:INTERNAL=' + str(self.src) + '\n')
        self.assertEqual(home.check(cache, self.src), 0)
        self.assertEqual(home.check(cache, self.p), 1)

    def test_batch_version_is_independent_from_guest(self):
        bat = (ROOT.parent.parent / 'run_game.bat').read_text(encoding='utf-8-sig')
        # Any cpp38e v8.x runtime stamp; the exact id changes with every runtime fix.
        self.assertRegex(bat, r'set "DCR_RUNTIME_BUILD_ID=cpp38e-v8\.\d+[-.\w]*"')
        self.assertIn('DCR_GUEST_IMPL_REV=cpp38e-hot-block-recovery-v1', bat)
        self.assertIn('verify_runtime_artifacts.py" check', bat)
        self.assertIn('verify_runtime_artifacts.py" write', bat)
        self.assertNotIn('powershell -NoProfile -Command "$line=Get-Content', bat)


if __name__ == '__main__': unittest.main()
