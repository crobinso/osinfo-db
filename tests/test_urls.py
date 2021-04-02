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


def _check_url(url, url_type):
    logging.info("url: %s, type: %s", url, url_type)
    headers = {'user-agent': 'Wget/1.0'}
    response = requests.head(url, allow_redirects=True, headers=headers)
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
    if url_type == UrlType.URL_ISO:
        return content_type in iso_content_types
    return True


def _collect_os_urls():
    """
    Iterate the OS list and return a list of pairs (shortid, [url list)
    """
    ret = []

    for osxml in util.DataFiles.oses():
        urls = []
        urls.extend([(i.url, UrlType.URL_GENERIC) for i in osxml.images if i.url])
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
                urls.append((url + t.initrd, UrlType.URL_GENERIC))
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
