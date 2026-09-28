# @file WindowsVsToolChain.py
# Plugin to configure the environment for the VS2019, VS2022, and VS2026 toolchains
##
# This plugin works in conjuncture with the tools_def
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import os

from edk2toolext.environment import shell_environment, version_aggregator
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
from edk2toollib.utility_functions import GetHostInfo
from edk2toollib.windows import locate_tools
from edk2toollib.windows.locate_tools import FindWithVsWhere


class WindowsVsToolChain(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        return 0

    def do_pre_build(self, thebuilder):
        self.Logger = logging.getLogger("WindowsVsToolChain")
        interesting_keys = ["ExtensionSdkDir", "INCLUDE", "LIB", "LIBPATH", "UniversalCRTSdkDir",
                            "UCRTVersion", "WindowsLibPath", "WindowsSdkBinPath", "WindowsSdkDir", "WindowsSdkVerBinPath",
                            "WindowsSDKVersion", "WindowsSDKLibVersion", "VCToolsInstallDir", "Path"]

        #
        # VS2019 - Follow VS2019 where there is potential for many versions of the tools.
        # If a specific version is required then the user must set both env variables:
        # VS160INSTALLPATH:  base install path on system to VC install dir.  Here you will find the VC folder, etc
        # VS160TOOLVER:      version number for the VC compiler tools
        # VS2019_PREFIX:     path to MSVC compiler folder with trailing slash (can be used instead of two vars above)
        # VS2019_HOST:       set the host architecture to use for host tools, and host libs, etc
        if thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "VS2019":

            # check to see if host is configured
            # HostType for VS2019 should be (defined in tools_def):
            # x86   == 32bit Intel
            # x64   == 64bit Intel
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
                "x86": "x86", "x64": "AMD64", "arm64": "not supported"}

            # check to see if full path already configured
            if shell_environment.GetEnvironment().get_shell_var("VS2019_PREFIX") is not None:
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
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="vs2019", vc_version=vc_ver)
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("VS2019_PREFIX")):
                self.Logger.error("Path for VS2019 toolchain is invalid")
                return -2

        #
        # VS2022 - VS2022 allows a user to install many copies/versions of the tools.
        # If a specific version is required then the user must set both env variables:
        # VS170INSTALLPATH:  base install path on system to VC install dir.  Here you will find the VC folder, etc
        # VS170TOOLVER:      version number for the VC compiler tools
        # VS2022_PREFIX:     path to MSVC compiler folder with trailing slash (can be used instead of two vars above)
        # VS2022_HOST:       set the host architecture to use for host tools, and host libs, etc
        elif thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "VS2022":

            # check to see if host is configured
            # HostType for VS2022 should be (defined in tools_def):
            # x86   == 32bit Intel
            # x64   == 64bit Intel
            # arm64 == 64bit Arm
            #
            HostType = shell_environment.GetEnvironment().get_shell_var("VS2022_HOST")
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

            # VS2022_HOST options are not exactly the same as QueryVcVariables. This translates.
            VC_HOST_ARCH_TRANSLATOR = {
                "x86": "x86", "x64": "AMD64", "arm64": "not supported"}

            # check to see if full path already configured
            if shell_environment.GetEnvironment().get_shell_var("VS2022_PREFIX") is not None:
                self.Logger.debug("VS2022_PREFIX is already set.")

            else:
                install_path = self._get_vs_install_path(
                    "VS2022".lower(), "VS170INSTALLPATH")
                vc_ver = self._get_vc_version(install_path, "VS170TOOLVER")

                if install_path is None or vc_ver is None:
                    self.Logger.error(
                        "Failed to configure environment for VS2022")
                    return -1

                version_aggregator.GetVersionAggregator().ReportVersion(
                    "Visual Studio Install Path", install_path, version_aggregator.VersionTypes.INFO)
                version_aggregator.GetVersionAggregator().ReportVersion(
                    "VC Version", vc_ver, version_aggregator.VersionTypes.TOOL)

                # make VS2022_PREFIX to align with tools_def.txt
                prefix = os.path.join(install_path, "VC",
                                      "Tools", "MSVC", vc_ver)
                prefix = prefix + os.path.sep
                shell_environment.GetEnvironment().set_shell_var("VS2022_PREFIX", prefix)
                shell_environment.GetEnvironment().set_shell_var("VS2022_HOST", HostType)

                shell_env = shell_environment.GetEnvironment()
                # Use the tools lib to determine the correct values for the vars that interest us.
                vs_vars = locate_tools.QueryVcVariables(
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="VS2022", vc_version=vc_ver)
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("VS2022_PREFIX")):
                self.Logger.error("Path for VS2022 toolchain is invalid")
                return -2

        #
        # VS2026 - VS2026 allows a user to install many copies/versions of the tools.
        # If a specific version is required then the user must set both env variables:
        # VS170INSTALLPATH:  base install path on system to VC install dir.  Here you will find the VC folder, etc
        # VS170TOOLVER:      version number for the VC compiler tools
        # VS2026_PREFIX:     path to MSVC compiler folder with trailing slash (can be used instead of two vars above)
        # VS2026_HOST:       set the host architecture to use for host tools, and host libs, etc
        elif thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "VS2026":

            # check to see if host is configured
            # HostType for VS2026 should be (defined in tools_def):
            # x86   == 32bit Intel
            # x64   == 64bit Intel
            # arm64 == 64bit Arm
            #
            HostType = shell_environment.GetEnvironment().get_shell_var("VS2026_HOST")
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

            # VS2026_HOST options are not exactly the same as QueryVcVariables. This translates.
            VC_HOST_ARCH_TRANSLATOR = {
                "x86": "x86", "x64": "AMD64", "arm64": "not supported"}

            # check to see if full path already configured
            if shell_environment.GetEnvironment().get_shell_var("VS2026_PREFIX") is not None:
                self.Logger.debug("VS2026_PREFIX is already set.")

            else:
                install_path = self._get_vs_install_path(
                    "VS2026".lower(), "VS180INSTALLPATH")
                vc_ver = self._get_vc_version(install_path, "VS180TOOLVER")

                if install_path is None or vc_ver is None:
                    self.Logger.error(
                        "Failed to configure environment for VS2026")
                    return -1

                version_aggregator.GetVersionAggregator().ReportVersion(
                    "Visual Studio Install Path", install_path, version_aggregator.VersionTypes.INFO)
                version_aggregator.GetVersionAggregator().ReportVersion(
                    "VC Version", vc_ver, version_aggregator.VersionTypes.TOOL)

                # make VS2026_PREFIX to align with tools_def.txt
                prefix = os.path.join(install_path, "VC",
                                      "Tools", "MSVC", vc_ver)
                prefix = prefix + os.path.sep
                shell_environment.GetEnvironment().set_shell_var("VS2026_PREFIX", prefix)
                shell_environment.GetEnvironment().set_shell_var("VS2026_HOST", HostType)

                shell_env = shell_environment.GetEnvironment()
                # Use the tools lib to determine the correct values for the vars that interest us.
                vs_vars = locate_tools.QueryVcVariables(
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="VS2026", vc_version=vc_ver)
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("VS2026_PREFIX")):
                self.Logger.error("Path for VS2026 toolchain is invalid")
                return -2

        return 0

    def _get_vs_install_path(self, vs_version, varname):
        # check if already specified
        path = None
        if varname is not None:
            path = shell_environment.GetEnvironment().get_shell_var(varname)

        if(path is None):
            # Not specified...find latest
            try:
                path = FindWithVsWhere(vs_version=vs_version)
            except (OSError, ValueError, RuntimeError) as e:
                self.Logger.error(str(e))
                return None

            if path is not None and os.path.exists(path):
                self.Logger.debug("Found VS instance for %s", vs_version)
            else:
                self.Logger.error(
                    f"VsWhere successfully executed, but could not find VS instance for {vs_version}.")
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
            dirs = os.listdir(p2)
            if len(dirs) > 1:
                self.Logger.debug(f"Multiple VC versions found: [{', '.join(dirs)}]. Using {dirs[-1]}")
            vc_ver = dirs[-1].strip()  # get last in list
            self.Logger.debug(f"Found VC Tool version is {vc_ver}")
        return vc_ver
