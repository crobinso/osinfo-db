# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import re

import lxml.etree as ET


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


class _XMLBase():
    """
    Simple base class for our XML parsers
    """
    def __init__(self, root):
        self._root = root
        self._cache = {}

    def _get_text(self, element_name, default=None):
        node = self._root.find(element_name)
        if node is not None:
            return node.text
        return default

    def _get_int(self, element_name, default=None):
        text = self._get_text(element_name)
        if text is not None:
            return int(text)
        return default


class Os(_XMLBase):
    def __init__(self, filename):
        self.filename = filename
        root = ET.parse(self.filename).getroot().find('os')
        super().__init__(root)

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
                if device.get("supported") != "false":
                    devices.append(device.get('id'))
        return devices

    @_cache_property
    def devices_unsupported(self):
        devices = []
        devicelist = self._root.find('devices')
        if devicelist is not None:
            for device in devicelist.findall('device'):
                if device.get("supported") == "false":
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
        return self._get_text('short-id')

    @_cache_property
    def shortids(self):
        shortids = []
        for shortid in self._root.findall('short-id'):
            shortids.append(shortid.text)
        return shortids

    @_cache_property
    def distro(self):
        return self._get_text('distro')

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

    @_cache_property
    def release_date(self):
        return self._get_text('release-date')

    @_cache_property
    def eol_date(self):
        return self._get_text('eol-date')

    @_cache_property
    def variants(self):
        return [v.attrib['id'] for v in self._root.findall('variant')]

    @_cache_property
    def firmwares(self):
        firmwares = []
        firmwarelist = self._root.findall('firmware')
        for firmware in firmwarelist:
            firmwares.append(Firmware(firmware))
        return firmwares

    @_cache_property
    def installscripts(self):
        return [s.attrib['id']
                for i in self._root.findall('installer')
                for s in i.findall('script')]


class Resources(_XMLBase):
    @_cache_property
    def cpu(self):
        return self._get_int('cpu')

    @_cache_property
    def n_cpus(self):
        return self._get_int('n-cpus')

    @_cache_property
    def ram(self):
        return self._get_int('ram')

    @_cache_property
    def storage(self):
        return self._get_int('storage')


class Firmware(_XMLBase):
    @_cache_property
    def arch(self):
        return self._root.attrib["arch"]

    @_cache_property
    def firmware_type(self):
        return self._root.attrib["type"]


class Image(_XMLBase):
    @_cache_property
    def url(self):
        return self._get_text('url')

    @_cache_property
    def variant(self):
        variant = self._root.find('variant')
        if variant is not None:
            return variant.attrib['id']
        return None


class Media(_XMLBase):
    @_cache_property
    def url(self):
        return self._get_text('url')

    @_cache_property
    def iso(self):
        iso = self._root.find('iso')
        if iso is not None:
            return ISO(iso)
        return None

    @_cache_property
    def variant(self):
        variant = self._root.find('variant')
        if variant is not None:
            return variant.attrib['id']
        return None

    @_cache_property
    def installscripts(self):
        return [s.attrib['id']
                for i in self._root.findall('installer')
                for s in i.findall('script')]


class Tree(_XMLBase):
    @_cache_property
    def url(self):
        return self._get_text('url')

    @_cache_property
    def variant(self):
        variant = self._root.find('variant')
        if variant is not None:
            return variant.attrib['id']
        return None

    @_cache_property
    def treeinfo(self):
        treeinfo = self._root.find('treeinfo')
        if treeinfo is not None:
            return Treeinfo(treeinfo)
        return None

    @_cache_property
    def kernel(self):
        return self._get_text('kernel')

    @_cache_property
    def initrd(self):
        return self._get_text('initrd')


class Treeinfo(_XMLBase):
    @_cache_property
    def arch(self):
        return re.compile(self._get_text('arch', default=''))

    @_cache_property
    def family(self):
        return re.compile(self._get_text('family', default=''))

    @_cache_property
    def variant(self):
        return re.compile(self._get_text('variant', default=''))

    @_cache_property
    def version(self):
        return re.compile(self._get_text('version', default=''))


class ISO(_XMLBase):
    @_cache_property
    def volumeid(self):
        return re.compile(self._get_text('volume-id', default=''))

    @_cache_property
    def publisherid(self):
        return re.compile(self._get_text('publisher-id', default=''))

    @_cache_property
    def applicationid(self):
        return re.compile(self._get_text('application-id', default=''))

    @_cache_property
    def systemid(self):
        return re.compile(self._get_text('system-id', default=''))

    @_cache_property
    def volumesize(self):
        return self._get_int('volume-size', default=0)


class Device(_XMLBase):
    def __init__(self, filename):
        self.filename = filename
        root = ET.parse(self.filename).getroot().find('device')
        super().__init__(root)

    @_cache_property
    def internal_id(self):
        return self._root.attrib["id"]

    @_cache_property
    def name(self):
        return self._get_text('name')


class Datamap(_XMLBase):
    def __init__(self, filename):
        self.filename = filename
        root = ET.parse(self.filename).getroot().find('datamap')
        super().__init__(root)

    def __repr__(self):
        return "<%s filename=%s>" % (self.__class__.__name__, self.filename)

    @_cache_property
    def internal_id(self):
        return self._root.get('id')


class InstallScript(_XMLBase):
    def __init__(self, filename):
        self.filename = filename
        root = ET.parse(self.filename).getroot().find('install-script')
        super().__init__(root)

    def __repr__(self):
        return "<%s filename=%s>" % (self.__class__.__name__, self.filename)

    @_cache_property
    def internal_id(self):
        return self._root.get('id')


class Platform(_XMLBase):
    def __init__(self, filename):
        self.filename = filename
        root = ET.parse(self.filename).getroot().find('platform')
        super().__init__(root)

    def __repr__(self):
        return "<%s filename=%s>" % (self.__class__.__name__, self.filename)

    @_cache_property
    def internal_id(self):
        return self._root.get('id')
