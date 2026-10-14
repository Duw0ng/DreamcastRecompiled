from pathlib import Path
import sys
root=Path(__file__).resolve().parents[3]
checks={
 'no_embed_option': '--no-embed-raw-image' in (root/'run_game.bat').read_text(),
 'portableize': '--portableize' in (root/'run_game.bat').read_text(),
 'auto_sync': 'sync_portable.py' in (root/'run_game.bat').read_text(),
 'external_runner': '--external-raw-image' in (root/'src/codegen/cpp_emitter.cpp').read_text(),
 'no_embed_codegen': 'embed_raw_image' in (root/'src/codegen/cpp_emitter.cpp').read_text(),
 'launcher': (root/'portable/DreamcastRecompiled.exe').is_file(),
 'sync_tool': (root/'tools/portable/sync_portable.py').is_file(),
}
for k,v in checks.items(): print(f'{k}: {"PASS" if v else "FAIL"}')
raise SystemExit(0 if all(checks.values()) else 1)
