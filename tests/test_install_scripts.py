# This work is licensed under the GNU GPLv2 or later.
# See the COPYING file in the top-level directory.

import contextlib
import copy
import logging
import os
from pathlib import Path
import re

import lxml.etree as ET
import pytest

from . import util
from . import osinfo


# parameters that have a default in libosinfo, so they need to be set
# even if they are not marked as required
params_with_default = {
    "l10n-keyboard": "en_US",
    "l10n-language": "en_US",
    "l10n-timezone": "America/New_York",
}


# parameters that may be marked as required: in this case, use the following
# replacements for them
params_known_replacements = {
    "admin-password": "th3_l33t+pwd!",  # notsecret
    "hardware-arch": "x86_64",
    "hostname": "test-os",
    "reg-product-key": "12345-ABCDE-67890-FGHIJ-12345",  # notsecret
    "user-login": "guest",
    "user-password": "guest@_l33t+pwd!",  # notsecret
    "user-realname": "Guest User",
}


class AutoYaSTSchemas:
    """
    Loader for locally available AutoYaST schemas.

    This class tries to look for schemas for AutoYaST, either as available
    in the yast2-schema-collection package in openSUSE, or in the location
    pointed by the $INTERNAL_OSINFO_DB_AUTOYAST_SCHEMAS environment variable
    (if set).

    The result of each lookup is cached (found or not).
    """

    def __init__(self):
        self.schemas = {}
        self.path = None

        candidate_paths = [
            # as available in yast2-schema-collection
            "/usr/share/YaST2/schema/autoyast/products/",
        ]
        # try checking using an environment variable: if this is set,
        # it is always checked first (making it possible to override
        # a system installation)
        env_path = os.environ.get("INTERNAL_OSINFO_DB_AUTOYAST_SCHEMAS", None)
        if env_path:
            candidate_paths.insert(0, env_path)
        base_path = next((p for p in candidate_paths if os.path.isdir(p)), None)
        if base_path:
            self.path = Path(base_path)

    def _get_schema_path(self, os_distro, os_version, arch):
        if os_distro == "opensuse":
            # unknown version: no way to tell which schema it follows
            if os_version == "unknown":
                return None
            # Factory is the development version / source of Tumbleweed,
            # so use the schemas of Tumbleweed for both
            if os_version in ["factory", "tumbleweed"]:
                distro_string = "tw"
            else:
                distro_string = "leap" + os_version
        elif os_distro in ["sle", "sles", "sled"]:
            m = re.fullmatch(r"(\d+)(?:\.(\d+))?", os_version)
            if m:
                major = int(m.group(1))
                if major < 12:
                    distro_string = "sles-%d" % major
                else:
                    distro_string = "sle-%d" % major
                if m.group(2):
                    distro_string += "-sp" + m.group(2)
        if not distro_string:
            logging.warning("autoyast: unknown distro: %s/%s", os_distro, os_version)
            return None

        distro_path = self.path / distro_string
        if not distro_path.is_dir():
            logging.warning("autoyast: OS directory not found: %s", distro_path)
            return None

        arch_path = distro_path / arch
        if not arch_path.is_dir():
            logging.warning("autoyast: arch directory not found: %s", arch_path)
            return None

        # profile.rng is the "entry point" of the schema
        schema_path = arch_path / "profile.rng"
        if not schema_path.is_file():
            logging.warning("autoyast: profile.rng not found in %s", arch_path)
            return None

        return schema_path

    def load_schema(self, osxml, arch):
        if self.path is None:
            return None

        # each schema may be different depending on the distribution,
        # the distribution version, and the architecture; hence, cache the
        # schemas based on all of these factors
        cache_key = (osxml.distro, osxml.version, arch)

        try:
            rng = self.schemas[cache_key]
        except KeyError:
            schema_path = self._get_schema_path(*cache_key)
            if schema_path:
                rng = ET.RelaxNG(ET.parse(schema_path.open("r")))
            else:
                rng = None
            self.schemas[cache_key] = rng
        return rng


@util.os_parametrize("osxml", filter_installscripts=True)
def test_install_scripts(osxml):
    for script in osxml.installscripts:
        assert util.DataFiles.installscripts()[script]


@util.os_parametrize("osxml", filter_media=True)
def test_media_install_scripts(osxml):
    for media in osxml.medias:
        for script in media.installscripts:
            assert util.DataFiles.installscripts()[script]


def _generate_initial_script_config_xml(
    osxml, script, install_source, media=None, tree=None
):
    # create the XML document with the config
    doc = ET.Element("install-script-config")

    def import_element(doc, xmlobj, tag):
        new_element = xmlobj.get_cloned_element()
        if new_element.tag != tag:
            new_element.tag = tag
        doc.append(new_element)
        return new_element

    # <os>
    import_element(doc, osxml, "os")
    # <script>
    script_doc = import_element(doc, script, "script")
    source_subtag = ET.SubElement(script_doc, "installation-source")
    if media:
        source_subtag.text = "media"
    elif tree:
        source_subtag.text = "network"
    # <media>
    if media:
        import_element(doc, media, "media")
    # <tree>
    if tree:
        import_element(doc, tree, "tree")

    # <config> element with the parameters
    config = {}
    for param in script.params:
        n = param.name
        v = None
        if n in params_with_default:
            v = params_with_default[n]
        elif param.policy == "required":
            v = params_known_replacements[n]
        else:
            continue
        value_map = param.value_map
        if value_map:
            with contextlib.suppress(KeyError):
                v = util.DataFiles.datamaps()[value_map][v]
            assert v
        config[n] = v
    if media:
        # extra elements for a media
        config["hardware-arch"] = media.arch
    if tree:
        # extra elements for a tree
        config["hardware-arch"] = tree.arch
    config_doc = ET.SubElement(doc, "config")
    for subtagname, subtagval in config.items():
        if subtagval is None:
            continue
        prop_subtag = ET.SubElement(config_doc, subtagname)
        prop_subtag.text = subtagval

    return doc


def _generate_testdata_for_methods(script, params_doc):
    ret = []

    for method in script.injection_methods:
        method_doc = copy.deepcopy(params_doc)
        injection_subtag = ET.SubElement(
            method_doc.find("script"), "preferred-injection-method"
        )
        injection_subtag.text = method
        ret.append((script, method_doc))
        # injection methods that support the "/command-line" template
        if method in ["disk", "initrd"]:
            cmdline_doc = copy.deepcopy(method_doc)
            cmdline_doc.tag = "command-line"
            ret.append((script, cmdline_doc))

    return ret


def _collect_for_script_and_media(osxml, script, media):
    params_doc = _generate_initial_script_config_xml(
        osxml, script, "media", media=media
    )

    return _generate_testdata_for_methods(script, params_doc)


def _collect_for_script_and_tree(osxml, script, tree):
    params_doc = _generate_initial_script_config_xml(
        osxml, script, "network", tree=tree
    )

    return _generate_testdata_for_methods(script, params_doc)


def _collect_scripts():
    ret = []

    for osxml in util.DataFiles.oses():
        combinations = []
        scripts = osxml.installscripts
        for media in osxml.medias:
            if not media.installer_script:
                continue
            if media.installscripts:
                media_scripts = media.installscripts
            else:
                media_scripts = scripts
            if not media_scripts:
                continue
            for script_id in media_scripts:
                script = util.DataFiles.installscripts()[script_id]
                items = [script, media]
                combinations.append(items)
        if scripts:
            for tree in osxml.trees:
                if not tree.url:
                    continue
                for script_id in scripts:
                    script = util.DataFiles.installscripts()[script_id]
                    items = [script, tree]
                    combinations.append(items)

        if combinations:
            ret.append(pytest.param(osxml, combinations, id=osxml.shortid))

    return ret


def _check_autoyast(osxml, autoyast_schemas, params_doc, xml):
    arch = params_doc.find("config/hardware-arch")
    assert arch is not None

    rng = autoyast_schemas.load_schema(osxml, arch.text)
    if rng is None:
        return

    rng.assertValid(xml)


@pytest.fixture(scope="module")
def autoyast_schemas():
    autoyast_schemas = AutoYaSTSchemas()
    return autoyast_schemas


@pytest.mark.parametrize("osxml,testdata", _collect_scripts())
def test_generate_scripts(osxml, testdata, autoyast_schemas):
    # We didn't call _collect_for_script_and... from _collect_scripts
    # as that creates massive memory usage for the whole duration of
    # the test suite
    fulltestdata = []
    for script, obj in testdata:
        if isinstance(obj, osinfo.Tree):
            items = _collect_for_script_and_tree(osxml, script, obj)
        else:
            items = _collect_for_script_and_media(osxml, script, obj)
        fulltestdata.extend(items)

    for script, params_doc in fulltestdata:
        template = script.template
        transform = ET.XSLT(template)
        transform_result = transform(params_doc)
        assert transform_result
        result = bytes(transform_result)
        assert result

        output_is_xml = (
            template.find("xsl:output", template.nsmap).get("method", "xml") == "xml"
        )

        if params_doc.tag == "install-script-config":
            # This is the actual install script
            if output_is_xml:
                # It is an XML document, check it can be parsed as such
                xml = ET.XML(result)
                if script.expected_filename == "autoinst.xml":
                    # Looks like it may be an AutoYaST file
                    _check_autoyast(osxml, autoyast_schemas, params_doc, xml)
        elif params_doc.tag == "command-line":
            # This is the kernel command line to use;
            # tolerate only one newline at the end (as it may happen when
            # the <xsl:output> is "xml")
            if output_is_xml and result.endswith(b"\n"):
                result = result[:-1]
            assert b"\n" not in result
            assert len(result) > 0
