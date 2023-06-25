# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import configparser
import contextlib
import logging
from pathlib import Path
import re
import pytest

from . import util
from . import isodata


def _load_languages(isofile):
    assert isofile.suffix == ".txt"
    langfile = isofile.with_suffix(".lng")
    config = configparser.ConfigParser()
    try:
        with langfile.open("r") as f:
            config.read_file(f)
    except FileNotFoundError:
        return None
    return set(config.get("general", "l10n-language").split(","))


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
        isofilepaths = list(osdir.glob("*.txt"))
        if len(isofilepaths):
            isodatapaths = []
            for isofile in isofilepaths:
                isodatapaths.append((isofile, _load_languages(isofile)))
            ret.append(pytest.param(osdir.name, isodatapaths, id=osdir.name))

    return ret


def _get_iso_languages(iso, isodatamedia):
    languages = set()
    regex = None
    l10n_language_map = None

    for l10n_language in iso.l10n_languages:
        value = l10n_language.value
        if l10n_language.regex:
            regex = value
            if l10n_language.l10n_language_map:
                l10n_language_map = l10n_language.l10n_language_map
        else:
            languages.add(value)

    # This matches the same logic in libosinfo (set_languages_for_media()
    # in OsinfoDb): if there is a regex specification for the language,
    # the result is used overring any other language previously specified
    if regex and isodatamedia.volumeid:
        m = re.match(regex, isodatamedia.volumeid)
        if m and m.lastindex:
            lang = m.group(1)
            with contextlib.suppress(KeyError):
                lang = util.DataFiles.datamaps()[l10n_language_map][lang]
            return {lang}

    return languages


@pytest.mark.parametrize("osname,isodatapaths", _get_isodatapaths())
def test_iso_detection(osname, isodatapaths):
    for isodatapath, languages in isodatapaths:
        # A symlink indicates an ISO from another version
        # that is ambiguous and so accidentally matches
        # this distro too. We ignore them here, because
        # we'll validate against the primary OS version
        if isodatapath.is_symlink():
            continue

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
                        if languages:
                            # If there are languages specified for this ISO,
                            # check they match what described in <iso>
                            iso_languages = _get_iso_languages(media.iso, isodatamedia)
                            assert languages == iso_languages
                    # For several distros we do not have the volume-size
                    # set as part of our DB, thus multiple detections may
                    # occur. Although this case is not the optimal, as long
                    # as we detect the very same distro it's okay-ish.
                    if osxml2.shortid not in detected:
                        detected.append(osxml2.shortid)

        if not detected:
            raise AssertionError(
                "isodata unmatched: %s [expected=%s]" % (isodatapath, osname)
            )

        ok = True
        for osid in detected:
            # Primary (correct) match
            if osid == osname:
                continue

            # Ambiguous (incorrect) matches, where we can't
            # distinguish from a different distro version
            link = Path(isodatapath.parent.parent, osid, isodatapath.name)
            if not link.exists():
                ok = False
        if ok:
            continue

        raise AssertionError(
            "isodata: %s\nMatched=%s but expected=%s"
            % (isodatapath, detected, [osname])
        )
