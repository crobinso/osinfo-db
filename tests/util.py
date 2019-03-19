# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import os
import xml.etree.ElementTree as ET

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

    def oses(self):
        if not self._oses_cache:
            for path in self._filter_xml('os'):
                osroot = ET.parse(path).getroot().find('os')
                self._oses_cache.append(osinfo.Os(osroot))
        return self._oses_cache

    def xmls(self):
        return self._get_all_xml()


DataFiles = _DataFiles()
