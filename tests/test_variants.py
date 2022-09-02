# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from . import util


@util.os_parametrize("osxml", filter_media=True)
def test_existing_variant_for_media(osxml):
    variants = osxml.variants
    for media in osxml.medias:
        variant = media.variant
        if variant and variant not in variants:
            raise AssertionError(
                "variants: media in OS %s has variant=%s "
                "but declared variants are %s" % (osxml.shortid, variant, variants)
            )


@util.os_parametrize("osxml", filter_trees=True)
def test_existing_variant_for_trees(osxml):
    variants = osxml.variants
    for tree in osxml.trees:
        variant = tree.variant
        if variant and variant not in variants:
            raise AssertionError(
                "variants: tree in OS %s has variant=%s "
                "but declared variants are %s" % (osxml.shortid, variant, variants)
            )


@util.os_parametrize("osxml", filter_images=True)
def test_existing_variant_for_images(osxml):
    variants = osxml.variants
    for image in osxml.images:
        variant = image.variant
        if variant and variant not in variants:
            raise AssertionError(
                "variants: image in OS %s has variant=%s "
                "but declared variants are %s" % (osxml.shortid, variant, variants)
            )
