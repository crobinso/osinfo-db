# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.os_parametrize("osxml", filter_media=True)
def test_existing_variants_for_media(osxml):
    os_variants = osxml.variants
    for media in osxml.medias:
        media_variants = media.variants
        for variant in media_variants:
            if variant not in os_variants:
                raise AssertionError(
                    "variants: media in OS %s has variant=%s "
                    "but declared variants are %s"
                    % (osxml.shortid, variant, os_variants)
                )


@util.os_parametrize("osxml", filter_trees=True)
def test_existing_variants_for_trees(osxml):
    os_variants = osxml.variants
    for tree in osxml.trees:
        tree_variants = tree.variants
        for variant in tree_variants:
            if variant not in os_variants:
                raise AssertionError(
                    "variants: tree in OS %s has variant=%s "
                    "but declared variants are %s"
                    % (osxml.shortid, variant, os_variants)
                )


@util.os_parametrize("osxml", filter_images=True)
def test_existing_variants_for_images(osxml):
    os_variants = osxml.variants
    for image in osxml.images:
        image_variants = image.variants
        for variant in image_variants:
            if variant not in os_variants:
                raise AssertionError(
                    "variants: media in OS %s has variant=%s "
                    "but declared variants are %s"
                    % (osxml.shortid, variant, os_variants)
                )
