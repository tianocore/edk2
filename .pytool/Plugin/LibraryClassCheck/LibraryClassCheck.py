# @file LibraryClassCheck.py
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import os
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toollib.uefi.edk2.parsers.dec_parser import DecParser
from edk2toollib.uefi.edk2.parsers.inf_parser import InfParser
from edk2toolext.environment.var_dict import VarDict


class LibraryClassCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that scans the code tree and library classes for undeclared
    files

    Configuration options:
    "LibraryClassCheck": {
        IgnoreHeaderFile: [],  # Ignore a file found on disk
        IgnoreLibraryClass: [] # Ignore a declaration found in dec file
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
        return ("Check library class declarations in " + packagename, packagename + ".LibraryClassCheck")

    def __GetPkgDec(self, rootpath):
        try:
            allEntries = os.listdir(rootpath)
            for entry in allEntries:
                if entry.lower().endswith(".dec"):
                    return(os.path.join(rootpath, entry))
        except Exception:
            logging.error("Unable to find DEC for package:{0}".format(rootpath))

        return None

    ##
    # External function of plugin.  This function is used to perform the task of the MuBuild Plugin
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
        LibraryClassIgnore = []

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(packagename)
        abs_dec_path = self.__GetPkgDec(abs_pkg_path)
        wsr_dec_path = Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(abs_dec_path)

        if abs_dec_path is None or wsr_dec_path == "" or not os.path.isfile(abs_dec_path):
            tc.SetSkipped()
            tc.LogStdError("No DEC file {0} in package {1}".format(abs_dec_path, abs_pkg_path))
            return -1

        # Get all include folders
        dec = DecParser()
        dec.SetBaseAbsPath(Edk2pathObj.WorkspacePath).SetPackagePaths(Edk2pathObj.PackagePathList)
        dec.ParseFile(wsr_dec_path)

        AllHeaderFiles = []

        for includepath in dec.IncludePaths:
            ## Get all header files in the library folder
            AbsLibraryIncludePath = os.path.join(abs_pkg_path, includepath, "Library")
            if(not os.path.isdir(AbsLibraryIncludePath)):
                continue

            hfiles = self.WalkDirectoryForExtension([".h"], AbsLibraryIncludePath)
            hfiles = [os.path.relpath(x,abs_pkg_path) for x in hfiles]  # make package root relative path
            hfiles = [x.replace("\\", "/") for x in hfiles]  # make package relative path

            AllHeaderFiles.extend(hfiles)

        if len(AllHeaderFiles) == 0:
            tc.SetSkipped()
            tc.LogStdError(f"No Library include folder in any Include path")
            return -1

        # Remove ignored paths
        if "IgnoreHeaderFile" in pkgconfig:
            for a in pkgconfig["IgnoreHeaderFile"]:
                try:
                    tc.LogStdOut("Ignoring Library Header File {0}".format(a))
                    AllHeaderFiles.remove(a)
                except:
                    tc.LogStdError("LibraryClassCheck.IgnoreHeaderFile -> {0} not found.  Invalid Header File".format(a))
                    logging.info("LibraryClassCheck.IgnoreHeaderFile -> {0} not found.  Invalid Header File".format(a))

        if "IgnoreLibraryClass" in pkgconfig:
            LibraryClassIgnore = pkgconfig["IgnoreLibraryClass"]


        ## Attempt to find library classes
        for lcd in dec.LibraryClasses:
            ## Check for correct file path separator
            if "\\" in lcd.path:
                tc.LogStdError("LibraryClassCheck.DecFilePathSeparator -> {0} invalid.".format(lcd.path))
                logging.error("LibraryClassCheck.DecFilePathSeparator -> {0} invalid.".format(lcd.path))
                overall_status += 1
                continue

            if lcd.name in LibraryClassIgnore:
                tc.LogStdOut("Ignoring Library Class Name {0}".format(lcd.name))
                LibraryClassIgnore.remove(lcd.name)
                continue

            logging.debug(f"Looking for Library Class {lcd.path}")
            try:
                AllHeaderFiles.remove(lcd.path)

            except ValueError:
                tc.LogStdError(f"Library {lcd.name} with path {lcd.path} not found in package filesystem")
                logging.error(f"Library {lcd.name} with path {lcd.path} not found in package filesystem")
                overall_status += 1

        ## any remaining AllHeaderFiles are not described in DEC
        for h in AllHeaderFiles:
            tc.LogStdError(f"Library Header File {h} not declared in package DEC {wsr_dec_path}")
            logging.error(f"Library Header File {h} not declared in package DEC {wsr_dec_path}")
            overall_status += 1

        ## Warn about any invalid library class names in the ignore list
        for r in LibraryClassIgnore:
            tc.LogStdError("LibraryClassCheck.IgnoreLibraryClass -> {0} not found.  Library Class not found".format(r))
            logging.info("LibraryClassCheck.IgnoreLibraryClass -> {0} not found.  Library Class not found".format(r))


        # If XML object exists, add result
        if overall_status != 0:
            tc.SetFailed("LibraryClassCheck {0} Failed.  Errors {1}".format(wsr_dec_path, overall_status), "CHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status
