# @file LicenseCheck.py
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import logging
import re
from io import StringIO
from typing import List, Tuple
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.var_dict import VarDict
from edk2toollib.utility_functions import RunCmd


class LicenseCheck(ICiBuildPlugin):

    """
    A CiBuildPlugin to check the license for new added files.

    Configuration options:
    "LicenseCheck": {
        "IgnoreFiles": []
    },
    """

    license_format_preflix = 'SPDX-License-Identifier'

    bsd2_patent = 'BSD-2-Clause-Patent'

    Readdedfileformat = re.compile(r'\+\+\+ b\/(.*)')

    file_extension_list = [".c", ".h", ".inf", ".dsc", ".dec", ".py", ".bat", ".sh", ".uni", ".yaml",
                           ".fdf", ".inc", "yml", ".asm", ".asm16", ".asl", ".vfr", ".s", ".S", ".aslc",
                           ".nasm", ".nasmb", ".idf", ".Vfr", ".H"]

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
        return ("Check for license for " + packagename, packagename + ".LicenseCheck")

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
        return_buffer = StringIO()
        params = "diff --unified=0 origin/master HEAD"
        RunCmd("git", params, outstream=return_buffer)
        p = return_buffer.getvalue().strip()
        patch = p.split("\n")
        return_buffer.close()

        ignore_files = []
        if "IgnoreFiles" in pkgconfig:
            ignore_files = pkgconfig["IgnoreFiles"]

        self.ok = True
        self.startcheck = False
        self.license = True
        self.all_file_pass = True
        count = len(patch)
        line_index = 0
        for line in patch:
            if line.startswith('--- /dev/null'):
                nextline = patch[line_index + 1]
                added_file = self.Readdedfileformat.search(nextline).group(1)
                added_file_extension = os.path.splitext(added_file)[1]
                if added_file_extension in self.file_extension_list and packagename in added_file:
                    if (self.IsIgnoreFile(added_file, ignore_files)):
                        line_index = line_index + 1
                        continue
                    self.startcheck = True
                    self.license = False
            if self.startcheck and self.license_format_preflix in line:
                if self.bsd2_patent in line:
                    self.license = True
            if line_index + 1 == count or patch[line_index + 1].startswith('diff --') and self.startcheck:
                if not self.license:
                    self.all_file_pass = False
                    error_message = "Invalid license in: " + added_file + " Hint: Only BSD-2-Clause-Patent is accepted."
                    logging.error(error_message)
                self.startcheck = False
                self.license = True
            line_index = line_index + 1

        if self.all_file_pass:
            tc.SetSuccess()
            return 0
        else:
            tc.SetFailed("License Check {0} Failed. ".format(packagename), "LICENSE_CHECK_FAILED")
            return 1

    def IsIgnoreFile(self, file: str, ignore_files: List[str]) -> bool:
        for f in ignore_files:
            if f in file:
                return True
        return False
