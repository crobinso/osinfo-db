# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import enum
import http
import logging

import pytest
import requests

from . import util


class UrlType(enum.Enum):
    URL_GENERIC = 1
    URL_ISO = 2
    URL_INITRD = 3
    URL_DISK_RAW = 4
    URL_DISK_QCOW2 = 5
    URL_DISK_VMDK = 6


iso_content_types = {
    # proper ISO mimetype
    'application/x-iso9660-image',
    # generic data
    'application/octet-stream',
    'binary/octet-stream',
    # ISO files on archive.netbsd.org
    'text/plain',
    # a few openSUSE Live images
    'application/x-up-download',
}


initrd_content_types = {
    # generic data
    'application/octet-stream',
    # gzip-compressed
    'application/x-gzip',
}


raw_content_types = {
}


qcow2_content_types = {
    # generic data
    'application/octet-stream',
    # qcow2 files on fedoraproject.org mirrors; similar issue of
    # https://pagure.io/fedora-infrastructure/issue/10766
    'application/x-troff-man',
}


vmdk_content_types = {
}


def _is_content_type_allowed(content_type, url_type):
    if url_type == UrlType.URL_ISO:
        return content_type in iso_content_types
    if url_type == UrlType.URL_INITRD:
        return content_type in initrd_content_types
    if url_type == UrlType.URL_DISK_RAW:
        return content_type in raw_content_types
    if url_type == UrlType.URL_DISK_QCOW2:
        return content_type in qcow2_content_types
    if url_type == UrlType.URL_DISK_VMDK:
        return content_type in vmdk_content_types
    return True


def _check_url(url, url_type):
    logging.info("url: %s, type: %s", url, url_type)
    headers = {'user-agent': 'Wget/1.0'}
    response = requests.head(url, allow_redirects=True, headers=headers, timeout=30)
    content_type = response.headers.get('content-type')
    if content_type:
        try:
            content_type = content_type[0:content_type.index(';')]
        except ValueError:
            pass
    logging.info("response: %s; code: %d; content-type: %s",
                 http.client.responses[response.status_code],
                 response.status_code, content_type)
    if not response.ok:
        return False
    if content_type and not _is_content_type_allowed(content_type, url_type):
        return False
    return True


def _collect_os_urls():
    """
    Iterate the OS list and return a list of pairs (shortid, [url list)
    """
    ret = []

    for osxml in util.DataFiles.oses():
        urls = []
        for i in osxml.images:
            if not i.url:
                continue
            url_type = UrlType.URL_GENERIC
            if i.format == 'raw':
                url_type = UrlType.URL_DISK_RAW
            elif i.format == 'qcow2':
                url_type = UrlType.URL_DISK_QCOW2
            elif i.format == 'vmdk':
                url_type = UrlType.URL_DISK_VMDK
            urls.append((i.url, url_type))
        urls.extend([(m.url, UrlType.URL_ISO) for m in osxml.medias if m.url])
        for t in osxml.trees:
            if not t.url:
                continue
            urls.append((t.url, UrlType.URL_GENERIC))
            url = t.url
            if not url.endswith('/'):
                url += '/'
            if t.kernel:
                urls.append((url + t.kernel, UrlType.URL_GENERIC))
            if t.initrd:
                urls.append((url + t.initrd, UrlType.URL_INITRD))
        if urls:
            ret.append((osxml.shortid, urls))

    return ret


@pytest.mark.parametrize('testdata', _collect_os_urls(),
        ids=lambda testdata: testdata[0])
def test_urls(testdata):
    urls = testdata[1]
    broken = []
    for (url, url_type) in urls:
        # As some distro URLs are flaky, let's give it a try 3 times
        # before actually failing.
        for i in range(3):
            ok = _check_url(url, url_type)
            if ok:
                break

        if not ok:
            broken.append(url)
    assert broken == []
