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

        if not os.path.exists(self.datadir):
            raise RuntimeError("INTERNAL_OSINFO_DB_DATA_DIR=%s "
                "doesn't exist" % self.datadir)

    def _get_files(self, directory):
        files = []
        root = os.path.join(self.datadir, directory)
        for (dirpath, _, filenames) in os.walk(root):
            for filename in filenames:
                if not filename.endswith('.xml'):
                    continue
                files.append(os.path.join(dirpath, filename))
        return files

    def oses(self):
        ret = []
        files = self._get_files('os')
        for path in files:
            osroot = ET.parse(path).getroot().find('os')
            ret.append(osinfo.Os(osroot))
        return ret

    def xmls(self):
        return self._get_files('')


DataFiles = _DataFiles()
