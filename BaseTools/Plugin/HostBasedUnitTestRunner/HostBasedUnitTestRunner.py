# @file HostBasedUnitTestRunner.py
# Plugin to located any host-based unit tests in the output directory and execute them.
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
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

            for test in testList:
                # Configure output name if test uses cmocka.
                shell_env.set_shell_var(
                    'CMOCKA_XML_FILE', test + ".CMOCKA.%g." + arch + ".result.xml")
                # Configure output name if test uses gtest.
                shell_env.set_shell_var(
                    'GTEST_OUTPUT', "xml:" + test + ".GTEST." + arch + ".result.xml")

                # Run the test.
                ret = RunCmd('"' + test + '"', "", workingdir=cp)
                if ret != 0:
                    logging.error("UnitTest Execution Error: " +
                                  os.path.basename(test))
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
                if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "GCC5":
                    self.gen_code_coverage_gcc(thebuilder)
                elif thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith ("VS"):
                    self.gen_code_coverage_msvc(thebuilder)
                else:
                    logging.info("Skipping code coverage. Currently, support GCC and MSVC compiler.")

        return failure_count

    def gen_code_coverage_gcc(self, thebuilder):
        logging.info("Generating UnitTest code coverage")

        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        workspace = thebuilder.env.GetValue("WORKSPACE")

        # Generate base code coverage for all source files
        ret = RunCmd("lcov", f"--no-external --capture --initial --directory {buildOutputBase} --output-file {buildOutputBase}/cov-base.info --rc lcov_branch_coverage=1")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to build initial coverage data.")
            return 1

        # Coverage data for tested files only
        ret = RunCmd("lcov", f"--capture --directory {buildOutputBase}/ --output-file {buildOutputBase}/coverage-test.info --rc lcov_branch_coverage=1")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to build coverage data for tested files.")
            return 1

        # Aggregate all coverage data
        ret = RunCmd("lcov", f"--add-tracefile {buildOutputBase}/cov-base.info --add-tracefile {buildOutputBase}/coverage-test.info --output-file {buildOutputBase}/total-coverage.info --rc lcov_branch_coverage=1")
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
        ret = RunCmd("lcov", f"{coverageFile} --output-file {workspace}/Build/all-coverage.info --rc lcov_branch_coverage=1")
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
        # Generate coverage file
        coverageFile = ""
        for testFile in testList:
            ret = RunCmd("OpenCppCoverage", f"--source {workspace} --export_type binary:{testFile}.cov -- {testFile}")
            coverageFile += " --input_coverage=" + testFile + ".cov"
            if ret != 0:
                logging.error("UnitTest Coverage: Failed to collect coverage data.")
                return 1

        # Generate and XML file if requested.by each package
        ret = RunCmd("OpenCppCoverage", f"--export_type cobertura:{os.path.join(buildOutputBase, 'coverage.xml')} --working_dir={workspace}Build {coverageFile}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate cobertura format xml in single package.")
            return 1

        # Generate total report XML file for all package
        testCoverageList = glob.glob(os.path.join(workspace, "Build", "**","*Test*.exe.cov"), recursive=True)
        coverageFile = ""
        for testCoverage in testCoverageList:
            coverageFile += " --input_coverage=" + testCoverage

        ret = RunCmd("OpenCppCoverage", f"--export_type cobertura:{workspace}Build/coverage.xml --working_dir={workspace}Build {coverageFile}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to generate cobertura format xml.")
            return 1

        return 0
