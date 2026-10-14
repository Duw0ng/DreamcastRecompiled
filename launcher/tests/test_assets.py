"""Static image-asset quality regression checks for the stable aqua launcher."""
from __future__ import annotations
import unittest
from pathlib import Path
try:
    from PIL import Image
except ImportError:
    Image=None
ASSETS=Path(__file__).resolve().parents[1]/'assets'
MAIN=('play','games','vmu','music','settings','controllers','display','network')
ADV=('audio','video','performance','saves','system','profiles','debug','language','themes')

@unittest.skipUnless(Image is not None,'Pillow required for image integrity audit, not for normal launcher')
class AssetTests(unittest.TestCase):
    def test_classic_background_is_from_original_and_not_new_swirl_pattern(self):
        for w in (800,960,1152):
            with Image.open(ASSETS/f'water_{w}.png') as image:
                self.assertEqual(image.size,(w,round(864*w/1152)))
                self.assertEqual(image.mode,'RGB')
        with Image.open(ASSETS/'classic_background_1152.png') as image:
            self.assertEqual(image.size,(1152,864))
        with Image.open(ASSETS/'aqua_native_background_1152.png') as legacy,Image.open(ASSETS/'water_1152.png') as classic:
            self.assertNotEqual(legacy.getpixel((490,250)),classic.getpixel((490,250)))

    def test_every_icon_has_three_sizes_and_true_alpha(self):
        for k in MAIN:
            for width in (800,960,1152):
                with self.subTest(name=k,width=width):
                    with Image.open(ASSETS/f'{k}_{width}.png') as im:
                        self.assertEqual(im.mode,'RGBA')
                        a=im.getchannel('A')
                        self.assertIsNotNone(a.getbbox())
                        self.assertLess(a.getextrema()[0],20)
        for k in ADV:
            for width in (800,960,1152):
                with self.subTest(name=k,width=width):
                    with Image.open(ASSETS/f'advanced_{k}_{width}.png') as im:
                        self.assertEqual(im.mode,'RGBA')
                        self.assertIsNotNone(im.getchannel('A').getbbox())

    def test_controllers_cutout_no_blue_bridge_under_grips(self):
        with Image.open(ASSETS/'controllers_1152.png') as a, Image.open(ASSETS/'original_cutouts'/'controllers.png') as b:
            area=(83,130,166,163)
            new=sum(a.convert('RGBA').getchannel('A').crop(area).get_flattened_data())
            old=sum(b.convert('RGBA').getchannel('A').crop(area).get_flattened_data())
            self.assertLess(new,old/20,'Old wallpaper was still opaque between the controller handles')

    def test_advanced_themes_caption_is_not_in_icon(self):
        with Image.open(ASSETS/'adv_themes.png') as im:
            self.assertIsNone(im.convert('RGBA').getchannel('A').crop((0,61,im.width,im.height)).getbbox())

    def test_classic_button_normal_and_active_sizes(self):
        # Verify all eight capsules have size-matched source and illuminated
        # states; no opaque rectangle or geometry change on hover.
        for key in MAIN:
            for width in (800,960,1152):
                with self.subTest(key=key,width=width):
                    with Image.open(ASSETS/f'button_{key}_normal_{width}.png') as normal, \
                         Image.open(ASSETS/f'button_{key}_active_{width}.png') as active:
                        self.assertEqual(normal.size,active.size)
                        self.assertEqual(normal.mode,'RGBA')
                        self.assertEqual(active.mode,'RGBA')
                        self.assertNotEqual(normal.tobytes(),active.tobytes())

if __name__=='__main__': unittest.main()

