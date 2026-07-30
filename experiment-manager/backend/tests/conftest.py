from __future__ import annotations

import os
import tempfile


os.environ["ORAN_MANAGER_DATABASE_URL"] = f"sqlite:///{tempfile.gettempdir()}/oran-manager-pytest-{os.getpid()}.db"
os.environ["ORAN_MANAGER_SEED"] = "0"
