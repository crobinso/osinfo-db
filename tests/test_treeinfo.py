# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import logging
from pathlib import Path
import pytest

from . import util
from . import treeinfodata


def _get_treeinfodatapaths():
    """
    Collect treeinfo data and return a list of tuples:
        (osname, treeinfodatapaths)
    """
    this = Path(__file__).resolve()
    treeinfodata_path = Path(this.parent, "treeinfodata")

    ret = []
    allpaths = treeinfodata_path.glob(str(Path("*", "*")))
    for osdir in sorted(allpaths, key=util.path_sort):
        # The .treeinfo may be under the following directories schema:
        # - distro/shortid/arch
        # - distro/shortid/variant/arch/
        # That's the reason we have to take check for the .treeinfo file recursively.
        treeinfodatapaths = list(osdir.rglob("*.treeinfo"))
        if len(treeinfodatapaths):
            ret.append(pytest.param(osdir.name, treeinfodatapaths, id=osdir.name))
    return ret


@pytest.mark.parametrize("osname,treeinfodatapaths", _get_treeinfodatapaths())
def test_treeinfo_detection(osname, treeinfodatapaths):
    for treeinfodatapath in treeinfodatapaths:
        detected = []
        data = treeinfodata.get_treeinfodata(treeinfodatapath)

        for osxml2 in util.DataFiles.oses():
            for tree in osxml2.trees:
                if tree.treeinfo is None:
                    continue

                if data.match(tree.treeinfo):
                    if osname != osxml2.shortid:
                        logging.warning(
                            "treeinfo '%s' was matched by OS '%s' while "
                            "it should only be matched by OS '%s'",
                            data.path,
                            osxml2.shortid,
                            osname,
                        )
                    else:
                        logging.info(
                            "treeinfo '%s' matched by OS '%s'",
                            data.path,
                            osxml2.shortid,
                        )

                    if osxml2.shortid not in detected:
                        detected.append(osxml2.shortid)

        if not detected:
            raise AssertionError(
                "treeinfodata unmatched: %s [expected=%s]" % (treeinfodatapath, osname)
            )

        if detected == [osname]:
            continue

        raise AssertionError(
            "data: %s\nMatched=%s but expected=%s"
            % (treeinfodatapath, detected, [osname])
        )
