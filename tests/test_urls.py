# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import enum
import http
import logging

import pytest
import requests
import urllib3

from . import util


class UniqueSet(set):
    """
    Special set that is never equal to any other set.

    This particular subclass of set is useful only as value in enum,
    to ensure that enum items that have the same items are still considered
    different; this is because enum.Enum turns duplicate values as aliases
    for the first item.

    Yes, it's an evil hack.
    """

    def __eq__(self, _):
        return False


class UrlType(UniqueSet, enum.Enum):
    URL_ISO = {
        # proper ISO mimetype
        "application/x-cd-image",
        "application/x-iso9660-image",
        "application/x-iso",
        # generic data
        "application/octet-stream",
        "binary/octet-stream",
        # ISO files on archive.netbsd.org
        "text/plain",
        # a few openSUSE Live images
        "application/x-up-download",
    }
    URL_INITRD = {
        # generic data
        "application/octet-stream",
        # gzip-compressed
        "application/x-gzip",
    }
    URL_DISK_RAW = {}
    URL_DISK_QCOW2 = {
        # generic data
        "application/octet-stream",
        # qcow2 files on some mirrors
        "application-x-qemu-disk",
        # qcow2 files on fedoraproject.org mirrors; similar issue of
        # https://pagure.io/fedora-infrastructure/issue/10766
        "application/x-troff-man",
        # qcow2 files on some opensuse.org mirrors
        "text/plain",
    }
    URL_DISK_VMDK = {}
    URL_TREEINFO = {
        # generic data
        "application/octet-stream",
        # on some Fedora mirrors
        "text/plain",
    }
    URL_DISK_CONTAINERDISK = {
        # image manifest
        "application/vnd.docker.distribution.manifest.v1+json",
    }
    URL_KERNEL = {
        # generic data
        "application/octet-stream",
    }
    URL_TREE = {
        # HTML page of file listing
        "text/html",
        "application/xhtml+xml",
    }
    URL_DISK_VHDX = {}


image_formats_types = {
    "containerdisk": UrlType.URL_DISK_CONTAINERDISK,
    "qcow2": UrlType.URL_DISK_QCOW2,
    "raw": UrlType.URL_DISK_RAW,
    "vhdx": UrlType.URL_DISK_VHDX,
    "vmdk": UrlType.URL_DISK_VMDK,
}


def _transform_docker_url(url):
    """
    Transform docker:// url into a docker registry API call
    See: https://docs.docker.com/registry/spec/api/#existing-manifests
    """
    url_parts = url.split("/")
    url = f"http://{url_parts[2]}/v2/"
    for i in range(3, len(url_parts) - 1):
        url += f"{url_parts[i]}/"
    image, tag = url_parts[-1].split(":")
    url += f"{image}/manifests/{tag}"
    return url


def _check_url(session: requests.Session, url, url_type, real_url=None):
    logging.info("url: %s, type: %s", real_url if real_url else url, url_type)
    headers = {"user-agent": "Wget/1.0"}
    try:
        response = session.head(url, allow_redirects=True, headers=headers, timeout=30)
    except requests.RequestException as e:
        # do not use logging.exception() here, as there is no need to log
        # the full traceback from requests
        logging.error("head: %s", e)
        return False
    content_type = response.headers.get("content-type")
    if content_type:
        try:
            content_type = content_type[0 : content_type.index(";")]
        except ValueError:
            pass

    msg = "response: %s; code: %d; content-type: %s; url: %s" % (
        http.client.responses[response.status_code],
        response.status_code,
        content_type,
        response.url,
    )

    if not response.ok:
        logging.error(msg)
        return False
    if content_type and content_type not in url_type.value:
        logging.error(msg)
        return False

    logging.info(msg)
    return True


def _collect_os_urls():
    """
    Iterate the OS list and return a list of pairs (shortid, [url list)
    """
    ret = []

    for osxml in util.DataFiles.oses():
        urls = []
        urls.extend(
            [(i.url, image_formats_types[i.format]) for i in osxml.images if i.url]
        )
        urls.extend([(m.url, UrlType.URL_ISO) for m in osxml.medias if m.url])
        for t in osxml.trees:
            if not t.url:
                continue
            urls.append((t.url, UrlType.URL_TREE))
            url = t.url
            if not url.endswith("/"):
                url += "/"
            if t.kernel:
                urls.append((url + t.kernel, UrlType.URL_KERNEL))
            if t.initrd:
                urls.append((url + t.initrd, UrlType.URL_INITRD))
            if t.treeinfo:
                urls.append((url + ".treeinfo", UrlType.URL_TREEINFO))
        if urls:
            urls_http = []
            urls_docker = []
            urls_other = []
            for url_pair in urls:
                u = url_pair[0]
                if u.startswith("http://") or u.startswith("https://"):
                    urls_http.append(url_pair)
                elif u.startswith("docker://"):
                    urls_docker.append(url_pair)
                else:
                    urls_other.append(url_pair)
            assert len(urls_http) + len(urls_docker) + len(urls_other) == len(urls)
            ret.append(
                pytest.param(urls_http, urls_docker, urls_other, id=osxml.shortid)
            )

    return ret


@pytest.fixture(scope="module")
def session():
    session = requests.Session()
    # As some distro URLs are flaky or rate limit requests (429) use Retry to
    # help navigate those at the expense of taking longer.
    # Use an high value for pool_connections: this represents the number of
    # urllib HTTP connection pools instantiated, each for a different host:
    # this way, we can reuse more connections across OSes.
    retries = urllib3.util.Retry(backoff_factor=1, status_forcelist=(429,), total=6)
    adapter = requests.adapters.HTTPAdapter(max_retries=retries, pool_connections=100)
    session.mount("https://", adapter)
    session.mount("http://", adapter)
    return session


@pytest.mark.parametrize("urls_http,urls_docker,urls_other", _collect_os_urls())
def test_urls(urls_http, urls_docker, urls_other, session):
    broken = []
    for url, url_type in urls_http:
        ok = _check_url(session, url, url_type)

        if not ok:
            broken.append(url)
    for url, url_type in urls_docker:
        http_url = _transform_docker_url(url)
        ok = _check_url(session, http_url, url_type, real_url=url)

        if not ok:
            broken.append(url)
    for url, _ in urls_other:
        logging.warning("unhandled URL: %s", url)
        broken.append(url)
    assert broken == []
