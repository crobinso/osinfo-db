# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import datetime

from . import util


def _parse_date(date_string):
    if not date_string:
        return None
    return datetime.date.fromisoformat(date_string)


@util.os_parametrize('osxml', filter_dates=True)
def test_dates(osxml):
    release_date = _parse_date(osxml.release_date)
    eol_date = _parse_date(osxml.eol_date)
    if release_date and eol_date:
        assert release_date < eol_date
