"""Pure orchestration regressions. No Windows SDK required."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
import json

CODE=Path(__file__).resolve().parents[1]/'build_native_patches.py'
spec=importlib.util.spec_from_file_location('dcr_patch_tool',CODE)
patch=importlib.util.module_from_spec(spec)
spec.loader.exec_module(patch)


class PatchTests(unittest.TestCase):
    def test_only_verified_boot_image_seeds(self):
        with tempfile.TemporaryDirectory() as t:
            file=Path(t)/'interp_seeds.txt'
            file.write_text('# comment\n0x8C010000 # caller=0x8C000000\n'
                            '# runtime-code 0x8C010008 caller=0x8C000000\n'
                            '0x8C010004\n0x8C010004\n0x8C010003\n'
                            '0x90000000\n',encoding='utf-8')
            self.assertEqual(patch.read_seeds(file,0x8C010000,0x200),[0x8C010000,0x8C010004])

    def test_base_map_exclusion(self):
        with tempfile.TemporaryDirectory() as t:
            file=Path(t)/'function_map.csv'
            file.write_text('address,name,instructions,known,unknown,blocks,literals,calls,dynamic_branches\n'
                            '0x8C010000,main,5,5,0,1,0,0,0\n')
            self.assertEqual(patch.known_base(file),{0x8C010000})

    def test_no_seeds_does_not_invoke_compiler(self):
        from unittest.mock import patch as mocked
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            game=root/'game'
            game.mkdir()
            (root/'BOOTSTRAP.BIN').write_bytes(b'\x09\0'*64)
            import sys
            with mocked.object(sys,'argv',['prog','--root',str(root),'--game-dir',str(game),
                 '--image',str(root/'BOOTSTRAP.BIN'),'--base=0x8C010000']):
                self.assertEqual(patch.main(),0)
            self.assertFalse((game/'build').exists())
            self.assertTrue((game/'incremental'/'current_boot_sha256.txt').is_file())

    def test_module_declares_abi_and_exact_image_hash(self):
        source=patch.exports_source('a'*64)
        self.assertIn('dcr_sh4_patch_abi_v1',source)
        self.assertIn('dcr_sh4_patch_boot_sha256_v1',source)
        self.assertIn('"'+'a'*64+'"',source)

    def test_build_does_not_include_base_sh4_dll(self):
        content=patch.cmake_source('test',Path('/tmp/DreamcastRuntime.lib'),Path('/tmp/out'))
        self.assertNotIn('dcr_game_module',content)
        self.assertIn('generated_program.cpp',content)
        self.assertIn('incremental_exports.cpp',content)

if __name__=='__main__':unittest.main()
