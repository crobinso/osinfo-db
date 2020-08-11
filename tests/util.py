# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from collections import defaultdict

import os
import re

import pytest

from . import osinfo


def human_sort(text):
    # natural/human sorting
    # https://stackoverflow.com/questions/5967500/how-to-correctly-sort-a-string-with-a-number-inside
    def atof(t):
        try:
            retval = float(t)
        except ValueError:
            retval = t
        return retval

    return [atof(c) for c in
            re.split(r'[+-]?([0-9]+(?:[.][0-9]*)?|[.][0-9]+)', text)]


class _Files():
    """
    Track a list of DATA_DIR files and provide APIs for querying them.
    """
    def __init__(self, dir_env, files_format):
        self.datadir = os.environ[dir_env]
        self.schema = os.path.join(self.datadir, 'schema', 'osinfo.rng')
        self._all_xml_cache = []
        self._oses_cache = []
        self._devices_cache = []
        self._datamaps_cache = []
        self._installscripts_cache = {}
        self._platforms_cache = []
        self._firmwares_cache = []
        self._os_related_cache = defaultdict(list)
        self._files_format = files_format

        if not os.path.exists(self.datadir):
            raise RuntimeError("%s=%s doesn't exist" % (dir_env, self.datadir))


    def _get_all_xml(self):
        """
        Get and cache the full list of all DATA_DIR .xml paths
        """
        if not self._all_xml_cache:
            for (dirpath, _, filenames) in os.walk(self.datadir):
                for filename in sorted(filenames, key=human_sort):
                    if not filename.endswith(self._files_format):
                        continue
                    self._all_xml_cache.append(os.path.join(dirpath, filename))
        return self._all_xml_cache

    def _filter_xml(self, dirname):
        """
        Filter XML paths by those in $DATA_DIR/$dirname
        """
        return [p for p in self._get_all_xml() if
                p.startswith(os.path.join(self.datadir, dirname))]

    def oses(self, filter_media=False, filter_trees=False, filter_images=False,
            filter_devices=False, filter_resources=False, filter_dates=False,
            filter_related=False, filter_firmwares=False):
        """
        Return a list of osinfo.Os objects

        :param filter_FOO: Only return objects that have at least one
            instance of a FOO object
        """
        if not self._oses_cache:
            for path in self._filter_xml('os'):
                self._oses_cache.append(osinfo.Os(path))

        oses = self._oses_cache[:]
        if filter_media:
            oses = [o for o in oses if o.medias]
        if filter_trees:
            oses = [o for o in oses if o.trees]
        if filter_images:
            oses = [o for o in oses if o.images]
        if filter_devices:
            oses = [o for o in oses if o.devices]
        if filter_resources:
            oses = [o for o in oses if o.resources_list]
        if filter_dates:
            oses = [o for o in oses if o.release_date or o.eol_date]
        if filter_related:
            oses = [o for o in oses if self.getosxml_related(o)]
        if filter_firmwares:
            oses = [o for o in oses if o.firmwares]
        return oses

    def getosxml_related(self, osxml):
        if osxml.internal_id not in self._os_related_cache:
            directly_related = []
            if osxml.derives_from is not None:
                for osxml2 in self.oses():
                    if osxml.derives_from == osxml2.internal_id:
                        directly_related.append(osxml2)
                        break

            if osxml.clones is not None:
                for osxml2 in self.oses():
                    if osxml.clones == osxml2.internal_id:
                        directly_related.append(osxml2)
                        break

            self._os_related_cache[osxml.internal_id].extend(directly_related)

            related = []
            for osxml2 in directly_related:
                related.extend(self.getosxml_related(osxml2))

            for osxml2 in related:
                if osxml2 not in self._os_related_cache[osxml.internal_id]:
                    self._os_related_cache[osxml.internal_id].append(osxml2)
        return self._os_related_cache[osxml.internal_id]

    def devices(self):
        if not self._devices_cache:
            for path in self._filter_xml('device'):
                self._devices_cache.append(osinfo.Device(path))
        return self._devices_cache

    def datamaps(self):
        if not self._datamaps_cache:
            for path in self._filter_xml('datamap'):
                self._datamaps_cache.append(osinfo.Datamap(path))
        return self._datamaps_cache

    def installscripts(self):
        if not self._installscripts_cache:
            for path in self._filter_xml('install-script'):
                script = osinfo.InstallScript(path)
                self._installscripts_cache[script.internal_id] = script
        return self._installscripts_cache

    def platforms(self):
        if not self._platforms_cache:
            for path in self._filter_xml('platform'):
                self._platforms_cache.append(osinfo.Platform(path))
        return self._platforms_cache

    def firmwares(self):
        if not self._firmwares_cache:
            for path in self._filter_xml('firmware'):
                self._firmwares_cache.append(osinfo.Firmware(path))
        return self._firmwares_cache

    def xmls(self):
        return self._get_all_xml()


class _DataFiles(_Files):
    """
    Track a list of DATA_DIR XML files and provide APIs for querying
    them. Meant to be initialized only once
    """
    def __init__(self):
        _Files.__init__(self, 'INTERNAL_OSINFO_DB_DATA_DIR', '.xml')


DataFiles = _DataFiles()


def _generic_ids_cb(obj, key):
    # pytest passes us a weird value when the list of entities is empty,
    # which might happen depending on how agressively we filter. So
    # we can't assume we are passed a specific entity instance.
    return getattr(obj, key, str(obj))


def _shortid_ids_cb(xml):
    return _generic_ids_cb(xml, "shortid")


def _filename_ids_cb(xml):
    return _generic_ids_cb(xml, "filename")


def os_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """
    oses = DataFiles.oses(**kwargs)
    return pytest.mark.parametrize(argname, oses, ids=_shortid_ids_cb)


def device_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """
    devices = DataFiles.devices(**kwargs)
    return pytest.mark.parametrize(argname, devices, ids=_filename_ids_cb)


def datamap_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """
    datamaps = DataFiles.datamaps(**kwargs)
    return pytest.mark.parametrize(argname, datamaps, ids=_filename_ids_cb)


def installscript_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """
    installscripts = DataFiles.installscripts(**kwargs).values()
    return pytest.mark.parametrize(argname, installscripts,
            ids=_filename_ids_cb)


def platform_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """
    platforms = DataFiles.platforms(**kwargs)
    return pytest.mark.parametrize(argname, platforms, ids=_filename_ids_cb)


class _SourceFiles(_Files):
    """
    Track a list of DATA_SRC_DIR XML.IN files and provide APIs for querying
    them. Meant to be initialized only once
    """
    def __init__(self):
        _Files.__init__(self, 'INTERNAL_OSINFO_DB_DATA_SRC_DIR', '.xml.in')


SourceFiles = _SourceFiles()


def os_sources_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """

    oses = SourceFiles.oses(**kwargs)
    return pytest.mark.parametrize(argname, oses, ids=_shortid_ids_cb)
