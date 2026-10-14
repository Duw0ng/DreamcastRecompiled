"""Compare a CMakeCache source directory to this extracted runtime path."""
from pathlib import Path
import os
import sys


def normalized(path: str) -> str:
    return os.path.normcase(os.path.normpath(os.path.abspath(path)))


def check(cache: Path, runtime: Path) -> int:
    try:
        text = cache.read_text(encoding='utf-8', errors='replace')
    except OSError:
        return 1
    source = next((line.partition('=')[2] for line in text.splitlines()
                   if line.startswith('CMAKE_HOME_DIRECTORY:INTERNAL=')), '')
    if source and normalized(source) == normalized(str(runtime)):
        return 0
    print('[RUNTIME CACHE] Source actual: ' + str(runtime))
    print('[RUNTIME CACHE] CMake apunta a: ' + (source or '<desconocida>'))
    return 1


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(2)
    sys.exit(check(Path(sys.argv[1]), Path(sys.argv[2])))
