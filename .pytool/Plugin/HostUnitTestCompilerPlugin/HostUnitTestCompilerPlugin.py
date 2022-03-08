# @file HostUnitTestCompilerPlugin.py
##
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import logging
import os
import re
from edk2toollib.uefi.edk2.parsers.dsc_parser import DscParser
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext import edk2_logging
from edk2toolext.environment.var_dict import VarDict
from edk2toollib.utility_functions import GetHostInfo


class HostUnitTestCompilerPlugin(ICiBuildPlugin):
    """
    A CiBuildPlugin that compiles the dsc for host based unit test apps.
    An IUefiBuildPlugin may be attached to this plugin that will run the
    unit tests and collect the results after successful compilation.

    Configuration options:
    "HostUnitTestCompilerPlugin": {
        "DscPath": "<path to dsc from root of pkg>"
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
        num,types = self.__GetHostUnitTestArch(environment)
        types = types.replace(" ", "_")

        return ("Compile and Run Host-Based UnitTests for " + packagename + " on arch " + types,
                packagename + ".HostUnitTestCompiler." + types)

    def RunsOnTargetList(self):
        return ["NOOPT"]

    #
    # Find the intersection of application types that can run on this host
    # and the TARGET_ARCH being build in this request.
    #
    # return tuple with (number of UEFI arch types, space separated string)
    def __GetHostUnitTestArch(self, environment):
        requested = environment.GetValue("TARGET_ARCH").split(' ')
        host = []
        if GetHostInfo().arch == 'x86':
            #assume 64bit can handle 64 and 32
            #assume 32bit can only handle 32
            ## change once IA32 issues resolved host.append("IA32")
            if GetHostInfo().bit == '64':
                host.append("X64")
        elif GetHostInfo().arch == 'ARM':
            if GetHostInfo().bit == '64':
                host.append("AARCH64")
            elif GetHostInfo().bit == '32':
                host.append("ARM")

        willrun = set(requested) & set(host)
        return (len(willrun), " ".join(willrun))


    ##
    # External function of plugin.  This function is used to perform the task of the ICiBuildPlugin Plugin
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
        self._env = environment
        environment.SetValue("CI_BUILD_TYPE", "host_unit_test", "Set in HostUnitTestCompilerPlugin")

        # Parse the config for required DscPath element
        if "DscPath" not in pkgconfig:
            tc.SetSkipped()
            tc.LogStdError("DscPath not found in config file.  Nothing to compile for HostBasedUnitTests.")
            return -1

        AP = Edk2pathObj.GetAbsolutePathOnThisSystemFromEdk2RelativePath(packagename)

        APDSC = os.path.join(AP, pkgconfig["DscPath"].strip())
        AP_Path = Edk2pathObj.GetEdk2RelativePathFromAbsolutePath(APDSC)
        if AP is None or AP_Path is None or not os.path.isfile(APDSC):
            tc.SetSkipped()
            tc.LogStdError("Package HostBasedUnitTest Dsc not found.")
            return -1

        logging.info("Building {0}".format(AP_Path))
        self._env.SetValue("ACTIVE_PLATFORM", AP_Path, "Set in Compiler Plugin")
        num, RUNNABLE_ARCHITECTURES = self.__GetHostUnitTestArch(environment)
        if(num == 0):
            tc.SetSkipped()
            tc.LogStdError("No host architecture compatibility")
            return -1

        if not environment.SetValue("TARGET_ARCH",
                                    RUNNABLE_ARCHITECTURES,
                                    "Update Target Arch based on Host Support"):
            #use AllowOverride function since this is a controlled attempt to change
            environment.AllowOverride("TARGET_ARCH")
            if not environment.SetValue("TARGET_ARCH",
                                        RUNNABLE_ARCHITECTURES,
                                        "Update Target Arch based on Host Support"):
                raise RuntimeError("Can't Change TARGET_ARCH as required")

        # Parse DSC to check for SUPPORTED_ARCHITECTURES
        dp = DscParser()
        dp.SetBaseAbsPath(Edk2pathObj.WorkspacePath)
        dp.SetPackagePaths(Edk2pathObj.PackagePathList)
        dp.ParseFile(AP_Path)
        if "SUPPORTED_ARCHITECTURES" in dp.LocalVars:
            SUPPORTED_ARCHITECTURES = dp.LocalVars["SUPPORTED_ARCHITECTURES"].split('|')
            TARGET_ARCHITECTURES = environment.GetValue("TARGET_ARCH").split(' ')

            # Skip if there is no intersection between SUPPORTED_ARCHITECTURES and TARGET_ARCHITECTURES
            if len(set(SUPPORTED_ARCHITECTURES) & set(TARGET_ARCHITECTURES)) == 0:
                tc.SetSkipped()
                tc.LogStdError("No supported architecutres to build for host unit tests")
                return -1

        uefiBuilder = UefiBuilder()
        # do all the steps
        # WorkSpace, PackagesPath, PInHelper, PInManager
        ret = uefiBuilder.Go(Edk2pathObj.WorkspacePath, os.pathsep.join(Edk2pathObj.PackagePathList), PLMHelper, PLM)
        if ret != 0:  # failure:
            tc.SetFailed("Compile failed for {0}".format(packagename), "Compile_FAILED")
            tc.LogStdError("{0} Compile failed with error code {1} ".format(AP_Path, ret))
            return 1

        else:
            tc.SetSuccess()
            return 0
