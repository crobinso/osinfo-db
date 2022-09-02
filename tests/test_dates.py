# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import datetime
import re
import sys

from . import util


if sys.version_info >= (3, 7):

    def _parse_iso_date(date_string):
        return datetime.date.fromisoformat(date_string)

else:

    def _parse_iso_date(date_string):
        m = re.match("([0-9]{4})-([0-9]{2})-([0-9]{2})", date_string)
        assert m
        return datetime.date(int(m.group(1)), int(m.group(2)), int(m.group(3)))


def _parse_date(date_string):
    if not date_string:
        return None
    return _parse_iso_date(date_string)


@util.os_parametrize("osxml", filter_dates=True)
def test_dates(osxml):
    release_date = _parse_date(osxml.release_date)
    eol_date = _parse_date(osxml.eol_date)
    if release_date and eol_date:
        assert release_date < eol_date
