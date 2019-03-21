# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from collections import defaultdict

import os
import xml.etree.ElementTree as ET

import pytest

from . import osinfo


class _DataFiles():
    """
    Track a list of DATA_DIR XML files and provide APIs for querying
    them. Meant to be initialized only once
    """
    def __init__(self):
        self.datadir = os.environ['INTERNAL_OSINFO_DB_DATA_DIR']
        self.schema = os.path.join(self.datadir, 'schema', 'osinfo.rng')
        self._all_xml_cache = []
        self._oses_cache = []
        self._os_related_cache = defaultdict(list)

        if not os.path.exists(self.datadir):
            raise RuntimeError("INTERNAL_OSINFO_DB_DATA_DIR=%s "
                "doesn't exist" % self.datadir)

    def _get_all_xml(self):
        """
        Get and cache the full list of all DATA_DIR .xml paths
        """
        if not self._all_xml_cache:
            for (dirpath, _, filenames) in os.walk(self.datadir):
                for filename in filenames:
                    if not filename.endswith('.xml'):
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
            filter_devices=False, filter_resources=False):
        """
        Return a list of osinfo.Os objects

        :param filter_FOO: Only return objects that have at least one
            instance of a FOO object
        """
        if not self._oses_cache:
            for path in self._filter_xml('os'):
                osroot = ET.parse(path).getroot().find('os')
                self._oses_cache.append(osinfo.Os(osroot))

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
        return oses

    def get_os_related(self, _os):
        if _os.internal_id not in self._os_related_cache:
            directly_related = []
            if _os.derives_from is not None:
                for __os in self.oses():
                    if _os.derives_from == __os.internal_id:
                        directly_related.append(__os)
                        break

            if _os.clones is not None:
                for __os in self.oses():
                    if _os.clones == __os.internal_id:
                        directly_related.append(__os)
                        break

            self._os_related_cache[_os.internal_id].extend(directly_related)

            related = []
            for __os in directly_related:
                related.extend(self.get_os_related(__os))

            for __os in related:
                if __os not in self._os_related_cache[_os.internal_id]:
                    self._os_related_cache[_os.internal_id].append(__os)
        return self._os_related_cache[_os.internal_id]

    def xmls(self):
        return self._get_all_xml()


DataFiles = _DataFiles()


def os_parametrize(argname, **kwargs):
    """
    Helper for parametrizing a test with an OS list. Passthrough any
    extra arguments to DataFiles.oses()
    """
    def ids_cb(osxml):
        # pytest passes us a weird value when oses is empty, like for
        # test_urls image testing at the time of this commit
        return getattr(osxml, "shortid", str(osxml))

    oses = DataFiles.oses(**kwargs)
    return pytest.mark.parametrize(argname, oses, ids=ids_cb)
