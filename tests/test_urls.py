# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os
import pytest

from . import util


OSES = util.oses()


def _os_id(_os):
    return _os.shortid


@pytest.mark.parametrize('_os', [*OSES], ids=_os_id)
@pytest.mark.skipif(os.environ.get('OSINFO_DB_NETWORK_TESTS') is None,
                    reason='Network related tests are not enabled')
def test_images_url(_os):
    broken = []
    for image in _os.images:
        if image.url:
            if not image.url.check():
                broken.append(image.url)
    assert broken == []


@pytest.mark.parametrize('_os', [*OSES], ids=_os_id)
@pytest.mark.skipif(os.environ.get('OSINFO_DB_NETWORK_TESTS') is None,
                    reason='Network related tests are not enabled')
def test_medias_url(_os):
    broken = []
    for media in _os.medias:
        if media.url:
            if not media.url.check():
                broken.append(media.url)
    assert broken == []


@pytest.mark.parametrize('_os', [*OSES], ids=_os_id)
@pytest.mark.skipif(os.environ.get('OSINFO_DB_NETWORK_TESTS') is None,
                    reason='Network related tests are not enabled')
def test_trees_url(_os):
    broken = []
    for tree in _os.trees:
        if tree.url:
            if not tree.url.check():
                broken.append(tree.url)
    assert broken == []
