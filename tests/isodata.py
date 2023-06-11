# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.


class _ISODataMedia:
    def __init__(
        self, path, volumeid, publisherid, systemid, applicationid, volumesize
    ):
        self.path = path
        self.volumeid = volumeid if volumeid is not None else ""
        self.publisherid = publisherid if publisherid is not None else ""
        self.systemid = systemid if systemid is not None else ""
        self.applicationid = applicationid if applicationid is not None else ""
        self.volumesize = volumesize if volumesize is not None else 0

    def match(self, media):
        volumesize = media.volumesize
        if volumesize == 0:
            volumesize = self.volumesize

        if (
            bool(media.volumeid.match(self.volumeid))
            and bool(media.publisherid.match(self.publisherid))
            and bool(media.applicationid.match(self.applicationid))
            and bool(media.systemid.match(self.systemid))
            and volumesize == self.volumesize
        ):
            return True

        return False


def _get_value(string, prefix, return_type=str):
    if string.startswith(prefix):
        return return_type(string[len(prefix) :].strip())
    return None


def _get_volumeid(string):
    return _get_value(string, "Volume id: ")


def _get_publisherid(string):
    return _get_value(string, "Publisher id: ")


def _get_systemid(string):
    return _get_value(string, "System id: ")


def _get_applicationid(string):
    return _get_value(string, "Application id: ")


def _get_logicalblock(string):
    return _get_value(string, "Logical block size is: ", int)


def _get_volumesize(string):
    return _get_value(string, "Volume size is: ", int)


def get_isodatamedia(filepath):
    volumeid = None
    publisherid = None
    systemid = None
    applicationid = None
    logicalblock = None
    volumesize = None

    with filepath.open("r") as out:
        for line in out.readlines():
            if volumeid is None:
                volumeid = _get_volumeid(line)
            if publisherid is None:
                publisherid = _get_publisherid(line)
            if systemid is None:
                systemid = _get_systemid(line)
            if applicationid is None:
                applicationid = _get_applicationid(line)
            if logicalblock is None:
                logicalblock = _get_logicalblock(line)
            if volumesize is None:
                volumesize = _get_volumesize(line)

    if logicalblock is not None and volumesize is not None:
        volumesize *= logicalblock
    else:
        volumesize = None

    return _ISODataMedia(
        filepath, volumeid, publisherid, systemid, applicationid, volumesize
    )
