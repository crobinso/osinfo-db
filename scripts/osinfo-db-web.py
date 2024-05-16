#!/usr/bin/env python3

from pathlib import Path
import re
import subprocess

DOWNLOAD_URL = "https://releases.pagure.org/libosinfo"
DOWNLOAD_FORMAT = "tar.xz"

GITLAB_REPO_URL = "https://gitlab.com/libosinfo/osinfo-db"
GITLAB_BRANCH = "main"
GITLAB_CI_JOB = "publish"
GITLAB_CI_ARTIFACT_BASEURL = f"{GITLAB_REPO_URL}/-/jobs/artifacts/{GITLAB_BRANCH}/raw"
GITLAB_CI_ARTIFACT_NIGHTLY = (
    f"{GITLAB_CI_ARTIFACT_BASEURL}/osinfo-db-latest.tar.xz?job={GITLAB_CI_JOB}"
)

WEB_ROOT = Path("public")

GPG_KEYS = (
    "berrange.asc",
    "fidencio.asc",
    "toso.asc",
)

scripts_dir = Path(__file__).parent

WEB_ROOT.mkdir(exist_ok=True)

for key in GPG_KEYS:
    subprocess.check_call(["gpg", "--import", scripts_dir / "keys" / key])

tag_proc = subprocess.run(
    ["git", "tag"], check=True, capture_output=True, encoding="utf-8"
)

tags = tag_proc.stdout.strip().splitlines()

versions = []
for tag in tags:
    m = re.match(r"^v(\d{8})$", tag)
    if m is None:
        continue

    # Ensure that we abort if we see what looks like a release tag,
    # but without a signature, or with an unexpected user's signature
    subprocess.check_call(["git", "tag", "--verify", tag])

    version = m.group(1)
    versions.append(version)

    taginfo = WEB_ROOT / (version + ".json")

    with open(taginfo, "w") as fh:
        print(
            f"""{{
    "version": 1,
    "release": {{
        "version": "{version}",
        "archive": "{DOWNLOAD_URL}/osinfo-db-{version}.{DOWNLOAD_FORMAT}",
        "signature": "{DOWNLOAD_URL}/osinfo-db-{version}.{DOWNLOAD_FORMAT}.asc"
    }}
}}""",
            file=fh,
        )


nightly = WEB_ROOT / "nightly.json"

with open(nightly, "w") as fh:
    print(
        f"""{{
    "version": 1,
    "release": {{
        "archive": "{GITLAB_CI_ARTIFACT_NIGHTLY}"
    }}
}}""",
        file=fh,
    )

if versions:
    versions.sort()
    latest_version = versions[-1]
else:
    latest_version = "nightly"

latest_src = latest_version + ".json"
latest_dst = WEB_ROOT / "latest.json"

latest_dst.unlink(missing_ok=True)
latest_dst.symlink_to(latest_src)
