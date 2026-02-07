# @file WindowsVsToolChain.py
# Plugin to configure the environment for the VS2017, VS2019, VS2022, and VS2026 toolchains
#
# This plugin also runs for CLANGPDB toolchain on Windows as that toolchain
# leverages nmake from VS and needs to the SDK paths for unit tests
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
                "x86": "x86", "x64": "AMD64", "arm64": "not supported"}

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
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="VS2022")
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
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType], vs_version="VS2026")
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

            # now confirm it exists
            if not os.path.exists(shell_environment.GetEnvironment().get_shell_var("VS2026_PREFIX")):
                self.Logger.error("Path for VS2026 toolchain is invalid")
                return -2

        #
        # CLANGPDB on Windows uses nmake from
        # the VS compiler toolchain.   Find a version and set
        # as the CLANG_HOST_BIN path if not already set.
        #
        # Also get the platform header files, SDK, etc based on the
        # host type.  This is used for unit test compilation.
        # If CLANG_VS_HOST is not set then find the host type based on Host Info.
        ##
        elif thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "CLANGPDB":

            #
            # Look for $(CLANG_BIN)/mingw32-make.exe to determine if we are in a
            # mingw32 environment
            #
            clang_bin = shell_environment.GetEnvironment().get_shell_var("CLANG_BIN")
            is_mingw32_env = (clang_bin and os.path.exists(os.path.join(clang_bin, "mingw32-make.exe")))
            if is_mingw32_env:
                self.Logger.debug("CLANG_BIN is set to a mingw32 toolchain.")

                #
                # CLANGPDB requires lld-link.exe to generate PE/COFF images. If
                # the mingw32 environment installed does not have lld-link.exe
                # then error out.
                #
                if not os.path.exists(os.path.join(clang_bin, "lld-link.exe")):
                    self.Logger.error("CLANG_BIN is set to a mingw32 toolchain but lld-link.exe not found.")
                    return -1

                #
                # Set environment variables for mingw32 CLANGPDB environment
                #
                basetools_mingw_build = shell_environment.GetEnvironment().get_shell_var("BASETOOLS_MINGW_BUILD") or 'TRUE'
                if basetools_mingw_build.upper() != 'TRUE':
                    self.Logger.error("BASETOOLS_MINGW_BUILD must be set to TRUE for CLANGPDB toolchain.")
                    return -1
                shell_environment.GetEnvironment().set_shell_var("BASETOOLS_MINGW_BUILD", basetools_mingw_build)

                clang_host_bin = shell_environment.GetEnvironment().get_shell_var("CLANG_HOST_BIN")
                if clang_host_bin:
                    self.Logger.debug("CLANG_HOST_BIN is already set.")
                else:
                    # The environment is mingw32 make add "mingw32-" to the end of the path.
                    # The rest of the command is derived from definitions in tools.def.
                    clang_host_bin = 'mingw32-'
                shell_environment.GetEnvironment().set_shell_var("CLANG_HOST_BIN", clang_host_bin)

            else:
                HostInfo = GetHostInfo()

                # check to see if host is configured
                # HostType for VS tools should be (defined in tools_def):
                # x86   == 32bit Intel
                # x64   == 64bit Intel
                # arm64 == 64bit Arm
                #
                HostType = shell_environment.GetEnvironment().get_shell_var("CLANG_VS_HOST")
                if HostType:
                    HostType = HostType.lower()
                    self.Logger.info(
                        f"CLANG_VS_HOST defined by environment.  Value is {HostType}")
                else:
                    #figure it out based on host info
                    if HostInfo.arch == "x86":
                        if HostInfo.bit == "32":
                            HostType = "x86"
                        elif HostInfo.bit == "64":
                            HostType = "x64"
                    else:
                        # anything other than x86 or x64 is not supported
                        self.Logger.error(f"CLANGPDB not supported for detected host [{HostType.arch}-{HostType.bit}]")
                        return -1

                # CLANG_VS_HOST options are not exactly the same as QueryVcVariables. This translates.
                VC_HOST_ARCH_TRANSLATOR = {
                    "x86": "x86", "x64": "AMD64", "arm64": "not supported"}

                # now get the environment variables for the platform
                shell_env = shell_environment.GetEnvironment()
                # Use the tools lib to determine the correct values for the vars that interest us.
                vs_vars = locate_tools.QueryVcVariables(
                    interesting_keys, VC_HOST_ARCH_TRANSLATOR[HostType])
                for (k, v) in vs_vars.items():
                    shell_env.set_shell_var(k, v)

                ##
                # If environment already has CLANG_HOST_BIN set then user has already
                # set the path to the mingw32 make utility
                ##
                if shell_environment.GetEnvironment().get_shell_var("CLANG_HOST_BIN") is not None:
                    self.Logger.debug("CLANG_HOST_BIN is already set.")

                else:
                    install_path = self._get_vs_install_path(None, None)
                    vc_ver = self._get_vc_version(install_path, None)

                    if install_path is None or vc_ver is None:
                        self.Logger.error("Failed to configure environment for VS")
                        return -1

                    version_aggregator.GetVersionAggregator().ReportVersion(
                        "Visual Studio Install Path", install_path, version_aggregator.VersionTypes.INFO)
                    version_aggregator.GetVersionAggregator().ReportVersion(
                        "VC Version", vc_ver, version_aggregator.VersionTypes.TOOL)

                    # make path align with tools_def.txt
                    prefix = os.path.join(install_path, "VC", "Tools", "MSVC", vc_ver)
                    clang_host_bin_prefix = os.path.join(prefix, "bin", "Host%s" % HostType, HostType)

                    # now confirm it exists
                    if not os.path.exists(clang_host_bin_prefix):
                        self.Logger.error("Path for VS toolchain is invalid")
                        return -2

                    # The environment is using nmake (not make) so add "n" to the end of the path.
                    # The rest of the command is derived from definitions in tools.def.
                    shell_environment.GetEnvironment().set_shell_var("CLANG_HOST_BIN", os.path.join(clang_host_bin_prefix, "n"))
        elif thebuilder.env.GetValue("TOOL_CHAIN_TAG") == "CLANGDWARF":
            ##
            # If environment already has CLANG_HOST_BIN set then user has already
            # set the path to the mingw32 make utility
            ##

            basetools_mingw_build = shell_environment.GetEnvironment().get_shell_var("BASETOOLS_MINGW_BUILD") or 'TRUE'
            if basetools_mingw_build.upper() != 'TRUE':
                self.Logger.error("BASETOOLS_MINGW_BUILD must be set to TRUE for CLANGDWARF toolchain.")
                return -1
            shell_environment.GetEnvironment().set_shell_var("BASETOOLS_MINGW_BUILD", basetools_mingw_build)

            clang_host_bin = shell_environment.GetEnvironment().get_shell_var("CLANG_HOST_BIN")
            if clang_host_bin:
                self.Logger.debug("CLANG_HOST_BIN is already set.")
            else:
                # The environment is mingw32 make add "mingw32-" to the end of the path.
                # The rest of the command is derived from definitions in tools.def.
                clang_host_bin = 'mingw32-'
            shell_environment.GetEnvironment().set_shell_var("CLANG_HOST_BIN", clang_host_bin)

            clang_bin = shell_environment.GetEnvironment().get_shell_var("CLANG_BIN")
            if clang_bin:
                self.Logger.debug("CLANG_BIN is already set.")
            else:
                # Assume default installation "c:\edk2-clang\bin\"
                clang_bin = "c:\\edk2-clang\\bin\\"

            # now confirm it exists
            if not os.path.exists(os.path.join(clang_bin, clang_host_bin + "make.exe")):
                self.Logger.error(f"mingw32 toolchain not found in CLANG_BIN path {clang_bin}")
                return -2

            shell_environment.GetEnvironment().set_shell_var("CLANG_BIN", clang_bin)

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
            except (EnvironmentError, ValueError, RuntimeError) as e:
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
            vc_ver = os.listdir(p2)[-1].strip()  # get last in list
            self.Logger.debug("Found VC Tool version is %s" % vc_ver)
        return vc_ver
