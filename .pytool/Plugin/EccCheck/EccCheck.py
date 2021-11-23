# @file EccCheck.py
#
# Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import shutil
import re
import csv
import xml.dom.minidom
from typing import List, Dict, Tuple
import logging
from io import StringIO
from edk2toolext.environment import shell_environment
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.var_dict import VarDict
from edk2toollib.utility_functions import RunCmd


class EccCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that finds the Ecc issues of newly added code in pull request.

    Configuration options:
    "EccCheck": {
        "ExceptionList": [],
        "IgnoreFiles": []
    },
    """

    FindModifyFile = re.compile(r'\+\+\+ b\/(.*)')
    LineScopePattern = (r'@@ -\d*\,*\d* \+\d*\,*\d* @@.*')
    LineNumRange = re.compile(r'@@ -\d*\,*\d* \+(\d*)\,*(\d*) @@.*')

    def GetTestName(self, packagename: str, environment: VarDict) -> tuple:
        """ Provide the testcase name and classname for use in reporting
            testclassname: a descriptive string for the testcase can include whitespace
            classname: should be patterned <packagename>.<plugin>.<optionally any unique condition>

            Args:
              packagename: string containing name of package to build
              environment: The VarDict for the test to run in
            Returns:
                a tuple containing the testcase name and the classname
                (testcasename, classname)
        """
        return ("Check for efi coding style for " + packagename, packagename + ".EccCheck")

    ##
    # External function of plugin.  This function is used to perform the task of the ci_build_plugin Plugin
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
        workspace_path = Edk2pathObj.WorkspacePath
        basetools_path = environment.GetValue("EDK_TOOLS_PATH")
        python_path = os.path.join(basetools_path, "Source", "Python")
        env = shell_environment.GetEnvironment()
        env.set_shell_var('PYTHONPATH', python_path)
        env.set_shell_var('WORKSPACE', workspace_path)
        env.set_shell_var('PACKAGES_PATH', os.pathsep.join(Edk2pathObj.PackagePathList))
        self.ECC_PASS = True

        # Create temp directory
        temp_path = os.path.join(workspace_path, 'Build', '.pytool', 'Plugin', 'EccCheck')
        try:
            # Delete temp directory
            if os.path.exists(temp_path):
                shutil.rmtree(temp_path)
            # Copy package being scanned to temp_path
            shutil.copytree (
              os.path.join(workspace_path, packagename),
              os.path.join(temp_path, packagename),
              symlinks=True
              )
            # Copy exception.xml to temp_path
            shutil.copyfile (
              os.path.join(basetools_path, "Source", "Python", "Ecc", "exception.xml"),
              os.path.join(temp_path, "exception.xml")
              )
            # Output file to use for git diff operations
            temp_diff_output = os.path.join (temp_path, 'diff.txt')

            self.ApplyConfig(pkgconfig, temp_path, packagename)
            modify_dir_list = self.GetModifyDir(packagename, temp_diff_output)
            patch = self.GetDiff(packagename, temp_diff_output)
            ecc_diff_range = self.GetDiffRange(patch, packagename, temp_path)
            #
            # Use temp_path as working directory when running ECC tool
            #
            self.GenerateEccReport(modify_dir_list, ecc_diff_range, temp_path, basetools_path)
            ecc_log = os.path.join(temp_path, "Ecc.log")
            if self.ECC_PASS:
                # Delete temp directory
                if os.path.exists(temp_path):
                    shutil.rmtree(temp_path)
                tc.SetSuccess()
                return 0
            else:
                with open(ecc_log, encoding='utf8') as output:
                    ecc_output = output.readlines()
                    for line in ecc_output:
                        logging.error(line.strip())
                # Delete temp directory
                if os.path.exists(temp_path):
                    shutil.rmtree(temp_path)
                tc.SetFailed("EccCheck failed for {0}".format(packagename), "CHECK FAILED")
                return 1
        except KeyboardInterrupt:
            # If EccCheck is interrupted by keybard interrupt, then return failure
            # Delete temp directory
            if os.path.exists(temp_path):
                shutil.rmtree(temp_path)
            tc.SetFailed("EccCheck interrupted for {0}".format(packagename), "CHECK FAILED")
            return 1
        else:
            # If EccCheck fails for any other exception type, raise the exception
            # Delete temp directory
            if os.path.exists(temp_path):
                shutil.rmtree(temp_path)
            tc.SetFailed("EccCheck exception for {0}".format(packagename), "CHECK FAILED")
            raise
            return 1

    def GetDiff(self, pkg: str, temp_diff_output: str) -> List[str]:
        patch = []
        #
        # Generate unified diff between origin/master and HEAD.
        #
        params = "diff --output={} --unified=0 origin/master HEAD".format(temp_diff_output)
        RunCmd("git", params)
        with open(temp_diff_output) as file:
            patch = file.read().strip().split('\n')
        return patch

    def GetModifyDir(self, pkg: str, temp_diff_output: str) -> List[str]:
        #
        # Generate diff between origin/master and HEAD using --diff-filter to
        # exclude deleted and renamed files that do not need to be scanned by
        # ECC.  Also use --name-status to only generate the names of the files
        # with differences.  The output format of this git diff command is a
        # list of files with the change status and the filename.  The filename
        # is always at the end of the line.  Examples:
        #
        #   M       MdeModulePkg/Application/CapsuleApp/CapsuleApp.h
        #   M       MdeModulePkg/Application/UiApp/FrontPage.h
        #
        params = "diff --output={} --diff-filter=dr --name-status origin/master HEAD".format(temp_diff_output)
        RunCmd("git", params)
        dir_list = []
        with open(temp_diff_output) as file:
            dir_list = file.read().strip().split('\n')

        modify_dir_list = []
        for modify_dir in dir_list:
            #
            # Parse file name from the end of the line
            #
            file_path = modify_dir.strip().split()
            #
            # Skip lines that do not have at least 2 elements (status and file name)
            #
            if len(file_path) < 2:
                continue
            #
            # Parse the directory name from the file name
            #
            file_dir = os.path.dirname(file_path[-1])
            #
            # Skip directory names that do not start with the package being scanned.
            #
            if file_dir.split('/')[0] != pkg:
                continue
            #
            # Skip directory names that are identical to the package being scanned.
            # The assumption here is that there are no source files at the package
            # root.  Instead, the only expected files in the package root are
            # EDK II meta data files (DEC, DSC, FDF).
            #
            if file_dir == pkg:
                continue
            #
            # Skip directory names that are already in the modified dir list
            #
            if file_dir in modify_dir_list:
                continue
            #
            # Add the candidate directory to scan to the modified dir list
            #
            modify_dir_list.append(file_dir)

        #
        # Remove duplicates from modify_dir_list
        # Given a folder path, ECC performs a recursive scan of that folder.
        # If a parent and child folder are both present in modify_dir_list,
        # then ECC will perform redudanct scans of source files.  In order
        # to prevent redundant scans, if a parent and child folder are both
        # present, then remove all the child folders.
        #
        # For example, if modified_dir_list contains the following elements:
        #   MdeModulePkg/Core/Dxe
        #   MdeModulePkg/Core/Dxe/Hand
        #   MdeModulePkg/Core/Dxe/Mem
        #
        # Then MdeModulePkg/Core/Dxe/Hand and MdeModulePkg/Core/Dxe/Mem should
        # be removed because the files in those folders are covered by a scan
        # of MdeModulePkg/Core/Dxe.
        #
        filtered_list = []
        for dir1 in modify_dir_list:
            Append = True
            for dir2 in modify_dir_list:
                if dir1 == dir2:
                    continue
                common = os.path.commonpath([dir1, dir2])
                if os.path.normpath(common) == os.path.normpath(dir2):
                    Append = False
                    break
            if Append and dir1 not in filtered_list:
                filtered_list.append(dir1)
        return filtered_list

    def GetDiffRange(self, patch_diff: List[str], pkg: str, temp_path: str) -> Dict[str, List[Tuple[int, int]]]:
        IsDelete = True
        StartCheck = False
        range_directory: Dict[str, List[Tuple[int, int]]] = {}
        for line in patch_diff:
            modify_file = self.FindModifyFile.findall(line)
            if modify_file and pkg in modify_file[0] and not StartCheck and os.path.isfile(modify_file[0]):
                modify_file_comment_dic = self.GetCommentRange(modify_file[0], temp_path)
                IsDelete = False
                StartCheck = True
                modify_file_dic = modify_file[0]
                modify_file_dic = modify_file_dic.replace("/", os.sep)
                range_directory[modify_file_dic] = []
            elif line.startswith('--- '):
                StartCheck = False
            elif re.match(self.LineScopePattern, line, re.I) and not IsDelete and StartCheck:
                start_line = self.LineNumRange.search(line).group(1)
                line_range = self.LineNumRange.search(line).group(2)
                if not line_range:
                    line_range = '1'
                range_directory[modify_file_dic].append((int(start_line), int(start_line) + int(line_range) - 1))
                for i in modify_file_comment_dic:
                    if int(i[0]) <= int(start_line) <= int(i[1]):
                        range_directory[modify_file_dic].append(i)
        return range_directory

    def GetCommentRange(self, modify_file: str, temp_path: str) -> List[Tuple[int, int]]:
        comment_range: List[Tuple[int, int]] = []
        modify_file_path = os.path.join(temp_path, modify_file)
        if not os.path.exists (modify_file_path):
            return comment_range
        with open(modify_file_path) as f:
            line_no = 1
            Start = False
            for line in f:
                if line.startswith('/**'):
                    start_no = line_no
                    Start = True
                if line.startswith('**/') and Start:
                    end_no = line_no
                    Start = False
                    comment_range.append((int(start_no), int(end_no)))
                line_no += 1

        if comment_range and comment_range[0][0] == 1:
            del comment_range[0]
        return comment_range

    def GenerateEccReport(self, modify_dir_list: List[str], ecc_diff_range: Dict[str, List[Tuple[int, int]]],
                           temp_path: str, basetools_path: str) -> None:
        ecc_need = False
        ecc_run = True
        config    = os.path.normpath(os.path.join(basetools_path, "Source", "Python", "Ecc", "config.ini"))
        exception = os.path.normpath(os.path.join(temp_path, "exception.xml"))
        report    = os.path.normpath(os.path.join(temp_path, "Ecc.csv"))
        for modify_dir in modify_dir_list:
            target = os.path.normpath(os.path.join(temp_path, modify_dir))
            logging.info('Run ECC tool for the commit in %s' % modify_dir)
            ecc_need = True
            ecc_params = "-c {0} -e {1} -t {2} -r {3}".format(config, exception, target, report)
            return_code = RunCmd("Ecc", ecc_params, workingdir=temp_path)
            if return_code != 0:
                ecc_run = False
                break
            if not ecc_run:
                logging.error('Fail to run ECC tool')
            self.ParseEccReport(ecc_diff_range, temp_path)

        if not ecc_need:
            logging.info("Doesn't need run ECC check")

        return

    def ParseEccReport(self, ecc_diff_range: Dict[str, List[Tuple[int, int]]], temp_path: str) -> None:
        ecc_log = os.path.join(temp_path, "Ecc.log")
        ecc_csv = os.path.join(temp_path, "Ecc.csv")
        row_lines = []
        ignore_error_code = self.GetIgnoreErrorCode()
        if os.path.exists(ecc_csv):
            with open(ecc_csv) as csv_file:
                reader = csv.reader(csv_file)
                for row in reader:
                    for modify_file in ecc_diff_range:
                        if modify_file in row[3]:
                            for i in ecc_diff_range[modify_file]:
                                line_no = int(row[4])
                                if i[0] <= line_no <= i[1] and row[1] not in ignore_error_code:
                                    row[0] = '\nEFI coding style error'
                                    row[1] = 'Error code: ' + row[1]
                                    row[3] = 'file: ' + row[3]
                                    row[4] = 'Line number: ' + row[4]
                                    row_line = '\n  *'.join(row)
                                    row_lines.append(row_line)
                                    break
                            break
        if row_lines:
            self.ECC_PASS = False

        with open(ecc_log, 'a') as log:
            all_line = '\n'.join(row_lines)
            all_line = all_line + '\n'
            log.writelines(all_line)
        return

    def ApplyConfig(self, pkgconfig: Dict[str, List[str]], temp_path: str, pkg: str) -> None:
        if "IgnoreFiles" in pkgconfig:
            for a in pkgconfig["IgnoreFiles"]:
                a = os.path.join(temp_path, pkg, a)
                a = a.replace(os.sep, "/")

                logging.info("Ignoring Files {0}".format(a))
                if os.path.exists(a):
                    if os.path.isfile(a):
                        os.remove(a)
                    elif os.path.isdir(a):
                        shutil.rmtree(a)
                else:
                    logging.error("EccCheck.IgnoreInf -> {0} not found in filesystem.  Invalid ignore files".format(a))

        if "ExceptionList" in pkgconfig:
            exception_list = pkgconfig["ExceptionList"]
            exception_xml = os.path.join(temp_path, "exception.xml")
            try:
                logging.info("Appending exceptions")
                self.AppendException(exception_list, exception_xml)
            except Exception as e:
                logging.error("Fail to apply exceptions")
                raise e
        return

    def AppendException(self, exception_list: List[str], exception_xml: str) -> None:
        error_code_list = exception_list[::2]
        keyword_list = exception_list[1::2]
        dom_tree = xml.dom.minidom.parse(exception_xml)
        root_node = dom_tree.documentElement
        for error_code, keyword in zip(error_code_list, keyword_list):
            customer_node = dom_tree.createElement("Exception")
            keyword_node = dom_tree.createElement("KeyWord")
            keyword_node_text_value = dom_tree.createTextNode(keyword)
            keyword_node.appendChild(keyword_node_text_value)
            customer_node.appendChild(keyword_node)
            error_code_node = dom_tree.createElement("ErrorID")
            error_code_text_value = dom_tree.createTextNode(error_code)
            error_code_node.appendChild(error_code_text_value)
            customer_node.appendChild(error_code_node)
            root_node.appendChild(customer_node)
        with open(exception_xml, 'w') as f:
            dom_tree.writexml(f, indent='', addindent='', newl='\n', encoding='UTF-8')
        return

    def GetIgnoreErrorCode(self) -> set:
        """
        Below are kinds of error code that are accurate in ecc scanning of edk2 level.
        But EccCheck plugin is partial scanning so they are always false positive issues.
        The mapping relationship of error code and error message is listed BaseTools/Sourc/Python/Ecc/EccToolError.py
        """
        ignore_error_code = {
                             "10000",
                             "10001",
                             "10002",
                             "10003",
                             "10004",
                             "10005",
                             "10006",
                             "10007",
                             "10008",
                             "10009",
                             "10010",
                             "10011",
                             "10012",
                             "10013",
                             "10015",
                             "10016",
                             "10017",
                             "10022",
                            }
        return ignore_error_code
