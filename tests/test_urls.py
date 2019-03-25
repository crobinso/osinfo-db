# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import http
import logging

import requests

from . import util


def _check_url(url):
    logging.info("url: %s", url)
    response = requests.head(url, allow_redirects=True)
    logging.info("response: %s; code: %d",
                 http.client.responses[response.status_code],
                 response.status_code)
    return response.ok


@util.os_parametrize('osxml', filter_images=True)
def test_images_url(osxml):
    broken = []
    for image in osxml.images:
        if image.url:
            if not _check_url(image.url):
                broken.append(image.url)
    assert broken == []


@util.os_parametrize('osxml', filter_trees=True)
def test_medias_url(osxml):
    broken = []
    for media in osxml.medias:
        if media.url:
            if not _check_url(media.url):
                broken.append(media.url)
    assert broken == []


@util.os_parametrize('osxml', filter_media=True)
def test_trees_url(osxml):
    broken = []
    for tree in osxml.trees:
        if tree.url:
            if not _check_url(tree.url):
                broken.append(tree.url)
    assert broken == []
