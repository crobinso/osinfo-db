# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.datamap_parametrize("datamapxml")
def test_duplicate_mappings(datamapxml):
    """
    Check that there are no duplicate mappings.
    """
    seen = set()
    duplicates = set(
        inval
        for (inval, _) in datamapxml.mapping_list
        if inval in seen or seen.add(inval)
    )
    assert duplicates == set()
