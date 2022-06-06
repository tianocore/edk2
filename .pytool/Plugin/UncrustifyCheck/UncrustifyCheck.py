# @file UncrustifyCheck.py
#
# An edk2-pytool based plugin wrapper for Uncrustify
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import configparser
import difflib
import errno
import logging
import os
import pathlib
import shutil
import timeit
from edk2toolext.environment import version_aggregator
from edk2toolext.environment.plugin_manager import PluginManager
from edk2toolext.environment.plugintypes.ci_build_plugin import ICiBuildPlugin
from edk2toolext.environment.plugintypes.uefi_helper_plugin import HelperFunctions
from edk2toolext.environment.var_dict import VarDict
from edk2toollib.gitignore_parser import parse_gitignore_lines
from edk2toollib.log.junit_report_format import JunitReportTestCase
from edk2toollib.uefi.edk2.path_utilities import Edk2Path
from edk2toollib.utility_functions import  RunCmd
from io import StringIO
from typing import Any, Dict, List, Tuple

#
# Provide more user friendly messages for certain scenarios
#
class UncrustifyException(Exception):
    def __init__(self, message, exit_code):
        super().__init__(message)
        self.exit_code = exit_code


class UncrustifyAppEnvVarNotFoundException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -101)


class UncrustifyAppVersionErrorException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -102)


class UncrustifyAppExecutionException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -103)


class UncrustifyStalePluginFormattedFilesException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -120)


class UncrustifyInputFileCreationErrorException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -121)

class UncrustifyInvalidIgnoreStandardPathsException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -122)

class UncrustifyGitIgnoreFileException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -140)


class UncrustifyGitSubmoduleException(UncrustifyException):
    def __init__(self, message):
        super().__init__(message, -141)


class UncrustifyCheck(ICiBuildPlugin):
    """
    A CiBuildPlugin that uses Uncrustify to check the source files in the
    package being tested for coding standard issues.

    By default, the plugin runs against standard C source file extensions but
    its configuration can be modified through its configuration file.

    Configuration options:
    "UncrustifyCheck": {
        "AdditionalIncludePaths": [], # Additional paths to check formatting (wildcards supported).
        "AuditOnly": False,           # Don't fail the build if there are errors.  Just log them.
        "ConfigFilePath": "",         # Custom path to an Uncrustify config file.
        "IgnoreStandardPaths": [],    # Standard Plugin defined paths that should be ignored.
        "OutputFileDiffs": False,     # Output chunks of formatting diffs in the test case log.
                                      # This can significantly slow down the plugin on very large packages.
        "SkipGitExclusions": False    # Don't exclude git ignored files and files in git submodules.
    }
    """

    #
    # By default, use an "uncrustify.cfg" config file in the plugin directory
    # A package can override this path via "ConfigFilePath"
    #
    # Note: Values specified via "ConfigFilePath" are relative to the package
    #
    DEFAULT_CONFIG_FILE_PATH = os.path.join(
        pathlib.Path(__file__).parent.resolve(), "uncrustify.cfg")

    #
    # The extension used for formatted files produced by this plugin
    #
    FORMATTED_FILE_EXTENSION = ".uncrustify_plugin"

    #
    # A package can add any additional paths with "AdditionalIncludePaths"
    # A package can remove any of these paths with "IgnoreStandardPaths"
    #
    STANDARD_PLUGIN_DEFINED_PATHS = ("*.c", "*.h")

    #
    # The Uncrustify application path should set in this environment variable
    #
    UNCRUSTIFY_PATH_ENV_KEY = "UNCRUSTIFY_CI_PATH"

    def GetTestName(self, packagename: str, environment: VarDict) -> Tuple:
        """ Provide the testcase name and classname for use in reporting

            Args:
              packagename: string containing name of package to build
              environment: The VarDict for the test to run in
            Returns:
                A tuple containing the testcase name and the classname
                (testcasename, classname)
                testclassname: a descriptive string for the testcase can include whitespace
                classname: should be patterned <packagename>.<plugin>.<optionally any unique condition>
        """
        return ("Check file coding standard compliance in " + packagename, packagename + ".UncrustifyCheck")

    def RunBuildPlugin(self, package_rel_path: str, edk2_path: Edk2Path, package_config: Dict[str, List[str]], environment_config: Any, plugin_manager: PluginManager, plugin_manager_helper: HelperFunctions, tc: JunitReportTestCase, output_stream=None) -> int:
        """
        External function of plugin. This function is used to perform the task of the CiBuild Plugin.

        Args:
          - package_rel_path: edk2 workspace relative path to the package
          - edk2_path: Edk2Path object with workspace and packages paths
          - package_config: Dictionary with the package configuration
          - environment_config: Environment configuration
          - plugin_manager: Plugin Manager Instance
          - plugin_manager_helper: Plugin Manager Helper Instance
          - tc: JUnit test case
          - output_stream: The StringIO output stream from this plugin (logging)

        Returns
          >0 : Number of errors found
          0  : Passed successfully
          -1 : Skipped for missing prereq
        """
        try:
            # Initialize plugin and check pre-requisites.
            self._initialize_environment_info(
                package_rel_path, edk2_path, package_config, tc)
            self._initialize_configuration()
            self._check_for_preexisting_formatted_files()

            # Log important context information.
            self._log_uncrustify_app_info()

            # Get template file contents if specified
            self._get_template_file_contents()

            # Create meta input files & directories
            self._create_temp_working_directory()
            self._create_uncrustify_file_list_file()

            self._run_uncrustify()

            # Post-execution actions.
            self._process_uncrustify_results()

        except UncrustifyException as e:
            self._tc.LogStdError(
                f"Uncrustify error {e.exit_code}. Details:\n\n{str(e)}")
            logging.warning(
                f"Uncrustify error {e.exit_code}. Details:\n\n{str(e)}")
            return -1
        else:
            if self._formatted_file_error_count > 0:
                if self._audit_only_mode:
                    logging.info(
                        "Setting test as skipped since AuditOnly is enabled")
                    self._tc.SetSkipped()
                    return -1
                else:
                    self._tc.SetFailed(
                        f"{self._plugin_name} failed due to {self._formatted_file_error_count} incorrectly formatted files.", "CHECK_FAILED")
            else:
                self._tc.SetSuccess()
            return self._formatted_file_error_count
        finally:
            self._cleanup_temporary_formatted_files()
            self._cleanup_temporary_directory()

    def _initialize_configuration(self) -> None:
        """
        Initializes plugin configuration.
        """
        self._initialize_app_info()
        self._initialize_config_file_info()
        self._initialize_file_to_format_info()
        self._initialize_test_case_output_options()

    def _check_for_preexisting_formatted_files(self) -> None:
        """
        Checks if any formatted files from prior execution are present.

        Existence of such files is an unexpected condition. This might result
        from an error that occurred during a previous run or a premature exit from a debug scenario. In any case, the package should be clean before starting a new run.
        """
        pre_existing_formatted_file_count = len(
            [str(path.resolve()) for path in pathlib.Path(self._abs_package_path).rglob(f'*{UncrustifyCheck.FORMATTED_FILE_EXTENSION}')])

        if pre_existing_formatted_file_count > 0:
            raise UncrustifyStalePluginFormattedFilesException(
                f"{pre_existing_formatted_file_count} formatted files already exist. To prevent overwriting these files, please remove them before running this plugin.")

    def _cleanup_temporary_directory(self) -> None:
        """
        Cleans up the temporary directory used for this execution instance.

        This removes the directory and all files created during this instance.
        """
        if hasattr(self, '_working_dir'):
            self._remove_tree(self._working_dir)

    def _cleanup_temporary_formatted_files(self) -> None:
        """
        Cleans up the temporary formmatted files produced by Uncrustify.

        This will recursively remove all formatted files generated by Uncrustify
        during this execution instance.
        """
        if hasattr(self, '_abs_package_path'):
            formatted_files = [str(path.resolve()) for path in pathlib.Path(
                self._abs_package_path).rglob(f'*{UncrustifyCheck.FORMATTED_FILE_EXTENSION}')]

            for formatted_file in formatted_files:
                os.remove(formatted_file)

    def _create_temp_working_directory(self) -> None:
        """
        Creates the temporary directory used for this execution instance.
        """
        self._working_dir = os.path.join(
            self._abs_workspace_path, "Build", ".pytool", "Plugin", f"{self._plugin_name}")

        try:
            pathlib.Path(self._working_dir).mkdir(parents=True, exist_ok=True)
        except OSError as e:
            raise UncrustifyInputFileCreationErrorException(
                f"Error creating plugin directory {self._working_dir}.\n\n{repr(e)}.")

    def _create_uncrustify_file_list_file(self) -> None:
        """
        Creates the file with the list of source files for Uncrustify to process.
        """
        self._app_input_file_path = os.path.join(
            self._working_dir, "uncrustify_file_list.txt")

        with open(self._app_input_file_path, 'w', encoding='utf8') as f:
            f.writelines(f"\n".join(self._abs_file_paths_to_format))

    def _execute_uncrustify(self) -> None:
        """
        Executes Uncrustify with the initialized configuration.
        """
        output = StringIO()
        self._app_exit_code = RunCmd(
            self._app_path,
            f"-c {self._app_config_file} -F {self._app_input_file_path} --if-changed --suffix {UncrustifyCheck.FORMATTED_FILE_EXTENSION}", outstream=output)
        self._app_output = output.getvalue().strip().splitlines()

    def _get_files_ignored_in_config(self):
        """"
        Returns a function that returns true if a given file string path is ignored in the plugin configuration file and false otherwise.
        """
        ignored_files = []
        if "IgnoreFiles" in self._package_config:
            ignored_files = self._package_config["IgnoreFiles"]

        # Pass "Package configuration file" as the source file path since
        # the actual configuration file name is unknown to this plugin and
        # this provides a generic description of the file that provided
        # the ignore file content.
        #
        # This information is only used for reporting (not used here) and
        # the ignore lines are being passed directly as they are given to
        # this plugin.
        return parse_gitignore_lines(ignored_files, "Package configuration file", self._abs_package_path)

    def _get_git_ignored_paths(self) -> List[str]:
        """"
        Returns a list of file absolute path strings to all files ignored in this git repository.

        If git is not found, an empty list will be returned.
        """
        if not shutil.which("git"):
            logging.warn(
                "Git is not found on this system. Git submodule paths will not be considered.")
            return []

        outstream_buffer = StringIO()
        exit_code = RunCmd("git", "ls-files --other",
                           workingdir=self._abs_workspace_path, outstream=outstream_buffer, logging_level=logging.NOTSET)
        if (exit_code != 0):
            raise UncrustifyGitIgnoreFileException(
                f"An error occurred reading git ignore settings. This will prevent Uncrustify from running against the expected set of files.")

        # Note: This will potentially be a large list, but at least sorted
        rel_paths = outstream_buffer.getvalue().strip().splitlines()
        abs_paths = []
        for path in rel_paths:
            abs_paths.append(
                os.path.normpath(os.path.join(self._abs_workspace_path, path)))
        return abs_paths

    def _get_git_submodule_paths(self) -> List[str]:
        """
        Returns a list of directory absolute path strings to the root of each submodule in the workspace repository.

        If git is not found, an empty list will be returned.
        """
        if not shutil.which("git"):
            logging.warn(
                "Git is not found on this system. Git submodule paths will not be considered.")
            return []

        if os.path.isfile(os.path.join(self._abs_workspace_path, ".gitmodules")):
            logging.info(
                f".gitmodules file found. Excluding submodules in {self._package_name}.")

            outstream_buffer = StringIO()
            exit_code = RunCmd("git", "config --file .gitmodules --get-regexp path", workingdir=self._abs_workspace_path, outstream=outstream_buffer, logging_level=logging.NOTSET)
            if (exit_code != 0):
                raise UncrustifyGitSubmoduleException(
                    f".gitmodule file detected but an error occurred reading the file. Cannot proceed with unknown submodule paths.")

            submodule_paths = []
            for line in outstream_buffer.getvalue().strip().splitlines():
                submodule_paths.append(
                    os.path.normpath(os.path.join(self._abs_workspace_path, line.split()[1])))

            return submodule_paths
        else:
            return []

    def _get_template_file_contents(self) -> None:
        """
        Gets the contents of Uncrustify template files if they are specified
        in the Uncrustify configuration file.
        """

        self._file_template_contents = None
        self._func_template_contents = None

        # Allow no value to allow "set" statements in the config file which do
        # not specify value assignment
        parser = configparser.ConfigParser(allow_no_value=True)
        with open(self._app_config_file, 'r') as cf:
            parser.read_string("[dummy_section]\n" + cf.read())

        try:
            file_template_name = parser["dummy_section"]["cmt_insert_file_header"]

            file_template_path = pathlib.Path(file_template_name)

            if not file_template_path.is_file():
                file_template_path = pathlib.Path(os.path.join(self._plugin_path, file_template_name))
                self._file_template_contents = file_template_path.read_text()
        except KeyError:
            logging.warn("A file header template is not specified in the config file.")
        except FileNotFoundError:
            logging.warn("The specified file header template file was not found.")
        try:
            func_template_name = parser["dummy_section"]["cmt_insert_func_header"]

            func_template_path = pathlib.Path(func_template_name)

            if not func_template_path.is_file():
                func_template_path = pathlib.Path(os.path.join(self._plugin_path, func_template_name))
                self._func_template_contents = func_template_path.read_text()
        except KeyError:
            logging.warn("A function header template is not specified in the config file.")
        except FileNotFoundError:
            logging.warn("The specified function header template file was not found.")

    def _initialize_app_info(self) -> None:
        """
        Initialize Uncrustify application information.

        This function will determine the application path and version.
        """
        # Verify Uncrustify is specified in the environment.
        if UncrustifyCheck.UNCRUSTIFY_PATH_ENV_KEY not in os.environ:
            raise UncrustifyAppEnvVarNotFoundException(
                f"Uncrustify environment variable {UncrustifyCheck.UNCRUSTIFY_PATH_ENV_KEY} is not present.")

        self._app_path = shutil.which('uncrustify', path=os.environ[UncrustifyCheck.UNCRUSTIFY_PATH_ENV_KEY])

        if self._app_path is None:
            raise FileNotFoundError(
                errno.ENOENT, os.strerror(errno.ENOENT), self._app_path)

        self._app_path = os.path.normcase(os.path.normpath(self._app_path))

        if not os.path.isfile(self._app_path):
            raise FileNotFoundError(
                errno.ENOENT, os.strerror(errno.ENOENT), self._app_path)

        # Verify Uncrustify is present at the expected path.
        return_buffer = StringIO()
        ret = RunCmd(self._app_path, "--version", outstream=return_buffer)
        if (ret != 0):
            raise UncrustifyAppVersionErrorException(
                f"Error occurred executing --version: {ret}.")

        # Log Uncrustify version information.
        self._app_version = return_buffer.getvalue().strip()
        self._tc.LogStdOut(f"Uncrustify version: {self._app_version}")
        version_aggregator.GetVersionAggregator().ReportVersion(
            "Uncrustify", self._app_version, version_aggregator.VersionTypes.INFO)

    def _initialize_config_file_info(self) -> None:
        """
        Initialize Uncrustify configuration file info.

        The config file path is relative to the package root.
        """
        self._app_config_file = UncrustifyCheck.DEFAULT_CONFIG_FILE_PATH
        if "ConfigFilePath" in self._package_config:
            self._app_config_file = self._package_config["ConfigFilePath"].strip()

            self._app_config_file = os.path.normpath(
                os.path.join(self._abs_package_path, self._app_config_file))

        if not os.path.isfile(self._app_config_file):
            raise FileNotFoundError(
                errno.ENOENT, os.strerror(errno.ENOENT), self._app_config_file)

    def _initialize_environment_info(self, package_rel_path: str, edk2_path: Edk2Path, package_config: Dict[str, List[str]], tc: JunitReportTestCase) -> None:
        """
        Initializes plugin environment information.
        """
        self._abs_package_path = edk2_path.GetAbsolutePathOnThisSystemFromEdk2RelativePath(
            package_rel_path)
        self._abs_workspace_path = edk2_path.WorkspacePath
        self._package_config = package_config
        self._package_name = os.path.basename(
            os.path.normpath(package_rel_path))
        self._plugin_name = self.__class__.__name__
        self._plugin_path = os.path.dirname(os.path.realpath(__file__))
        self._rel_package_path = package_rel_path
        self._tc = tc

    def _initialize_file_to_format_info(self) -> None:
        """
        Forms the list of source files for Uncrustify to process.
        """
        # Create a list of all the package relative file paths in the package to run against Uncrustify.
        rel_file_paths_to_format = list(
            UncrustifyCheck.STANDARD_PLUGIN_DEFINED_PATHS)

        # Allow the ci.yaml to remove any of the pre-defined standard paths
        if "IgnoreStandardPaths" in self._package_config:
            for a in self._package_config["IgnoreStandardPaths"]:
                if a.strip() in rel_file_paths_to_format:
                    self._tc.LogStdOut(
                        f"Ignoring standard path due to ci.yaml ignore: {a}")
                    rel_file_paths_to_format.remove(a.strip())
                else:
                    raise UncrustifyInvalidIgnoreStandardPathsException(f"Invalid IgnoreStandardPaths value: {a}")

        # Allow the ci.yaml to specify additional include paths for this package
        if "AdditionalIncludePaths" in self._package_config:
            rel_file_paths_to_format.extend(
                self._package_config["AdditionalIncludePaths"])

        self._abs_file_paths_to_format = []
        for path in rel_file_paths_to_format:
            self._abs_file_paths_to_format.extend(
                [str(path.resolve()) for path in pathlib.Path(self._abs_package_path).rglob(path)])

        # Remove files ignore in the plugin configuration file
        plugin_ignored_files = list(filter(self._get_files_ignored_in_config(), self._abs_file_paths_to_format))

        if plugin_ignored_files:
            logging.info(
                f"{self._package_name} file count before plugin ignore file exclusion: {len(self._abs_file_paths_to_format)}")
            for path in plugin_ignored_files:
                if path in self._abs_file_paths_to_format:
                    logging.info(f"  File ignored in plugin config file: {path}")
                    self._abs_file_paths_to_format.remove(path)
            logging.info(
                f"{self._package_name} file count after plugin ignore file exclusion: {len(self._abs_file_paths_to_format)}")

        if not "SkipGitExclusions" in self._package_config or not self._package_config["SkipGitExclusions"]:
            # Remove files ignored by git
            logging.info(
                f"{self._package_name} file count before git ignore file exclusion: {len(self._abs_file_paths_to_format)}")

            ignored_paths = self._get_git_ignored_paths()
            self._abs_file_paths_to_format = list(
                set(self._abs_file_paths_to_format).difference(ignored_paths))

            logging.info(
                f"{self._package_name} file count after git ignore file exclusion: {len(self._abs_file_paths_to_format)}")

            # Remove files in submodules
            logging.info(
                f"{self._package_name} file count before submodule exclusion: {len(self._abs_file_paths_to_format)}")

            submodule_paths = tuple(self._get_git_submodule_paths())
            for path in submodule_paths:
                logging.info(f"  submodule path: {path}")

            self._abs_file_paths_to_format = [
                f for f in self._abs_file_paths_to_format if not f.startswith(submodule_paths)]

            logging.info(
                f"{self._package_name} file count after submodule exclusion: {len(self._abs_file_paths_to_format)}")

        # Sort the files for more consistent results
        self._abs_file_paths_to_format.sort()

    def _initialize_test_case_output_options(self) -> None:
        """
        Initializes options that influence test case output.
        """
        self._audit_only_mode = False
        self._output_file_diffs = True

        if "AuditOnly" in self._package_config and self._package_config["AuditOnly"]:
            self._audit_only_mode = True

        if "OutputFileDiffs" in self._package_config and not self._package_config["OutputFileDiffs"]:
            self._output_file_diffs = False

    def _log_uncrustify_app_info(self) -> None:
        """
        Logs Uncrustify application information.
        """
        self._tc.LogStdOut(f"Found Uncrustify at {self._app_path}")
        self._tc.LogStdOut(f"Uncrustify version: {self._app_version}")
        self._tc.LogStdOut('\n')
        logging.info(f"Found Uncrustify at {self._app_path}")
        logging.info(f"Uncrustify version: {self._app_version}")
        logging.info('\n')

    def _process_uncrustify_results(self) -> None:
        """
        Process the results from Uncrustify.

        Determines whether formatting errors are present and logs failures.
        """
        formatted_files = [str(path.resolve()) for path in pathlib.Path(
            self._abs_package_path).rglob(f'*{UncrustifyCheck.FORMATTED_FILE_EXTENSION}')]

        self._formatted_file_error_count = len(formatted_files)

        if self._formatted_file_error_count > 0:
            logging.error(
                "Visit the following instructions to learn "
                "how to find the detailed formatting errors in Azure "
                "DevOps CI: "
                "https://github.com/tianocore/tianocore.github.io/wiki/EDK-II-Code-Formatting#how-to-find-uncrustify-formatting-errors-in-continuous-integration-ci")
            self._tc.LogStdError("Files with formatting errors:\n")

            if self._output_file_diffs:
                logging.info("Calculating file diffs. This might take a while...")

        for formatted_file in formatted_files:
            pre_formatted_file = formatted_file[:-
                                                len(UncrustifyCheck.FORMATTED_FILE_EXTENSION)]
            logging.error(pre_formatted_file)

            if (self._output_file_diffs or
                    self._file_template_contents is not None or
                    self._func_template_contents is not None):
                self._tc.LogStdError(
                    f"Formatting errors in {os.path.relpath(pre_formatted_file, self._abs_package_path)}\n")

                with open(formatted_file) as ff:
                    formatted_file_text = ff.read()

                    if (self._file_template_contents is not None and
                            self._file_template_contents in formatted_file_text):
                        self._tc.LogStdError(f"File header is missing in {os.path.relpath(pre_formatted_file, self._abs_package_path)}\n")

                    if (self._func_template_contents is not None and
                            self._func_template_contents in formatted_file_text):
                        self._tc.LogStdError(f"A function header is missing in {os.path.relpath(pre_formatted_file, self._abs_package_path)}\n")

                    if self._output_file_diffs:
                        with open(pre_formatted_file) as pf:
                            pre_formatted_file_text = pf.read()

                        for line in difflib.unified_diff(pre_formatted_file_text.split('\n'), formatted_file_text.split('\n'), fromfile=pre_formatted_file, tofile=formatted_file, n=3):
                            self._tc.LogStdError(line)

                        self._tc.LogStdError('\n')
            else:
                self._tc.LogStdError(pre_formatted_file)

    def _remove_tree(self, dir_path: str, ignore_errors: bool = False) -> None:
        """
        Helper for removing a directory. Over time there have been
        many private implementations of this due to reliability issues in the
        shutil implementations. To consolidate on a single function this helper is added.

        On error try to change file attributes. Also add retry logic.

        This function is temporarily borrowed from edk2toollib.utility_functions
        since the version used in edk2 is not recent enough to include the
        function.

        This function should be replaced by "RemoveTree" when it is available.

        Args:
          - dir_path: Path to directory to remove.
          - ignore_errors: Whether to ignore errors during removal
        """

        def _remove_readonly(func, path, _):
            """
            Private function to attempt to change permissions on file/folder being deleted.
            """
            os.chmod(path, os.stat.S_IWRITE)
            func(path)

        for _ in range(3):  # retry up to 3 times
            try:
                shutil.rmtree(dir_path, ignore_errors=ignore_errors, onerror=_remove_readonly)
            except OSError as err:
                logging.warning(f"Failed to fully remove {dir_path}: {err}")
            else:
                break
        else:
            raise RuntimeError(f"Failed to remove {dir_path}")

    def _run_uncrustify(self) -> None:
        """
        Runs Uncrustify for this instance of plugin execution.
        """
        logging.info("Executing Uncrustify. This might take a while...")
        start_time = timeit.default_timer()
        self._execute_uncrustify()
        end_time = timeit.default_timer() - start_time

        execution_summary = f"Uncrustify executed against {len(self._abs_file_paths_to_format)} files in {self._package_name} in {end_time:.2f} seconds.\n"

        self._tc.LogStdOut(execution_summary)
        logging.info(execution_summary)

        if self._app_exit_code != 0 and self._app_exit_code != 1:
            raise UncrustifyAppExecutionException(
                f"Error {str(self._app_exit_code)} returned from Uncrustify:\n\n{str(self._app_output)}")
