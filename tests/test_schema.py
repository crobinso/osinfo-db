# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import libxml2
import pytest

from . import util


SCHEMA = util.DataFiles.schema
PARSER = libxml2.relaxNGNewParserCtxt(SCHEMA)
VALID = PARSER.relaxNGParse().relaxNGNewValidCtxt()


@pytest.mark.parametrize("path", util.DataFiles.xmls(), ids=lambda path: path)
def test_schema(path):
    doc = libxml2.parseFile(path)
    assert VALID.relaxNGValidateDoc(doc) == 0
