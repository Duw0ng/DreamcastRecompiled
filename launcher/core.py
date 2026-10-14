"""Safe configuration and command building for Dreamcast Recompiled v1.2.

Pure standard-library module; importable without a graphical desktop.
stable engine code is intentionally not modified by this launcher.
"""
from __future__ import annotations

import json
import copy
import os
from pathlib import Path
import tempfile

SUPPORTED_SUFFIXES = {'.cdi', '.iso'}  # dc_disc_probe in this build documents CDI and ISO.
RENDERERS = ('auto', 'dx11', 'software')
VIDEO_CHOICES = {'resolution': ('native', '960p', '1080p', '1440p', 'desktop'),
                 'aspect': ('4:3', '16:9', 'auto'),
                 'filter': ('nearest', 'bilinear', 'sharp'),
                 'output_backend': ('dx11', 'gdi')}
VIDEO_DEFAULTS = {'resolution': 'native', 'aspect': '4:3', 'output_backend': 'dx11',
                  'filter': 'bilinear', 'fullscreen': False, 'vsync': False,
                  'fps_counter': False}
AUDIO_DEFAULTS = {'volume': 100, 'device': 'default', 'latency_ms': 92}
CONTROLLER_DEFAULTS = {'deadzone_pct': 15, 'rumble': True, 'player_index': 1, 'player_devices': {}, 'calibrations': {}}
LAUNCHER_WIDTHS = ('auto', '800', '960', '1152')

# Only settings in SUPPORTED_RUNTIME_KEYS have concrete command-line effects.
# Future video/audio settings are deliberately NOT converted into runtime flags.
SUPPORTED_RUNTIME_KEYS = frozenset(('renderer','audio','perf','debug','pvr_profile','maple_trace',
 'gdrom_trace','spg_fast_forward','hot_recovery','heartbeats','code_reloc_trace',
 'sh4_interp','sh4_interp_trace','network_mode','build_mode','native_seeds',
 'native_patches','show_console','hotpc','fpu_mode','audio_mode','dynamic_aot','bad_target_trace'))

DEFAULTS = {
    'library': [], 'last_game': '', 'history': [], 'language': 'es', 'theme': 'classic',
    'advanced_open': True,
    'renderer': 'auto', 'audio': True, 'perf': False, 'debug': False,
    'show_console': False, 'heartbeats': False, 'code_reloc_trace': False,
    'network_mode': 'auto', 'build_mode': 'incremental', 'audio_mode': 'full',
    'native_seeds': True, 'native_patches': True, 'fpu_mode': 'superblock', 'hotpc': False,
    # interp-fallback-v1: unknown SH-4 targets are interpreted (default ON).
    'sh4_interp': True, 'sh4_interp_trace': False,
    'dynamic_aot': False, 'bad_target_trace': False,
    'pvr_profile': False, 'maple_trace': False, 'gdrom_trace': False,
    'spg_fast_forward': True, 'hot_recovery': True,
    'game_overrides': {}, 'launcher_width': 'auto',
    'video_future': VIDEO_DEFAULTS, 'audio_future': AUDIO_DEFAULTS,
    'controller_future': CONTROLLER_DEFAULTS,
}



def sanitize_video(raw: object) -> dict:
    result = copy.deepcopy(VIDEO_DEFAULTS)
    if not isinstance(raw, dict):
        return result
    for key, allowed in VIDEO_CHOICES.items():
        if raw.get(key) in allowed:
            result[key] = raw[key]
    for key in ('fullscreen', 'vsync', 'fps_counter'):
        if type(raw.get(key)) is bool:
            result[key] = raw[key]
    return result


def sanitize_audio(raw: object) -> dict:
    result = copy.deepcopy(AUDIO_DEFAULTS)
    if not isinstance(raw, dict):
        return result
    if type(raw.get('volume')) is int and 0 <= raw['volume'] <= 100:
        result['volume'] = raw['volume']
    if type(raw.get('latency_ms')) is int and 20 <= raw['latency_ms'] <= 250:
        result['latency_ms'] = raw['latency_ms']
    if raw.get('device') == 'default':
        result['device'] = 'default'
    return result


def sanitize_controller(raw: object) -> dict:
    result = copy.deepcopy(CONTROLLER_DEFAULTS)
    if not isinstance(raw, dict):
        return result
    if type(raw.get('deadzone_pct')) is int and 0 <= raw['deadzone_pct'] <= 40:
        result['deadzone_pct'] = raw['deadzone_pct']
    if type(raw.get('rumble')) is bool:
        result['rumble'] = raw['rumble']
    if type(raw.get('player_index')) is int and 1 <= raw['player_index'] <= 4:
        result['player_index'] = raw['player_index']
    devices=raw.get('player_devices',{})
    if isinstance(devices,dict):
        for player in ('1','2','3','4'):
            entry=devices.get(player,{})
            if not isinstance(entry,dict):continue
            backend=entry.get('backend')
            index=entry.get('index')
            if backend in ('auto','xinput','winmm','keyboard_mouse','none') and type(index) is int and 0<=index<=15:
                result['player_devices'][player]={'backend':backend,'index':index}
    calibrations=raw.get('calibrations',{})
    if isinstance(calibrations,dict):
        from controller_profiles import BINDING_PATTERN
        for player in ('1','2','3','4'):
            if player not in calibrations:continue
            entry=calibrations.get(player,{})
            if not isinstance(entry,dict):continue
            mappings=entry.get('bindings',{})
            if not isinstance(mappings,dict):continue
            allowed=('joy_x','joy_y','joy2_x','joy2_y','ltrig','rtrig','a','b','x','y','start','up','down','left','right','c','z')
            safe={k:v for k,v in mappings.items() if k in allowed and isinstance(v,str) and BINDING_PATTERN.fullmatch(v)}
            deadzone=entry.get('deadzone_pct',result['deadzone_pct'])
            if type(deadzone) is not int or not 0<=deadzone<=50:
                deadzone=result['deadzone_pct']
            backend=entry.get('backend','auto')
            index=entry.get('index',int(player)-1)
            if backend not in ('auto','xinput','winmm'):
                backend='auto'
            if type(index) is not int or not 0<=index<=15:
                index=int(player)-1
            result['calibrations'][player]={'bindings':safe,'deadzone_pct':deadzone,'backend':backend,'index':index}
    return result


def reset_to_stable(settings: dict) -> dict:
    """Restore stable runtime defaults, retaining games, UI and file records."""
    current = sanitize_settings(settings)
    fresh = sanitize_settings({})
    for key in ('library', 'last_game', 'history', 'language', 'theme', 'advanced_open',
                'launcher_width', 'game_overrides'):
        fresh[key] = copy.deepcopy(current[key])
    return fresh


def effective_game_settings(settings: dict, disc: str | Path) -> dict:
    """Return effective supported + staged game preferences; no side effects."""
    base = sanitize_settings(settings)
    override = base['game_overrides'].get(normalized_disc(disc), {})
    ren = override.get('renderer', 'inherit')
    if ren != 'inherit':
        base['renderer'] = ren
    audio = override.get('audio', 'inherit')
    if audio != 'inherit':
        base['audio'] = (audio == 'on')
    if 'video_future' in override:
        base['video_future'] = sanitize_video(override['video_future'])
    return base

def config_directory() -> Path:
    """Keep user data separate from SH-4 guest cache and source tree."""
    base = os.environ.get('LOCALAPPDATA')
    if base:
        return Path(base) / 'DreamcastRecompiled' / 'launcher_v1_2'
    return Path.home() / '.dreamcast_recompiled' / 'launcher_v1_2'


def cache_directory() -> Path:
    base = os.environ.get('LOCALAPPDATA')
    if base:
        return Path(base) / 'DreamcastRecompiled'
    return Path.home() / '.dreamcast_recompiled'


def normalized_disc(value: str | Path) -> str:
    return str(Path(value).expanduser().resolve())


def is_supported_disc(value: str | Path) -> bool:
    p = Path(value)
    return p.suffix.lower() in SUPPORTED_SUFFIXES and p.is_file()


def sanitize_settings(raw: object) -> dict:
    result = copy.deepcopy(DEFAULTS)
    if not isinstance(raw, dict):
        return result
    if isinstance(raw.get('library'), list):
        result['library'] = list(dict.fromkeys(x for x in raw['library'] if isinstance(x, str) and x))[:1500]
    if isinstance(raw.get('history'), list):
        for item in raw['history'][-250:]:
            if not isinstance(item, dict):continue
            if not isinstance(item.get('game'), str) or len(item['game'])>2048:continue
            if not isinstance(item.get('when'), str) or len(item['when'])>40:continue
            mode = item.get('mode','play')
            if mode not in ('play','prepare'):continue
            result['history'].append({'game':item['game'],'when':item['when'],'mode':mode})
    if isinstance(raw.get('last_game'), str):
        result['last_game'] = raw['last_game']
    for k in ('language', 'theme'):
        values = {'language': ('es', 'en'), 'theme': ('classic', 'dark', 'night', 'starry')}[k]
        if raw.get(k) in values:
            result[k] = 'night' if k == 'theme' and raw[k] == 'dark' else raw[k]
    if raw.get('launcher_width') in LAUNCHER_WIDTHS:
        result['launcher_width'] = raw['launcher_width']
    result['video_future'] = sanitize_video(raw.get('video_future'))
    result['audio_future'] = sanitize_audio(raw.get('audio_future'))
    result['controller_future'] = sanitize_controller(raw.get('controller_future'))
    if raw.get('renderer') in RENDERERS:
        result['renderer'] = raw['renderer']
    for key, allowed in {
        'network_mode': ('auto','dcnet','off'),
        'build_mode': ('incremental','reuse','full'),
        'audio_mode': ('full','basic','off'),
        'fpu_mode': ('superblock','regionplus'),
    }.items():
        if raw.get(key) in allowed:
            result[key] = raw[key]
    for k in ('advanced_open', 'audio', 'perf', 'debug', 'pvr_profile', 'maple_trace', 'gdrom_trace',
              'spg_fast_forward', 'hot_recovery', 'show_console', 'heartbeats', 'code_reloc_trace',
              'sh4_interp', 'sh4_interp_trace', 'native_seeds', 'native_patches', 'hotpc',
              'dynamic_aot', 'bad_target_trace'):
        if type(raw.get(k)) is bool:
            result[k] = raw[k]
    ov = raw.get('game_overrides')
    if isinstance(ov, dict):
        for game, value in list(ov.items())[:1500]:
            if not isinstance(game, str) or not isinstance(value, dict):
                continue
            clean = {}
            if value.get('renderer') in (*RENDERERS, 'inherit'):
                clean['renderer'] = value['renderer']
            if value.get('audio') in ('inherit', 'on', 'off'):
                clean['audio'] = value['audio']
            if isinstance(value.get('video_future'), dict):
                clean['video_future'] = sanitize_video(value['video_future'])
            if clean:
                result['game_overrides'][game] = clean
    return result


def load_settings(path: Path | None = None) -> dict:
    path = path or config_directory() / 'settings.json'
    try:
        with path.open('r', encoding='utf-8') as f:
            return sanitize_settings(json.load(f))
    except (FileNotFoundError, ValueError, OSError, UnicodeError):
        return sanitize_settings({})


def save_settings(settings: dict, path: Path | None = None) -> Path:
    path = path or config_directory() / 'settings.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix='settings-', suffix='.tmp', dir=path.parent)
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as f:
            json.dump(sanitize_settings(settings), f, indent=2, ensure_ascii=False)
            f.write('\n')
        os.replace(name, path)
    finally:
        if os.path.exists(name):
            os.unlink(name)
    return path


def add_to_library(settings: dict, paths: list[str | Path]) -> int:
    seen = {p.casefold() for p in settings['library']}
    count = 0
    for p in paths:
        if not is_supported_disc(p):
            continue
        path = normalized_disc(p)
        if path.casefold() in seen:
            continue
        settings['library'].append(path)
        seen.add(path.casefold())
        count += 1
    return count


def build_launch_command(root: Path, disc: str | Path, settings: dict, *, prepare: bool = False) -> list[str]:
    """Launch using the authoritative incremental runner, never the historical BAT.

    No arguments are passed through shell interpolation by Python.  The BAT
    handles the per-title ABI1 cache and incremental seed-DLL modules.
    """
    root = Path(root)
    if not is_supported_disc(disc):
        raise ValueError('El disco no existe o su formato no esta soportado (CDI / ISO).')
    script = root / 'scripts' / 'windows' / 'run_game.bat'
    if not script.is_file():
        raise FileNotFoundError(str(script))
    opts = sanitize_settings(settings)
    overrides = opts.get('game_overrides', {}).get(normalized_disc(disc), {})
    renderer = overrides.get('renderer','inherit')
    if renderer not in RENDERERS: renderer = opts['renderer']
    audio = opts['audio']
    if overrides.get('audio') == 'on': audio = True
    elif overrides.get('audio') == 'off': audio = False
    args = [str(script), normalized_disc(disc), '--no-pause', '--renderer='+renderer]
    mode = opts['build_mode']
    if prepare or mode == 'full': args.append('--recompile-game')
    elif mode == 'reuse': args.append('--reuse')
    amode = opts['audio_mode'] if audio else 'off'
    if amode == 'off': args.append('--no-audio')
    elif amode == 'basic': args.append('--audio-basic')
    if not opts['native_patches']: args.append('--no-native-patches')
    elif not opts['native_seeds']: args.append('--no-native-seeds')
    if not opts['sh4_interp']: args.append('--no-interp')
    if opts['sh4_interp_trace']: args.append('--interp-trace')
    if opts['dynamic_aot']: args.append('--dynamic-aot')
    if opts['bad_target_trace']: args.append('--bad-target-trace')
    if opts['fpu_mode'] == 'regionplus': args.append('--fpu-regionplus')
    for flag, option in (
        ('perf','--perf'),('debug','--debug'),('pvr_profile','--profile'),
        ('maple_trace','--maple-trace'),('gdrom_trace','--gdrom-trace'),
        ('code_reloc_trace','--code-reloc-trace'),('hotpc','--hotpc')):
        if opts[flag]: args.append(option)
    args.append('--heartbeat-on' if opts['heartbeats'] else '--heartbeat-off')
    if not opts['spg_fast_forward']: args.append('--spg-ff-off')
    if not opts['hot_recovery']: args.append('--hot-recovery-off')
    return args

def video_output_environment(settings: dict, disc: str | Path,
                             inherited: dict | None = None) -> dict:
    """Set V8 host-only presentation env, independent of guest flags/caches.

    The DXGI/GDI presentation layer reads these once when initializing the
    process. VSync, fullscreen and filters remain unchanged/unimplemented.
    """
    env = dict(os.environ if inherited is None else inherited)
    env.pop('DCR_VIDEO_RESOLUTION', None)
    env.pop('DCR_VIDEO_ASPECT', None)
    env.pop('DCR_VIDEO_OUTPUT_BACKEND', None)
    video = effective_game_settings(settings, disc)['video_future']
    resolution = video['resolution']
    aspect = video['aspect']
    if resolution != 'native':
        env['DCR_VIDEO_RESOLUTION'] = resolution
    if aspect == '16:9':
        env['DCR_VIDEO_ASPECT'] = '16:9'
    if video['output_backend'] == 'gdi':
        env['DCR_VIDEO_OUTPUT_BACKEND'] = 'gdi'
    return env


def launcher_environment(settings: dict, disc: str | Path,
                         inherited: dict | None = None) -> dict:
    """Combine established video env with the actual per-port Maple host devices."""
    from controller_devices import port_environment
    env = video_output_environment(settings, disc, inherited)
    env.update(port_environment(settings))
    env['DCR_NETWORK_MODE'] = sanitize_settings(settings)['network_mode']
    return env


def list_log_files() -> list[Path]:
    base = cache_directory() / 'games'
    if not base.is_dir():
        return []
    found = []
    for directory in base.iterdir():
        if directory.is_dir():
            logs = directory / 'logs'
            if logs.is_dir():
                found.extend(p for p in logs.glob('*.log') if p.is_file())
    return sorted(found, key=lambda p: p.stat().st_mtime, reverse=True)[:200]


def list_game_profiles() -> list[Path]:
    base = cache_directory() / 'games'
    if not base.is_dir():
        return []
    return sorted((p for p in base.glob('*/game.cfg') if p.is_file()), key=lambda x: x.parent.name.casefold())


def system_directory() -> Path:
    return cache_directory() / 'system'
