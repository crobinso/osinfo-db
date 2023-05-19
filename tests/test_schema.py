# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import lxml.etree as ET
import pytest

from . import util


@pytest.fixture(scope="module")
def schema():
    schema = ET.RelaxNG(ET.parse(util.DataFiles.schema.open("r")))
    return schema


@pytest.mark.parametrize("path", util.DataFiles.xmls())
def test_schema(path, schema):
    if not schema.validate(ET.parse(path.open("r"))):
        # pylint: disable=no-member
        raise AssertionError(str(schema.error_log.last_error))


@pytest.mark.parametrize("path", util.SourceFiles.xmls(), ids=lambda p: p.name)
def test_translations_in_source_files(path):
    xml = ET.parse(path.open("r"))
    assert xml.xpath("//*[@xml:lang]") == []
