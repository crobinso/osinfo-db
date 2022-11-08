# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import re

from . import util


DEVICE_MAP = {d.internal_id: d for d in util.DataFiles.devices()}
DEVICE_MAP_SRC = {d.internal_id: d for d in util.SourceFiles.devices()}


def _check_duplicate_devices(osxml):
    """
    Ensure an OS doesn't list a device that's enabled, and not eventually
    disabled, in a parent OS
    """
    related = util.DataFiles.getosxml_related(osxml)
    inherited_devices = set()
    for osxml2 in reversed(related):
        inherited_devices.update(set(osxml2.devices))
        inherited_devices.difference_update(osxml2.devices_unsupported)

    assert set(osxml.devices).intersection(inherited_devices) == set()


def _check_uncommented_devices(osxml):
    """
    Ensure every device listed in the XML is followed by a comment with
    the device string name in it. This helps readability/grepability
    """
    badlines = []
    devlines = [
        line
        for line in osxml.path.open("r").read().splitlines()
        if "<device id" in line
    ]

    for devid in osxml.devices:
        devname = DEVICE_MAP_SRC[devid].name
        for devline in devlines:
            if devid not in devline:
                continue
            if not re.search(r"<!--.*%s.*-->" % devname, devline):
                badlines.append(devline)

    if badlines:
        raise AssertionError(
            "shortid=%s device lines don't contain a "
            "comment with the device name:\n%s" % (osxml.shortid, badlines)
        )


@util.os_parametrize("osxml", filter_devices=True)
def test_devices_duplication(osxml):
    _check_duplicate_devices(osxml)


@util.os_sources_parametrize("osxml", filter_devices=True)
def test_devices_comments(osxml):
    _check_uncommented_devices(osxml)
