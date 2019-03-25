# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.os_parametrize('osxml', filter_images=True)
def test_images_url(osxml):
    broken = []
    for image in osxml.images:
        if image.url:
            if not image.url.check():
                broken.append(image.url)
    assert broken == []


@util.os_parametrize('osxml', filter_trees=True)
def test_medias_url(osxml):
    broken = []
    for media in osxml.medias:
        if media.url:
            if not media.url.check():
                broken.append(media.url)
    assert broken == []


@util.os_parametrize('osxml', filter_media=True)
def test_trees_url(osxml):
    broken = []
    for tree in osxml.trees:
        if tree.url:
            if not tree.url.check():
                broken.append(tree.url)
    assert broken == []
