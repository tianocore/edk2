# @file HostBasedUnitTestRunner.py
# Plugin to located any host-based unit tests in the output directory and execute them.
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
import io
import re
import os
import logging
import glob
import stat
import xml.etree.ElementTree
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toolext import edk2_logging
import edk2toollib.windows.locate_tools as locate_tools
from edk2toolext.environment import shell_environment
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import GetHostInfo
from textwrap import dedent


class HostBasedUnitTestRunner(IUefiBuildPlugin):

    def do_pre_build(self, thebuilder):
        '''
        Run Prebuild
        '''

        return 0

    def do_post_build(self, thebuilder):
        '''
        After a build, will automatically locate and run all host-based unit tests. Logs any
        failures with Warning severity and will return a count of the failures as the return code.

        EXPECTS:
        - Build Var 'CI_BUILD_TYPE' - If not set to 'host_unit_test', will not do anything.

        UPDATES:
        - Shell Var 'CMOCKA_XML_FILE'
        '''
        ci_type = thebuilder.env.GetValue('CI_BUILD_TYPE')
        if ci_type != 'host_unit_test':
            return 0

        shell_env = shell_environment.GetEnvironment()
        logging.log(edk2_logging.get_section_level(),
                    "Run Host based Unit Tests")
        path = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")

        failure_count = 0

        # Do not catch exceptions in gtest so they are handled by address sanitizer
        shell_env.set_shell_var('GTEST_CATCH_EXCEPTIONS', '0')

        # Disable address sanitizer memory leak detection
        shell_env.set_shell_var('ASAN_OPTIONS', 'detect_leaks=0')

        # Set up the reporting type for Cmocka.
        shell_env.set_shell_var('CMOCKA_MESSAGE_OUTPUT', 'xml')

        for arch in thebuilder.env.GetValue("TARGET_ARCH").split():
            logging.log(edk2_logging.get_subsection_level(),
                        "Testing for architecture: " + arch)
            cp = os.path.join(path, arch)

            # If any old results XML files exist, clean them up.
            for old_result in glob.iglob(os.path.join(cp, "*.result.xml")):
                os.remove(old_result)

            # Find and Run any Host Tests
            if GetHostInfo().os.upper() == "LINUX":
                testList = glob.glob(os.path.join(cp, "*Test*"))
                for a in testList[:]:
                    p = os.path.join(cp, a)
                    # It must be a file
                    if not os.path.isfile(p):
                        testList.remove(a)
                        logging.debug(f"Remove directory file: {p}")
                        continue
                    # It must be executable
                    if os.stat(p).st_mode & (stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH) == 0:
                        testList.remove(a)
                        logging.debug(f"Remove non-executable file: {p}")
                        continue

                    logging.info(f"Test file found: {p}")

            elif GetHostInfo().os.upper() == "WINDOWS":
                testList = glob.glob(os.path.join(cp, "*Test*.exe"))
            else:
                raise NotImplementedError("Unsupported Operating System")

            if not testList:
                logging.warning(dedent("""
                    UnitTest Coverage:
                      No unit tests discovered. Test coverage will not be generated.

                      Prevent this message by:
                      1. Adding host-based unit tests to this package
                      2. Ensuring tests have the word "Test" in their name
                      3. Disabling HostUnitTestCompilerPlugin in the package CI YAML file
                    """).strip())
                return 0

            for test in testList:
                #
                # If environment variable "CODE_COVERAGE" is None,
                # then initialize this variable to "FALSE".
                #
                # The "CODE_COVERAGE" variable need to be initialized in the
                # Windows command prompt or Linux shell.
                #
                # If the variable not be initialized then getvariable return None
                # directly, it made code flow not rubust.
                #
                if (thebuilder.env.GetValue("CODE_COVERAGE") == None):
                    thebuilder.env.SetValue("CODE_COVERAGE", "FALSE", "default settings")
                #
                # The unit tests are executed in 2 places in Windows.
                # One is in this loop.
                # The other is in the method gen_code_coverage_msvc() that runs
                # each unit test through OpenCppCoverage.
                #
                # The goal of this change is to make sure the call to
                # gen_code_coverage_msvc() is the only time the unit test is
                # executed when code coverage information is being collected.
                #
                # If code coverage information is not being collected then
                # the call to gen_code_coverage_msvc() will not be made and
                # the unit tests must be executed in this loop.
                #
                # Also, if the unit tests are being executed in a Linux env,
                # then gen_code_coverage_msvc() will never be called and
                # OpenCppCoverage will never be called.
                #
                # Created below small logic table to make it clear.
                # ______________________________________________________________
                # | CODE_COVERAGE | TOOL_CHAIN_TAG | Where Unit Tests Run      |
                # |____________________________________________________________|
                # | FALSE         | GCC5           | do_post_build() loop      |
                # | FALSE         | VSxxxx         | do_post_build() loop      |
                # | TRUE          | GCC5           | do_post_build() loop      |
                # | TRUE          | VSxxxx         | gen_code_coverage_msvc()  |
                # |_______________|________________|___________________________|
                #
                if (thebuilder.env.GetValue("CODE_COVERAGE")) and \
                    (not thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith("GCC")):
                    continue

                # Configure output name if test uses cmocka.
                shell_env.set_shell_var(
                    'CMOCKA_XML_FILE', test + ".CMOCKA.%g." + arch + ".result.xml")
                # Configure output name if test uses gtest.
                shell_env.set_shell_var(
                    'GTEST_OUTPUT', "xml:" + test + ".GTEST." + arch + ".result.xml")

                ret = RunCmd('"' + test + '"', "", workingdir=cp)
                if ret != 0:
                    logging.error("UnitTest Execution Error: " +
                                  os.path.basename(test))
                    failure_count += 1
                else:
                    logging.info("UnitTest Completed: " +
                                 os.path.basename(test))
                    file_match_pattern = test + ".*." + arch + ".result.xml"
                    xml_results_list = glob.glob(file_match_pattern)
                    for xml_result_file in xml_results_list:
                        root = xml.etree.ElementTree.parse(
                            xml_result_file).getroot()
                        for suite in root:
                            for case in suite:
                                for result in case:
                                    if result.tag == 'failure':
                                        logging.warning(
                                            "%s Test Failed" % os.path.basename(test))
                                        logging.warning(
                                            "  %s - %s" % (case.attrib['name'], result.text))
                                        failure_count += 1

            if thebuilder.env.GetValue("CODE_COVERAGE") != "FALSE":
                if thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith("GCC"):
                    ret = self.gen_code_coverage_gcc(thebuilder)
                    if ret != 0:
                        failure_count += 1
                elif thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith ("VS"):
                    ret = self.gen_code_coverage_msvc(thebuilder)
                    if ret != 0:
                        failure_count += 1
                else:
                    logging.info("Skipping code coverage. Currently, support GCC and MSVC compiler.")

        return failure_count

    def get_lcov_version(self):
        """Get lcov version number"""
        lcov_ver = io.StringIO()
        ret = RunCmd("lcov", "--version", outstream=lcov_ver)
        if ret != 0:
            return None
        (major, _minor) = re.search(r"version (\d+)\.(\d+)", lcov_ver.getvalue()).groups()
        return int(major)


    def gen_code_coverage_gcc(self, thebuilder):
        logging.info("Generating UnitTest code coverage")

        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        workspace = thebuilder.env.GetValue("WORKSPACE")

        lcov_version_major = self.get_lcov_version()
        if not lcov_version_major:
            logging.error("UnitTest Coverage: Failed to determine lcov version")
            return 1
        logging.info(f"Got lcov version {lcov_version_major}")

        # Generate base code coverage for all source files
        # `--ignore-errors mismatch` needed to make lcov v2.0+/gcov work.
        lcov_error_settings = "--ignore-errors mismatch" if lcov_version_major >= 2 else ""
        ret = RunCmd("lcov", f"--no-external --capture --initial --directory {buildOutputBase} --output-file {buildOutputBase}/cov-base.info --rc lcov_branch_coverage=1 {lcov_error_settings}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to build initial coverage data.")
            return 1

        # Coverage data for tested files only
        ret = RunCmd("lcov", f"--capture --directory {buildOutputBase}/ --output-file {buildOutputBase}/coverage-test.info --rc lcov_branch_coverage=1 {lcov_error_settings}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to build coverage data for tested files.")
            return 1

        # Aggregate all coverage data
        ret = RunCmd("lcov", f"--add-tracefile {buildOutputBase}/cov-base.info --add-tracefile {buildOutputBase}/coverage-test.info --output-file {buildOutputBase}/total-coverage.info --rc lcov_branch_coverage=1 {lcov_error_settings}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to aggregate coverage data.")
            return 1

        # Generate coverage XML
        ret = RunCmd("lcov_cobertura",f"{buildOutputBase}/total-coverage.info -o {buildOutputBase}/compare.xml")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate coverage XML.")
            return 1

        # Filter out auto-generated and test code
        ret = RunCmd("lcov_cobertura",f"{buildOutputBase}/total-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {buildOutputBase}/coverage.xml")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed generate filtered coverage XML.")
            return 1

        # Generate all coverage file
        testCoverageList = glob.glob (f"{workspace}/Build/**/total-coverage.info", recursive=True)

        coverageFile = ""
        for testCoverage in testCoverageList:
            coverageFile += " --add-tracefile " + testCoverage
        ret = RunCmd("lcov", f"{coverageFile} --output-file {workspace}/Build/all-coverage.info --rc lcov_branch_coverage=1 {lcov_error_settings}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed generate all coverage file.")
            return 1

        # Generate and XML file if requested.for all package
        if os.path.isfile(f"{workspace}/Build/coverage.xml"):
            os.remove(f"{workspace}/Build/coverage.xml")
        ret = RunCmd("lcov_cobertura",f"{workspace}/Build/all-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {workspace}/Build/coverage.xml")

        return 0


    def gen_code_coverage_msvc(self, thebuilder):
        logging.info("Generating UnitTest code coverage")


        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        testList = glob.glob(os.path.join(buildOutputBase, "**","*Test*.exe"), recursive=True)
        workspace = thebuilder.env.GetValue("WORKSPACE")
        workspace = (workspace + os.sep) if workspace[-1] != os.sep else workspace
        workspaceBuild = os.path.join(workspace, 'Build')
        # Generate coverage file
        coverageFile = ""
        for testFile in testList:
            ret = RunCmd("OpenCppCoverage", f"--source {workspace} --export_type binary:{testFile}.cov -- {testFile}")
            if ret != 0:
                logging.error("UnitTest Coverage: Failed to collect coverage data.")
                return 1

            coverageFile  = f" --input_coverage={testFile}.cov"
            totalCoverageFile = os.path.join(buildOutputBase, 'coverage.cov')
            if os.path.isfile(totalCoverageFile):
                coverageFile += f" --input_coverage={totalCoverageFile}"
            ret = RunCmd(
                "OpenCppCoverage",
                f"--export_type binary:{totalCoverageFile} " +
                f"--working_dir={workspaceBuild} " +
                f"{coverageFile}"
                )
            if ret != 0:
                logging.error("UnitTest Coverage: Failed to collect coverage data.")
                return 1

        # Generate and XML file if requested.by each package
        ret = RunCmd(
            "OpenCppCoverage",
            f"--export_type cobertura:{os.path.join(buildOutputBase, 'coverage.xml')} " +
            f"--working_dir={workspaceBuild} " +
            f"--input_coverage={totalCoverageFile} "
            )
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate cobertura format xml in single package.")
            return 1

        # Generate total report XML file for all package
        testCoverageList = glob.glob(os.path.join(workspace, "Build", "**", "*Test*.exe.cov"), recursive=True)
        coverageFile = ""
        totalCoverageFile = os.path.join(workspaceBuild, 'coverage.cov')
        for testCoverage in testCoverageList:
            coverageFile  = f" --input_coverage={testCoverage}"
            if os.path.isfile(totalCoverageFile):
                coverageFile += f" --input_coverage={totalCoverageFile}"
            ret = RunCmd(
                "OpenCppCoverage",
                f"--export_type binary:{totalCoverageFile} " +
                f"--working_dir={workspaceBuild} " +
                f"{coverageFile}"
                )
            if ret != 0:
                logging.error("UnitTest Coverage: Failed to collect coverage data.")
                return 1

        ret = RunCmd(
            "OpenCppCoverage",
            f"--export_type cobertura:{os.path.join(workspaceBuild, 'coverage.xml')} " +
            f"--working_dir={workspaceBuild} " +
            f"--input_coverage={totalCoverageFile}"
            )
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate cobertura format xml.")
            return 1

        return 0
