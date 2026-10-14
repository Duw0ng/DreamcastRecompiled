"""MSVC regression: never include legacy winsock.h before Winsock2.

The failure occurs when windows.h transitively includes winsock.h ahead of
winsock2.h. Keep this check with the release launcher since Linux builds
cannot compile the Windows SDK headers themselves.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]

class WindowsHeaderOrderTests(unittest.TestCase):
    def test_runtime_includes_winsock2_before_windows_or_focus_guard(self):
        source = (ROOT / 'runtime' / 'dc_runtime.cpp').read_text()
        self.assertLess(source.index('#include <winsock2.h>'), source.index('#include <windows.h>'))
        self.assertLess(source.index('#include <winsock2.h>'), source.index('#include "dcr_input_focus.hpp"'))
        self.assertLess(source.index('#define WIN32_LEAN_AND_MEAN'), source.index('#include <winsock2.h>'))

    def test_focus_header_is_safe_as_first_windows_header(self):
        header = (ROOT / 'runtime' / 'include' / 'dcr_input_focus.hpp').read_text()
        self.assertLess(header.index('#define WIN32_LEAN_AND_MEAN'), header.index('#include <windows.h>'))
        self.assertLess(header.index('#define NOMINMAX'), header.index('#include <windows.h>'))

    def test_cmake_guards_runtime_and_cache_is_fresh(self):
        cmake = (ROOT / 'runtime' / 'CMakeLists.txt').read_text()
        bat = (ROOT / 'scripts' / 'windows' / 'run_game.bat').read_text(encoding='utf-8-sig')
        self.assertIn('target_compile_definitions(DreamcastRuntime PRIVATE WIN32_LEAN_AND_MEAN)',cmake)
        # Any cpp38e v8.x runtime stamp; the exact id changes with every runtime fix.
        self.assertRegex(bat, r'set "DCR_RUNTIME_BUILD_ID=cpp38e-v8\.\d+[-.\w]*"')
        self.assertIn('DCR_GUEST_IMPL_REV=cpp38e-hot-block-recovery-v1',bat)
        self.assertIn('verify_runtime_artifacts.py" check',bat)

if __name__=='__main__': unittest.main()
