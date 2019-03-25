# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os


def _setup_env():
    key = "INTERNAL_OSINFO_DB_DATA_DIR"
    if key not in os.environ:
        os.environ[key] = os.path.realpath(os.path.join(
            os.path.dirname(__file__), "..", "data"))


_setup_env()


def pytest_addoption(parser):
    parser.addoption("--network-tests", action="store_true", default=False,
            help=("Run osinfo-db network tests. Same tests as triggered "
                  "by setting env variable OSINFO_DB_NETWORK_TESTS"))


def pytest_ignore_collect(path, config):
    """
    Entirely skip loading test_urls.py if the option wasn't specified
    """
    run_network = bool(config.getoption("--network-tests") or
            os.environ.get("OSINFO_DB_NETWORK_TESTS"))
    if os.path.basename(str(path)) == "test_urls.py" and not run_network:
        return True


# This will trigger some DATA_DIR validation
from . import util
dummy = util