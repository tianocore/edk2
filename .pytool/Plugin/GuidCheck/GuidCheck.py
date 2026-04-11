# @file GuidCheck.py
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toollib.uefi.edk2.guid_list import GuidList
from edk2toolext.environment.var_dict import VarDict


class GuidCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that scans the code tree and looks for duplicate guids
    from the package being tested.

    Configuration options:
    "GuidCheck": {
        "IgnoreGuidName": [], # provide in format guidname=guidvalue or just guidname
        "IgnoreGuidValue": [],
        "IgnoreFoldersAndFiles": [],
        "IgnoreDuplicates": [] # Provide in format guidname=guidname=guidname...
    }
    """

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
        return ("Confirm GUIDs are unique in " + packagename, packagename + ".GuidCheck")

    def _FindConflictingGuidValues(self, guidlist: list) -> list:
        """ Find all duplicate guids by guid value and report them as errors
        """
        # Sort the list by guid
        guidsorted = sorted(
            guidlist, key=lambda x: x.guid.upper(), reverse=True)

        previous = None  # Store previous entry for comparison
        error = None
        errors = []
        for index in range(len(guidsorted)):
            i = guidsorted[index]
            if(previous is not None):
                if i.guid == previous.guid:  # Error
                    if(error is None):
                        # Catch errors with more than 1 conflict
                        error = ErrorEntry("guid")
                        error.entries.append(previous)
                        errors.append(error)
                    error.entries.append(i)
                else:
                    # no match.  clear error
                    error = None
            previous = i
        return errors

    def _FindConflictingGuidNames(self, guidlist: list) -> list:
        """ Find all duplicate guids by name and if they are not all
        from inf files report them as errors.  It is ok to have
        BASE_NAME duplication.

        Is this useful?  It would catch two same named guids in dec file
        that resolve to different values.
        """
        # Sort the list by guid
        namesorted = sorted(guidlist, key=lambda x: x.name.upper())

        previous = None  # Store previous entry for comparison
        error = None
        errors = []
        for index in range(len(namesorted)):
            i = namesorted[index]
            if(previous is not None):
                # If name matches
                if i.name == previous.name:
                    if(error is None):
                        # Catch errors with more than 1 conflict
                        error = ErrorEntry("name")
                        error.entries.append(previous)
                        errors.append(error)
                    error.entries.append(i)
                else:
                    # no match.  clear error
                    error = None
            previous = i

            # Loop thru and remove any errors where all files are infs as it is ok if
            # they have the same inf base name.
            for e in errors[:]:
                if len( [en for en in e.entries if not en.absfilepath.lower().endswith(".inf")]) == 0:
                    errors.remove(e)

        return errors

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
        Errors = []

        abs_pkg_path = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            packagename)

        if abs_pkg_path is None:
            tc.SetSkipped()
            tc.LogStdError("No package {0}".format(packagename))
            return -1

        All_Ignores = ["/Build", "/Conf"]
        # Parse the config for other ignores
        if "IgnoreFoldersAndFiles" in pkgconfig:
            All_Ignores.extend(pkgconfig["IgnoreFoldersAndFiles"])

        # Parse the workspace for all GUIDs
        gs = GuidList.guidlist_from_filesystem(
            Edk2pathObj.WorkspacePath, ignore_lines=All_Ignores)

        # Remove ignored guidvalue
        if "IgnoreGuidValue" in pkgconfig:
            for a in pkgconfig["IgnoreGuidValue"]:
                try:
                    tc.LogStdOut("Ignoring Guid {0}".format(a.upper()))
                    for b in gs[:]:
                        if b.guid == a.upper():
                            gs.remove(b)
                except:
                    tc.LogStdError("GuidCheck.IgnoreGuid -> {0} not found.  Invalid ignore guid".format(a.upper()))
                    logging.info("GuidCheck.IgnoreGuid -> {0} not found.  Invalid ignore guid".format(a.upper()))

        # Remove ignored guidname
        if "IgnoreGuidName" in pkgconfig:
            for a in pkgconfig["IgnoreGuidName"]:
                entry = a.split("=")
                if(len(entry) > 2):
                    tc.LogStdError("GuidCheck.IgnoreGuidName -> {0} Invalid Format.".format(a))
                    logging.info("GuidCheck.IgnoreGuidName -> {0} Invalid Format.".format(a))
                    continue
                try:
                    tc.LogStdOut("Ignoring Guid {0}".format(a))
                    for b in gs[:]:
                        if b.name == entry[0]:
                            if(len(entry) == 1):
                                gs.remove(b)
                            elif(len(entry) == 2 and b.guid.upper() == entry[1].upper()):
                                gs.remove(b)
                            else:
                                c.LogStdError("GuidCheck.IgnoreGuidName -> {0} incomplete match.  Invalid ignore guid".format(a))

                except:
                    tc.LogStdError("GuidCheck.IgnoreGuidName -> {0} not found.  Invalid ignore name".format(a))
                    logging.info("GuidCheck.IgnoreGuidName -> {0} not found.  Invalid ignore name".format(a))

        # Find conflicting Guid Values
        Errors.extend(self._FindConflictingGuidValues(gs))

        # Check if there are expected duplicates and remove it from the error list
        if "IgnoreDuplicates" in pkgconfig:
            for a in pkgconfig["IgnoreDuplicates"]:
                names = a.split("=")
                if len(names) < 2:
                    tc.LogStdError("GuidCheck.IgnoreDuplicates -> {0} invalid format".format(a))
                    logging.info("GuidCheck.IgnoreDuplicates -> {0} invalid format".format(a))
                    continue

                for b in Errors[:]:
                    if b.type != "guid":
                        continue
                    ## Make a list of the names that are not in the names list.  If there
                    ## are any in the list then this error should not be ignored.
                    t = [x for x in b.entries if x.name not in names]
                    if(len(t) == len(b.entries)):
                        ## did not apply to any entry
                        continue
                    elif(len(t) == 0):
                        ## full match - ignore duplicate
                        tc.LogStdOut("GuidCheck.IgnoreDuplicates -> {0}".format(a))
                        Errors.remove(b)
                    elif(len(t) < len(b.entries)):
                        ## partial match
                        tc.LogStdOut("GuidCheck.IgnoreDuplicates -> {0} incomplete match".format(a))
                        logging.info("GuidCheck.IgnoreDuplicates -> {0} incomplete match".format(a))
                    else:
                        tc.LogStdOut("GuidCheck.IgnoreDuplicates -> {0} unknown error.".format(a))
                        logging.info("GuidCheck.IgnoreDuplicates -> {0} unknown error".format(a))



        # Find conflicting Guid Names
        Errors.extend(self._FindConflictingGuidNames(gs))

        # Log errors for anything within the package under test
        for er in Errors[:]:
            InMyPackage = False
            for a in er.entries:
                if abs_pkg_path in a.absfilepath:
                    InMyPackage = True
                    break
            if(not InMyPackage):
                Errors.remove(er)
            else:
                logging.error(str(er))
                tc.LogStdError(str(er))

        # add result to test case
        overall_status = len(Errors)
        if overall_status != 0:
            tc.SetFailed("GuidCheck {0} Failed.  Errors {1}".format(
                packagename, overall_status), "CHECK_FAILED")
        else:
            tc.SetSuccess()
        return overall_status


class ErrorEntry():
    """ Custom/private class for reporting errors in the GuidList
    """

    def __init__(self, errortype):
        self.type = errortype  # 'guid' or 'name' depending on error type
        self.entries = []  # GuidListEntry that are in error condition

    def __str__(self):
        a = f"Error Duplicate {self.type}: "
        if(self.type == "guid"):
            a += f" {self.entries[0].guid}"
        elif(self.type == "name"):
            a += f" {self.entries[0].name}"

        a += f" ({len(self.entries)})\n"

        for e in self.entries:
            a += "\t" + str(e) + "\n"
        return a
