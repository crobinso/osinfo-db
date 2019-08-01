# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import re

from . import util


@util.os_parametrize('osxml')
def test_validate_short_ids(osxml):
    invalids = []

    pattern = r"[a-z0-9\.\-]"
    for shortid in osxml.shortids:
        result = re.match(pattern, shortid)
        if not result:
            invalids.append(shortid)

    assert invalids == []
