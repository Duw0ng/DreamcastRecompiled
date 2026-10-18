"""Regression tests for the English-first GitHub website and release documentation."""
from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[1]

class EnglishGitHubContentTests(unittest.TestCase):
    def test_public_readme_no_longer_points_to_spanish_guide(self):
        readme=(ROOT/"README.md").read_text(encoding="utf-8")
        self.assertIn("[Language support]",readme)
        self.assertNotIn("[Español]",readme)
        self.assertIn("SGGG",readme)

    def test_stable_release_description_is_english(self):
        release=(ROOT/"RELEASE_NOTES_V1.2.md").read_text(encoding="utf-8")
        self.assertIn("English/Spanish UI",release)
        self.assertNotIn("Español/Inglés",release)

    def test_compatibility_data_uses_english_filter_values(self):
        data=json.loads((ROOT/"web/data/compatibility.json").read_text(encoding="utf-8"))
        rows=data["games"]+data["additionalGames"]
        self.assertTrue(rows)
        for game in rows:
            self.assertIn(game["confidence"],("High","Medium","Needs review"))
            self.assertIn(game["priority"],("High","Medium","Low"))
            self.assertNotIn("sin PC",game["group"])
            self.assertNotIn("Prueba adicional",game["group"])

    def test_web_priority_filter_matches_json(self):
        html=(ROOT/"web/compatibility.html").read_text(encoding="utf-8")
        for value in ("High","Medium","Low"):
            self.assertIn("<option>"+value+"</option>",html)
        self.assertNotIn("<option>Alta</option>",html)

    def test_portable_release_notes_preserve_sggg_fix(self):
        from importlib.util import spec_from_file_location, module_from_spec
        script=ROOT/"game-portables/tools/english_release_notes.py"
        spec=spec_from_file_location("dcr_release_notes",script)
        module=module_from_spec(spec)
        spec.loader.exec_module(module)
        for game in module.GAMES:
            body=module.release_text(game)
            self.assertIn("plain/scrambled",body)
            self.assertIn("32 KiB",body)
            self.assertNotIn("Novedades de esta versión",body)
        self.assertIn("KYONB",module.release_text("SGGG"))
        self.assertIn("KYONEX",module.release_text("SGGG"))
        self.assertIn("repeated navigation/menu",module.release_text("SGGG"))

if __name__=="__main__":
    unittest.main()
