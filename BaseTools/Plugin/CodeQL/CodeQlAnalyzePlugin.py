# @file CodeQAnalyzePlugin.py
#
# A build plugin that analyzes a CodeQL database.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import json
import logging
import os
import yaml

from analyze import analyze_filter
from common import codeql_plugin

from edk2toolext import edk2_logging
from edk2toolext.environment.plugintypes.uefi_build_plugin import \
    IUefiBuildPlugin
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toollib.uefi.edk2.path_utilities import Edk2Path
from edk2toollib.utility_functions import RunCmd
from pathlib import Path


class CodeQlAnalyzePlugin(IUefiBuildPlugin):

    def do_post_build(self, builder: UefiBuilder) -> int:
        """CodeQL analysis post-build functionality.

        Args:
            builder (UefiBuilder): A UEFI builder object for this build.

        Returns:
            int: The number of CodeQL errors found. Zero indicates that
            AuditOnly mode is enabled or no failures were found.
        """
        self.builder = builder
        self.package = builder.edk2path.GetContainingPackage(
            builder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                builder.env.GetValue("ACTIVE_PLATFORM")
            )
        )

        self.package_path = Path(
            builder.edk2path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
                self.package
            )
        )
        self.target = builder.env.GetValue("TARGET")

        self.codeql_db_path = codeql_plugin.get_codeql_db_path(
                                builder.ws, self.package, self.target,
                                new_path=False)

        self.codeql_path = codeql_plugin.get_codeql_cli_path()
        if not self.codeql_path:
            logging.critical("CodeQL build enabled but CodeQL CLI application "
                             "not found.")
            return -1

        codeql_sarif_dir_path = self.codeql_db_path[
                                        :self.codeql_db_path.rindex('-')]
        codeql_sarif_dir_path = codeql_sarif_dir_path.replace(
                                        "-db-", "-analysis-")
        self.codeql_sarif_path = os.path.join(
                                        codeql_sarif_dir_path,
                                        (os.path.basename(
                                            self.codeql_db_path) +
                                            ".sarif"))

        edk2_logging.log_progress(f"Analyzing {self.package} ({self.target}) "
                                  f"CodeQL database at:\n"
                                  f"           {self.codeql_db_path}")
        edk2_logging.log_progress(f"Results will be written to:\n"
                                  f"           {self.codeql_sarif_path}")

        # Packages are allowed to specify package-specific query specifiers
        # in the package CI YAML file that override the global query specifier.
        audit_only = False
        global_audit_only = builder.env.GetValue("STUART_CODEQL_AUDIT_ONLY")
        if global_audit_only:
            if global_audit_only.strip().lower() == "true":
                audit_only = True

        query_specifiers = None
        package_config_file = Path(os.path.join(
                                self.package_path, self.package + ".ci.yaml"))
        plugin_data = None
        if package_config_file.is_file():
            with open(package_config_file, 'r') as cf:
                package_config_file_data = yaml.safe_load(cf)
                if "CodeQlAnalyze" in package_config_file_data:
                    plugin_data = package_config_file_data["CodeQlAnalyze"]
                    if "AuditOnly" in plugin_data:
                        audit_only = plugin_data["AuditOnly"]
                    if "QuerySpecifiers" in plugin_data:
                        logging.debug(f"Loading CodeQL query specifiers in "
                                      f"{str(package_config_file)}")
                        query_specifiers = plugin_data["QuerySpecifiers"]

        if audit_only:
            logging.info(f"CodeQL Analyze plugin is in audit only mode for "
                         f"{self.package} ({self.target}).")

        # Builds can override the query specifiers defined in this plugin
        # by setting the value in the STUART_CODEQL_QUERY_SPECIFIERS
        # environment variable.
        if not query_specifiers:
            query_specifiers = builder.env.GetValue(
                                "STUART_CODEQL_QUERY_SPECIFIERS")

        # Use this plugins query set file as the default fallback if it is
        # not overridden. It is possible the file is not present if modified
        # locally. In that case, skip the plugin.
        plugin_query_set = Path(Path(__file__).parent, "CodeQlQueries.qls")

        if not query_specifiers and plugin_query_set.is_file():
            query_specifiers = str(plugin_query_set.resolve())

        if not query_specifiers:
            logging.warning("Skipping CodeQL analysis since no CodeQL query "
                            "specifiers were provided.")
            return 0

        codeql_params = (f'database analyze {self.codeql_db_path} '
                         f'{query_specifiers} --format=sarifv2.1.0 '
                         f'--output={self.codeql_sarif_path} --download '
                         f'--threads=0')

        # CodeQL requires the sarif file parent directory to exist already.
        Path(self.codeql_sarif_path).parent.mkdir(exist_ok=True, parents=True)

        cmd_ret = RunCmd(self.codeql_path, codeql_params)
        if cmd_ret != 0:
            logging.critical(f"CodeQL CLI analysis failed with return code "
                             f"{cmd_ret}.")

        if not os.path.isfile(self.codeql_sarif_path):
            logging.critical(f"The sarif file {self.codeql_sarif_path} was "
                             f"not created. Analysis cannot continue.")
            return -1

        filter_pattern_data = []
        global_filter_file_value = builder.env.GetValue(
                                    "STUART_CODEQL_FILTER_FILES")
        if global_filter_file_value:
            global_filter_files = global_filter_file_value.strip().split(',')
            global_filter_files = [Path(f) for f in global_filter_files]

            for global_filter_file in global_filter_files:
                if global_filter_file.is_file():
                    with open(global_filter_file, 'r') as ff:
                        global_filter_file_data = yaml.safe_load(ff)
                        if "Filters" in global_filter_file_data:
                            current_pattern_data = \
                                global_filter_file_data["Filters"]
                            if type(current_pattern_data) is not list:
                                logging.critical(
                                    f"CodeQL pattern data must be a list of "
                                    f"strings. Data in "
                                    f"{str(global_filter_file.resolve())} is "
                                    f"invalid. CodeQL analysis is incomplete.")
                                return -1
                            filter_pattern_data += current_pattern_data
                        else:
                            logging.critical(
                                f"CodeQL global filter file "
                                f"{str(global_filter_file.resolve())} is  "
                                f"malformed. Missing Filters section. CodeQL "
                                f"analysis is incomplete.")
                            return -1
                else:
                    logging.critical(
                        f"CodeQL global filter file "
                        f"{str(global_filter_file.resolve())} was not found. "
                        f"CodeQL analysis is incomplete.")
                    return -1

        if plugin_data and "Filters" in plugin_data:
            if type(plugin_data["Filters"]) is not list:
                logging.critical(
                    "CodeQL pattern data must be a list of strings. "
                    "CodeQL analysis is incomplete.")
                return -1
            filter_pattern_data.extend(plugin_data["Filters"])

        if filter_pattern_data:
            logging.info("Applying CodeQL SARIF result filters.")
            analyze_filter.filter_sarif(
                self.codeql_sarif_path,
                self.codeql_sarif_path,
                filter_pattern_data,
                split_lines=False)

        with open(self.codeql_sarif_path, 'r') as sf:
            sarif_file_data = json.load(sf)

        try:
            # Perform minimal JSON parsing to find the number of errors.
            total_errors = 0
            for run in sarif_file_data['runs']:
                total_errors += len(run['results'])
        except KeyError:
            logging.critical("Sarif file does not contain expected data. "
                             "Analysis cannot continue.")
            return -1

        if total_errors > 0:
            if audit_only:
                # Show a warning message so CodeQL analysis is not forgotten.
                # If the repo owners truly do not want to fix CodeQL issues,
                # analysis should be disabled entirely.
                logging.warning(f"{self.package} ({self.target}) CodeQL "
                                f"analysis ignored {total_errors} errors due "
                                f"to audit mode being enabled.")
                return 0
            else:
                logging.error(f"{self.package} ({self.target}) CodeQL "
                              f"analysis failed with {total_errors} errors.")

        return total_errors
