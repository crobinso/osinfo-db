# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from http.client import responses

import logging
import requests


def _cache_property(fn):
    """
    Decorator to use self._cache to cache property lookup results
    """
    def _wrapper(*args):
        self = args[0]
        key = str(fn)
        cache = self._cache  # pylint: disable=protected-access
        if key not in cache:
            cache[key] = fn(*args)
        return cache[key]
    return property(_wrapper)


class Os():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_images(self):
        images = []
        for image in self._root.findall('image'):
            images.append(Image(image))
        return images
    images = _cache_property(_get_images)

    def _get_medias(self):
        medias = []
        for media in self._root.findall('media'):
            medias.append(Media(media))
        return medias
    medias = _cache_property(_get_medias)

    def _get_trees(self):
        trees = []
        for tree in self._root.findall('tree'):
            trees.append(Tree(tree))
        return trees
    trees = _cache_property(_get_trees)

    def _get_shortid(self):
        shortid = self._root.find('short-id')
        return shortid.text
    shortid = _cache_property(_get_shortid)

    def _get_distro(self):
        distro = self._root.find('distro')
        return distro.text
    distro = _cache_property(_get_distro)


class Image():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_url(self):
        url = self._root.find('url')
        if url is not None:
            return URL(url.text)
        return None
    url = _cache_property(_get_url)


class Media():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_url(self):
        url = self._root.find('url')
        if url is not None:
            return URL(url.text)
        return None
    url = _cache_property(_get_url)

    def _get_iso(self):
        iso = self._root.find('iso')
        if iso is not None:
            return ISO(iso)
        return None
    iso = _cache_property(_get_iso)


class Tree():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_url(self):
        url = self._root.find('url')
        if url is not None:
            return URL(url.text)
        return None
    url = _cache_property(_get_url)


class URL():
    def __init__(self, url):
        self._url = url

    def check(self):
        logging.info("url: %s", self._url)
        response = requests.head(self._url, allow_redirects=True)
        logging.info("response: %s; code: %d",
                     responses[response.status_code], response.status_code)
        return response.ok


class ISO():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_value(self, name, return_type=str, default=''):
        entry = self._root.find(name)
        return return_type(entry.text) if entry is not None else default

    def _get_volumeid(self):
        return self._get_value('volume-id')
    volumeid = _cache_property(_get_volumeid)

    def _get_publisherid(self):
        return self._get_value('publisher-id')
    publisherid = _cache_property(_get_publisherid)

    def _get_applicationid(self):
        return self._get_value('application-id')
    applicationid = _cache_property(_get_applicationid)

    def _get_systemid(self):
        return self._get_value('system-id')
    systemid = _cache_property(_get_systemid)

    def _get_volumesize(self):
        return self._get_value('volume-size', int, 0)
    volumesize = _cache_property(_get_volumesize)
