# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


def _check_duplicate_firmwares(osxml):
    """
    Ensure an OS doesn't list a firmware that's defined in the parent
    """
    broken = []
    related = util.DataFiles.getosxml_related(osxml)
    for osxml2 in related:
        if osxml2.firmwares is not None:
            for firmware2 in osxml2.firmwares:
                for firmware in osxml.firmwares:
                    if (
                        firmware.arch == firmware2.arch
                        and firmware.firmware_type == firmware2.firmware_type
                    ):
                        broken.append([firmware.firmware_type, firmware.arch])
    assert broken == []


@util.os_parametrize("osxml", filter_firmwares=True)
def test_firmwares_duplication(osxml):
    _check_duplicate_firmwares(osxml)
