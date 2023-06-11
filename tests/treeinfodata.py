# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import configparser


class _TreeinfoData:
    def __init__(self, path, arch, family, variant, version):
        self.path = path
        self.arch = arch or ""
        self.family = family or ""
        self.variant = variant or ""
        self.version = version or ""

    def match(self, treeinfo):
        if (
            bool(treeinfo.arch.match(self.arch))
            and bool(treeinfo.family.match(self.family))
            and bool(treeinfo.variant.match(self.variant))
            and bool(treeinfo.version.match(self.version))
        ):
            return True

        return False


def get_treeinfodata(filepath):
    arch = None
    family = None
    variant = None
    version = None

    config = configparser.ConfigParser()
    with filepath.open("r") as out:
        config.read_file(out)
        arch = ""
        family = ""
        variant = ""
        version = ""

        if "arch" in config["general"]:
            arch = config["general"]["arch"]

        if "family" in config["general"]:
            family = config["general"]["family"]

        if "variant" in config["general"]:
            variant = config["general"]["variant"]

        if "version" in config["general"]:
            version = config["general"]["version"]

    return _TreeinfoData(filepath, arch, family, variant, version)
