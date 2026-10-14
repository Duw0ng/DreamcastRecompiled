"""Automatic, compiler-free launch of validated per-game native portables.

The portable guest DLL is never copied into a different game's cache.  Disc
metadata is read from the *user's* CDI/ISO with the game's bundled probe, then
IP.BIN, the executable and the disc map are staged locally for the host runtime.
This module is stdlib-only and can be run by the existing reusable Aqua host.
"""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass

EXPECTED_ABI = '1'
EXPECTED_RUNTIME = 'cpp38e-v8.6-sh4-dynamic-ram-aot-v1'
CHUCHU_ID = re.compile(r'CHU\s*CHU\s*ROCKET', re.IGNORECASE)

REQUIRED = (
    'bin/dreamcast_program.exe',
    'bin/dcr_game_module.dll',
    'bin/DreamcastRuntime.dll',
    'bin/dc_disc_probe.exe',
    'bin/dc_boot_prepare.exe',
    'bin/plugins/dcr_gdrom.dll',
    'bin/plugins/dcr_maple.dll',
    'bin/plugins/dcr_network.dll',
)

@dataclass(frozen=True)
class NativeBundle:
    folder: Path
    game: str
    title: str


def parse_manifest(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding='utf-8-sig').splitlines():
        line = line.strip()
        if not line or line.startswith(('#',';')):
            continue
        if '=' not in line:
            raise ValueError(f'Manifiesto invalido: {path}')
        name, value = line.split('=', 1)
        values[name.strip().upper()] = value.strip()
    return values


def _sha_manifest(bundle_dir: Path) -> None:
    """Optional distribution signature: guard packaged bytes against corruption.

    SHA-256 is an integrity check against accidental file changes, not a trust
    guarantee or anti-piracy mechanism.  Do not accept paths outside package.
    """
    manifest = bundle_dir / 'checksums.sha256'
    if not manifest.is_file():
        return
    for row in manifest.read_text(encoding='ascii').splitlines():
        if not row.strip():
            continue
        digest, rel = row.split('  ', 1)
        if not re.fullmatch(r'[a-fA-F0-9]{64}', digest):
            raise ValueError('Hash invalido en checksums.sha256')
        if rel not in REQUIRED:
            raise ValueError(f'Archivo no autorizado en checksum: {rel}')
        file = bundle_dir / rel
        checksum = hashlib.sha256()
        with file.open('rb') as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b''):
                checksum.update(chunk)
        if checksum.hexdigest().lower() != digest.lower():
            raise ValueError(f'Portable modificado o incompleto: {rel}')


def load_bundle(root: Path, game: str = 'CHUCHU_ROCKET', *, verify_hashes: bool = True) -> NativeBundle | None:
    path = root / 'games' / game
    manifest = path / 'portable_manifest.txt'
    if not manifest.is_file():
        return None
    info = parse_manifest(manifest)
    for key, value in (('DCR_PORTABLE_GUEST','1'), ('GUEST_ABI',EXPECTED_ABI),
                       ('RUNTIME',EXPECTED_RUNTIME), ('GAME',game)):
        if info.get(key) != value:
            raise ValueError(f'Portable {game} incompatible: {key}={info.get(key)!r}, se necesita {value!r}')
    for name in REQUIRED:
        file = path / name
        if not file.is_file() or file.stat().st_size < 256:
            raise FileNotFoundError(f'Falta un binario portable: {file}')
        with file.open('rb') as binary:
            magic = binary.read(2)
        if magic != b'MZ':
            raise ValueError(f'Binario portable Windows invalido: {file}')
    if verify_hashes:
        _sha_manifest(path)
    return NativeBundle(path,game,info.get('TITLE',game))


def disc_metadata(disc_map: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for row in disc_map.read_text(encoding='utf-8-sig', errors='replace').splitlines():
        key, _, value = row.partition('=')
        if key.casefold().strip() in ('title','volume'):
            fields[key.lower().strip()] = value.strip()
    return fields


def disc_matches(bundle: NativeBundle, fields: dict[str, str]) -> bool:
    identity = ' '.join(fields.get(key,'') for key in ('title','volume'))
    if bundle.game == 'CHUCHU_ROCKET':
        return CHUCHU_ID.search(identity) is not None
    return False


def _check_run(cmd: list[str], *, cwd: Path, env: dict[str,str] | None = None) -> None:
    result = subprocess.run(cmd, cwd=str(cwd), env=env, check=False)
    if result.returncode != 0:
        raise RuntimeError(f'Falló {Path(cmd[0]).name} (RC={result.returncode}); ver launcher_console.log')


def create_native_command(bundle: NativeBundle, disc_map: Path, bootstrap: Path,
                          disc: Path, settings: dict) -> tuple[list[str], dict[str,str],Path]:
    """Reproduce flags and environment of the cpp38e run_game.bat native path."""
    from core import effective_game_settings, launcher_environment, cache_directory
    opts = effective_game_settings(settings, disc)
    renderer = opts['renderer'] if opts['renderer'] != 'auto' else 'dx11'
    env = launcher_environment(settings, disc)
    system = cache_directory() / 'system'
    system.mkdir(parents=True, exist_ok=True)
    env.update({
        'DCR_PLUGIN_DIR': str(bundle.folder / 'bin' / 'plugins'),
        'DCR_EXTERNAL_RAW_IMAGE': str(bootstrap),
        'DCR_EXTERNAL_RAW_BASE': '0x8C008000',
        'DCR_SYSTEM_DATA_DIR': str(system),
        'DCR_GAME_TITLE': bundle.title,
        'DCR_WINDOW_TITLE': f'DCRV1.2 - {bundle.title}',
        'DCR_STATUS_WINDOW_TITLE': f'DCRV1.2 Diagnostics - {bundle.title}',
        'DCR_FPU_MODE': opts['fpu_mode'],
        'DCR_FPU_METRICS': '0',
        'DCR_HOST_SYNC_QUANTUM': '262144',
        'DCR_SPG_WAIT_FF': '1' if opts['spg_fast_forward'] else '0',
        'DCR_CPP38E_HOT_RECOVERY': '1' if opts['hot_recovery'] else '0',
        'DCR_SH4_INTERP': '1' if opts['sh4_interp'] else '0',
        'DCR_SH4_INTERP_TRACE': '1' if opts['sh4_interp_trace'] else '0',
        'DCR_SH4_DYNAMIC_AOT': '1' if opts['dynamic_aot'] else '0',
        'DCR_SH4_BAD_TARGET_TRACE': '1' if opts['bad_target_trace'] else '0',
        'DCR_TRACE_HISTORY': '1' if opts['debug'] else '0',
        'DCR_MAPLE_TRACE': '1' if opts['maple_trace'] else '0',
        'DCR_GDROM_TRACE': '1' if opts['gdrom_trace'] else '0',
        'DCR_CODE_RELOC_TRACE': '1' if opts['code_reloc_trace'] else '0',
    })
    args = [str(bundle.folder / 'bin' / 'dreamcast_program.exe'),
            '--commercial-boot','--commercial-boot-stage=plain',
            '--disc-map='+str(disc_map),
            '--pvr-window','--pvr-frame-sync']
    if renderer == 'dx11':
        args += ['--pvr-gpu','--pvr-render-done-scheduled','--pvr-mt']
    else:
        args += ['--pvr-mt']
    args += ['--fast-dispatch','--direct-dispatch','--sh4-tick-batch=256','--maple-host-input']
    if opts['audio']:
        if opts['audio_mode'] == 'basic':
            args += ['--aica-arm7','--aica-play']
        elif opts['audio_mode'] == 'off':
            args += ['--aica-arm7','--aica-arm7-slice=128','--aica-arm7-boot=32768']
        else:
            args += ['--aica-arm7','--aica-arm7-slice=128','--aica-arm7-boot=32768','--aica-play']
    else:
        args += ['--aica-arm7','--aica-arm7-slice=128','--aica-arm7-boot=32768']
    controller_profile = bundle.folder.parent.parent / 'profiles' / 'controller_profile.ini'
    if controller_profile.is_file():
        args += ['--controller-profile='+str(controller_profile)]
    args += ['--device-clock-host','--device-clock-host-max-catchup=4096']
    if opts['perf']: args += ['--perf-profile','--perf-sample-stride=64']
    if opts['debug']: args += ['--host-window']
    if opts['pvr_profile']: args += ['--pvr-profile','--host-window']
    args += ['--diag-heartbeat-ms='+('1000' if opts['heartbeats'] else '0')]
    return args, env, bundle.folder / 'bin'


def launch_auto_game(root: Path, disc: Path) -> int:
    """Called from the separate Aqua host process, never the Tk event loop."""
    from core import build_launch_command, launcher_environment, load_settings, is_supported_disc
    root = root.resolve()
    disc = disc.resolve()
    settings = load_settings()
    if not is_supported_disc(disc):
        raise ValueError(f'CDI/ISO inválido o no encontrado: {disc}')
    bundle = load_bundle(root)
    if bundle is None:
        return _legacy(root,disc,settings)
    # Session data lives outside the installation and never contains a bundled
    # game image; it is discarded after the running game process exits.
    from core import cache_directory
    sessions = cache_directory() / 'games' / bundle.game / 'portable_sessions'
    sessions.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='boot_', dir=str(sessions)) as directory:
        work = Path(directory)
        probe = str(bundle.folder / 'bin' / 'dc_disc_probe.exe')
        cmd = [probe,str(disc),
               '--extract-ip='+str(work/'IP.BIN'),
               '--extract-boot='+str(work/'BOOT.DISC.BIN'),
               '--extract-disc-map='+str(work/'disc.map')]
        print('[AQUA PRECOMPILED] Disc probe:',disc,flush=True)
        result = subprocess.run(cmd, cwd=str(bundle.folder/'bin'), check=False)
        if result.returncode != 0:
            print('[AQUA PRECOMPILED] Disc probe fallo; usando runner convencional.',flush=True)
            return _legacy(root,disc,settings)
        metadata = disc_metadata(work/'disc.map')
        if not disc_matches(bundle, metadata):
            print('[AQUA PRECOMPILED] Otro juego detectado:',metadata,'-> runner convencional.',flush=True)
            return _legacy(root,disc,settings)
        print('[AQUA PRECOMPILED] CHUCHU_ROCKET detectado, sin recompilar SH-4.',flush=True)
        _check_run([str(bundle.folder/'bin'/'dc_boot_prepare.exe'),
                    str(work/'IP.BIN'),str(work/'BOOT.DISC.BIN'),
                    '--mode=auto','--boot-out='+str(work/'BOOT.BIN'),
                    '--combined-out='+str(work/'BOOTSTRAP.BIN')],cwd=bundle.folder/'bin')
        args, env, cwd = create_native_command(bundle,work/'disc.map',work/'BOOTSTRAP.BIN',disc,settings)
        print('[AQUA PRECOMPILED] Loading',args[0],flush=True)
        return subprocess.call(args,cwd=str(cwd),env=env)


def _legacy(root: Path, disc: Path, settings: dict) -> int:
    from core import build_launch_command, launcher_environment
    print('[AQUA] No hay portable compatible para este disco; modo incremental normal.',flush=True)
    args = build_launch_command(root,disc,settings)
    return subprocess.call(args,cwd=str(root),env=launcher_environment(settings,disc))


def auto_game_command(root: Path, disc: Path) -> list[str]:
    """Launch same persistent Python interpreter, no extra Python dependency."""
    root = root.resolve()
    if getattr(sys, 'frozen', False):
        return [sys.executable, '--auto-game', str(disc)]
    return [sys.executable, str(root/'launcher'/'offline_host.py'), '--auto-game', str(disc)]


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if len(args) != 2 or args[0] != '--auto-game':
        print('Uso: --auto-game <ruta.cdi|ruta.iso>',file=sys.stderr)
        return 2
    root = (Path(sys.executable).resolve().parent if getattr(sys,'frozen',False)
            else Path(__file__).resolve().parent.parent)
    try:
        return launch_auto_game(root,Path(args[1]))
    except Exception as exc:
        print('[AQUA PRECOMPILED] ERROR:',exc,file=sys.stderr,flush=True)
        return 14


if __name__ == '__main__':
    raise SystemExit(main())
