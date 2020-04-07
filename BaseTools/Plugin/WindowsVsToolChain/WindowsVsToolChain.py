# @file WindowsVsToolChain.py
# Plugin to configures paths for the VS2017 and VS2019 tool chain
##
# This plugin works in conjuncture with the tools_def
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
import edk2toollib.windows.locate_tools as locate_tools
from edk2toollib.windows.locate_tools import FindWithVsWhere
from edk2toolext.environment import shell_environment
from edk2toolext.environment import version_aggregator
from edk2toollib.utility_functions import GetHostInfo


class WindowsVsToolChain(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        return 0

    def do_pre_build(self, thebuilder):
        self.Logger = logging.getLogger("WindowsVsToolChain")
        interesting_keys = ["ExtensionSdkDir", "INCLUDE", "LIB", "LIBPATH", "UniversalCRTSdkDir",
                            "UCRTVersion", "WindowsLibPath", "WindowsSdkBinPath", "WindowsSdkDir", "WindowsSdkVerBinPath",
                            "WindowsSDKVersion", "VCToolsInstallDir", "Path"]

        #
        # VS2017 - Follow VS2017 where there is potential for many versions of the tools.
        # If a specific version is required then the user must set both env variables:
        # VS150INSTALLPATH:  base install path on system to VC install dir.  Here you will find the VC folder, etc
        # VS150TOOLVER:      version number for the VC compiler tools
        # VS2017_PREFIX:     path to MSVC compiler folder with trailing slash (can be used instead of two vars above)
        # VS2017_HOST:       set the host architecture to use for host tools, and host libs, etc
        if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "VS2017":

            # check to see if host is configured
            # HostType for VS2017 should be (defined in tools_def):
            # x86   == 32bit Intel
            # x64   == 64bit Intel
            # arm   == 32bit Arm
            # arm64 == 64bit Arm
            #
            HostType = shell_environment.GetEnvironment().get_shell_var("VS2017_HOST")
            if HostType is not None:
                HostType = HostType.lower()
                self.Logger.info(
                    f"HOST TYPE defined by environment.  Host Type is {HostType}")
            else:
                HostInfo = GetHostInfo()
                if HostInfo.arch == "x86":
                    if HostInfo.bit == "32":
                        HostType = "x86"
                    elif HostInfo.bit == "64":
                        HostType = "x64"
                else:
                    raise NotImplementedError()

            # VS2017_HOST options are not exactly the same as QueryVcVariables. This translates.
            VC_HOST_ARCH_TRANSLATOR = {
                "x86": "x86", "x64": "AMD64", "arm": "not supported", "arm64": "not supported"}

            # check to see if full path already configured
            if shell_environment.GetEnvironment().get_shell_var("VS2017_PREFIX") != None:
                self.Logger.info("VS2017_PREFIX is already set.")

            else:
                install_path = self._get_vs_install_path(
                    "VS2017".lower(), "VS150INSTALLPATH")
                vc_ver = self._get_vc_version(install_path, "VS150TOOLVER")

                if install_path is None or vc_ver is None:
                    self.Logger.error(
                        "Failed to configure environment for VS2017")
                    return -1

                version_aggregator.GetVersionAggregator().ReportVersion(
                    "Visual Studio Install Path", install_path, version_aggregator.VersionTypes.INFO)
                version_aggregator.GetVersionAggregator().ReportVersion(
                    "VC Version", vc_ver, version_aggregator.VersionTypes.TOOL)

                # make VS2017_PREFIX to align with tools_def.txt
                prefix = os.path.join(install_path, "VC",
                                      "Tools", "MSVC", vc_ver)
                prefix = prefix + os.path.sep
                shell_environment.GetEnvironment().set_shell_var("VS2017_PREFIX", prefix)
                shell_environment.GetEnvironment().set_shell_var("VS2017_HOST", HostType)

                shell_env = shell_environment.GetEnvironment()
                # Use the tools lib to determine the correct values for the vars that interest us.
                vs_vars = locate_tools.QueryVcVariables(
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="vs2017")
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("VS2017_PREFIX")):
                self.Logger.error("Path for VS2017 toolchain is invalid")
                return -2

        #
        # VS2019 - Follow VS2019 where there is potential for many versions of the tools.
        # If a specific version is required then the user must set both env variables:
        # VS160INSTALLPATH:  base install path on system to VC install dir.  Here you will find the VC folder, etc
        # VS160TOOLVER:      version number for the VC compiler tools
        # VS2019_PREFIX:     path to MSVC compiler folder with trailing slash (can be used instead of two vars above)
        # VS2017_HOST:       set the host architecture to use for host tools, and host libs, etc
        elif thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "VS2019":

            # check to see if host is configured
            # HostType for VS2019 should be (defined in tools_def):
            # x86   == 32bit Intel
            # x64   == 64bit Intel
            # arm   == 32bit Arm
            # arm64 == 64bit Arm
            #
            HostType = shell_environment.GetEnvironment().get_shell_var("VS2019_HOST")
            if HostType is not None:
                HostType = HostType.lower()
                self.Logger.info(
                    f"HOST TYPE defined by environment.  Host Type is {HostType}")
            else:
                HostInfo = GetHostInfo()
                if HostInfo.arch == "x86":
                    if HostInfo.bit == "32":
                        HostType = "x86"
                    elif HostInfo.bit == "64":
                        HostType = "x64"
                else:
                    raise NotImplementedError()

            # VS2019_HOST options are not exactly the same as QueryVcVariables. This translates.
            VC_HOST_ARCH_TRANSLATOR = {
                "x86": "x86", "x64": "AMD64", "arm": "not supported", "arm64": "not supported"}

            # check to see if full path already configured
            if shell_environment.GetEnvironment().get_shell_var("VS2019_PREFIX") != None:
                self.Logger.info("VS2019_PREFIX is already set.")

            else:
                install_path = self._get_vs_install_path(
                    "VS2019".lower(), "VS160INSTALLPATH")
                vc_ver = self._get_vc_version(install_path, "VS160TOOLVER")

                if install_path is None or vc_ver is None:
                    self.Logger.error(
                        "Failed to configure environment for VS2019")
                    return -1

                version_aggregator.GetVersionAggregator().ReportVersion(
                    "Visual Studio Install Path", install_path, version_aggregator.VersionTypes.INFO)
                version_aggregator.GetVersionAggregator().ReportVersion(
                    "VC Version", vc_ver, version_aggregator.VersionTypes.TOOL)

                # make VS2019_PREFIX to align with tools_def.txt
                prefix = os.path.join(install_path, "VC",
                                      "Tools", "MSVC", vc_ver)
                prefix = prefix + os.path.sep
                shell_environment.GetEnvironment().set_shell_var("VS2019_PREFIX", prefix)
                shell_environment.GetEnvironment().set_shell_var("VS2019_HOST", HostType)

                shell_env = shell_environment.GetEnvironment()
                # Use the tools lib to determine the correct values for the vars that interest us.
                vs_vars = locate_tools.QueryVcVariables(
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="vs2019")
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("VS2019_PREFIX")):
                self.Logger.error("Path for VS2019 toolchain is invalid")
                return -2

        return 0

    def _get_vs_install_path(self, vs_version, varname):
        # check if already specified
        path = shell_environment.GetEnvironment().get_shell_var(varname)
        if(path is None):
            # Not specified...find latest
            (rc, path) = FindWithVsWhere(vs_version=vs_version)
            if rc == 0 and path is not None and os.path.exists(path):
                self.Logger.debug("Found VS instance for %s", vs_version)
            else:
                self.Logger.error(
                    "Failed to find VS instance with VsWhere (%d)" % rc)
        return path

    def _get_vc_version(self, path, varname):
        # check if already specified
        vc_ver = shell_environment.GetEnvironment().get_shell_var(varname)
        if (path is None):
            self.Logger.critical(
                "Failed to find Visual Studio tools.  Might need to check for VS install")
            return vc_ver
        if(vc_ver is None):
            # Not specified...find latest
            p2 = os.path.join(path, "VC", "Tools", "MSVC")
            if not os.path.isdir(p2):
                self.Logger.critical(
                    "Failed to find VC tools.  Might need to check for VS install")
                return vc_ver
            vc_ver = os.listdir(p2)[-1].strip()  # get last in list
            self.Logger.debug("Found VC Tool version is %s" % vc_ver)
        return vc_ver
