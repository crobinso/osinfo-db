# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import glob
import logging
import os
import pytest

from . import util
from . import treeinfodata


def _get_treeinfodatapaths():
    """
    Collect treeinfo data and return a list of tuples:
        (osname, treeinfodatapaths)
    """
    treeinfodata_path = os.path.join(
        os.path.dirname(os.path.realpath(__file__)),
        'treeinfodata')

    ret = []
    allpaths = glob.glob(os.path.join(treeinfodata_path, "*", "*"))
    for osdir in sorted(allpaths, key=util.human_sort):
        osname = os.path.basename(osdir)
        # The .treeinfo may be under the following directories schema:
        # - distro/shortid/arch
        # - distro/shortid/variant/arch/
        # That's the reason we have to take check for the .treeinfo file recursively.
        treeinfodatapaths = glob.glob(os.path.join(osdir, "**", ".treeinfo"), recursive=True)
        if len(treeinfodatapaths):
            ret.append((osname, treeinfodatapaths))
    return ret


@pytest.mark.parametrize("testdata", _get_treeinfodatapaths(), ids=lambda d: d[0])
def test_treeinfo_detection(testdata):
    osname, treeinfodatapaths = testdata
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
                            'treeinfo \'%s\' was matched by OS \'%s\' while '
                            'it should only be matched by OS \'%s\'',
                            data.filename, osxml2.shortid, osname)
                    else:
                        logging.info('treeinfo \'%s\' matched by OS \'%s\'',
                                     data.filename, osxml2.shortid)

                    if osxml2.shortid not in detected:
                        detected.append(osxml2.shortid)

        if detected == [osname]:
            continue

        raise AssertionError("data: %s\nMatched=%s but expected=%s" %
                (treeinfodatapath, detected, [osname]))
