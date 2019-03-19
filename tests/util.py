# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import logging
import os
import xml.etree.ElementTree as ET

from . import osinfo


def _get_files(directory):
    files = []
    datadir = os.environ.get('INTERNAL_OSINFO_DB_DATA_DIR')
    if datadir is not None:
        root = os.path.join(datadir, directory)
        for (dirpath, _, filenames) in os.walk(root):
            for filename in filenames:
                if not filename.endswith('.xml'):
                    continue
                files.append(os.path.join(dirpath, filename))
    else:
        logging.error('INTERNAL_OSINFO_DB_DATA_DIR is not set')
    return files


def _get_os(path):
    tree = ET.parse(path)
    root = tree.getroot()

    _os = root.find('os')
    return _os


def oses():
    _oses = []
    files = _get_files('os')
    if files:
        for _file in files:
            _oses.append(osinfo.Os(_get_os(_file)))
    return _oses


def xmls():
    return _get_files('')


def schema():
    _schema = None
    datadir = os.environ.get('INTERNAL_OSINFO_DB_DATA_DIR')
    if datadir is not None:
        _schema = os.path.join(datadir, 'schema', 'osinfo.rng')
    else:
        logging.error('INTERNAL_OSINFO_DB_DATA_DIR is not set')
    return _schema
