# @file EccCheck.py
#
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

    ReModifyFile = re.compile(r'[B-Q,S-Z]+[\d]*\t(.*)')
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
        edk2_path = Edk2pathObj.WorkspacePath
        python_path = os.path.join(edk2_path, "BaseTools", "Source", "Python")
        env = shell_environment.GetEnvironment()
        env.set_shell_var('PYTHONPATH', python_path)
        env.set_shell_var('WORKSPACE', edk2_path)
        self.ECC_PASS = True
        self.ApplyConfig(pkgconfig, edk2_path, packagename)
        modify_dir_list = self.GetModifyDir(packagename)
        patch = self.GetDiff(packagename)
        ecc_diff_range = self.GetDiffRange(patch, packagename, edk2_path)
        self.GenerateEccReport(modify_dir_list, ecc_diff_range, edk2_path)
        ecc_log = os.path.join(edk2_path, "Ecc.log")
        self.RevertCode()
        if self.ECC_PASS:
            tc.SetSuccess()
            self.RemoveFile(ecc_log)
            return 0
        else:
            with open(ecc_log, encoding='utf8') as output:
                ecc_output = output.readlines()
                for line in ecc_output:
                    logging.error(line.strip())
            self.RemoveFile(ecc_log)
            tc.SetFailed("EccCheck failed for {0}".format(packagename), "Ecc detected issues")
            return 1

    def RevertCode(self) -> None:
        submoudle_params = "submodule update --init"
        RunCmd("git", submoudle_params)
        reset_params = "reset HEAD --hard"
        RunCmd("git", reset_params)

    def GetDiff(self, pkg: str) -> List[str]:
        return_buffer = StringIO()
        params = "diff --unified=0 origin/master HEAD"
        RunCmd("git", params, outstream=return_buffer)
        p = return_buffer.getvalue().strip()
        patch = p.split("\n")
        return_buffer.close()

        return patch

    def RemoveFile(self, file: str) -> None:
        if os.path.exists(file):
            os.remove(file)
        return

    def GetModifyDir(self, pkg: str) -> List[str]:
        return_buffer = StringIO()
        params = "diff --name-status" + ' HEAD' + ' origin/master'
        RunCmd("git", params, outstream=return_buffer)
        p1 = return_buffer.getvalue().strip()
        dir_list = p1.split("\n")
        return_buffer.close()
        modify_dir_list = []
        for modify_dir in dir_list:
            file_path = self.ReModifyFile.findall(modify_dir)
            if file_path:
                file_dir = os.path.dirname(file_path[0])
            else:
                continue
            if pkg in file_dir and file_dir != pkg:
                modify_dir_list.append('%s' % file_dir)
            else:
                continue

        modify_dir_list = list(set(modify_dir_list))
        return modify_dir_list

    def GetDiffRange(self, patch_diff: List[str], pkg: str, workingdir: str) -> Dict[str, List[Tuple[int, int]]]:
        IsDelete = True
        StartCheck = False
        range_directory: Dict[str, List[Tuple[int, int]]] = {}
        for line in patch_diff:
            modify_file = self.FindModifyFile.findall(line)
            if modify_file and pkg in modify_file[0] and not StartCheck and os.path.isfile(modify_file[0]):
                modify_file_comment_dic = self.GetCommentRange(modify_file[0], workingdir)
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

    def GetCommentRange(self, modify_file: str, workingdir: str) -> List[Tuple[int, int]]:
        modify_file_path = os.path.join(workingdir, modify_file)
        with open(modify_file_path) as f:
            line_no = 1
            comment_range: List[Tuple[int, int]] = []
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
                          edk2_path: str) -> None:
        ecc_need = False
        ecc_run = True
        config = os.path.join(edk2_path, "BaseTools", "Source", "Python", "Ecc", "config.ini")
        exception = os.path.join(edk2_path, "BaseTools", "Source", "Python", "Ecc", "exception.xml")
        report = os.path.join(edk2_path, "Ecc.csv")
        for modify_dir in modify_dir_list:
            target = os.path.join(edk2_path, modify_dir)
            logging.info('Run ECC tool for the commit in %s' % modify_dir)
            ecc_need = True
            ecc_params = "-c {0} -e {1} -t {2} -r {3}".format(config, exception, target, report)
            return_code = RunCmd("Ecc", ecc_params, workingdir=edk2_path)
            if return_code != 0:
                ecc_run = False
                break
            if not ecc_run:
                logging.error('Fail to run ECC tool')
            self.ParseEccReport(ecc_diff_range, edk2_path)

        if not ecc_need:
            logging.info("Doesn't need run ECC check")

        revert_params = "checkout -- {}".format(exception)
        RunCmd("git", revert_params)
        return

    def ParseEccReport(self, ecc_diff_range: Dict[str, List[Tuple[int, int]]], edk2_path: str) -> None:
        ecc_log = os.path.join(edk2_path, "Ecc.log")
        ecc_csv = "Ecc.csv"
        file = os.listdir(edk2_path)
        row_lines = []
        ignore_error_code = self.GetIgnoreErrorCode()
        if ecc_csv in file:
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

    def ApplyConfig(self, pkgconfig: Dict[str, List[str]], edk2_path: str, pkg: str) -> None:
        if "IgnoreFiles" in pkgconfig:
            for a in pkgconfig["IgnoreFiles"]:
                a = os.path.join(edk2_path, pkg, a)
                a = a.replace(os.sep, "/")

                logging.info("Ignoring Files {0}".format(a))
                if os.path.exists(a):
                    if os.path.isfile(a):
                        self.RemoveFile(a)
                    elif os.path.isdir(a):
                        shutil.rmtree(a)
                else:
                    logging.error("EccCheck.IgnoreInf -> {0} not found in filesystem.  Invalid ignore files".format(a))

        if "ExceptionList" in pkgconfig:
            exception_list = pkgconfig["ExceptionList"]
            exception_xml = os.path.join(edk2_path, "BaseTools", "Source", "Python", "Ecc", "exception.xml")
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
