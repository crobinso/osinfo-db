# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import http
import logging

import pytest
import requests

from . import util


def _check_url(url):
    logging.info("url: %s", url)
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
    return response.ok


def _collect_os_urls():
    """
    Iterate the OS list and return a list of pairs (shortid, [url list)
    """
    ret = []

    for osxml in util.DataFiles.oses():
        urls = []
        urls.extend([i.url for i in osxml.images if i.url])
        urls.extend([m.url for m in osxml.medias if m.url])
        for t in osxml.trees:
            if not t.url:
                continue
            urls.append(t.url)
            url = t.url
            if not url.endswith('/'):
                url += '/'
            if t.kernel:
                urls.append(url + t.kernel)
            if t.initrd:
                urls.append(url + t.initrd)
        if urls:
            ret.append((osxml.shortid, urls))

    return ret


@pytest.mark.parametrize('testdata', _collect_os_urls(),
        ids=lambda testdata: testdata[0])
def test_urls(testdata):
    urls = testdata[1]
    broken = []
    for url in urls:
        if not _check_url(url):
            broken.append(url)
    assert broken == []
