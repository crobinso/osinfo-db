# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import lxml.etree as ET
import pytest

from . import util


SCHEMA = util.DataFiles.schema
RELAXNG = ET.RelaxNG(ET.parse(SCHEMA))


@pytest.mark.parametrize("path", util.DataFiles.xmls())
def test_schema(path):
    if not RELAXNG.validate(ET.parse(path)):
        # pylint: disable=no-member
        raise AssertionError(str(RELAXNG.error_log.last_error))
