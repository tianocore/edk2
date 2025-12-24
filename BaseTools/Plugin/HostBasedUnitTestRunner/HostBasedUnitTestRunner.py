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

        # Configure LLVM code coverage collection to generate unique file for
        # each host based unit test executable.
        shell_env.set_shell_var('LLVM_PROFILE_FILE', '%m.profraw')

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
                elif thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith("CLANG"):
                    ret = self.gen_code_coverage_clang(thebuilder)
                    if ret != 0:
                        failure_count += 1
                elif thebuilder.env.GetValue("TOOL_CHAIN_TAG").startswith ("VS"):
                    ret = self.gen_code_coverage_msvc(thebuilder)
                    if ret != 0:
                        failure_count += 1
                else:
                    logging.info("Skipping code coverage. Currently, support GCC, CLANG, and MSVC compiler.")

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

        # Generate and XML file if requested for all package
        if os.path.isfile(f"{workspace}/Build/coverage.xml"):
            os.remove(f"{workspace}/Build/coverage.xml")
        ret = RunCmd("lcov_cobertura",f"{workspace}/Build/all-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {workspace}/Build/coverage.xml")

        return 0


    def gen_code_coverage_clang(self, thebuilder):
        logging.info("Generating UnitTest code coverage")

        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        workspace = thebuilder.env.GetValue("WORKSPACE")

        if GetHostInfo().os.upper() == "LINUX":
            # Collect test executables with no file extension
            testList = glob.glob(os.path.join(buildOutputBase, "**", "*Test*"), recursive=True)
            testList = [f for f in testList if os.path.isfile(f) and os.path.splitext(f)[1] == ""]
            allTestList = glob.glob(os.path.join(workspace, "Build", "**", "*Test*"), recursive=True)
            allTestList = [f for f in allTestList if os.path.isfile(f) and os.path.splitext(f)[1] == ""]
        elif GetHostInfo().os.upper() == "WINDOWS":
            # Collect test executables with a .exe file extension
            testList = glob.glob(os.path.join(buildOutputBase, "**","*Test*.exe"), recursive=True)
            allTestList = glob.glob(os.path.join(workspace, "Build", "**","*Test*.exe"), recursive=True)
        else:
            raise NotImplementedError("Unsupported Operating System")
        if not testList:
            logging.warning("UnitTest Coverage: No test binaries found.")
            return 0

        profrawlistFile = os.path.join(buildOutputBase, 'profrawlist.txt')
        mergedProfData = os.path.join(buildOutputBase, 'merged.profdata')
        mergedCoverageLcov = os.path.join(buildOutputBase, 'coverage.lcov')
        mergedCoverageXml = os.path.join(buildOutputBase, 'coverage.xml')

        # Generate LCOV and XML file for each unit test executable
        if self.clang_gen_profdata(buildOutputBase, profrawlistFile, mergedProfData) != 0:
            return 1
        for testFile in testList:
            if self.clang_gen_lcov_xml(mergedProfData, [testFile], f"{testFile}.coverage.lcov", f"{testFile}.coverage.xml") != 0:
                return 1

        # Generate LCOV and XML for the package
        if self.clang_gen_lcov_xml(mergedProfData, testList, mergedCoverageLcov, mergedCoverageXml) != 0:
            return 1

        allProfrawlistFile = os.path.join(workspace, 'Build', 'profrawlist.txt')
        allMergedProfData = os.path.join(workspace, 'Build', 'merged.profdata')
        allCoverageLcov = os.path.join(workspace, 'Build', 'coverage.lcov')
        allCoverageXml = os.path.join(workspace, 'Build', 'coverage.xml')

        # Generate LCOV and XML for all packages
        if self.clang_gen_profdata(workspace, allProfrawlistFile, allMergedProfData) != 0:
            return 1
        if self.clang_gen_lcov_xml(allMergedProfData, allTestList, allCoverageLcov, allCoverageXml) != 0:
            return 1

        return 0

    def clang_gen_profdata(self, basepath, output_profrawlistfile, output_profdata):
        """
        Merge multiple LLVM profraw files into a single profdata file.
        """
        if not os.path.isdir(basepath):
            logging.error(f"clang_gen_profdata: Base path {basepath} is not a directory.")
            return 1
        if not output_profrawlistfile:
            logging.error("clang_gen_profdata: No profraw list file provided.")
            return 1
        if not output_profdata:
            logging.error("clang_gen_profdata: No output profdata file provided.")
            return 1

        for file in [output_profrawlistfile, output_profdata]:
            if os.path.isfile(file):
                os.remove(file)

        with open(output_profrawlistfile, "w") as f:
            f.write("\n".join(glob.glob(os.path.join(basepath, "**", "*.profraw"), recursive=True)))
        ret = RunCmd("llvm-profdata", f"merge -sparse --input-files {output_profrawlistfile} --output {output_profdata}")
        if ret != 0:
            logging.error(f"clang_gen_profdata: Failed to merge coverage data for {basepath}.")
            return 1
        return 0

    def clang_gen_lcov_xml(self, profdata, testfiles, output_lcov, output_xml):
        """
        Generate lcov coverage data from LLVM profdata and test files.
        """
        if not profdata or not os.path.isfile(profdata):
            logging.error(f"clang_gen_lcov_xml: Invalid profdata file provided. {profdata}")
            return 1
        if not testfiles:
            logging.error("clang_gen_lcov_xml: No test files provided.")
            return 1
        if not output_lcov:
            logging.error("clang_gen_lcov_xml: No output lcov file provided.")
            return 1
        if not output_xml:
            logging.error("clang_gen_lcov_xml: No output xml file provided.")
            return 1

        for file in [output_lcov, output_xml]:
            if os.path.isfile(file):
                os.remove(file)

        resp_file = output_lcov + ".rsp"
        with open(resp_file, "w") as f:
            f.write('\n-object\n'.join(testfiles))

        # llvm-cov supports response file with '@' to pass list of object files.
        # Use response file to avoid command line length limit.
        ret = RunCmd("llvm-cov", f"export -format=lcov --instr-profile={profdata} @{resp_file} > {output_lcov}")
        if ret != 0:
            logging.error(f"clang_gen_lcov_xml: Failed to generate coverage lcov. {output_lcov}")
            return 1
        ret = RunCmd("lcov_cobertura",f"{output_lcov} --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {output_xml}")
        if ret != 0:
            logging.error(f"clang_gen_lcov_xml: Failed generate filtered coverage XML. {output_xml}")
            return 1

        if os.path.isfile(resp_file):
            os.remove(resp_file)

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

        # Generate XML file if requested by each package
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
