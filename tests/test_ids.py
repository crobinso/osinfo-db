# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os

from . import util


def _test_validate_ids(xml, entity_type):
    """
    Ensure the ids are the ones supported by OsinfoLoader.

    This check tries to mimic, in a pythonic way, the very same
    check done by OsinfoLoader::osinfo_loader_check_id()
    """
    relpath = os.path.relpath(xml.filename)
    expected_filename = relpath.split(entity_type + "/")[1]

    extension = bool(".d/" in expected_filename)

    suffix = xml.internal_id[len('http://') :]
    vendor = suffix.split('/', 1)[0]
    entity_name = suffix.split('/', 1)[1].replace("/", "-")

    if extension:
        filename = vendor + "/" + entity_name + ".d"
        assert filename == expected_filename.rsplit("/", 1)[0]
    else:
        filename = vendor + "/" + entity_name + ".xml"
        assert filename == expected_filename


@util.os_parametrize('osxml')
def test_validate_os_ids(osxml):
    """
    Ensure the OS ids are the ones supported by OsinfoLoader.
    """
    return _test_validate_ids(osxml, "os")


@util.device_parametrize('devicexml')
def test_validate_device_ids(devicexml):
    """
    Ensure the Device ids are the ones supported by OsinfoLoader.
    """
    return _test_validate_ids(devicexml, "device")


@util.datamap_parametrize('datamapxml')
def test_validate_datamap_ids(datamapxml):
    """
    Ensure the Datamap ids are the ones supported by OsinfoLoader.
    """
    return _test_validate_ids(datamapxml, "datamap")


@util.installscript_parametrize('installscriptxml')
def test_validate_installscript_ids(installscriptxml):
    """
    Ensure the InstallScript ids are the ones supported by OsinfoLoader.
    """
    return _test_validate_ids(installscriptxml, "install-script")


@util.platform_parametrize('platformxml')
def test_validate_platform_ids(platformxml):
    """
    Ensure the Platform ids are the ones supported by OsinfoLoader.
    """
    return _test_validate_ids(platformxml, "platform")
