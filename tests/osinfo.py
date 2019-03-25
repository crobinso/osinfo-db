# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import re


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

    @_cache_property
    def internal_id(self):
        return self._root.get('id')

    @_cache_property
    def derives_from(self):
        derives_from = self._root.find('derives-from')
        if derives_from is not None:
            return derives_from.get('id')
        return None

    @_cache_property
    def clones(self):
        clones = self._root.find('clones')
        if clones is not None:
            return clones.get('id')
        return None

    @_cache_property
    def devices(self):
        devices = []
        devicelist = self._root.find('devices')
        if devicelist is not None:
            for device in devicelist.findall('device'):
                devices.append(device.get('id'))
        return devices

    @_cache_property
    def images(self):
        images = []
        for image in self._root.findall('image'):
            images.append(Image(image))
        return images

    @_cache_property
    def medias(self):
        medias = []
        for media in self._root.findall('media'):
            medias.append(Media(media))
        return medias

    @_cache_property
    def trees(self):
        trees = []
        for tree in self._root.findall('tree'):
            trees.append(Tree(tree))
        return trees

    @_cache_property
    def shortid(self):
        shortid = self._root.find('short-id')
        return shortid.text

    @_cache_property
    def distro(self):
        distro = self._root.find('distro')
        return distro.text

    @_cache_property
    def resources_list(self):
        return self._root.findall('resources')

    def _get_resources(self, node, resource_type):
        valid_resources = ['minimum',
                           'recommended',
                           'maximum',
                           'network-install']
        if resource_type not in valid_resources:
            return None
        # pylint: disable=unsupported-membership-test
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

    @_cache_property
    def cpu(self):
        return self._get_value('cpu')

    @_cache_property
    def n_cpus(self):
        return self._get_value('n-cpus')

    @_cache_property
    def ram(self):
        return self._get_value('ram')

    @_cache_property
    def storage(self):
        return self._get_value('storage')


class Image():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    @_cache_property
    def url(self):
        url = self._root.find('url')
        if url is not None:
            return url.text
        return None


class Media():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    @_cache_property
    def url(self):
        url = self._root.find('url')
        if url is not None:
            return url.text
        return None

    @_cache_property
    def iso(self):
        iso = self._root.find('iso')
        if iso is not None:
            return ISO(iso)
        return None


class Tree():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    @_cache_property
    def url(self):
        url = self._root.find('url')
        if url is not None:
            return url.text
        return None


class ISO():
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_value(self, name, return_type=str, default=''):
        entry = self._root.find(name)
        return return_type(entry.text) if entry is not None else default

    @_cache_property
    def volumeid(self):
        return re.compile(self._get_value('volume-id'))

    @_cache_property
    def publisherid(self):
        return re.compile(self._get_value('publisher-id'))

    @_cache_property
    def applicationid(self):
        return re.compile(self._get_value('application-id'))

    @_cache_property
    def systemid(self):
        return re.compile(self._get_value('system-id'))

    @_cache_property
    def volumesize(self):
        return self._get_value('volume-size', int, 0)
