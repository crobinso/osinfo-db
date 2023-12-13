#!/usr/bin/env python3

import argparse
import os
import shutil
import sys
import tempfile
import time
from pathlib import Path


topdir = os.path.realpath(os.path.join(os.path.dirname(__file__), ".."))
datadir = os.path.join(topdir, "data")
sys.path.insert(0, topdir)
os.environ["INTERNAL_OSINFO_DB_DATA_DIR"] = datadir
os.environ["INTERNAL_OSINFO_DB_DATA_SRC_DIR"] = datadir

import tests.isodata
import tests.util


def fail(msg):
    print(msg)
    sys.exit(1)


##############################
# main() and option handling #
##############################


def _parse_args():
    desc = (
        "Helper script for adding iso test data to the test "
        "suite, and matching <os> <media> data to the DB"
    )
    parser = argparse.ArgumentParser(description=desc)

    parser.add_argument("shortid", help="Which <os> short-id the ISO is media for")
    parser.add_argument("iso", help="The path to the ISO media")
    parser.add_argument(
        "--arch",
        default="x86_64",
        help="The OS architecture the media is for. default=x86_64",
    )

    options = parser.parse_args()
    return options


def _main():
    """
    This is a template for new command line programs. Copy and edit it!
    """
    options = _parse_args()

    iso = os.path.realpath(os.path.abspath(options.iso))
    isoinfobin = shutil.which("isoinfo")
    if not os.path.exists(iso):
        fail("iso does not exist: %s" % iso)
    if not isoinfobin:
        fail("isoinfo is not installed")

    osxml = None
    for o in tests.util.DataFiles.oses():
        if o.shortid == options.shortid:
            osxml = o
            break
    if not osxml:
        fail("Did not find any os shortid=%s" % options.shortid)
        return

    destdir = os.path.join(topdir, "tests", "isodata", osxml.distro, options.shortid)
    destpath = os.path.join(destdir, os.path.basename(iso) + ".txt")

    tmp = tempfile.NamedTemporaryFile()
    ret = os.system("isoinfo -d -i %s > %s" % (iso, tmp.name))
    if ret != 0:
        fail("Command failed, returncode=%s" % ret)

    # parse isoinfo
    # output an example media block
    isodata = tests.isodata.get_isodatamedia(Path(tmp.name))
    print("XML to add to %s :" % (str(osxml.path)[len(topdir) + 1 :] + ".in"))
    print()
    print('    <media arch="%s">' % options.arch)
    print("      <url>XXX</url>")
    print("      <iso>")

    if isodata.volumeid:
        print("        <volume-id>%s</volume-id>" % isodata.volumeid)
    if isodata.systemid:
        print("        <system-id>%s</system-id>" % isodata.systemid)
    if isodata.publisherid:
        print("        <publisher-id>%s</publisher-id>" % isodata.publisherid)
    if isodata.applicationid:
        print("        <application-id>%s</application-id>" % isodata.applicationid)
    if isodata.volumesize:
        print("        <volume-size>%s</volume-size>" % isodata.volumesize)

    print("      </iso>")
    print("      <kernel>XXX</kernel>")
    print("      <initrd>XXX</initrd>")
    print("    </media>")
    print()

    print("\n\nSleeping 5 seconds before writing test data to:\n%s" % destpath)
    time.sleep(5)
    if not os.path.exists(destdir):
        os.system("mkdir -p %s" % destdir)
    os.system("cp %s %s" % (tmp.name, destpath))
    print("Done.")

    return 0


if __name__ == "__main__":
    sys.exit(_main())
