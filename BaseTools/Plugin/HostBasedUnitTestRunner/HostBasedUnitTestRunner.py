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

        # Generate and XML file if requested.for all package
        if os.path.isfile(f"{workspace}/Build/coverage.xml"):
            os.remove(f"{workspace}/Build/coverage.xml")
        ret = RunCmd("lcov_cobertura",f"{workspace}/Build/all-coverage.info --excludes ^.*UnitTest\|^.*MU\|^.*Mock\|^.*DEBUG -o {workspace}/Build/coverage.xml")

        return 0


    def gen_code_coverage_clang(self, thebuilder):
        logging.info("Generating UnitTest code coverage")

        buildOutputBase = thebuilder.env.GetValue("BUILD_OUTPUT_BASE")
        if GetHostInfo().os.upper() == "LINUX":
            # Collect test executables with no file extension
            testList = glob.glob(os.path.join(buildOutputBase, "**", "*Test*"), recursive=True)
            testList = [f for f in testList if os.path.isfile(f) and os.path.splitext(f)[1] == ""]
        elif GetHostInfo().os.upper() == "WINDOWS":
            # Collect test executables with a .exe file extension
            testList = glob.glob(os.path.join(buildOutputBase, "**","*Test*.exe"), recursive=True)
        else:
            raise NotImplementedError("Unsupported Operating System")
        if not testList:
            logging.warning("UnitTest Coverage: No test binaries found.")
            return 0

        profrawlistFile = os.path.join(buildOutputBase, 'profrawlist.txt')
        mergedProfData = os.path.join(buildOutputBase, 'merged.profdata')
        mergedCoverageXml = os.path.join(buildOutputBase, 'coverage.xml')

        with open(profrawlistFile, "w") as f:
            f.write("\n".join(glob.glob(os.path.join(buildOutputBase, "**", "*.profraw"), recursive=True)))
        # Generate coverage file
        ret = RunCmd("llvm-profdata", f"merge -sparse --input-files {profrawlistFile} --output {mergedProfData}")
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to merge coverage data.")
            return 1
        # Generate and LCOV and XML file for each unit test executable
        for testFile in testList:
            lcovFile = f"{testFile}.lcov"
            ret = RunCmd("llvm-cov", f"export -format=lcov --instr-profile={mergedProfData} {testFile} > {lcovFile} ")
            if ret != 0:
                logging.error("UnitTest Coverage: Failed to generate coverage data for " + testFile)
                return 1
            coveragexmlFile = f"{testFile}.coverage.xml"
            ret = RunCmd("lcov_cobertura",f"{lcovFile} -o {coveragexmlFile}")
            if ret != 0:
                logging.error("UnitTest Coverage: Failed generate filtered coverage XML.")
                return 1
        # Merge all XML files
        ret = self.merge_cobertura_xml_files([f"{testFile}.coverage.xml" for testFile in testList], mergedCoverageXml)
        if ret != 0:
            logging.error("UnitTest Coverage: Failed to merge coverage XML files.")
            return 1
        return 0

    def merge_cobertura_xml_files(self, xml_file_list, output_file):
        """
        Merge multiple Cobertura XML files into a single output file.

        Args:
            xml_file_list (list): List of Cobertura XML file paths to merge.
            output_file (str): Path to the merged output XML file.
        """
        if not xml_file_list:
            logging.warning("No Cobertura XML files provided for merging.")
            return 1

        try:
            # Parse the first file as the base
            base_tree = xml.etree.ElementTree.parse(xml_file_list[0])
            base_root = base_tree.getroot()
            base_packages = base_root.find("packages")
            if base_packages is None:
                base_packages = xml.etree.ElementTree.SubElement(base_root, "packages")

            # Merge the rest
            for xml_file in xml_file_list[1:]:
                tree = xml.etree.ElementTree.parse(xml_file)
                root = tree.getroot()
                packages = root.find("packages")
                if packages is not None:
                    for package in packages.findall("package"):
                        base_packages.append(package)

            # Recalculate line-rate and branch-rate for the merged XML
            lines_covered = 0
            lines_valid = 0
            branches_covered = 0
            branches_valid = 0

            def parse_condition_coverage(cond_cov: str) -> int:
                """
                Parse a condition-coverage string like '50% (1/2)'.
                Returns the number of covered branches.
                """
                try:
                    inside_parens = cond_cov.split("(", 1)[1].split(")", 1)[0]
                    covered, total = map(int, inside_parens.split("/"))
                    return covered
                except Exception:
                    return 0

            for package in base_packages.findall("package"):
                for packageclass in package.findall("classes/class"):
                    lines = packageclass.find("lines")
                    if lines is None:
                        continue
                    for line in lines.findall("line"):
                        lines_valid += 1
                        # line coverage
                        if "hits" in line.attrib and int(line.attrib["hits"]) > 0:
                            lines_covered += 1
                        # branch coverage
                        if "branch" in line.attrib and line.attrib["branch"] == "true":
                            branches_valid += 1
                        if "condition-coverage" in line.attrib:
                            # Example: "50% (1/2)"
                            branches_covered += parse_condition_coverage(
                                                  line.attrib["condition-coverage"]
                                                  )

            def safe_rate(covered, valid):
                return float(covered) / float(valid) if valid else 0.0

            base_root.set("lines-covered", str(lines_covered))
            base_root.set("lines-valid", str(lines_valid))
            base_root.set("line-rate", str(safe_rate(lines_covered, lines_valid)))
            base_root.set("branches-covered", str(branches_covered))
            base_root.set("branches-valid", str(branches_valid))
            base_root.set("branch-rate", str(safe_rate(branches_covered, branches_valid)))

            # Write merged XML
            base_tree.write(output_file, encoding="utf-8", xml_declaration=True)
            logging.info(f"Merged Cobertura XML written to {output_file}")
            return 0
        except Exception as e:
            logging.error(f"Failed to merge Cobertura XML files: {e}")
            return 1

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
