#!/usr/bin/env python3


from http.client import responses

import logging
import requests


class Os():
    def __init__(self, root):
        self._root = root

    def _get_images(self):
        images = []
        for image in self._root.findall('image'):
            images.append(Image(image))
        return images
    images = property(_get_images)

    def _get_medias(self):
        medias = []
        for media in self._root.findall('media'):
            medias.append(Media(media))
        return medias
    medias = property(_get_medias)

    def _get_trees(self):
        trees = []
        for tree in self._root.findall('tree'):
            trees.append(Tree(tree))
        return trees
    trees = property(_get_trees)

    def _get_shortid(self):
        shortid = self._root.find('short-id')
        return shortid.text
    shortid = property(_get_shortid)


class Image():
    def __init__(self, root):
        self._root = root

    def _get_url(self):
        url = self._root.find('url')
        if url is not None:
            return URL(url.text)
        return None
    url = property(_get_url)


class Media():
    def __init__(self, root):
        self._root = root

    def _get_url(self):
        url = self._root.find('url')
        if url is not None:
            return URL(url.text)
        return None
    url = property(_get_url)


class Tree():
    def __init__(self, root):
        self._root = root

    def _get_url(self):
        url = self._root.find('url')
        if url is not None:
            return URL(url.text)
        return None
    url = property(_get_url)


class URL():
    def __init__(self, url):
        self._url = url

    def check(self):
        logging.info("url: %s", self._url)
        response = requests.head(self._url, allow_redirects=True)
        logging.info("response: %s; code: %d",
                     responses[response.status_code], response.status_code)
        return response.ok
