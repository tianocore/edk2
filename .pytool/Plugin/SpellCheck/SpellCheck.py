# @file SpellCheck.py
#
# An edk2-pytool based plugin wrapper for cspell
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import json
import yaml
from io import StringIO
import os
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toollib.utility_functions import RunCmd
from edk2toolext.environment.var_dict import VarDict
from edk2toollib.gitignore_parser import parse_gitignore_lines
from edk2toolext.environment import version_aggregator


class SpellCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that uses the cspell node module to scan the files
    from the package being tested for spelling errors.  The plugin contains
    the base cspell.json file then thru the configuration options other settings
    can be changed or extended.

    Configuration options:
    "SpellCheck": {
        "AuditOnly": False,          # Don't fail the build if there are errors.  Just log them
        "IgnoreFiles": [],           # use gitignore syntax to ignore errors in matching files
        "ExtendWords": [],           # words to extend to the dictionary for this package
        "IgnoreStandardPaths": [],   # Standard Plugin defined paths that should be ignore
        "AdditionalIncludePaths": [] # Additional paths to spell check (wildcards supported)
    }
    """

    #
    # A package can remove any of these using IgnoreStandardPaths
    #
    STANDARD_PLUGIN_DEFINED_PATHS = ("*.c", "*.h",
                                     "*.nasm", "*.asm", "*.masm", "*.s",
                                     "*.asl",
                                     "*.dsc", "*.dec", "*.fdf", "*.inf",
                                     "*.md", "*.txt"
                                     )

    def GetTestName(self, packagename: str, environment: VarDict) -> tuple:
        """ Provide the testcase name and classname for use in reporting

            Args:
              packagename: string containing name of package to build
              environment: The VarDict for the test to run in
            Returns:
                a tuple containing the testcase name and the classname
                (testcasename, classname)
                testclassname: a descriptive string for the testcase can include whitespace
                classname: should be patterned <packagename>.<plugin>.<optionally any unique condition>
        """
        return ("Spell check files in " + packagename, packagename + ".SpellCheck")

    ##
    # External function of plugin.  This function is used to perform the task of the CiBuild Plugin
    #
    #   - package is the edk2 path to package.  This means workspace/packagepath relative.
    #   - edk2path object configured with workspace and packages path
    #   - PkgConfig Object (dict) for the pkg
    #   - EnvConfig Object
    #   - Plugin Manager Instance
    #   - Plugin Helper Obj Instance
    #   - Junit Logger
    #   - output_stream the StringIO output stream from this plugin via logging

    def RunBuildPlugin(self, packagename, Edk2pathObj, pkgconfig, environment, PLM, PLMHelper, tc, output_stream=None):
        Errors = []

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            packagename)

        if abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError("No package {0}".format(packagename))
            return -1

        # check for node
        return_buffer = StringIO()
        ret = RunCmd("node", "--version", outstream=return_buffer)
        if (ret != 0):
            tc.SetSkipped()
            tc.LogStdError("NodeJs not installed. Test can't run")
            logging.warning("NodeJs not installed. Test can't run")
            return -1
        node_version = return_buffer.getvalue().strip()  # format vXX.XX.XX
        tc.LogStdOut(f"Node version: {node_version}")
        version_aggregator.GetVersionAggregator().ReportVersion(
            "NodeJs", node_version, version_aggregator.VersionTypes.INFO)

        # Check for cspell
        return_buffer = StringIO()
        ret = RunCmd("cspell", "--version", outstream=return_buffer)
        if (ret != 0):
            tc.SetSkipped()
            tc.LogStdError("cspell not installed.  Test can't run")
            logging.warning("cspell not installed.  Test can't run")
            return -1
        cspell_version = return_buffer.getvalue().strip()  # format XX.XX.XX
        tc.LogStdOut(f"CSpell version: {cspell_version}")
        version_aggregator.GetVersionAggregator().ReportVersion(
            "CSpell", cspell_version, version_aggregator.VersionTypes.INFO)

        # copy the default as a list
        package_relative_paths_to_spell_check = list(SpellCheck.STANDARD_PLUGIN_DEFINED_PATHS)

        #
        # Allow the ci.yaml to remove any of the above standard paths
        #
        if("IgnoreStandardPaths" in pkgconfig):
            for a in pkgconfig["IgnoreStandardPaths"]:
                if(a in package_relative_paths_to_spell_check):
                    tc.LogStdOut(
                        f"ignoring standard path due to ci.yaml ignore: {a}")
                    package_relative_paths_to_spell_check.remove(a)
                else:
                    tc.LogStdOut(f"Invalid IgnoreStandardPaths value: {a}")

        #
        # check for any additional include paths defined by package config
        #
        if("AdditionalIncludePaths" in pkgconfig):
            package_relative_paths_to_spell_check.extend(
                pkgconfig["AdditionalIncludePaths"])

        #
        # Make the path string for cspell to check
        #
        relpath = os.path.relpath(abs_pkg_path)
        cpsell_paths = " ".join(
            # Double quote each path to defer expansion to cspell parameters
            [f'"{relpath}/**/{x}"' for x in package_relative_paths_to_spell_check])

        # Make the config file
        config_file_path = os.path.join(
            Edk2pathObj.WorkspacePath, "Build", packagename, "cspell_actual_config.json")
        mydir = os.path.dirname(os.path.abspath(__file__))
        # load as yaml so it can have comments
        base = os.path.join(mydir, "cspell.base.yaml")
        with open(base, "r") as i:
            config = yaml.safe_load(i)

        if("ExtendWords" in pkgconfig):
            config["words"].extend(pkgconfig["ExtendWords"])
        with open(config_file_path, "w") as o:
            json.dump(config, o)  # output as json so compat with cspell

        All_Ignores = []
        # Parse the config for other ignores
        if "IgnoreFiles" in pkgconfig:
            All_Ignores.extend(pkgconfig["IgnoreFiles"])

        # spell check all the files
        ignore = parse_gitignore_lines(All_Ignores, os.path.join(
            abs_pkg_path, "nofile.txt"), abs_pkg_path)

        # result is a list of strings like this
        #  C:\src\sp-edk2\edk2\FmpDevicePkg\FmpDevicePkg.dec:53:9 - Unknown word (Capule)
        EasyFix = []
        results = self._check_spelling(cpsell_paths, config_file_path)
        for r in results:
            path, _, word = r.partition(" - Unknown word ")
            if len(word) == 0:
                # didn't find pattern
                continue

            pathinfo = path.rsplit(":", 2)  # remove the line no info
            if(ignore(pathinfo[0])):  # check against ignore list
                tc.LogStdOut(f"ignoring error due to ci.yaml ignore: {r}")
                continue

            # real error
            EasyFix.append(word.strip().strip("()"))
            Errors.append(r)

        # Log all errors tc StdError
        for l in Errors:
            tc.LogStdError(l.strip())

        # Helper - Log the syntax needed to add these words to dictionary
        if len(EasyFix) > 0:
            EasyFix = sorted(set(a.lower() for a in EasyFix))
            tc.LogStdOut("\n Easy fix:")
            OneString = "If these are not errors add this to your ci.yaml file.\n"
            OneString += '"SpellCheck": {\n  "ExtendWords": ['
            for a in EasyFix:
                tc.LogStdOut(f'\n"{a}",')
                OneString += f'\n    "{a}",'
            logging.info(OneString.rstrip(",") + '\n  ]\n}')

        # add result to test case
        overall_status = len(Errors)
        if overall_status != 0:
            if "AuditOnly" in pkgconfig and pkgconfig["AuditOnly"]:
                # set as skipped if AuditOnly
                tc.SetSkipped()
                return -1
            else:
                tc.SetFailed("SpellCheck {0} Failed.  Errors {1}".format(
                    packagename, overall_status), "CHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status

    def _check_spelling(self, abs_file_to_check: str, abs_config_file_to_use: str) -> []:
        output = StringIO()
        ret = RunCmd(
            "cspell", f"--config {abs_config_file_to_use} {abs_file_to_check}", outstream=output)
        if ret == 0:
            return []
        else:
            return output.getvalue().strip().splitlines()
