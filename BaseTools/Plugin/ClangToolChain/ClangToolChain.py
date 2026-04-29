# @file ClangToolChain.py
# Plugin to configures paths for the Clang tool chain
##
# This plugin works in conjunction with the tools_def
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
import shutil
from pathlib import Path
from io import StringIO
from edk2toolext.environment.plugintypes.uefi_build_plugin import (
    IUefiBuildPlugin,
)
from edk2toolext.environment import shell_environment
from edk2toolext.environment import version_aggregator
from edk2toollib.utility_functions import GetHostInfo
from edk2toollib.utility_functions import RunCmd

class ClangToolChain(IUefiBuildPlugin):

    def do_post_build(self, thebuilder):
        """No-op post-build hook required by IUefiBuildPlugin."""
        return 0

    def do_pre_build(self, thebuilder):
        """Configure CLANG_BIN and report tool versions for CLANGPDB/CLANGDWARF.

        Resolves the clang binary in this order:
          1. CLANG_BIN environment variable
          2. System PATH
          3. Default LLVM install path (Windows)
          4. VS-installed clang (Windows only)

        Returns 0 on success, 1 if clang cannot be found.
        """
        self.Logger = logging.getLogger("ClangPdbToolChain")
        ##
        # CLANGPDB/CLANGDWARF
        # - Need to find the clang path.
        # - Report path and version for logging
        #
        # if CLANG_BIN already set the plugin will confirm it exists and
        # get the version of clang
        # If not set it will look for clang on the path.  If found it will
        # configure for that.
        # if still not found it will try the default install directory.
        # finally an error will be reported if not found
        ##
        tool_chain_tag = thebuilder.env.GetValue("TOOL_CHAIN_TAG")
        if tool_chain_tag in ("CLANGPDB", "CLANGDWARF"):
            HostInfo = GetHostInfo()
            clang_bin_env = shell_environment.GetEnvironment().get_shell_var(
                "CLANG_BIN"
            )
            ClangBin = Path(clang_bin_env, "clang") if clang_bin_env else None

            # 1. Check CLANG_BIN from the environment
            if not ClangBin or not ClangBin.is_file():
                # 2. CLANG_BIN didn't point to a valid directory.
                # Check system path.
                ClangBin = shutil.which("clang")
                if ClangBin:
                    shell_environment.GetEnvironment().set_shell_var(
                        "CLANG_BIN",
                        Path(ClangBin).parent.as_posix() + "/",
                    )
                elif HostInfo.os == "Windows":
                    # 3. Check Windows Llvm Install Path.
                    ClangBin = Path("C:/Program Files/LLVM/bin/clang")
                    if ClangBin.is_file():
                        shell_environment.GetEnvironment().set_shell_var(
                            "CLANG_BIN",
                            ClangBin.parent.as_posix() + "/",
                        )
                    else:
                        # 4.Finally try MSVC LLVM install
                        vs_clang_path = (
                            self._configure_windows_vs_host_for_clang(
                                HostInfo
                            )
                        )
                        if vs_clang_path and vs_clang_path.is_file():
                            ClangBin = vs_clang_path
            if ClangBin and Path(ClangBin).is_file():
                clang_bin_parent = Path(ClangBin).parent
                shell_environment.GetEnvironment().set_shell_var(
                    "CLANG_BIN",
                    clang_bin_parent.as_posix() + "/",
                )
                version_aggregator.GetVersionAggregator().ReportVersion(
                    "CLANG_BIN",
                    str(ClangBin),
                    version_aggregator.VersionTypes.INFO,
                )
                clang_version = self._get_clang_version(clang_bin_parent)
                if clang_version:
                    version_aggregator.GetVersionAggregator().ReportVersion(
                        "CLANG",
                        clang_version,
                        version_aggregator.VersionTypes.TOOL,
                    )
                else:
                    self.Logger.warning("Could not determine clang version")

                # Report the linker version based on toolchain
                if tool_chain_tag == "CLANGPDB":
                    lld_version = self._get_linker_version(clang_bin_parent, "lld-link")
                    if lld_version:
                        version_aggregator.GetVersionAggregator().ReportVersion(
                            "lld-link",
                            lld_version,
                            version_aggregator.VersionTypes.TOOL,
                        )
                    else:
                        self.Logger.warning("Could not determine lld-link version")
                elif tool_chain_tag == "CLANGDWARF":
                    lld_version = self._get_linker_version(clang_bin_parent, "lld")
                    if lld_version:
                        version_aggregator.GetVersionAggregator().ReportVersion(
                            "lld",
                            lld_version,
                            version_aggregator.VersionTypes.TOOL,
                        )
                    else:
                        self.Logger.warning("Could not determine lld version")
            else:
                self.Logger.error("Could not find clang executable.")
                return 1

        return 0

    def _configure_windows_vs_host_for_clang(self, host_info):
        """Configure the VS host environment and locate the VS-bundled clang.

        Sets CLANG_HOST_BIN and relevant VS shell variables, then searches the
        VS LLVM install tree for clang.exe.

        Args:
            host_info: HostInfo object from GetHostInfo().

        Returns:
            Path to clang.exe if found, otherwise None.
        """
        import edk2toollib.windows.locate_tools as locate_tools

        interesting_keys = [
            "ExtensionSdkDir",
            "INCLUDE",
            "LIB",
            "LIBPATH",
            "UniversalCRTSdkDir",
            "UCRTVersion",
            "WindowsLibPath",
            "WindowsSdkBinPath",
            "WindowsSdkDir",
            "WindowsSdkVerBinPath",
            "WindowsSDKVersion",
            "VCToolsInstallDir",
            "Path",
        ]

        # check to see if host is configured
        # HostType for VS tools should be (defined in tools_def):
        # x86   == 32bit Intel
        # x64   == 64bit Intel
        # arm64 == 64bit Arm
        host_type = shell_environment.GetEnvironment().get_shell_var(
            "CLANG_VS_HOST"
        )
        if host_type is not None:
            host_type = host_type.lower()
            self.Logger.info(
                f"CLANG_VS_HOST defined by environment.  Value is {host_type}"
            )
        else:
            # figure it out based on host info
            if host_info.arch == "x86":
                if host_info.bit == "32":
                    host_type = "x86"
                elif host_info.bit == "64":
                    host_type = "x64"
            else:
                # anything other than x86 or x64 is not supported
                raise NotImplementedError()

        # CLANG_VS_HOST options are not exactly the same as QueryVcVariables.
        # This translates.
        vc_host_arch_translator = {
            "x86": "x86",
            "x64": "AMD64",
            "arm64": "not supported",
        }

        # now get the environment variables for the platform
        shell_env = shell_environment.GetEnvironment()
        # Use the tools lib to determine the correct values for the vars
        # that interest us.
        vs_vars = locate_tools.QueryVcVariables(
            interesting_keys, vc_host_arch_translator[host_type]
        )
        for k, v in vs_vars.items():
            shell_env.set_shell_var(k, v)

        # If environment already has CLANG_HOST_BIN set then user has already
        # set the path to the VS tools like nmake.exe.
        if (
            shell_environment.GetEnvironment().get_shell_var("CLANG_HOST_BIN")
            is not None
        ):
            self.Logger.debug("CLANG_HOST_BIN is already set.")
            clang_bin = shell_environment.GetEnvironment().get_shell_var(
                "CLANG_BIN"
            )
            if clang_bin:
                clang_path = Path(clang_bin) / "clang.exe"
                return clang_path if clang_path.is_file() else None
            return None

        install_path = self._get_vs_install_path(None, None)
        vc_ver = self._get_vc_version(install_path, None)

        if install_path is None or vc_ver is None:
            self.Logger.error("Failed to configure environment for VS")
            return None

        version_aggregator.GetVersionAggregator().ReportVersion(
            "Visual Studio Install Path",
            install_path,
            version_aggregator.VersionTypes.INFO,
        )
        version_aggregator.GetVersionAggregator().ReportVersion(
            "VC Version", vc_ver, version_aggregator.VersionTypes.TOOL
        )

        prefix = Path(install_path, "VC", "Tools", "MSVC", vc_ver)
        clang_host_bin_prefix = (
            prefix / "bin" / f"Host{host_type}" / host_type
        )
        self.Logger.info("VS host tools path: %s", clang_host_bin_prefix)

        # now confirm it exists
        if not clang_host_bin_prefix.exists():
            self.Logger.error("Path for VS toolchain is invalid")
            return None

        # The environment is using nmake (not make) so add "n" to the end
        # of the path. The rest of the command is derived from definitions
        # in tools.def.
        shell_environment.GetEnvironment().set_shell_var(
            "CLANG_HOST_BIN", str(clang_host_bin_prefix / "n")
        )

        # VS 2019+ installs 64-bit clang under Llvm\x64\bin; older/32-bit
        # installs use Llvm\bin. Try the arch-specific path first.
        llvm_base = Path(install_path, "VC", "Tools", "Llvm")
        self.Logger.info("Searching for clang under: %s", llvm_base)
        for candidate in (
            Path(llvm_base, host_type, "bin", "clang.exe"),
            Path(llvm_base, "bin", "clang.exe"),
        ):
            self.Logger.debug("Checking candidate: %s", candidate)
            if candidate.is_file():
                self.Logger.info("Selected VS clang path: %s", candidate)
                return candidate
        self.Logger.error(
            "Could not find clang.exe under %s", llvm_base
        )
        return None

    def _get_vs_install_path(self, vs_version, varname):
        """Return the Visual Studio install path.

        Checks the shell variable *varname* first; falls back to vswhere.

        Args:
            vs_version: VS version string passed to FindWithVsWhere (e.g. "vs2022").
            varname: Shell variable name to check first, or None.

        Returns:
            Install path string, or None if not found.
        """
        from edk2toollib.windows.locate_tools import FindWithVsWhere

        # check if already specified
        path = None
        if varname is not None:
            path = shell_environment.GetEnvironment().get_shell_var(varname)

        if path is None:
            # Not specified...find latest
            try:
                path = FindWithVsWhere(vs_version=vs_version)
            except (EnvironmentError, ValueError, RuntimeError) as e:
                self.Logger.error(str(e))
                return None

            if path is not None and Path(path).exists():
                self.Logger.debug("Found VS instance for %s", vs_version)
            else:
                self.Logger.error(
                    "VsWhere successfully executed, but could not find "
                    f"VS instance for {vs_version}."
                )
        return path

    def _get_vc_version(self, path, varname):
        """Return the VC tools version string.

        Checks the shell variable *varname* first; falls back to scanning
        the MSVC tools directory under *path*.

        Args:
            path: VS install path string, or None.
            varname: Shell variable name to check first, or None.

        Returns:
            VC tools version string (e.g. "14.38.33130"), or None if not found.
        """
        # check if already specified
        vc_ver = None
        if varname is not None:
            vc_ver = shell_environment.GetEnvironment().get_shell_var(varname)

        if path is None:
            self.Logger.critical(
                "Failed to find Visual Studio tools. "
                " Might need to check for VS install"
            )
            return vc_ver

        if vc_ver is None:
            # Not specified...find latest
            vc_tools_path = Path(path) / "VC" / "Tools" / "MSVC"
            if not vc_tools_path.is_dir():
                self.Logger.critical(
                    "Failed to find VC tools. "
                    " Might need to check for VS install"
                )
                return vc_ver

            dirs = [
                entry.name
                for entry in vc_tools_path.iterdir()
                if entry.is_dir()
            ]
            if not dirs:
                self.Logger.critical(
                    "Failed to find VC tools version directories. "
                    " Might need to check for VS install"
                )
                return vc_ver
            if len(dirs) > 1:
                logging.warning(
                    f"Multiple VC versions found: [{', '.join(dirs)}]."
                    f" Using {dirs[-1]}"
                )
            vc_ver = dirs[-1].strip()  # get last in list
            self.Logger.debug("Found VC Tool version is %s", vc_ver)

        return vc_ver

    def _get_linker_version(self, clang_bin_path, linker_name):
        """Return the version string for an LLD-based linker.

        Runs ``<linker> --version`` and parses the ``LLD X.Y.Z`` prefix.
        Handles lld-link returning a non-zero exit code for ``--version``.

        Args:
            clang_bin_path: Directory containing the linker binary.
            linker_name: Binary name without extension (``"lld"`` or ``"lld-link"``).

        Returns:
            Version string (e.g. ``"18.1.8"``), or None if parsing fails.
        """
        return_buffer = StringIO()
        linker_path = Path(clang_bin_path) / linker_name
        if not linker_path.exists():
            linker_path = linker_path.with_suffix(".exe")
        ret = RunCmd(str(linker_path), "--version", outstream=return_buffer)
        # lld-link --version may return non-zero; treat output as valid if present
        output = return_buffer.getvalue()
        lines = output.splitlines()
        if not lines:
            self.Logger.warning("%s --version produced no output (exit %d)", linker_name, ret)
            return None
        line = lines[0].strip()
        # Output is typically "LLD X.Y.Z (compatible with GNU linkers)" etc.
        marker = "LLD "
        idx = line.find(marker)
        if idx >= 0:
            # Take everything up to the first space or paren after the version
            version_str = line[idx + len(marker):].split()[0].rstrip(",)")
            return version_str
        self.Logger.warning("Unexpected %s --version format: %r", linker_name, line)
        return line

    def _get_clang_version(self, clang_bin_path):
        """Return the clang version string.

        Runs ``clang --version`` and parses the ``clang version X.Y.Z`` line.
        Handles both upstream (``clang version X.Y.Z``) and distro-prefixed
        (``Ubuntu clang version X.Y.Z``) output formats.

        Args:
            clang_bin_path: Directory containing the clang binary.

        Returns:
            Version string (e.g. ``"18.1.8"``), or None if parsing fails.
        """
        return_buffer = StringIO()
        clang_path = Path(clang_bin_path) / "clang"
        if not clang_path.exists():
            clang_path = clang_path.with_suffix(".exe")
        ret = RunCmd(str(clang_path), "--version", outstream=return_buffer)
        if ret != 0:
            self.Logger.warning("clang --version exited with code %d", ret)
            return None
        output = return_buffer.getvalue()
        lines = output.splitlines()
        if not lines:
            self.Logger.warning("clang --version produced no output")
            return None
        line = lines[0].strip()
        # Output is typically "clang version X.Y.Z" or "Ubuntu clang version X.Y.Z"
        marker = "clang version "
        idx = line.find(marker)
        if idx >= 0:
            return line[idx + len(marker):].strip()
        self.Logger.warning("Unexpected clang --version format: %r", line)
        return line
