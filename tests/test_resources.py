# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from collections import defaultdict

import logging

from . import util


def _test_resources_uniqueness_by_arch(osxml):
    """Ensure there's no more than one resource element per architecture"""
    result = defaultdict(list)
    for resources in osxml.resources_list:
        result[resources.get("arch")].append(resources)

    for value in result.values():
        assert len(value) == 1


@util.os_parametrize("osxml", filter_resources=True)
def test_resources(osxml):
    _test_resources_uniqueness_by_arch(osxml)

    # Ensure minimum resources are <= recommended resources
    _resources_helper(
        osxml,
        osxml.get_minimum_resources,
        "minimum",
        osxml.get_recommended_resources,
        "recommended",
    )

    # Ensure minimum resources are <= maximum resources
    _resources_helper(
        osxml,
        osxml.get_minimum_resources,
        "minimum",
        osxml.get_maximum_resources,
        "maximum",
    )

    # Ensure recommended resources are <= maximum resources
    _resources_helper(
        osxml,
        osxml.get_recommended_resources,
        "recommended",
        osxml.get_maximum_resources,
        "maximum",
    )

    # Ensure minimum resources <= network resources
    _resources_helper(
        osxml,
        osxml.get_minimum_resources,
        "minimum",
        osxml.get_network_install_resources,
        "network-install",
    )

    # Ensure recommended resources <= network resources
    _resources_helper(
        osxml,
        osxml.get_recommended_resources,
        "recommended",
        osxml.get_network_install_resources,
        "network-install",
    )

    # Ensure network resources <= maximum resources
    _resources_helper(
        osxml,
        osxml.get_network_install_resources,
        "network-install",
        osxml.get_maximum_resources,
        "maximum",
    )


def _resources_helper(osxml, smaller_func, smaller_str, bigger_func, bigger_str):
    broken = []
    for resource in osxml.resources_list:
        logging.info("resources | arch: %s", resource.get("arch"))
        smaller = smaller_func(resource)
        bigger = bigger_func(resource)

        if smaller is None or bigger is None:
            continue

        if not _resources_check(smaller, smaller_str, bigger, bigger_str):
            broken.append([smaller, bigger])
    assert broken == []


def _resources_check(smaller, smaller_str, bigger, bigger_str):
    ret = True
    if smaller.cpu is not None and bigger.cpu is not None:
        if smaller.cpu > bigger.cpu:
            logging.warning(
                "cpu value of %s should not be bigger than %s " "('%d' > '%d')",
                smaller_str,
                bigger_str,
                smaller.cpu,
                bigger.cpu,
            )
            ret = False
    if smaller.n_cpus is not None and bigger.n_cpus is not None:
        if smaller.n_cpus > bigger.n_cpus:
            logging.warning(
                "n-cpus value of %s should not be bigger than %s " "('%d' > '%d')",
                smaller_str,
                bigger_str,
                smaller.n_cpus,
                bigger.n_cpus,
            )
            ret = False
    if smaller.ram is not None and bigger.ram is not None:
        if smaller.ram > bigger.ram:
            logging.warning(
                "ram value of %s should not be bigger than %s " "('%d' > '%d')",
                smaller_str,
                bigger_str,
                smaller.ram,
                bigger.ram,
            )
            ret = False
    if smaller.storage is not None and bigger.storage is not None:
        if smaller.storage > bigger.storage:
            logging.warning(
                "storage value of %s should not be bigger than %s " "('%d' > '%d')",
                smaller_str,
                bigger_str,
                smaller.storage,
                bigger.storage,
            )
            ret = False
    return ret
