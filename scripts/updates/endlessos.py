#!/usr/bin/env python3
"""
Refreshes data from the index of downloadable images published by Endless.

Each OS branch gets a separate entry in the database; when a new release is made in
that series, the URLs are refreshed to mark the newest point release. When a new branch
is released, it is annotated as updating and deriving from the previous branch, and the
old branch is marked as end-of-life as of the first release on the new branch.
"""
# Copyright © 2019–2023 Endless OS Foundation LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
import argparse
import datetime
import json
import jinja2
import os
import packaging.version
import requests
import shutil
import subprocess
import tempfile

import gi

gi.require_version("GnomeDesktop", "4.0")
from gi.repository import GnomeDesktop  # noqa: E402

BASE_URL = "https://images-dl.endlessm.com"
MANIFEST_URL = BASE_URL + "/releases-eos-3.json"
DATA_DIR = os.path.relpath(
    os.path.abspath(
        os.path.join(
            os.path.dirname(__file__), "..", "..", "data", "os", "endlessos.com"
        )
    )
)
ISODATA_DIR = os.path.relpath(
    os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "..", "tests", "isodata", "eos")
    )
)

# Endless OS images have a "personality", which determines which content they have
# pre-installed and perhaps some settings overrides. (The base OS is the same in all
# cases.) For our downloadable images, the personality is currently either a locale ID
# ('en', 'pt_BR') or 'base' (which contains almost no pre-installed apps).
NON_LOCALE_PERSONALITIES = {"base": "Basic", "sea": "Southeast Asia"}


def personality_name(personality):
    # TODO: this is adapted from our installer, but we should include this information
    # in the releases-eos-3.json file.
    try:
        return NON_LOCALE_PERSONALITIES[personality]
    except KeyError:
        pass

    # Get the name of 'personality' in US English. This data actually comes from the
    # iso-codes package, in the iso_639 and iso_3166 gettext domains, with extra logic
    # to suppress the country name if the language is "unique".
    #
    # Without appending .UTF-8, these come out as (eg):
    #   Portuguese (Brazil) [ISO-8859-1]
    personality_as_locale = f"{personality}.UTF-8"
    locale_name = GnomeDesktop.get_language_from_locale(personality_as_locale, "en_US")
    if locale_name:
        # TODO: consider using the same API to generate translations from
        # "Endless OS Portuguese (Brazil)" to "Endless OS Portugais (Brésil)". At
        # present, we do not transliterate "Endless OS" to (eg) "Ендлесс ОС" but if this
        # changed we would need to be a bit careful.
        return locale_name

    return personality


def publisher_id(branch):
    if packaging.version.Version(branch) >= packaging.version.Version("3.9"):
        return "ENDLESS OS FOUNDATION LLC"
    else:
        return "ENDLESS COMPUTERS"


def series(branch):
    eos_prefix_len = len("eos")
    return branch[eos_prefix_len:]


def adj(series, delta):
    major, minor = series.split(".")
    previous_minor = int(minor) + delta
    return f"{major}.{previous_minor}"


def predecessor(series):
    # Branching/Numbering scheme changed slightly in the transition from
    # eos3.9 → eos4.0 and eos5.0
    if series == "4.0":
        return "3.9"
    if series == "5.0":
        return "4.0"
    if series == "6.0":
        return "5.1"
    return adj(series, -1)


def successor(series):
    # Branching/Numbering scheme changed slightly in the transition from
    # eos3.9 → eos4.0 and eos5.0
    if series == "3.9":
        return "4.0"
    if series == "4.0":
        return "5.0"
    if series == "5.1":
        return "6.0"

    return adj(series, 1)


def guess_release_date(image):
    release_dates = [
        datetime.datetime.fromisoformat(i["iso"]["last_modified"]).date()
        for i in image["personality_images"].values()
        if "iso" in i
    ]
    return min(release_dates, default=None)


def all_personalities_for_branch(images):
    # We sometimes add and remove personalities within a series. For example, in
    # 3.4.5 we removed ar, bn, es_GT, es_MX and zh_CN from the set of downloadable
    # images.
    return {
        personality for image in images for personality in image["personality_images"]
    }


def fetch_isodata(branch, iso):
    iso_url = BASE_URL + "/" + iso["file"]
    isodata_file = os.path.join(ISODATA_DIR, branch, os.path.basename(iso_url) + ".txt")

    if os.path.exists(isodata_file):
        return
    print(f"Fetching first 2MiB of {iso_url}...")

    response = requests.get(iso_url, headers={"Range": "bytes=0-2097152"}, stream=True)
    with tempfile.NamedTemporaryFile(suffix=os.path.basename(iso_url)) as f:
        for chunk in response.iter_content(chunk_size=8096):
            f.write(chunk)

        f.flush()

        isodata_dir = os.path.dirname(isodata_file)
        os.makedirs(isodata_dir, exist_ok=True)
        with tempfile.NamedTemporaryFile(dir=isodata_dir) as g:
            subprocess.run(
                ("isoinfo", "-d", "-i", f.name),
                stdout=g,
                check=True,
            )
            shutil.copyfile(g.name, isodata_file)


def fetch_all_isodata(image):
    for personality_images in image["personality_images"].values():
        fetch_isodata(image["branch"], personality_images["iso"])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--releases-json",
        type=argparse.FileType("r"),
        metavar="PATH",
        help=f"path to releases-eos-3.json file (default: fetch from {MANIFEST_URL})",
    )
    args = parser.parse_args()

    if args.releases_json is not None:
        manifest = json.load(args.releases_json)
    else:
        response = requests.get(MANIFEST_URL)
        response.raise_for_status()
        manifest = response.json()

    images = list(manifest["images"].values())
    images.sort(key=lambda i: packaging.version.Version(i["version"]))
    images_by_branch = {}
    for image in images:
        images_by_branch.setdefault(image["branch"], []).append(image)

    # Prepare template
    env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(DATA_DIR),
        autoescape=True,
        undefined=jinja2.StrictUndefined,
        keep_trailing_newline=True,
    )
    env.filters["personality_name"] = personality_name
    env.filters["publisher_id"] = publisher_id
    template = env.get_template("eos-3.xml.in.in")

    for branch in images_by_branch:
        oldest_image = images_by_branch[branch][0]
        image = images_by_branch[branch][-1]
        release_date = guess_release_date(oldest_image)
        if not release_date:
            # eos3.0 did not have ISOs; pretend it never existed!
            continue

        current_series = series(image["branch"])

        previous_series = predecessor(current_series)
        if guess_release_date(images_by_branch[f"eos{previous_series}"][-1]) is None:
            # eos3.0 did not have ISOs; pretend it never existed!
            previous_series = None

        next_series = successor(current_series)
        try:
            next_series_images = images_by_branch[f"eos{next_series}"]
        except KeyError:
            eol_date = None
        else:
            if current_series == "4.0":
                # Endless OS 4 was an LTS release, supported in parallel with
                # Endless OS 5.x for a time; it was discontinued on this date.
                eol_date = datetime.date(2024, 2, 29)
            else:
                # Date of first release in next series
                eol_date = guess_release_date(next_series_images[0])

        retired_personalities = (
            all_personalities_for_branch(images_by_branch[branch])
            - image["personality_images"].keys()
        )

        xml = template.render(
            base_url=BASE_URL,
            image=image,
            release_date=release_date,
            current_series=current_series,
            previous_series=previous_series,
            eol_date=eol_date,
            retired_personalities=retired_personalities,
        )
        with open(os.path.join(DATA_DIR, f"eos-{current_series}.xml.in"), "w") as f:
            f.write(xml)

        if eol_date is None:
            fetch_all_isodata(image)

        # We had an entry for eos3.10 in osinfo-db, which actually ended up being
        # released as eos4.0. Since we can never remove entries from osinfo-db,
        # generate it as an EOLed clone of eos4.0, with no images.
        if current_series == "4.0":
            xml = template.render(
                base_url=BASE_URL,
                image={"branch": "eos3.10", "personality_images": {}},
                release_date=None,
                current_series="3.10",
                previous_series=previous_series,
                eol_date=release_date,
                retired_personalities=[],
            )
            with open(os.path.join(DATA_DIR, "eos-3.10.xml.in"), "w") as f:
                f.write(xml)


if __name__ == "__main__":
    main()
