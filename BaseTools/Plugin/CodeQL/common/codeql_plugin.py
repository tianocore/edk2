# @file codeql_plugin.py
#
# Common logic shared across the CodeQL plugin.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import shutil
from os import PathLike

from edk2toollib.utility_functions import GetHostInfo


def get_codeql_db_path(workspace: PathLike, package: str, target: str,
                       new_path: bool = True) -> str:
    """Return the CodeQL database path for this build.

    Args:
        workspace (PathLike): The workspace path.
        package (str): The package name (e.g. "MdeModulePkg")
        target (str): The target (e.g. "DEBUG")
        new_path (bool, optional): Whether to create a new database path or
                                   return an existing path. Defaults to True.

    Returns:
        str: The absolute path to the CodeQL database directory.
    """
    codeql_db_dir_name = "codeql-db-" + package + "-" + target
    codeql_db_dir_name = codeql_db_dir_name.lower()
    codeql_db_path = os.path.join("Build", codeql_db_dir_name)
    codeql_db_path = os.path.join(workspace, codeql_db_path)

    i = 0
    while os.path.isdir(f"{codeql_db_path + '-%s' % i}"):
        i += 1

    if not new_path:
        if i == 0:
            return None
        else:
            i -= 1

    return codeql_db_path + f"-{i}"


def get_codeql_cli_path() -> str:
    """Return the current CodeQL CLI path.

    Returns:
        str: The absolute path to the CodeQL CLI application to use for
             this build.
    """
    # The CodeQL executable path can be passed via the
    # STUART_CODEQL_PATH environment variable (to override with a
    # custom value for this run) or read from the system path.
    codeql_path = None

    if "STUART_CODEQL_PATH" in os.environ:
        codeql_path = os.environ["STUART_CODEQL_PATH"]

        if GetHostInfo().os == "Windows":
            codeql_path = os.path.join(codeql_path, "codeql.exe")
        else:
            codeql_path = os.path.join(codeql_path, "codeql")

        if not os.path.isfile(codeql_path):
            codeql_path = None

    if not codeql_path:
        codeql_path = shutil.which("codeql")

    return codeql_path
