# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.os_parametrize('osxml', filter_devices=True)
def test_devices_duplication(osxml):
    broken = []
    related = util.DataFiles.getosxml_related(osxml)
    for osxml2 in related:
        if osxml2.devices is not None:
            for device in osxml2.devices:
                if device in osxml.devices:
                    broken.append(device)
    assert broken == []
