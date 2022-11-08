# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import logging
from pathlib import Path
import pytest

from . import util
from . import isodata


def _get_isodatapaths():
    """
    Collect iso media data and return a list of tuples:
        (osname, isodatapaths)
    """
    this = Path(__file__).resolve()
    isodata_path = Path(this.parent, "isodata")

    ret = []
    osdirs = [d for d in isodata_path.glob(str(Path("*", "*"))) if d.is_dir()]

    for osdir in sorted(osdirs, key=util.path_sort):
        isodatapaths = list(osdir.glob("*.txt"))
        if len(isodatapaths):
            ret.append((osdir.name, isodatapaths))

    return ret


@pytest.mark.parametrize("testdata", _get_isodatapaths(), ids=lambda d: d[0])
def test_iso_detection(testdata):
    osname, isodatapaths = testdata
    for isodatapath in isodatapaths:
        detected = []
        isodatamedia = isodata.get_isodatamedia(isodatapath)
        for osxml2 in util.DataFiles.oses():
            for media in osxml2.medias:
                if isodatamedia.match(media.iso):
                    if osname != osxml2.shortid:
                        logging.warning(
                            "ISO '%s' was matched by OS '%s' while it "
                            "should only be matched by OS '%s'",
                            isodatamedia.path,
                            osxml2.shortid,
                            osname,
                        )
                    else:
                        logging.info(
                            "ISO '%s' matched by OS '%s'",
                            isodatamedia.path,
                            osxml2.shortid,
                        )
                    # For several distros we do not have the volume-size
                    # set as part of our DB, thus multiple detections may
                    # occur. Although this case is not the optimal, as long
                    # as we detect the very same distro it's okay-ish.
                    if osxml2.shortid not in detected:
                        detected.append(osxml2.shortid)

        if detected == [osname]:
            continue

        raise AssertionError(
            "isodata: %s\nMatched=%s but expected=%s"
            % (isodatapath, detected, [osname])
        )
