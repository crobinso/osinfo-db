# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.os_parametrize("osxml", filter_related=True)
def test_related(osxml):
    related = util.DataFiles.getosxml_related(osxml)
    upgrades = osxml.upgrades
    if upgrades:
        found = util.DataFiles.getosxml_by_id(upgrades)
        assert found
        related.append(found)

    for os in related:
        assert os is not osxml
