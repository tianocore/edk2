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
from io import StringIO
from pathlib import Path

from edk2toolext.environment import shell_environment, version_aggregator
from edk2toolext.environment.plugintypes.uefi_build_plugin import (
    IUefiBuildPlugin,
)
from edk2toollib.utility_functions import GetHostInfo, RunCmd
from edk2toollib.windows import locate_tools
from edk2toollib.windows.locate_tools import FindWithVsWhere

SUPPORTED_TOOL_CHAINS = ("CLANGPDB", "CLANGDWARF")

# VS environment variables needed to configure SDK/include/lib paths for
# both CLANGPDB (nmake host) and VS-bundled clang resolution.
_VS_INTERESTING_KEYS = [
    "ExtensionSdkDir", "INCLUDE", "LIB", "LIBPATH", "UniversalCRTSdkDir",
    "UCRTVersion", "WindowsLibPath", "WindowsSdkBinPath", "WindowsSdkDir",
    "WindowsSdkVerBinPath", "WindowsSDKVersion", "WindowsSDKLibVersion",
    "VCToolsInstallDir", "Path",
]

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
        if tool_chain_tag.upper() not in SUPPORTED_TOOL_CHAINS:
            return 0

        HostInfo = GetHostInfo()

        #
        # Windows-specific host build tool setup for CLANGPDB and CLANGDWARF.
        # CLANGPDB on Windows uses nmake from the VS compiler toolchain or
        # mingw32-make. CLANGDWARF always uses mingw32-make.
        #
        if HostInfo.os == "Windows":
            env = shell_environment.GetEnvironment()

            if tool_chain_tag == "CLANGPDB":
                # Look for $(CLANG_BIN)/mingw32-make.exe to determine if in a mingw32 environment
                clang_bin_str = env.get_shell_var("CLANG_BIN")
                clang_bin_path = Path(clang_bin_str) if clang_bin_str else None
                if clang_bin_path and (clang_bin_path / "mingw32-make.exe").exists():
                    self.Logger.debug("CLANG_BIN is set to a mingw32 toolchain.")
                    # CLANGPDB requires lld-link.exe to generate PE/COFF images.
                    if not (clang_bin_path / "lld-link.exe").exists():
                        self.Logger.error("CLANG_BIN is set to a mingw32 toolchain but lld-link.exe not found.")
                        return -1
                    if self._configure_mingw32_host("CLANGPDB") is None:
                        return -1

                else:
                    # VS nmake path: resolve host type and query VS environment variables.
                    HostType = self._apply_vs_vars(HostInfo)
                    if not HostType:
                        self.Logger.error(f"CLANGPDB not supported for detected host [{HostInfo.arch}-{HostInfo.bit}]")
                        return -1

                    # Add path to IA32 toolchain DLLs for unit test execution
                    if vc_tools_install_dir := env.get_shell_var("VCToolsInstallDir"):
                        ia32_dll_path = Path(vc_tools_install_dir) / 'bin' / 'Hostx64' / 'x86'
                        if ia32_dll_path.exists():
                            env.append_path(str(ia32_dll_path))
                        else:
                            self.Logger.warning("IA32 toolchain DLLs not found in expected path %s. IA32 unit tests may fail to run." % ia32_dll_path)

                    if env.get_shell_var("CLANG_HOST_BIN") is not None:
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
                        clang_host_bin_prefix = Path(install_path, "VC", "Tools", "MSVC", vc_ver, "bin", f"Host{HostType}", HostType)
                        if not clang_host_bin_prefix.exists():
                            self.Logger.error("Path for VS toolchain is invalid")
                            return -2
                        # The environment is using nmake (not make) so add "n" to the end of the path.
                        # The rest of the command is derived from definitions in tools.def.
                        env.set_shell_var("CLANG_HOST_BIN", str(clang_host_bin_prefix / "n"))

            elif tool_chain_tag == "CLANGDWARF":
                clang_host_bin = self._configure_mingw32_host("CLANGDWARF")
                if clang_host_bin is None:
                    return -1
                clang_bin_str = env.get_shell_var("CLANG_BIN")
                if clang_bin_str:
                    self.Logger.debug("CLANG_BIN is already set.")
                else:
                    clang_bin_str = "c:\\edk2-clang\\bin\\"
                if not (Path(clang_bin_str) / (clang_host_bin + "make.exe")).exists():
                    self.Logger.error(f"mingw32 toolchain not found in CLANG_BIN path {clang_bin_str}")
                    return -2
                env.set_shell_var("CLANG_BIN", clang_bin_str)

        ClangBin = self._resolve_clang_bin(HostInfo)
        if ClangBin is None:
            return 1

        clang_bin_parent = ClangBin.parent
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
        linker_name = {"CLANGPDB": "lld-link", "CLANGDWARF": "lld"}.get(tool_chain_tag)
        if linker_name:
            lld_version = self._get_linker_version(clang_bin_parent, linker_name)
            if lld_version:
                version_aggregator.GetVersionAggregator().ReportVersion(
                    linker_name,
                    lld_version,
                    version_aggregator.VersionTypes.TOOL,
                )
            else:
                self.Logger.warning(f"Could not determine {linker_name} version")

        return 0

    def _resolve_clang_bin(self, host_info):
        """Locate the clang executable, trying in order:
          1. CLANG_BIN environment variable (validated if set)
          2. System PATH
          3. Default LLVM install path (Windows)
          4. VS-installed clang (Windows only)

        Sets CLANG_BIN in the environment when resolved via steps 2-4.
        Returns a Path to the clang binary, or None if not found (errors are logged).
        """
        env = shell_environment.GetEnvironment()
        clang_exe = "clang.exe" if host_info.os == "Windows" else "clang"
        clang_bin_env = env.get_shell_var("CLANG_BIN")

        # 1. Check CLANG_BIN from the environment
        ClangBin = Path(clang_bin_env, clang_exe) if clang_bin_env else None
        if ClangBin and not ClangBin.is_file():
            self.Logger.error(
                "CLANG_BIN is set to '%s' but '%s' was not found.",
                clang_bin_env,
                ClangBin,
            )
            return None
        # 2. CLANG_BIN was not set; check system path.
        if not ClangBin:
            ClangBin = Path(w).resolve() if (w := shutil.which("clang")) else None
        # 3. Check Windows LLVM install path.
        if not ClangBin and host_info.os == "Windows":
            ClangBin = Path("C:/Program Files/LLVM/bin/clang.exe")
        # 4. Finally try MSVC LLVM install.
        if (not ClangBin or not ClangBin.is_file()) and host_info.os == "Windows":
            vs_clang_path = self._configure_windows_vs_host_for_clang(host_info)
            ClangBin = vs_clang_path if vs_clang_path and vs_clang_path.is_file() else None

        if not (ClangBin and ClangBin.is_file()):
            self.Logger.error("Could not find clang executable.")
            return None

        # If CLANG_BIN was not originally set, write it back from the discovered path.
        if not clang_bin_env:
            env.set_shell_var("CLANG_BIN", ClangBin.parent.as_posix() + "/")

        return ClangBin

    def _configure_mingw32_host(self, tag):
        """Validate BASETOOLS_MINGW_BUILD and resolve CLANG_HOST_BIN for a mingw32 environment.

        Returns the CLANG_HOST_BIN prefix string, or None on validation error.
        """
        env = shell_environment.GetEnvironment()
        basetools_mingw_build = env.get_shell_var("BASETOOLS_MINGW_BUILD") or 'TRUE'
        if basetools_mingw_build.upper() != 'TRUE':
            self.Logger.error(f"BASETOOLS_MINGW_BUILD must be set to TRUE for {tag} toolchain.")
            return None
        env.set_shell_var("BASETOOLS_MINGW_BUILD", basetools_mingw_build)

        clang_host_bin = env.get_shell_var("CLANG_HOST_BIN")
        if clang_host_bin:
            self.Logger.debug("CLANG_HOST_BIN is already set.")
        else:
            # The environment is mingw32 make; add "mingw32-" prefix.
            # The rest of the command is derived from definitions in tools.def.
            clang_host_bin = 'mingw32-'
        env.set_shell_var("CLANG_HOST_BIN", clang_host_bin)
        return clang_host_bin

    def _apply_vs_vars(self, host_info):
        """Resolve the VS host type, query VS env variables, and apply them to the shell environment.

        Returns the resolved host type string (e.g. 'x86', 'x64'), or None if unsupported.
        """
        env = shell_environment.GetEnvironment()
        host_type = env.get_shell_var("CLANG_VS_HOST")
        if host_type:
            host_type = host_type.lower()
            self.Logger.info(f"CLANG_VS_HOST defined by environment.  Value is {host_type}")
        elif host_info.arch == "x86":
            host_type = "x86" if host_info.bit == "32" else "x64" if host_info.bit == "64" else None
        if not host_type:
            return None
        # CLANG_VS_HOST options are not exactly the same as QueryVcVariables. This translates.
        vc_host_arch_translator = {"x86": "x86", "x64": "AMD64", "arm64": "not supported"}
        vs_vars = locate_tools.QueryVcVariables(_VS_INTERESTING_KEYS, vc_host_arch_translator[host_type])
        for k, v in vs_vars.items():
            env.set_shell_var(k, v)
        return host_type

    def _configure_windows_vs_host_for_clang(self, host_info):
        """Configure the VS host environment and locate the VS-bundled clang.

        Sets CLANG_HOST_BIN and relevant VS shell variables, then searches the
        VS LLVM install tree for clang.exe.

        Args:
            host_info: HostInfo object from GetHostInfo().

        Returns:
            Path to clang.exe if found, otherwise None.
        """

        host_type = self._apply_vs_vars(host_info)
        if not host_type:
            self.Logger.error("VS host type could not be determined; unsupported host architecture.")
            return None

        env = shell_environment.GetEnvironment()
        # If environment already has CLANG_HOST_BIN set then user has already
        # set the path to the VS tools like nmake.exe.
        if env.get_shell_var("CLANG_HOST_BIN") is not None:
            self.Logger.debug("CLANG_HOST_BIN is already set.")
            clang_bin = env.get_shell_var("CLANG_BIN")
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

        clang_host_bin_prefix = Path(install_path, "VC", "Tools", "MSVC", vc_ver, "bin", f"Host{host_type}", host_type)
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
        # check if already specified
        path = None
        if varname is not None:
            path = shell_environment.GetEnvironment().get_shell_var(varname)

        if path is None:
            # Not specified...find latest
            try:
                path = FindWithVsWhere(vs_version=vs_version)
            except (OSError, ValueError, RuntimeError) as e:
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
