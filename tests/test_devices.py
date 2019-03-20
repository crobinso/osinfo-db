# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import pytest

from . import util


@pytest.mark.parametrize('_os', util.DataFiles.oses(), ids=lambda o: o.shortid)
def test_devices_duplication(_os):
    broken = []
    related = util.DataFiles.get_os_related(_os)
    for __os in related:
        if __os.devices is not None:
            for device in __os.devices:
                if device in _os.devices:
                    broken.append(device)
    assert broken == []
