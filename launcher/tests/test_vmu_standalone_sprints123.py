"""VMU standalone / companion regression: no commercial image required."""
from __future__ import annotations
import json
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from vmu_pixels import LCD
from vmu_companion import PetStore,VMUShell
from vmu_standalone import independent_command
from vmu_lcd import read_frame, MAGIC
import struct

class PureVMUTests(unittest.TestCase):
    def test_pixel_contract(self):
        v=LCD();v.text(24,8,'HELLO',center=True);v.pet()
        self.assertEqual(len(v.bitmap()),192)
        self.assertNotEqual(v.bitmap(),bytes(192))

    def test_controls_and_autonomous_apps(self):
        with tempfile.TemporaryDirectory() as d:
            store=PetStore(Path(d)/'pet.json')
            shell=VMUShell(store)
            self.assertFalse(shell.power)
            shell.press('SLEEP');self.assertTrue(shell.power)
            shell.press('RIGHT');shell.press('RIGHT');shell.press('A')
            self.assertEqual(shell.page,'pet')
            before=store.pet['full'];shell.press('A')
            self.assertGreater(store.pet['full'],before)
            shell.press('RIGHT');shell.press('A')
            self.assertEqual(shell.page,'games')
            shell.press('A');self.assertEqual(shell.page,'catch')
            shell.press('B');self.assertEqual(shell.page,'games')
            shell.press('RIGHT');shell.press('A');self.assertEqual(shell.page,'memory')
            self.assertEqual(len(shell.sequence),3)
            for _ in range(13):shell.tick()
            for v in shell.sequence:shell.press(v)
            self.assertEqual(shell.page,'game_result')
            self.assertGreater(store.pet['xp'],0)
            shell.press('MODE');self.assertEqual(shell.page,'home')
            shell.press('SLEEP');self.assertEqual(shell.render(),bytes(192))
            self.assertTrue(store.path.exists())

    def test_history_awards_only_new_launches(self):
        with tempfile.TemporaryDirectory() as d:
            store=PetStore(Path(d)/'pet.json')
            settings=Path(d)/'settings.json'
            old={'game':'C:/games/ChuChu Rocket.cdi','when':'2026-10-13T01:00:00','mode':'play'}
            settings.write_text(json.dumps({'history':[old]}))
            self.assertEqual(store.track_launcher_history(settings),0)
            self.assertEqual(store.pet['launches'],0)
            new={'game':'C:/games/Sonic Adventure.cdi','when':'2026-10-13T02:00:00','mode':'play'}
            settings.write_text(json.dumps({'history':[old,new]}))
            self.assertEqual(store.track_launcher_history(settings),1)
            self.assertEqual(store.track_launcher_history(settings),0)
            self.assertEqual(store.pet['launches'],1)
            self.assertIn('ZAPATILLAS',store.pet['items'])
            self.assertEqual(PetStore(store.path).pet['xp'],5)

    def test_first_launch_after_empty_settings_awards_xp(self):
        with tempfile.TemporaryDirectory() as d:
            store=PetStore(Path(d)/'pet.json')
            settings=Path(d)/'settings.json'
            self.assertEqual(store.track_launcher_history(settings),0)
            settings.write_text(json.dumps({'history':[{'game':'C:/games/ChuChu Rocket.cdi',
                                  'when':'2026-10-13T10:00:00','mode':'play'}]}))
            self.assertEqual(store.track_launcher_history(settings),1)
            self.assertIn('QUESITO',store.pet['items'])

    def test_corrupt_pet_does_not_get_overwritten(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'pet.json';path.write_text('broken')
            store=PetStore(path)
            self.assertTrue(store.invalid)
            self.assertFalse(store.save())
            self.assertEqual(path.read_text(),'broken')

    def test_real_lcd_remains_separate(self):
        with tempfile.TemporaryDirectory() as d:
            vmu=Path(d)/'vmu_a1.bin';vmu.write_bytes(bytes(128*1024))
            p=vmu.with_suffix('.lcd')
            now=int(time.time()*1000);p.write_bytes(MAGIC+struct.pack('<Q',now)+bytes([7])*192)
            saved=vmu.read_bytes()
            self.assertEqual(read_frame(p).bitmap,bytes([7])*192)
            store=PetStore(Path(d)/'pet.json');VMUShell(store).render()
            self.assertEqual(vmu.read_bytes(),saved)

    def test_launcher_standalone_command(self):
        cmd=independent_command('C:/vmus/a1.bin')
        self.assertIn('--vmu',cmd)
        self.assertIn('--vmu-image',cmd)
        self.assertNotIn('go',str(cmd).lower())

@unittest.skipIf(sys.platform=='win32' and not os.environ.get('DCR_TEST_GUI'),'Gui tests opt-in on Windows')
class WindowTests(unittest.TestCase):
    def test_gui_button_and_live_lcd(self):
        try:import tkinter as tk
        except ImportError:self.skipTest('Tk unavailable')
        with tempfile.TemporaryDirectory() as d:
            with patch('vmu_standalone.config_directory',return_value=Path(d)), \
                 patch('vmu_standalone.system_directory',return_value=Path(d)), \
                 patch('vmu_standalone.cache_directory',return_value=Path(d)):
                try:root=tk.Tk()
                except tk.TclError:self.skipTest('No DISPLAY')
                from vmu_standalone import StandaloneVMU
                try:
                    app=StandaloneVMU(root)
                    root.update_idletasks();root.update()
                    self.assertEqual(app.shell.page,'home')
                    app.shell.press('SLEEP');app._redraw()
                    self.assertTrue(app.shell.power)
                    app._menu_variable('skin','sapphire')
                    self.assertEqual(app.skin,'sapphire')
                    app._menu_variable('scale',0.75)
                    self.assertEqual(app.scale,0.75)
                    app._menu_variable('compact',True)
                    self.assertTrue(app.compact)
                    app._menu_variable('compact',False)
                    self.assertFalse(app.compact)
                    # A valid Maple sidecar should override the local screen.
                    image=Path(d)/'dreamcast_vmu_a1.bin'
                    image.write_bytes(bytes(128*1024))
                    image.with_suffix('.lcd').write_bytes(
                        MAGIC+struct.pack('<Q',int(time.time()*1000))+bytes([0xAA])*192)
                    app._set_image(image)
                    app.lcd_mode='auto';app._redraw()
                    self.assertEqual(app._frame_key[0],bytes([0xAA])*192)
                    app.lcd_mode='shell';app._redraw()
                    self.assertNotEqual(app._frame_key[0],bytes([0xAA])*192)
                finally:app.close()

if __name__=='__main__':unittest.main()
