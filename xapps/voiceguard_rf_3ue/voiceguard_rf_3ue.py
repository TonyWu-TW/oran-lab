#!/usr/bin/env python3
"""Launch the shared VoiceGuard RF runtime with the 3 UE feature module.

The closed-loop state machine is shared with the original 10 UE demo.  This
entry point deliberately injects this directory's state/action definitions so
the runtime reads UE1/UE2 video, UE3 voice, and the 3 UE model feature schema.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


HERE = Path(__file__).resolve().parent
LAB_ROOT = HERE.parents[1]


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


load("common", HERE / "common.py")
runtime = load(
    "voiceguard_rf_shared_runtime",
    LAB_ROOT / "xapps" / "voiceguard_rf" / "voiceguard_rf.py",
)


if __name__ == "__main__":
    raise SystemExit(runtime.main())
