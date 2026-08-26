from __future__ import annotations

import os
from pathlib import Path

import uvicorn

from .config import load_config
from .main import create_app


def main() -> None:
    configured_path = os.environ.get("VOR_HOST_CONFIG")
    config = load_config(Path(configured_path) if configured_path else None)
    app = create_app(config)
    uvicorn.run(app, host=config.web.host, port=config.web.port, log_level="info")


if __name__ == "__main__":
    main()
