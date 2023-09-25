# @file CodeQlBuildPlugin.py
#
# A build plugin that produces CodeQL results for the present build.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import glob
import logging
import os
import stat
from common import codeql_plugin
from pathlib import Path

from edk2toolext import edk2_logging
from edk2toolext.environment.plugintypes.uefi_build_plugin import \
    IUefiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toollib.uefi.edk2.path_utilities import Edk2Path
from edk2toollib.utility_functions import GetHostInfo, RemoveTree


class CodeQlBuildPlugin(IUefiBuildPlugin):

    def do_pre_build(self, builder: UefiBuilder) -> int:
        """CodeQL pre-build functionality.

        Args:
            builder (UefiBuilder): A UEFI builder object for this build.

        Returns:
            int: The plugin return code. Zero indicates the plugin ran
            successfully. A non-zero value indicates an unexpected error
            occurred during plugin execution.
        """

        if not builder.SkipBuild:
            self.builder = builder
            self.package = builder.edk2path.GetContainingPackage(
                builder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                    builder.env.GetValue("ACTIVE_PLATFORM")
                )
            )

            self.target = builder.env.GetValue("TARGET")

            self.build_output_dir = builder.env.GetValue("BUILD_OUTPUT_BASE")

            self.codeql_db_path = codeql_plugin.get_codeql_db_path(
                                    builder.ws, self.package, self.target)

            edk2_logging.log_progress(f"{self.package} will be built for CodeQL")
            edk2_logging.log_progress(f"  CodeQL database will be written to "
                                    f"{self.codeql_db_path}")

            self.codeql_path = codeql_plugin.get_codeql_cli_path()
            if not self.codeql_path:
                logging.critical("CodeQL build enabled but CodeQL CLI application "
                                "not found.")
                return -1

            # CodeQL can only generate a database on clean build
            #
            # Note: builder.CleanTree() cannot be used here as some platforms
            #       have build steps that run before this plugin that store
            #       files in the build output directory.
            #
            #       CodeQL does not care about with those files or many others such
            #       as the FV directory, build logs, etc. so instead focus on
            #       removing only the directories with compilation/linker output
            #       for the architectures being built (that need clean runs for
            #       CodeQL to work).
            targets = self.builder.env.GetValue("TARGET_ARCH").split(" ")
            for target in targets:
                directory_to_delete = Path(self.build_output_dir, target)

                if directory_to_delete.is_dir():
                    logging.debug(f"Removing {str(directory_to_delete)} to have a "
                                f"clean build for CodeQL.")
                    RemoveTree(str(directory_to_delete))

            # CodeQL CLI does not handle spaces passed in CLI commands well
            # (perhaps at all) as discussed here:
            #   1. https://github.com/github/codeql-cli-binaries/issues/73
            #   2. https://github.com/github/codeql/issues/4910
            #
            # Since it's unclear how quotes are handled and may change in the
            # future, this code is going to use the workaround to place the
            # command in an executable file that is instead passed to CodeQL.
            self.codeql_cmd_path = Path(self.build_output_dir, "codeql_build_command")

            build_params = self._get_build_params()

            codeql_build_cmd = ""
            if GetHostInfo().os == "Windows":
                self.codeql_cmd_path = self.codeql_cmd_path.parent / (
                    self.codeql_cmd_path.name + '.bat')
            elif GetHostInfo().os == "Linux":
                self.codeql_cmd_path = self.codeql_cmd_path.parent / (
                    self.codeql_cmd_path.name + '.sh')
                codeql_build_cmd += f"#!/bin/bash{os.linesep * 2}"
            codeql_build_cmd += "build " + build_params

            self.codeql_cmd_path.parent.mkdir(exist_ok=True, parents=True)
            self.codeql_cmd_path.write_text(encoding='utf8', data=codeql_build_cmd)

            if GetHostInfo().os == "Linux":
                os.chmod(self.codeql_cmd_path,
                        os.stat(self.codeql_cmd_path).st_mode | stat.S_IEXEC)
                for f in glob.glob(os.path.join(
                    os.path.dirname(self.codeql_path), '**/*'), recursive=True):
                        os.chmod(f, os.stat(f).st_mode | stat.S_IEXEC)

            codeql_params = (f'database create {self.codeql_db_path} '
                            f'--language=cpp '
                            f'--source-root={builder.ws} '
                            f'--command={self.codeql_cmd_path}')

            # Set environment variables so the CodeQL build command is picked up
            # as the active build command.
            #
            # Note: Requires recent changes in edk2-pytool-extensions (0.20.0)
            #       to support reading these variables.
            builder.env.SetValue(
                "EDK_BUILD_CMD", self.codeql_path, "Set in CodeQL Build Plugin")
            builder.env.SetValue(
                "EDK_BUILD_PARAMS", codeql_params, "Set in CodeQL Build Plugin")

        return 0

    def _get_build_params(self) -> str:
        """Returns the build command parameters for this build.

        Based on the well-defined `build` command-line parameters.

        Returns:
            str: A string representing the parameters for the build command.
        """
        build_params = f"-p {self.builder.env.GetValue('ACTIVE_PLATFORM')}"
        build_params += f" -b {self.target}"
        build_params += f" -t {self.builder.env.GetValue('TOOL_CHAIN_TAG')}"

        max_threads = self.builder.env.GetValue('MAX_CONCURRENT_THREAD_NUMBER')
        if max_threads is not None:
            build_params += f" -n {max_threads}"

        rt = self.builder.env.GetValue("TARGET_ARCH").split(" ")
        for t in rt:
            build_params += " -a " + t

        if (self.builder.env.GetValue("BUILDREPORTING") == "TRUE"):
            build_params += (" -y " +
                             self.builder.env.GetValue("BUILDREPORT_FILE"))
            rt = self.builder.env.GetValue("BUILDREPORT_TYPES").split(" ")
            for t in rt:
                build_params += " -Y " + t

        # add special processing to handle building a single module
        mod = self.builder.env.GetValue("BUILDMODULE")
        if (mod is not None and len(mod.strip()) > 0):
            build_params += " -m " + mod
            edk2_logging.log_progress("Single Module Build: " + mod)

        build_vars = self.builder.env.GetAllBuildKeyValues(self.target)
        for key, value in build_vars.items():
            build_params += " -D " + key + "=" + value

        return build_params
