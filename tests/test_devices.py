# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import re

from . import util


DEVICE_MAP = {d.internal_id: d for d in util.DataFiles.devices()}


def _check_duplicate_devices(osxml):
    """
    Ensure an OS doesn't list a device that's defined in the parent
    """
    broken = []
    related = util.DataFiles.getosxml_related(osxml)
    for osxml2 in related:
        if osxml2.devices is not None:
            for device in osxml2.devices:
                if device in osxml.devices:
                    broken.append(device)
    assert broken == []


def _check_uncommented_devices(osxml):
    """
    Ensure every device listed in the XML is followed by a comment with
    the device string name in it. This helps readability/grepability
    """
    badlines = []
    sourcefile = osxml.filename + ".in"
    devlines = [l for l in open(sourcefile).read().splitlines() if
                "<device id" in l]

    for devid in osxml.devices:
        devname = DEVICE_MAP[devid].name
        for devline in devlines:
            if devid not in devline:
                continue
            if not re.search(r"<!--.*%s.*-->" % devname, devline):
                badlines.append(devline)

    if badlines:
        raise AssertionError("shortid=%s device lines don't contain a "
                "comment with the device name:\n%s" %
                (osxml.shortid, badlines))


@util.os_parametrize('osxml', filter_devices=True)
def test_devices_duplication(osxml):
    _check_duplicate_devices(osxml)
    _check_uncommented_devices(osxml)
