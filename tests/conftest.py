# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os


def _setup_env():
    key = "INTERNAL_OSINFO_DB_DATA_DIR"
    if key not in os.environ:
        os.environ[key] = os.path.realpath(os.path.join(
            os.path.dirname(__file__), "..", "data"))


_setup_env()
