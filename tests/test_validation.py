# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import libxml2
import pytest

from . import util


XMLS = util.DataFiles.xmls()
SCHEMA = util.DataFiles.schema
PARSER = libxml2.relaxNGNewParserCtxt(SCHEMA)
VALID = PARSER.relaxNGParse().relaxNGNewValidCtxt()


def _file_id(_file):
    return _file


@pytest.mark.parametrize('_file', [*XMLS], ids=_file_id)
def test_validation(_file):
    doc = libxml2.parseFile(_file)
    assert VALID.relaxNGValidateDoc(doc) == 0
