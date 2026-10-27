"""One-command local launcher for the VOR web host."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import threading
import time
import urllib.request
import venv
import webbrowser
from pathlib import Path


ROOT = Path(__file__).resolve().parent
REQUIREMENTS = ROOT / "requirements.txt"
VENV_DIR = ROOT / ".venv"


def _venv_python() -> Path:
    if os.name == "nt":
        return VENV_DIR / "Scripts" / "python.exe"
    return VENV_DIR / "bin" / "python"


def _inside_venv(python: Path) -> bool:
    try:
        return Path(sys.executable).resolve() == python.resolve()
    except OSError:
        return False


def _ensure_environment(skip_install: bool) -> bool:
    """Return True after relaunching; False when already in the project venv."""
    python = _venv_python()
    if not python.exists():
        print("[VOR] Creating local Python environment: .venv")
        venv.EnvBuilder(with_pip=True).create(VENV_DIR)

    if not _inside_venv(python):
        command = [str(python), str(Path(__file__).resolve()), *sys.argv[1:]]
        raise SystemExit(subprocess.call(command, cwd=ROOT))

    if not skip_install:
        try:
            import fastapi  # noqa: F401
            import serial  # noqa: F401
            import uvicorn  # noqa: F401
        except ImportError:
            print("[VOR] Installing dependencies (first start may take a while)...")
            subprocess.check_call(
                [str(python), "-m", "pip", "install", "-r", str(REQUIREMENTS)],
                cwd=ROOT,
            )
    return False


def _open_when_ready(url: str) -> None:
    for _ in range(100):
        try:
            with urllib.request.urlopen(url, timeout=0.25):
                webbrowser.open(url)
                return
        except Exception:
            time.sleep(0.1)


def main() -> None:
    parser = argparse.ArgumentParser(description="Start the VOR local web host")
    parser.add_argument(
        "--no-browser", action="store_true", help="do not open the browser automatically"
    )
    parser.add_argument(
        "--skip-install", action="store_true", help="do not install missing dependencies"
    )
    args = parser.parse_args()

    _ensure_environment(args.skip_install)
    os.chdir(ROOT)

    import uvicorn

    from app.config import load_config
    from app.main import create_app

    config = load_config()
    browser_host = "127.0.0.1" if config.web.host in {"0.0.0.0", "::"} else config.web.host
    url = f"http://{browser_host}:{config.web.port}"
    print(f"[VOR] Starting local service: {url}")
    print("[VOR] Press Ctrl+C to stop.")
    if not args.no_browser:
        threading.Thread(target=_open_when_ready, args=(url,), daemon=True).start()
    uvicorn.run(create_app(config), host=config.web.host, port=config.web.port, log_level="info")


if __name__ == "__main__":
    main()
