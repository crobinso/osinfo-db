# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os

from . import util


@util.os_parametrize('osxml')
def test_validate_os_ids(osxml):
    """
    Ensure the OS ids are the ones supported by OsinfoLoader.

    This check tries to mimic, in a pythonic way, the very same
    check done by OsinfoLoader::osinfo_loader_check_id()
    """
    suffix = osxml.internal_id[len('http://'):]
    vendor = suffix.split('/', 1)[0]
    os_name = suffix.split('/', 1)[1].replace("/", "-")

    filename = "os/" + vendor + "/" + os_name + ".xml"
    expected_filename = os.path.relpath(osxml.filename).split('data/')[1]

    assert filename == expected_filename
