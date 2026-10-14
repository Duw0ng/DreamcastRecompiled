"""Regression tests for compiler-free, per-title precompiled native launch."""
import contextlib
import importlib.util
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
from unittest.mock import patch

LAUNCHER = Path(__file__).resolve().parents[1]
ROOT = LAUNCHER.parent
sys.path.insert(0, str(LAUNCHER))
import portable_games as pg
from core import DEFAULTS


class PortableTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.package = self.root / 'games' / 'CHUCHU_ROCKET'
        self.package.mkdir(parents=True)
        (self.package/'portable_manifest.txt').write_text((ROOT/'games'/'CHUCHU_ROCKET'/'portable_manifest.txt').read_text())
        for rel in pg.REQUIRED:
            dst = self.package/rel
            dst.parent.mkdir(parents=True,exist_ok=True)
            dst.write_bytes(b'MZ'+b'\0'*300)

    @unittest.skipUnless((ROOT/'games/CHUCHU_ROCKET/bin/dreamcast_program.exe').is_file(), 'requires packaged Windows binaries')
    def test_actual_bundle_from_user_zip(self):
        bundle=pg.load_bundle(ROOT)
        self.assertEqual(bundle.game,'CHUCHU_ROCKET')
        self.assertEqual(bundle.title,'CHUCHU ROCKET')

    def test_manifest_and_pe_validation(self):
        self.assertIsNotNone(pg.load_bundle(self.root))
        (self.package/'bin/dc_disc_probe.exe').write_bytes(b'NO'+b'0'*300)
        with self.assertRaises(ValueError): pg.load_bundle(self.root)

    def test_abi_mismatch_rejected(self):
        man=self.package/'portable_manifest.txt'
        man.write_text(man.read_text().replace('GUEST_ABI=1','GUEST_ABI=999'))
        with self.assertRaises(ValueError): pg.load_bundle(self.root)

    def test_missing_plugin_rejected(self):
        (self.package/'bin/plugins/dcr_network.dll').unlink()
        with self.assertRaises(FileNotFoundError):pg.load_bundle(self.root)

    def test_disc_match_requires_metadata_not_filename(self):
        bundle=pg.load_bundle(self.root)
        self.assertTrue(pg.disc_matches(bundle,{'title':'CHUCHU ROCKET'}))
        self.assertTrue(pg.disc_matches(bundle,{'title':'  CHU CHU ROCKET !'}))
        self.assertFalse(pg.disc_matches(bundle,{'title':'SONIC ADVENTURE','volume':'ccr.cdi'}))
        self.assertFalse(pg.disc_matches(bundle,{'title':'MARS MATRIX'}))
        mapfile=self.root/'disc.map'
        mapfile.write_text('DCR_DISC_MAP_V2\ntitle=  ChuChu Rocket!  \nvolume=CHUCHU\n')
        self.assertTrue(pg.disc_matches(bundle,pg.disc_metadata(mapfile)))

    def test_native_flags_bootstrap_and_plugins(self):
        bundle=pg.load_bundle(self.root)
        with patch('core.cache_directory',return_value=self.root/'appdata'):
            args,env,cwd=pg.create_native_command(bundle,self.root/'disc.map',self.root/'BOOTSTRAP.BIN',self.root/'ccr.cdi',DEFAULTS)
        self.assertTrue(str(args[0]).endswith('dreamcast_program.exe'))
        self.assertIn('--commercial-boot',args)
        self.assertIn('--commercial-boot-stage=plain',args)
        self.assertIn('--disc-map='+str(self.root/'disc.map'),args)
        self.assertIn('--pvr-gpu',args)
        self.assertIn('--maple-host-input',args)
        self.assertEqual(env['DCR_EXTERNAL_RAW_BASE'],'0x8C008000')
        self.assertEqual(env['DCR_EXTERNAL_RAW_IMAGE'],str(self.root/'BOOTSTRAP.BIN'))
        self.assertEqual(env['DCR_PLUGIN_DIR'],str(self.package/'bin/plugins'))
        self.assertEqual(cwd,self.package/'bin')

    def test_no_commercial_resources_in_bundle(self):
        files={x.name.lower() for x in (ROOT/'games'/'CHUCHU_ROCKET').rglob('*') if x.is_file()}
        for name in ['ip.bin','boot.disc.bin','bootstrap.bin','disc.map','game.cdi','game.gdi']:
            self.assertNotIn(name,files)

    def test_correct_fallback_when_not_chuchu(self):
        bundle=pg.load_bundle(self.root)
        self.assertFalse(pg.disc_matches(bundle,{'title':'SEGA TETRIS'}))

    def test_full_auto_route_uses_precompiled_exe_without_compiler(self):
        bundle = pg.load_bundle(self.root)
        disc = self.root/'renamed_anything.cdi'
        disc.write_bytes(b'fake image')
        commands = []
        def fake_run(cmd, **kwargs):
            commands.append(list(cmd))
            if 'dc_disc_probe.exe' in cmd[0]:
                Path(next(v[19:] for v in cmd if v.startswith('--extract-disc-map='))).write_text(
                    'DCR_DISC_MAP_V2\ntitle=CHUCHU ROCKET\nvolume=CHUCHU\n',encoding='utf-8')
            from types import SimpleNamespace
            return SimpleNamespace(returncode=0)
        def fake_call(cmd, **kwargs):
            commands.append(list(cmd));return 0
        with patch('core.cache_directory',return_value=self.root/'cache'), \
             patch('core.load_settings',return_value=DEFAULTS), \
             patch.object(pg.subprocess,'run',side_effect=fake_run), \
             patch.object(pg.subprocess,'call',side_effect=fake_call):
            self.assertEqual(pg.launch_auto_game(self.root,disc),0)
        self.assertEqual(len(commands),3)
        self.assertTrue(commands[0][0].endswith('dc_disc_probe.exe'))
        self.assertTrue(commands[1][0].endswith('dc_boot_prepare.exe'))
        self.assertTrue(commands[2][0].endswith('dreamcast_program.exe'))
        self.assertFalse(any('cmake' in ' '.join(cmd).lower() for cmd in commands))

    def test_full_auto_route_other_game_invokes_existing_runner(self):
        bundle = pg.load_bundle(self.root)
        disc = self.root/'ccr.cdi'  # intentionally misleading filename
        disc.write_bytes(b'fake image')
        commands = []
        (self.root/'scripts'/'windows').mkdir(parents=True)
        (self.root/'scripts'/'windows'/'run_game.bat').write_bytes(b'@echo off')
        def fake_run(cmd, **kwargs):
            commands.append(list(cmd))
            Path(next(v[19:] for v in cmd if v.startswith('--extract-disc-map='))).write_text(
                'DCR_DISC_MAP_V2\ntitle=SEGA TETRIS\nvolume=TETRIS\n',encoding='utf-8')
            from types import SimpleNamespace
            return SimpleNamespace(returncode=0)
        def fake_call(cmd, **kwargs):
            commands.append(list(cmd));return 0
        with patch('core.cache_directory',return_value=self.root/'cache'), \
             patch('core.load_settings',return_value=DEFAULTS), \
             patch.object(pg.subprocess,'run',side_effect=fake_run), \
             patch.object(pg.subprocess,'call',side_effect=fake_call):
            self.assertEqual(pg.launch_auto_game(self.root,disc),0)
        self.assertEqual(len(commands),2)
        self.assertTrue(commands[1][0].endswith('run_game.bat'))
        self.assertFalse(any('dreamcast_program.exe' in ' '.join(cmd) for cmd in commands))

    def test_loader_using_settings_with_audio_disabled(self):
        bundle=pg.load_bundle(self.root)
        settings={**DEFAULTS,'audio':False,'renderer':'software','network_mode':'off'}
        with patch('core.cache_directory',return_value=self.root/'cache'):
            args,env,_=pg.create_native_command(bundle,self.root/'map',self.root/'boot',self.root/'disc.cdi',settings)
        self.assertNotIn('--aica-play',args)
        self.assertNotIn('--pvr-gpu',args)
        self.assertIn('--pvr-mt',args)
        self.assertEqual(env['DCR_NETWORK_MODE'],'off')

if __name__=='__main__':unittest.main()
