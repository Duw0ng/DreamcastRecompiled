"""Functional GUI tests (run in desktop/Xvfb, Tkinter stdlib)."""
import os
import sys
import unittest
import tkinter as tk
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from main import Launcher
from aqua_ui import MAIN, ADV

@unittest.skipUnless(os.environ.get('DISPLAY') or sys.platform == 'win32', 'Desktop required')
class GuiSmokeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root=tk.Tk()
        cls.ui=Launcher(cls.root)
        cls.ui.scene.extended=True
        cls.ui.scene._layout()
        cls.ui.scene.draw()
        cls.root.update()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()

    def test_main_and_advanced_buttons_have_clickable_centers(self):
        keys=(*MAIN,*ADV)
        for key in keys:
            b=self.ui.scene.hotspots[key]
            x=(b[0]+b[2])/2
            y=(b[1]+b[3])/2
            self.assertEqual(self.ui.scene.hit_test(round(x*self.ui.scene.scale),round(y*self.ui.scene.scale)), key)

    def test_clock_is_live_native_text(self):
        text=self.ui.scene.canvas.itemcget(self.ui.scene.clock_text,'text')
        date=self.ui.scene.canvas.itemcget(self.ui.scene.clock_date,'text')
        self.assertIn('/',date)
        self.assertIn(':',text)
        self.assertNotIn('1998',text)
        self.assertEqual(self.ui.root.winfo_children()[0],self.ui.canvas)
        self.assertEqual(len([w for w in self.ui.root.winfo_children() if isinstance(w,tk.Frame)]),0)

    def test_advanced_toggle_changes_layout_and_persists(self):
        orig=self.ui.scene.extended
        self.ui.scene.toggle_advanced()
        self.root.update()
        self.assertNotIn('audio', self.ui.scene.hotspots)
        self.assertFalse(self.ui.settings['advanced_open'])
        self.assertLess(self.ui.scene.canvas.winfo_height(),round(800*self.ui.scene.scale))
        self.ui.scene.toggle_advanced()
        self.root.update()
        self.assertIn('audio',self.ui.scene.hotspots)
        self.assertTrue(self.ui.settings['advanced_open'])
        self.assertEqual(self.ui.scene.extended,orig)

    def test_keyboard_navigation(self):
        self.ui.scene.focus='play'
        self.ui.scene.navigate(1)
        self.assertEqual(self.ui.scene.focus,'games')
        self.ui.scene.navigate(4)
        self.assertEqual(self.ui.scene.focus,'controllers')

    def test_illumination_not_rectangle_selection(self):
        original_focus=self.ui.scene.focus
        scene=self.ui.scene
        scene.pointer_mode='mouse'
        scene.hover='play';scene._set_selection()
        self.assertEqual(scene.focus,original_focus)
        self.assertEqual(scene._selected,'play')
        self.assertEqual(scene.canvas.itemcget(scene.button_items['play'],'image'),
                         str(scene.images['button_play_active']))
        scene.hover=None;scene._set_selection()

    def test_hover_redraw_releases_old_clock_items(self):
        for key in ('play','games','vmu','music',None)*5:
            self.ui.scene.hover=key
            self.ui.scene.draw()
        self.assertEqual(len(self.ui.scene.canvas.find_withtag('clock')),2)
        self.ui.scene.hover=None


    def test_mouse_motion_does_not_destroy_canvas_items(self):
        # The old version destroyed/recreated the whole canvas on each hover,
        # exposing a solid blue background for ~1 second on Windows.
        import time
        scene=self.ui.scene
        scene.draw()
        before=tuple(scene.canvas.find_all())
        start=time.perf_counter()
        for index in range(250):
            key=MAIN[index % len(MAIN)]
            cx=(scene.hotspots[key][0]+scene.hotspots[key][2])/2
            cy=(scene.hotspots[key][1]+scene.hotspots[key][3])/2
            scene.motion(type('Motion',(),{'x':round(cx*scene.scale),'y':round(cy*scene.scale)}))
        elapsed=time.perf_counter()-start
        self.assertEqual(before,tuple(scene.canvas.find_all()),'Hover rebuilt canvas objects')
        self.assertLess(elapsed,1.0,'Hover takes too long even without Tk redraw')
        scene.leave()

    def test_selection_only_one_illuminated(self):
        scene=self.ui.scene
        scene.draw()
        scene.pointer_mode='mouse';scene.hover='controllers';scene._set_selection()
        self.assertEqual(scene._selected,'controllers')
        active=[key for key in MAIN if scene.canvas.itemcget(scene.button_items[key],'image') ==
                str(scene.images[f'button_{key}_active'])]
        self.assertEqual(active,['controllers'])
        scene.hover=None;scene._set_selection()
        self.assertIsNone(scene._selected)
        self.assertTrue(all(scene.canvas.itemcget(scene.button_items[key],'image') ==
                str(scene.images[f'button_{key}_normal']) for key in MAIN))

    def test_hover_gap_never_resurrects_stale_keyboard_focus(self):
        scene=self.ui.scene
        scene.focus='play'
        scene.pointer_mode='keyboard';scene._set_selection()
        self.assertEqual(scene._selected,'play')
        for k in ('games',None,'controllers',None,'network',None):
            if k is None:
                scene.motion(type('Motion',(),{'x':round(580*scene.scale),'y':round(395*scene.scale)}))
                self.assertIsNone(scene._selected)
            else:
                b=scene.hotspots[k]
                scene.motion(type('Motion',(),{'x':round((b[0]+b[2])/2*scene.scale),'y':round((b[1]+b[3])/2*scene.scale)}))
                self.assertEqual(scene._selected,k)
            self.assertNotEqual(scene._selected,'play')

    def test_hitboxes_do_not_overlap_between_rows(self):
        scene=self.ui.scene
        for i in range(4):
            upper=scene.hotspots[MAIN[i]]
            lower=scene.hotspots[MAIN[i+4]]
            self.assertLess(upper[3],lower[1])
        self.assertGreater(scene.hotspots['audio'][1],scene.hotspots['advanced'][1])

    def test_button_hover_only_updates_existing_artwork(self):
        scene=self.ui.scene
        scene.pointer_mode='mouse';scene.hover=None;scene._set_selection()
        ids=tuple(scene.canvas.find_all())
        for idx in range(2000):
            scene.hover=MAIN[idx%len(MAIN)];scene._set_selection()
            scene.hover=None;scene._set_selection()
        self.assertEqual(ids,tuple(scene.canvas.find_all()))
        self.assertIsNone(scene._selected)

    def test_advanced_collapse_clears_old_illumination(self):
        scene=self.ui.scene
        scene.extended=True;scene._layout();scene.draw()
        scene.hover='audio';scene.pointer_mode='mouse';scene._set_selection()
        self.assertEqual(scene._selected,'audio')
        scene.toggle_advanced()
        self.assertIsNone(scene._selected)
        self.assertEqual(scene.canvas.itemcget(scene.advanced_rings['audio'],'fill'),'#cee9ff')
        scene.toggle_advanced()
        self.assertEqual(scene.canvas.itemcget(scene.advanced_rings['audio'],'fill'),'#cee9ff')
        scene.hover=None;scene._set_selection()

    def test_collapse_no_scene_rebuild(self):
        scene=self.ui.scene
        scene.extended=True;scene._layout();scene.draw()
        ids=tuple(scene.canvas.find_all())
        scene.toggle_advanced()
        self.root.update()
        self.assertEqual(ids,tuple(scene.canvas.find_all()))
        self.assertEqual(scene.canvas.itemcget(scene.advanced_arrow,'text'),'⌄')
        self.assertEqual(scene.canvas.itemcget(scene.canvas.find_withtag('advanced_content')[0],'state'),'hidden')
        scene.toggle_advanced()
        self.root.update()
        self.assertEqual(ids,tuple(scene.canvas.find_all()))
        self.assertEqual(scene.canvas.itemcget(scene.advanced_arrow,'text'),'⌃')

    def test_dynamic_clock_date_and_time_have_separate_positions(self):
        scene=self.ui.scene
        scene.draw()
        date_bbox=scene.canvas.bbox(scene.clock_date)
        time_bbox=scene.canvas.bbox(scene.clock_time)
        self.assertLess(date_bbox[2],time_bbox[0])
        self.assertGreater(time_bbox[0]-date_bbox[2],round(6*scene.scale))

    def test_themes_icon_has_no_old_caption_pixels(self):
        try:
            from PIL import Image
        except ImportError:
            self.skipTest('Pillow optional: used only for sprite quality audit')
        with Image.open(Path(__file__).resolve().parents[1]/'assets'/'advanced_themes_1152.png') as image:
            image=image.convert('RGBA')
            # The previous asset accidentally contained two old text rows
            # and a blue background fragment underneath the paint palette.
            self.assertFalse(image.getchannel('A').crop((0,61,image.width,image.height)).getbbox())

    def test_inline_pages_construct_without_toplevel(self):
        for key in ('settings','games','controllers','vmu','music','network',
                    'profiles','debug','system','language','themes','video','audio','performance'):
            self.ui.activate(key)
            self.root.update()
            self.assertIsNotNone(self.ui.scene.page_frame)
            self.assertTrue(self.ui.scene.page_frame.winfo_ismapped())
            windows=[x for x in self.root.winfo_children() if isinstance(x,tk.Toplevel)]
            self.assertEqual(windows,[],f'{key} unexpectedly created a popup')
            self.ui.scene.close_inline()
            self.root.update()

    def test_canvas_click_opens_settings_inline(self):
        b=self.ui.scene.hotspots['settings'];scale=self.ui.scene.scale
        x=round(((b[0]+b[2])/2)*scale);y=round(((b[1]+b[3])/2)*scale)
        self.ui.canvas.event_generate('<Button-1>',x=x,y=y)
        self.root.update()
        self.assertIsNotNone(self.ui.scene.page_frame)
        self.assertEqual([x for x in self.root.winfo_children() if isinstance(x,tk.Toplevel)],[])
        self.ui.scene.close_inline()
        self.root.update()

if __name__=='__main__': unittest.main()
