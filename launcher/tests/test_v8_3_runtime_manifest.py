"""Ensure the game uses freshly validated runtime DLLs, not the cpp38e stamp alone."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
BAT = ROOT / 'scripts' / 'windows' / 'run_game.bat'
RUNTIME = ROOT / 'runtime' / 'dc_runtime.cpp'
HELPER = ROOT / 'tools' / 'runtime_cache' / 'verify_runtime_artifacts.py'


class RuntimeManifestContract(unittest.TestCase):
    def test_binary_version_and_guest_revision_differ(self):
        bat = BAT.read_text(encoding='utf-8-sig')
        # Any cpp38e v8.x runtime stamp; the exact id changes with every runtime fix.
        self.assertRegex(bat, r'set "DCR_RUNTIME_BUILD_ID=cpp38e-v8\.\d+[-.\w]*"')
        self.assertIn('DCR_GUEST_IMPL_REV=cpp38e-hot-block-recovery-v1', bat)
        self.assertNotIn('powershell -NoProfile -Command "$line=Get-Content', bat)

    def test_manifest_checked_before_cache_reuse(self):
        bat=BAT.read_text(encoding='utf-8-sig')
        self.assertLess(bat.index('verify_runtime_artifacts.py" check'),bat.index('goto :execute_runner',bat.index('RUNTIME_STAMP=')))
        self.assertIn('verify_runtime_artifacts.py" write',bat)
        self.assertIn('CMAKE_SUPPRESS_REGENERATION=ON',bat)
        self.assertIn('--clean-first --parallel 1',bat)
        self.assertTrue(HELPER.is_file())

    def test_runtime_provides_observable_backend_version(self):
        cpp=RUNTIME.read_text()
        self.assertIn('video-scaler=v8.4/dxgi-blit-native-copy', cpp)
        self.assertIn('presentation=v8.4/GDI', cpp)
        self.assertIn('Present(0u, 0u)',cpp)


if __name__=='__main__': unittest.main()
