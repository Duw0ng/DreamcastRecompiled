"""Edit SH-4 recompiler budgets in a per-game game.cfg, preserving other keys.

Pure stdlib: logic is independently testable without Tkinter/PowerShell/Windows.
"""
from __future__ import annotations

import os
from pathlib import Path
import tempfile

LIMIT_KEYS = ("max_functions", "max_closure_entries", "closure_passes")
RECOMPILE_MARKER = ".dcr_limits_recompile_required"
RECOMMENDED_SGGG = {"max_functions": 10240, "max_closure_entries": 7680, "closure_passes": 64}


def read_limits(path: Path) -> dict[str, int]:
    """Read the effective numeric budgets of an existing game.cfg."""
    text = path.read_text(encoding="utf-8-sig")
    items: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith(("#", ";")) or "=" not in line:
            continue
        key, value = line.split("=", 1)
        items[key.strip().lower()] = value.strip()
    if any(k not in items for k in LIMIT_KEYS):
        missing = ", ".join(k for k in LIMIT_KEYS if k not in items)
        raise ValueError(f"Faltan límites en el perfil: {missing}")
    result = {k: parse_positive_int(items[k], k) for k in LIMIT_KEYS}
    validate_limits(**result)
    return result


def parse_positive_int(value: object, name: str) -> int:
    value = str(value).strip()
    if not value.isascii() or not value.isdecimal() or len(value) > 6:
        raise ValueError(f"{name}: ingresá un número entero no negativo")
    return int(value)


def validate_limits(max_functions: int, max_closure_entries: int, closure_passes: int) -> None:
    if not 64 <= max_functions <= 100000:
        raise ValueError("max_functions debe estar entre 64 y 100000")
    if not 0 <= max_closure_entries < max_functions:
        raise ValueError("max_closure_entries debe ser 0 (automático) o menor a max_functions")
    if not 1 <= closure_passes <= 256:
        raise ValueError("closure_passes debe estar entre 1 y 256")


def save_limits(path: Path, max_functions: object, max_closure_entries: object, closure_passes: object) -> bool:
    """Save only the three limit keys. Flag guest regeneration iff changed."""
    path = Path(path)
    new_values = {
        "max_functions": parse_positive_int(max_functions, "max_functions"),
        "max_closure_entries": parse_positive_int(max_closure_entries, "max_closure_entries"),
        "closure_passes": parse_positive_int(closure_passes, "closure_passes"),
    }
    validate_limits(**new_values)
    previous = read_limits(path)
    if previous == new_values:
        return False
    original = path.read_text(encoding="utf-8-sig")
    result: list[str] = []
    updated: set[str] = set()
    for line in original.splitlines(keepends=True):
        key = line.split("=", 1)[0].strip().lower() if "=" in line else ""
        if key in new_values:
            eol = "\r\n" if line.endswith("\r\n") else "\n" if line.endswith("\n") else ""
            result.append(f"{key}={new_values[key]}{eol}")
            updated.add(key)
        else:
            result.append(line)
    if updated != set(LIMIT_KEYS):
        raise ValueError("El perfil no contiene todos los límites")
    # Atomic replacement on same volume; never overwrite a user's other settings.
    temp_name = None
    try:
        with tempfile.NamedTemporaryFile("w", encoding="utf-8", dir=path.parent, prefix=".dcr_limits_", suffix=".tmp", delete=False, newline="") as fp:
            temp_name = fp.name
            fp.write("".join(result))
        os.replace(temp_name, path)
        # Only clear the marker after a successful guest recompile, never here.
        (path.parent / RECOMPILE_MARKER).write_text("Updated per-game SH-4 budgets\n", encoding="utf-8")
    finally:
        if temp_name is not None and os.path.exists(temp_name):
            os.unlink(temp_name)
    return True
