# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import locale
from pathlib import Path
import os


def pytest_addoption(parser):
    parser.addoption(
        "--network-tests",
        action="store_true",
        default=False,
        help=(
            "Run osinfo-db network tests. Same tests as triggered "
            "by setting env variable OSINFO_DB_NETWORK_TESTS"
        ),
    )


def pytest_configure(config):
    top = Path(__file__).parent.parent
    key = "INTERNAL_OSINFO_DB_DATA_DIR"
    if key not in os.environ:
        os.environ[key] = str(Path(top, "data").resolve())

    key = "INTERNAL_OSINFO_DB_DATA_SRC_DIR"
    if key not in os.environ:
        os.environ[key] = str(Path(top, "data").resolve())

    # Needed for test reproducibility on any system not using a UTF-8 locale
    locale.setlocale(locale.LC_ALL, "C")
    for loc in ["C.UTF-8", "C.utf8", "UTF-8", "en_US.UTF-8"]:
        try:
            locale.setlocale(locale.LC_CTYPE, loc)
            break
        except locale.Error:
            pass
    else:
        raise locale.Error("No UTF-8 locale found")

    # Default to --log-level=info if not otherwise specified
    if hasattr(config.option, "log_level") and config.option.log_level is None:
        config.option.log_level = "info"

    # This will trigger some DATA_DIR validation
    from . import util

    _ = util


def pytest_ignore_collect(path, config):
    """
    Entirely skip loading test_urls.py if the option wasn't specified
    """
    run_network = bool(
        config.getoption("--network-tests") or os.environ.get("OSINFO_DB_NETWORK_TESTS")
    )
    if Path(path).name == "test_urls.py" and not run_network:
        return True
