## @file WinRcPath.py
# Plugin to find Windows SDK Resource Compiler rc.exe
##
# This plugin works in conjuncture with the tools_def to support rc.exe
#
# Copyright (c) Microsoft Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import logging
from edk2toolext.environment.plugintypes.uefi_build_plugin import IUefiBuildPlugin
import edk2toollib.windows.locate_tools as locate_tools
from edk2toolext.environment import shell_environment
from edk2toolext.environment import version_aggregator
from pathlib import Path


class WinRcPath(IUefiBuildPlugin):

    def do_pre_build(self, thebuilder):
        # Check if the rc.exe path is already cached and still exists
        cache_path = Path(thebuilder.ws, "Conf", ".rc_path")
        if cache_path.exists():
            with open(cache_path, "r") as f:
                rc_path = Path(f.readline().strip()).absolute()
                if (rc_path / "rc.exe").exists():
                    logging.debug(f"Found rc.exe folder in cache: {rc_path}")
                    self._set_path(rc_path)
                    return 0

        # If it does not exist, try to find it with FindToolInWinSdk
        path = locate_tools.FindToolInWinSdk("rc.exe")
        if path is None:
            logging.critical("Failed to find rc.exe")
            return 1

        path = Path(path).absolute().parent
        self._set_path(path)
        cache_path.unlink(missing_ok=True)
        with cache_path.open("w") as f:
            f.write(str(path))
        return 0

    def _set_path(self, path: Path):
        shell_environment.GetEnvironment().set_shell_var("WINSDK_PATH_FOR_RC_EXE", str(path))
        version_aggregator.GetVersionAggregator().ReportVersion("WINSDK_PATH_FOR_RC_EXE", str(path), version_aggregator.VersionTypes.INFO)
