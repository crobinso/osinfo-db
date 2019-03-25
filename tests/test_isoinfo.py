# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import glob
import logging
import os
import pytest

from . import util
from . import isodata


def _get_isodatapaths():
    """
    Collect iso media data and return a list of tuples:
        (osname, isodatapaths)
    """
    isodata_path = os.path.join(
        os.path.dirname(os.path.realpath(__file__)),
        'isodata')

    ret = []
    allpaths = glob.glob(os.path.join(isodata_path, "*", "*"))
    for osdir in sorted(allpaths, key=util.human_sort):
        osname = os.path.basename(osdir)
        isodatapaths = glob.glob(os.path.join(osdir, "*"))
        ret.append((osname, isodatapaths))
    return ret


@pytest.mark.parametrize("testdata", _get_isodatapaths(), ids=lambda d: d[0])
def test_iso_detection(testdata):
    osname, isodatapaths = testdata
    for isodatapath in isodatapaths:
        if isodatapath.endswith(".lng"):
            # libosinfo handled these specially, we should too
            continue

        detected = []
        isodatamedia = isodata.get_isodatamedia(isodatapath)
        for osxml2 in util.DataFiles.oses():
            for media in osxml2.medias:
                if isodatamedia.match(media.iso):
                    if osname != osxml2.shortid:
                        logging.warning(
                            'ISO \'%s\' was matched by OS \'%s\' while it '
                            'should only be matched by OS \'%s\'',
                            isodatamedia.filename, osxml2.shortid, osname)
                    else:
                        logging.info('ISO \'%s\' matched by OS \'%s\'',
                                     isodatamedia.filename, osxml2.shortid)
                    # For several distros we do not have the volume-size
                    # set as part of our DB, thus multiple detections may
                    # occur. Although this case is not the optimal, as long
                    # as we detect the very same distro it's okay-ish.
                    if osxml2.shortid not in detected:
                        detected.append(osxml2.shortid)

        if detected == [osname]:
            continue

        raise AssertionError("isodata: %s\nMatched=%s but expected=%s" %
                (isodatapath, detected, [osname]))
