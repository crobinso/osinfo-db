# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.os_parametrize('osxml', filter_installscripts=True)
def test_install_scripts(osxml):
    for script in osxml.installscripts:
        assert util.DataFiles.installscripts()[script]


@util.os_parametrize('osxml', filter_media=True)
def test_media_install_scripts(osxml):
    for media in osxml.medias:
        for script in media.installscripts:
            assert util.DataFiles.installscripts()[script]
