# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os

from . import util


def _test_validate_ids(xml, entity_type):
    """
    Ensure the ids are the ones supported by OsinfoLoader.

    This check tries to mimic, in a pythonic way, the very same
    check done by OsinfoLoader::osinfo_loader_check_id()
    """
    suffix = xml.internal_id[len('http://'):]
    vendor = suffix.split('/', 1)[0]
    entity_name = suffix.split('/', 1)[1].replace("/", "-")

    filename = vendor + "/" + entity_name + ".xml"
    relpath = os.path.relpath(xml.filename)
    expected_filename = relpath.split(entity_type + "/")[1]

    assert filename == expected_filename


@util.os_parametrize('osxml')
def test_validate_os_ids(osxml):
    """
    Ensure the OS ids are the ones supported by OsinfoLoader.
    """
    return _test_validate_ids(osxml, "os")
