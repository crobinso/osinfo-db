# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from http.client import responses
import logging
import re

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

    def __repr__(self):
        return "<%s shortid=%s>" % (self.__class__.__name__, self.shortid)

    def _get_id(self):
        return self._root.get('id')
    internal_id = _cache_property(_get_id)

    def _get_derives_from(self):
        derives_from = self._root.find('derives-from')
        if derives_from is not None:
            return derives_from.get('id')
        return None
    derives_from = _cache_property(_get_derives_from)

    def _get_clones(self):
        clones = self._root.find('clones')
        if clones is not None:
            return clones.get('id')
        return None
    clones = _cache_property(_get_clones)

    def _get_devices(self):
        devices = []
        devicelist = self._root.find('devices')
        if devicelist is not None:
            for device in devicelist.findall('device'):
                devices.append(device.get('id'))
        return devices
    devices = _cache_property(_get_devices)

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

    def _get_resources_list(self):
        return self._root.findall('resources')
    resources_list = _cache_property(_get_resources_list)

    def _get_resources(self, node, resource_type):
        valid_resources = ['minimum',
                           'recommended',
                           'maximum',
                           'network-install']
        if resource_type not in valid_resources:
            return None
        if node not in self.resources_list:
            return None
        resource = node.find(resource_type)
        if resource is not None:
            return Resources(resource)
        return None

    def get_minimum_resources(self, node):
        return self._get_resources(node, 'minimum')

    def get_recommended_resources(self, node):
        return self._get_resources(node, 'recommended')

    def get_maximum_resources(self, node):
        return self._get_resources(node, 'maximum')

    def get_network_install_resources(self, node):
        return self._get_resources(node, 'network-install')


class Resources():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_value(self, string):
        value = self._root.find(string)
        if value is not None:
            return int(value.text)
        return None

    def _get_cpu(self):
        return self._get_value('cpu')
    cpu = _cache_property(_get_cpu)

    def _get_n_cpus(self):
        return self._get_value('n-cpus')
    n_cpus = _cache_property(_get_n_cpus)

    def _get_ram(self):
        return self._get_value('ram')
    ram = _cache_property(_get_ram)

    def _get_storage(self):
        return self._get_value('storage')
    storage = _cache_property(_get_storage)


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
        return re.compile(self._get_value('volume-id'))
    volumeid = _cache_property(_get_volumeid)

    def _get_publisherid(self):
        return re.compile(self._get_value('publisher-id'))
    publisherid = _cache_property(_get_publisherid)

    def _get_applicationid(self):
        return re.compile(self._get_value('application-id'))
    applicationid = _cache_property(_get_applicationid)

    def _get_systemid(self):
        return re.compile(self._get_value('system-id'))
    systemid = _cache_property(_get_systemid)

    def _get_volumesize(self):
        return self._get_value('volume-size', int, 0)
    volumesize = _cache_property(_get_volumesize)
