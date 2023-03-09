# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import collections
import logging
import re

from . import util


@util.os_parametrize("osxml")
def test_validate_short_ids(osxml):
    invalids = []

    pattern = r"[a-z0-9\.\-]"
    for shortid in osxml.shortids:
        result = re.match(pattern, shortid)
        if not result:
            invalids.append(shortid)

    assert invalids == []


def test_duplicate_short_ids():
    ids = collections.defaultdict(set)
    # collect the mapping of short-id -> IDs that have it
    for osxml in util.DataFiles.oses():
        for shortid in osxml.shortids:
            ids[shortid].add(osxml.internal_id)
    # drop all the mappings with only one ID
    for shortid in list(ids.keys()):
        if len(ids[shortid]) == 1:
            del ids[shortid]
    # log all the problematic ones
    for shortid, ids in ids.items():
        logging.info("shortid=%s is used by %s", shortid, sorted(list(ids)))
    assert ids == {}
