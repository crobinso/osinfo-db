#!/usr/bin/env python3

# With Nix, run in:
# nix-shell -p "python3.withPackages (p: [ p.lxml p.requests ])" -p cdrkit

"""
This script is supposed to be run when a new stable version of NixOS is released.
It takes the new release number, codename, and release date as inputs and does
the following:

1) Creates new stable osinfo-db entry. This entry is based on the previous entry XML by
   substituting the release numbers and removing unnecessary tags.

2) Updates the unstable entry by bumping the release number to the version that comes
   after the new release, the current in-development version. Also <upgrades> and
   <derives-from> tags are updated to point to the new stable entry.

3) Updates the unknown entry with volume-id regular expression that matches all nixos
   ISOs newer than the unstable release.

4) Downloads ISOs of the new release, runs isoinfo -d on them, and saves the result
   to tests/isodata/nixos.

5) Regenerates the isodata of the unstable release - deletes the old isodata,
   downloads new ISOs, saves the result to tests/isodata/nixos.

Please note the "derives-from" (as well as "upgrades") relation maintains following
chain:

 +-------+      +-------+               +-----------------+      +----------+      +---------+
 | 20.03 | <--- | 20.09 | <--- ... <--- | (latest stable) | <--- | unstable | <--- | unknown |
 +-------+      +-------+               +-----------------+      +----------+      +---------+

When a release is derived from another it inherits its list of supported devices,
firmware types, as well as its logo[1]. The purpose of the unknown entry is to
recognize new NixOS releases on systems with outdated osinfo-db.

It should be safe to run this script multiple times with the same --release argument.
Doctests can be executed with "python -m doctest -v nixos.py". Future archaeologists
might be interested in [2][3]. Good luck!

[1] https://gitlab.gnome.org/GNOME/gnome-boxes/-/blob/master/data/osinfo/nixos-20.03.xml
[2] https://gitlab.com/libosinfo/osinfo-db/-/merge_requests/107
[3] https://github.com/NixOS/nixpkgs/pull/83551

"""

import argparse
import glob
import os.path
import logging
import re
import shutil
import subprocess
import sys

from lxml import etree
import requests

DATA_DIR = os.path.relpath(
    os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..", "data", "os", "nixos.org")
    )
)

TEST_DIR = os.path.relpath(
    os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..", "tests", "isodata", "nixos")
    )
)

UNSTABLE = os.path.join(DATA_DIR, "nixos-unstable.xml.in")
UNKNOWN = os.path.join(DATA_DIR, "nixos-unknown.xml.in")
ISO_BYTES = 2 * 1024 * 1024


def fatal(*args):
    logging.critical(*args)
    sys.exit(1)


def guess_next_release(release):
    """
    Guess next release number by adding 6 months.
    >>> guess_next_release("21.09")
    '22.03'
    """
    (y, m) = release.split(".")
    m = int(m) + 6
    y = int(y) + (m / 12)
    m = m % 12
    return "%02d.%02d" % (y, m)


def str_regex(pattern, suggested_format):
    def validate(arg):
        if not re.match(pattern, arg):
            raise argparse.ArgumentTypeError(
                f"invalid format, must be {suggested_format}"
            )
        return arg

    return validate


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--release",
        type=str_regex(r"^\d\d[.]\d\d$", "YY.MM"),
        help="release series number (e.g. 20.03)",
        required=True,
    )
    parser.add_argument(
        "--codename",
        type=str,
        help="release code name (e.g. Markhor)",
        required=True,
    )
    parser.add_argument(
        "--release-date",
        type=str_regex(r"^20\d\d-[01]\d-[0-3]\d$", "YYYY-MM-DD"),
        help="release date (e.g. 2020-04-20)",
        required=True,
    )
    parser.add_argument(
        "--next-release",
        type=str_regex(r"^\d\d[.]\d\d$", "YY.MM"),
        help="next release number after --release (plus 6 months by default)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print debugging output",
    )
    args = parser.parse_args()

    if args.next_release is None:
        args.next_release = guess_next_release(args.release)

    return args


def check_isoinfo():
    exe = shutil.which("isoinfo")
    if exe is None:
        fatal("Command 'isoinfo' not in $PATH, please install cdrkit or cdrtools")
    logging.debug("Found isoinfo in %s", exe)


def find_prev_stable(new_release):
    """
    Finds previous stable entry to use as a template for the new release.
    """
    files = glob.glob(DATA_DIR + "/nixos-[0-9][0-9].[0-9][0-9].xml.in")

    # discard the new release from list if it's there to make it
    # possible to run this script multiple times
    files = sorted(filter(lambda f: new_release not in f, files))
    if len(files) < 1:
        fatal("Can't find previous stable release file in %s", DATA_DIR)
    prev_file = files[-1]

    m = re.search(r"nixos-([0-9][0-9][.][0-9][0-9]).xml.in$", prev_file)
    if m is None:
        fatal("Can't parse previous release number: %s", prev_file)

    logging.info(
        "Creating new release %s with %s as a template", new_release, prev_file
    )
    return (m[1], etree.parse(prev_file))


def tag_replace_text(xml, path, text, replacement):
    """
    Performs search and replace on all elements matching given XPath.
    """
    for e in xml.findall(path):
        replaced = str.replace(e.text, text, replacement)
        logging.debug("%s: %s -> %s", path, e.text, replaced)
        e.text = str.replace(e.text, text, replacement)


def set_derives_from(xml_os, release):
    """
    Sets the derives-from and upgrades tags to the given release.
    """
    logging.debug("Setting upgrades, derives-from to %s", release)
    release_id = f"http://nixos.org/nixos/{release}"
    if xml_os.find("upgrades") is not None and xml_os.find("derives-from") is not None:
        xml_os.find("upgrades").set("id", release_id)
        xml_os.find("derives-from").set("id", release_id)
    else:
        # This branch is supposed to be used by 20.09 only and can be removed
        # afterwards.
        sib = xml_os.find("codename")
        e = etree.Element("derives-from", id=release_id)
        sib.addnext(e)
        e = etree.Element("upgrades", id=release_id)
        e.tail = "\n    "
        sib.addnext(e)
        sib.tail = "\n\n    "


def digit_gt(s):
    """
    Helper function that returns regular expression matching all digits greater than s.

    >>> digit_gt("0")
    '[1-9]'
    >>> digit_gt("1")
    '[2-9]'
    >>> digit_gt("2")
    '[3-9]'
    >>> digit_gt("7")
    '[8-9]'
    >>> digit_gt("8")
    '9'
    """
    assert len(s) == 1 and s != "9"
    n = int(s) + 1
    return "9" if n == 9 else "[%s-9]" % n


def regex_higher_than(release):
    """
    Returns regular expression matching all release numbers newer than the function
    argument. Please note that the returned expression also matches release numbers
    that do not have valid month number on the left side of the dot (e.g. 23.66).
    Should not matter, it is only important to not match any release number that are
    older than the argument.

    >>> regex_higher_than("20.03")
    '20.0[4-9]|20.[1-9]\\\\d|2[1-9].\\\\d\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("20.09")
    '20.[1-9]\\\\d|2[1-9].\\\\d\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("23.03")
    '23.0[4-9]|23.[1-9]\\\\d|2[4-9].\\\\d\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("23.09")
    '23.[1-9]\\\\d|2[4-9].\\\\d\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("23.11")
    '23.1[2-9]|23.[2-9]\\\\d|2[4-9].\\\\d\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("29.09")
    '29.[1-9]\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("29.10")
    '29.1[1-9]|29.[2-9]\\\\d|[3-9]\\\\d.\\\\d\\\\d'
    >>> regex_higher_than("29.99")
    '[3-9]\\\\d.\\\\d\\\\d'
    """
    variants = []
    (y, m) = release.split(".")

    # same year
    if m[1] != "9":
        variants.append(y + "." + m[0] + digit_gt(m[1]))
    if m[0] != "9":
        variants.append(y + "." + digit_gt(m[0]) + r"\d")
    # same decade
    if y[1] != "9":
        variants.append(y[0] + digit_gt(y[1]) + r".\d\d")
    # further
    if y[0] != "9":
        variants.append(digit_gt(y[0]) + r"\d.\d\d")

    return "|".join(variants)


def create_new_stable(release, codename, release_date):
    """
    Creates new stable entry based on the previous stable entry. Replaces release
    numbers, sets codename, release date, points derives-from to the previous entry,
    removes redundant tags that are inherited by the derives-from pointer.
    """
    prev_release, xml = find_prev_stable(release)
    xml_os = xml.find("os")
    xml_os.set("id", f"http://nixos.org/nixos/{release}")
    tag_replace_text(xml_os, "short-id", prev_release, release)
    tag_replace_text(xml_os, "name", prev_release, release)
    tag_replace_text(xml_os, "version", prev_release, release)
    tag_replace_text(xml_os, "variant/name", prev_release, release)
    tag_replace_text(xml_os, "media/url", prev_release, release)
    tag_replace_text(xml_os, "media/iso/volume-id", prev_release, release)
    xml_os.find("codename").text = codename
    xml_os.find("release-date").text = release_date

    for tag_to_remove in ("devices", "firmware"):
        for e in xml_os.findall(tag_to_remove):
            logging.debug(
                "Removing <%s> because it is inherited via derives-from", tag_to_remove
            )
            xml_os.remove(e)

    set_derives_from(xml_os, prev_release)
    filename = os.path.join(DATA_DIR, f"nixos-{release}.xml.in")
    xml.write(filename)
    logging.info("Created new stable entry %s", filename)
    return xml


def update_unstable(new_release, next_release):
    """
    Updates the unstable entry to have volume-id matching the next, yet-unreleased,
    version number. Changes derives-from to point to the newly added stable release.
    """
    logging.info("Updating %s to %s", UNSTABLE, next_release)
    xml = etree.parse(UNSTABLE)
    xml_os = xml.find("os")
    if new_release not in xml_os.find("media/iso/volume-id").text:
        logging.warning(
            "Current unstable volume-id does not contain %s - make sure the new value "
            "is correct",
            new_release,
        )
    tag_replace_text(xml_os, "media/iso/volume-id", new_release, next_release)
    set_derives_from(xml_os, new_release)
    xml.write(UNSTABLE)
    logging.info("Updated unstable entry %s", UNSTABLE)
    return xml


def update_unknown(next_release):
    """
    Updates the regular expression of the Unknown NixOS osinfo-db entry. The expression
    should match any version that is higher than what Unstable matches. The purpose
    of the Unknown entry is to make it possible to match NixOS ISOs even on system with
    outdated osinfo-db.
    """
    xml = etree.parse(UNKNOWN)
    regex = regex_higher_than(next_release)
    xml.find("os/media/iso/volume-id").text = "nixos-.*-(" + regex + ")-.*"
    xml.write(UNKNOWN)
    logging.info("Updated unknown entry %s", UNKNOWN)


def fetch_iso(url, directory):
    """
    Downloads first 2 MiB of ISO image to the given directory. As of spring 2020
    the URLs in the entries are redirects pointing to URLs with the ISO filename
    that includes build number and git commit, we make sure to save the ISOs under
    this more descriptive name.
    """
    r = requests.get(url, stream=True)
    r.raise_for_status()
    filename = r.request.url.split("/")[-1]
    logging.debug("%s filename: %s", url, filename)
    filename = os.path.join(directory, filename)
    logging.info("Downloading %s", url)
    with open(filename, "wb") as fh:
        fh.write(next(r.iter_content(chunk_size=ISO_BYTES)))
    logging.debug("Downloaded %s", filename)
    return filename


def run_isoinfo(isofile, outfile):
    """
    Runs isoinfo -d -i isofile and saves it to outfile.
    """
    res = subprocess.run(["isoinfo", "-d", "-i", isofile], capture_output=True)
    if res.returncode != 0:
        logging.error("isoinfo failed on %s:\n%s", isofile, res.stderr)
        return

    with open(outfile, "wb") as fh:
        fh.write(res.stdout)
    logging.info("Saved isoinfo to %s", outfile)


def process_iso(url, directory):
    """
    Download the beginning of ISO image and run isoinfo on it. Errors are not fatal
    to accommodate for scenario where variants are removed.
    """
    try:
        isofile = fetch_iso(url, directory)
    except requests.RequestException as e:
        logging.error("Failed to fetch %s: %s", url, str(e))
        return []
    try:
        testdata_file = isofile + ".txt"
        run_isoinfo(isofile, testdata_file)
    finally:
        os.remove(isofile)
    return [testdata_file]


def update_testdata(new_release, stable_xml, unstable_xml):
    """
    Update testdata used by "make check". Creates isoinfo for the new release.
    Deletes it for the previous unstable release and regenerates it from current
    unstable isos. "Unknown" entry has no testdata.
    """
    add = []
    remove = []

    logging.info("Creating ISO test data for %s", new_release)
    new_release_dir = os.path.join(TEST_DIR, f"nixos-{new_release}")
    os.makedirs(new_release_dir, exist_ok=True)
    for e in stable_xml.findall("os/media/url"):
        add += process_iso(e.text, new_release_dir)

    logging.info("Updating ISO test data for Unstable")
    unstable_dir = os.path.join(TEST_DIR, "nixos-unstable")
    for f in os.listdir(unstable_dir):
        if f.endswith(".iso.txt"):
            logging.warning("Deleting unstable testdata %s", f)
            os.remove(os.path.join(unstable_dir, f))
            remove.append(os.path.join(unstable_dir, f))
    for e in unstable_xml.findall("os/media/url"):
        add += process_iso(e.text, unstable_dir)

    return (add, remove)


def print_instructions(release, git_add, git_rm):
    """
    Remind the user of manual steps that need to be done before submitting merge
    request with the new release.
    """
    new_release_file = os.path.join(DATA_DIR, f"nixos-{release}.xml.in")
    logging.warning(
        "Make sure <variant>s are up to date in %s and %s", new_release_file, UNSTABLE
    )
    logging.warning(
        "Make sure system <resources> are appropriate in %s and %s and %s",
        new_release_file,
        UNSTABLE,
        UNKNOWN,
    )
    logging.warning(
        "If new virtual hardware is supported, add new <devices> to %s",
        new_release_file,
    )
    logging.warning(
        "Run git add:\n%s",
        "\n".join(map(lambda f: f"\tgit add {f}", [new_release_file] + git_add)),
    )
    logging.warning(
        "Run git rm:\n%s", "\n".join(map(lambda f: f"\tgit rm {f}", git_rm))
    )
    logging.warning("Run git commit -a, make check, and go create a merge request!")


def main():
    args = parse_args()
    logging.basicConfig(
        format="%(levelname)s\t%(message)s",
        level=(logging.DEBUG if args.verbose else logging.INFO),
    )
    check_isoinfo()

    stable_xml = create_new_stable(args.release, args.codename, args.release_date)
    unstable_xml = update_unstable(args.release, args.next_release)
    update_unknown(args.next_release)
    add, remove = update_testdata(args.release, stable_xml, unstable_xml)
    print_instructions(args.release, add, remove)


if __name__ == "__main__":
    main()
