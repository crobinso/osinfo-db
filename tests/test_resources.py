# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

from collections import defaultdict

import logging

from . import util


@util.os_parametrize('_os', filter_resources=True)
def test_resources_uniqueness_by_arch(_os):
    """ Ensure there's no more than one resource element per architecture """
    result = defaultdict(list)
    for resources in _os.resources_list:
        result[resources.get('arch')].append(resources)

    for value in result.values():
        assert len(value) == 1


@util.os_parametrize('_os', filter_resources=True)
def test_minimum_recommended_resources(_os):
    """ Ensure there's no minimum resources with bigger values than recommended
        resources
    """
    _resources_helper(_os,
                      _os.get_minimum_resources,
                      'minimum',
                      _os.get_recommended_resources,
                      'recommended')


@util.os_parametrize('_os', filter_resources=True)
def test_recommended_maximum_resources(_os):
    """ Ensure there's no recommended resources with bigger values than maximum
        resources
    """
    _resources_helper(_os,
                      _os.get_recommended_resources,
                      'recommended',
                      _os.get_maximum_resources,
                      'maximum')


@util.os_parametrize('_os', filter_resources=True)
def test_recommended_network_install_resources(_os):
    """ Ensure there's no recommended resources with bigger values than
        network-install resources
    """
    _resources_helper(_os,
                      _os.get_recommended_resources,
                      'recommended',
                      _os.get_network_install_resources,
                      'network-install')


@util.os_parametrize('_os', filter_resources=True)
def test_network_install_maximum_resources(_os):
    """ Ensure there's no network-install resources with bigger values than
        maximum resources
    """
    _resources_helper(_os,
                      _os.get_network_install_resources,
                      'network-install',
                      _os.get_maximum_resources,
                      'maximum')


def _resources_helper(_os, smaller_func, smaller_str, bigger_func, bigger_str):
    broken = []
    for resource in _os.resources_list:
        logging.info("resources | arch: %s", resource.get('arch'))
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
            logging.warning("cpu value of %s should not be bigger than %s "
                            "('%d' > '%d')",
                            smaller_str, bigger_str, smaller.cpu, bigger.cpu)
            ret = False
    if smaller.n_cpus is not None and bigger.n_cpus is not None:
        if smaller.n_cpus > bigger.n_cpus:
            logging.warning("n-cpus value of %s should not be bigger than %s "
                            "('%d' > '%d')",
                            smaller_str, bigger_str,
                            smaller.n_cpus, bigger.n_cpus)
            ret = False
    if smaller.ram is not None and bigger.ram is not None:
        if smaller.ram > bigger.ram:
            logging.warning("ram value of %s should not be bigger than %s "
                            "('%d' > '%d')",
                            smaller_str, bigger_str, smaller.ram, bigger.ram)
            ret = False
    if smaller.storage is not None and bigger.storage is not None:
        if smaller.storage > bigger.storage:
            logging.warning("storage value of %s should not be bigger than %s "
                            "('%d' > '%d')",
                            smaller_str, bigger_str,
                            smaller.storage, bigger.storage)
            ret = False
    return ret
