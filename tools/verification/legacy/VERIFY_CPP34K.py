from pathlib import Path
import hashlib, sys
root=Path(__file__).resolve().parents[3]
checks=[]
def check(name, ok):
    checks.append((name, bool(ok)))
    print(('PASS' if ok else 'FAIL')+': '+name)
version=(root/'VERSION').read_text().strip()
check('VERSION cpp34k', version=='v1.2-cpp34k-STACKSAFE_IPBIN_REENTRY_ABI1')
hdr=(root/'runtime/include/dc_runtime.hpp').read_bytes()
check('DCRuntime ABI1 header unchanged', hashlib.sha256(hdr).hexdigest()=='ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739')
rt=(root/'runtime/dc_runtime.cpp').read_text()
for token in ['v1.2-cpp34k-stacksafe-ipbin-reentry','live-page-mask','live-pages-preserved','mark_live_page(live_sp)','preserved_dirty','static-pages=']:
    check('runtime token '+token, token in rt)
check('dispatch passes SP/PR', 'ipbin_restore_for_reentry(runtime, target, "dispatch", ctx.r[15], ctx.pr)' in rt)
plugin=(root/'runtime/plugins/dcr_network.cpp').read_text()
check('lazy fullstack remains', 'cpp34k LAZY FULLSTACK loaded (dormant until modem MMIO)' in plugin)
run=(root/'run_game.bat').read_text(errors='replace')
check('guest ABI remains 1', 'set "DCR_GUEST_ABI=1"' in run)
test=(root/'scripts/tests/TEST_CPP34K_HOMEPAGE_STACKSAFE.bat').read_text(errors='replace')
check('test forbids guest recompilation', '--reuse --rebuild-runtime' in test)
if not all(ok for _,ok in checks): sys.exit(1)
print('cpp34k verification complete')
