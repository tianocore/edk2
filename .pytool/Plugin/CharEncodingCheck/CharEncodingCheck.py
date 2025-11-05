# @file CharEncodingCheck.py
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##


import os
import logging
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.var_dict import VarDict

##
# map
##
EcodingMap = {
    ".md": 'utf-8',
    ".dsc": 'utf-8',
    ".dec": 'utf-8',
    ".c": 'utf-8',
    ".h": 'utf-8',
    ".asm": 'utf-8',
    ".masm": 'utf-8',
    ".nasm": 'utf-8',
    ".s": 'utf-8',
    ".inf": 'utf-8',
    ".asl": 'utf-8',
    ".uni": 'utf-8',
    ".py": 'utf-8'
}


class CharEncodingCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that scans each file in the code tree and confirms the encoding is correct.

    Configuration options:
    "CharEncodingCheck": {
        "IgnoreFiles": []
    }
    """

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
        return ("Check for valid file encoding for " + packagename, packagename + ".CharEncodingCheck")

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
        overall_status = 0
        files_tested = 0

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(packagename)

        if abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError("No Package folder {0}".format(abs_pkg_path))
            return 0

        for (ext, enc) in EcodingMap.items():
            files = self.WalkDirectoryForExtension([ext], abs_pkg_path)
            files = [Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(x) for x in files]  # make edk2relative path so can process ignores

            if "IgnoreFiles" in pkgconfig:
                for a in pkgconfig["IgnoreFiles"]:
                    a = a.replace(os.sep, "/")
                    try:
                        tc.LogStdOut("Ignoring File {0}".format(a))
                        files.remove(a)
                    except:
                        tc.LogStdError("CharEncodingCheck.IgnoreInf -> {0} not found in filesystem.  Invalid ignore file".format(a))
                        logging.info("CharEncodingCheck.IgnoreInf -> {0} not found in filesystem.  Invalid ignore file".format(a))

            files = [Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(x) for x in files]
            for a in files:
                files_tested += 1
                if not self.TestEncodingOk(a, enc):
                    tc.LogStdError("Encoding Failure in {0}.  Not {1}".format(a, enc))
                    overall_status += 1

        tc.LogStdOut("Tested Encoding on {0} files".format(files_tested))
        if overall_status != 0:
            tc.SetFailed("CharEncoding {0} Failed.  Errors {1}".format(packagename, overall_status), "CHAR_ENCODING_CHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status

    def TestEncodingOk(self, apath, encodingValue):
        try:
            with open(apath, "rb") as fobj:
                fobj.read().decode(encodingValue)
        except Exception as exp:
            logging.error("Encoding failure: file: {0} type: {1}".format(apath, encodingValue))
            logging.debug("EXCEPTION: while processing {1} - {0}".format(exp, apath))
            return False

        return True
