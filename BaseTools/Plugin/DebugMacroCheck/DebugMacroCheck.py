# @file DebugMacroCheck.py
#
# A script that checks if DEBUG macros are formatted properly.
#
# In particular, that print format specifiers are defined
# with the expected number of arguments in the variable
# argument list.
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from argparse import RawTextHelpFormatter
import logging
import os
import re
import regex
import sys
import shutil
import timeit
import yaml

from edk2toollib.utility_functions import RunCmd
from io import StringIO
from pathlib import Path, PurePath
from typing import Dict, Iterable, List, Optional, Tuple


PROGRAM_NAME = "Debug Macro Checker"


class GitHelpers:
    """
    Collection of Git helpers.

    Will be moved to a more generic module and imported in the future.
    """

    @staticmethod
    def get_git_ignored_paths(directory_path: PurePath) -> List[Path]:
        """Returns ignored files in this git repository.

        Args:
            directory_path (PurePath): Path to the git directory.

        Returns:
            List[Path]: List of file absolute paths to all files ignored
                        in this git repository. If git is not found, an empty
                        list will be returned.
        """
        if not shutil.which("git"):
            logging.warn(
                "Git is not found on this system. Git submodule paths will "
                "not be considered.")
            return []

        out_stream_buffer = StringIO()
        exit_code = RunCmd("git", "ls-files --other",
                           workingdir=str(directory_path),
                           outstream=out_stream_buffer,
                           logging_level=logging.NOTSET)
        if exit_code != 0:
            return []

        rel_paths = out_stream_buffer.getvalue().strip().splitlines()
        abs_paths = []
        for path in rel_paths:
            abs_paths.append(Path(directory_path, path))
        return abs_paths

    @staticmethod
    def get_git_submodule_paths(directory_path: PurePath) -> List[Path]:
        """Returns submodules in the given workspace directory.

        Args:
            directory_path (PurePath): Path to the git directory.

        Returns:
            List[Path]: List of directory absolute paths to the root of
            each submodule found from this folder. If submodules are not
            found, an empty list will be returned.
        """
        if not shutil.which("git"):
            return []

        if os.path.isfile(directory_path.joinpath(".gitmodules")):
            out_stream_buffer = StringIO()
            exit_code = RunCmd(
                "git", "config --file .gitmodules --get-regexp path",
                workingdir=str(directory_path),
                outstream=out_stream_buffer,
                logging_level=logging.NOTSET)
            if exit_code != 0:
                return []

            submodule_paths = []
            for line in out_stream_buffer.getvalue().strip().splitlines():
                submodule_paths.append(
                    Path(directory_path, line.split()[1]))

            return submodule_paths
        else:
            return []


class QuietFilter(logging.Filter):
    """A logging filter that temporarily suppresses message output."""

    def __init__(self, quiet: bool = False):
        """Class constructor method.

        Args:
            quiet (bool, optional): Indicates if messages are currently being
            printed (False) or not (True). Defaults to False.
        """

        self._quiet = quiet

    def filter(self, record: logging.LogRecord) -> bool:
        """Quiet filter method.

        Args:
            record (logging.LogRecord): A log record object that the filter is
            applied to.

        Returns:
            bool: True if messages are being suppressed. Otherwise, False.
        """
        return not self._quiet


class ProgressFilter(logging.Filter):
    """A logging filter that suppresses 'Progress' messages."""

    def filter(self, record: logging.LogRecord) -> bool:
        """Progress filter method.

        Args:
            record (logging.LogRecord): A log record object that the filter is
            applied to.

        Returns:
            bool: True if the message is not a 'Progress' message. Otherwise,
            False.
        """
        return not record.getMessage().startswith("\rProgress")


class CacheDuringProgressFilter(logging.Filter):
    """A logging filter that suppresses messages during progress operations."""

    _message_cache = []

    @property
    def message_cache(self) -> List[logging.LogRecord]:
        """Contains a cache of messages accumulated during time of operation.

        Returns:
            List[logging.LogRecord]: List of log records stored while the
            filter was active.
        """
        return self._message_cache

    def filter(self, record: logging.LogRecord):
        """Cache progress filter that suppresses messages during progress
           display output.

        Args:
            record (logging.LogRecord): A log record to cache.
        """
        self._message_cache.append(record)


def check_debug_macros(macros: Iterable[Dict[str, str]],
                       file_dbg_path: str,
                       **macro_subs: str
                       ) -> Tuple[int, int, int]:
    """Checks if debug macros contain formatting errors.

    Args:
        macros (Iterable[Dict[str, str]]): : A groupdict of macro matches.
        This is an iterable of dictionaries with group names from the regex
        match as the key and the matched string as the value for the key.

        file_dbg_path (str): The file path (or other custom string) to display
        in debug messages.

        macro_subs (Dict[str,str]): Variable-length keyword and replacement
        value string pairs to substitute during debug macro checks.

    Returns:
        Tuple[int, int, int]: A tuple of the number of formatting errors,
        number of print specifiers, and number of arguments for the macros
        given.
    """

    macro_subs = {k.lower(): v for k, v in macro_subs.items()}

    arg_cnt, failure_cnt, print_spec_cnt = 0, 0, 0
    for macro in macros:
        # Special Specifier Handling
        processed_dbg_str = macro['dbg_str'].strip().lower()

        logging.debug(f"Inspecting macro: {macro}")

        # Make any macro substitutions so further processing is applied
        # to the substituted value.
        for k in macro_subs.keys():
            processed_dbg_str = processed_dbg_str.replace(k, macro_subs[k])

        logging.debug("Debug macro string after replacements: "
                      f"{processed_dbg_str}")

        # These are very rarely used in debug strings. They are somewhat
        # more common in HII code to control text displayed on the
        # console. Due to the rarity and likelihood usage is a mistake,
        # a warning is shown if found.
        specifier_display_replacements = ['%n', '%h', '%e', '%b', '%v']
        for s in specifier_display_replacements:
            if s in processed_dbg_str:
                logging.warning(f"File: {file_dbg_path}")
                logging.warning(f"  {s} found in string and ignored:")
                logging.warning(f"  \"{processed_dbg_str}\"")
                processed_dbg_str = processed_dbg_str.replace(s, '')

        # These are miscellaneous print specifiers that do not require
        # special parsing and simply need to be replaced since they do
        # have a corresponding argument associated with them.
        specifier_other_replacements = ['%%', '\r', '\n']
        for s in specifier_other_replacements:
            if s in processed_dbg_str:
                processed_dbg_str = processed_dbg_str.replace(s, '')

        processed_dbg_str = re.sub(
            r'%[.\-+ ,Ll0-9]*\*[.\-+ ,Ll0-9]*[a-zA-Z]', '%_%_',
            processed_dbg_str)
        logging.debug(f"Final macro before print specifier scan: "
                      f"{processed_dbg_str}")

        print_spec_cnt = processed_dbg_str.count('%')

        # Need to take into account parentheses between args in function
        # calls that might be in the args list. Use regex module for
        # this one since the recursive pattern match helps simplify
        # only matching commas outside nested call groups.
        if macro['dbg_args'] is None:
            processed_arg_str = ""
        else:
            processed_arg_str = macro['dbg_args'].strip()

        argument_other_replacements = ['\r', '\n']
        for r in argument_other_replacements:
            if s in processed_arg_str:
                processed_arg_str = processed_arg_str.replace(s, '')
        processed_arg_str = re.sub(r'  +', ' ', processed_arg_str)

        # Handle special case of commas in arg strings - remove them for
        # final count to pick up correct number of argument separating
        # commas.
        processed_arg_str = re.sub(
                                r'([\"\'])(?:|\\.|[^\\])*?(\1)',
                                '',
                                processed_arg_str)

        arg_matches = regex.findall(
            r'(?:\((?:[^)(]+|(?R))*+\))|(,)',
            processed_arg_str,
            regex.MULTILINE)

        arg_cnt = 0
        if processed_arg_str != '':
            arg_cnt = arg_matches.count(',')

        if print_spec_cnt != arg_cnt:
            logging.error(f"File: {file_dbg_path}")
            logging.error(f"  Message         = {macro['dbg_str']}")
            logging.error(f"  Arguments       = \"{processed_arg_str}\"")
            logging.error(f"  Specifier Count = {print_spec_cnt}")
            logging.error(f"  Argument Count  = {arg_cnt}")

            failure_cnt += 1

    return failure_cnt, print_spec_cnt, arg_cnt


def get_debug_macros(file_contents: str) -> List[Dict[str, str]]:
    """Extract debug macros from the given file contents.

    Args:
        file_contents (str): A string of source file contents that may
        contain debug macros.

    Returns:
        List[Dict[str, str]]: A groupdict of debug macro regex matches
        within the file contents provided.
    """

    # This is the main regular expression that is responsible for identifying
    # DEBUG macros within source files and grouping the macro message string
    # and macro arguments strings so they can be further processed.
    r = regex.compile(
        r'(?>(?P<prologue>DEBUG\s*\(\s*\((?:.*?,))(?:\s*))(?P<dbg_str>.*?(?:\"'
        r'(?:[^\"\\]|\\.)*\".*?)*)(?:(?(?=,)(?<dbg_args>.*?(?=(?:\s*\)){2}\s*;'
        r'))))(?:\s*\)){2,};?',
        regex.MULTILINE | regex.DOTALL)
    return [m.groupdict() for m in r.finditer(file_contents)]


def check_macros_in_string(src_str: str,
                           file_dbg_path: str,
                           **macro_subs: str) -> Tuple[int, int, int]:
    """Checks for debug macro formatting errors in a string.

    Args:
        src_str (str): Contents of the string with debug macros.

        file_dbg_path (str): The file path (or other custom string) to display
        in debug messages.

        macro_subs (Dict[str,str]): Variable-length keyword and replacement
        value string pairs to substitute during debug macro checks.

    Returns:
        Tuple[int, int, int]: A tuple of the number of formatting errors,
        number of print specifiers, and number of arguments for the macros
        in the string given.
    """
    return check_debug_macros(
                get_debug_macros(src_str), file_dbg_path, **macro_subs)


def check_macros_in_file(file: PurePath,
                         file_dbg_path: str,
                         show_utf8_decode_warning: bool = False,
                         **macro_subs: str) -> Tuple[int, int, int]:
    """Checks for debug macro formatting errors in a file.

    Args:
        file (PurePath): The file path to check.

        file_dbg_path (str): The file path (or other custom string) to display
        in debug messages.

        show_utf8_decode_warning (bool, optional): Indicates whether to show
        warnings if UTF-8 files fail to decode. Defaults to False.

        macro_subs (Dict[str,str]): Variable-length keyword and replacement
        value string pairs to substitute during debug macro checks.

    Returns:
        Tuple[int, int, int]: A tuple of the number of formatting errors,
        number of print specifiers, and number of arguments for the macros
        in the file given.
    """
    try:
        return check_macros_in_string(
                    file.read_text(encoding='utf-8'), file_dbg_path,
                    **macro_subs)
    except UnicodeDecodeError as e:
        if show_utf8_decode_warning:
            logging.warning(
                f"{file_dbg_path} UTF-8 decode error.\n"
                "         Debug macro code check skipped!\n"
                f"         -> {str(e)}")
    return 0, 0, 0


def check_macros_in_directory(directory: PurePath,
                              file_extensions: Iterable[str] = ('.c',),
                              ignore_git_ignore_files: Optional[bool] = True,
                              ignore_git_submodules: Optional[bool] = True,
                              show_progress_bar: Optional[bool] = True,
                              show_utf8_decode_warning: bool = False,
                              **macro_subs: str
                              ) -> int:
    """Checks files with the given extension in the given directory for debug
       macro formatting errors.

    Args:
        directory (PurePath): The path to the directory to check.
        file_extensions (Iterable[str], optional): An iterable of strings
        representing file extensions to check. Defaults to ('.c',).

        ignore_git_ignore_files (Optional[bool], optional): Indicates whether
        files ignored by git should be ignored for the debug macro check.
        Defaults to True.

        ignore_git_submodules (Optional[bool], optional): Indicates whether
        files located in git submodules should not be checked. Defaults to
        True.

        show_progress_bar (Optional[bool], optional): Indicates whether to
        show a progress bar to show progress status while checking macros.
        This is more useful on a very large directories. Defaults to True.

        show_utf8_decode_warning (bool, optional): Indicates whether to show
        warnings if UTF-8 files fail to decode. Defaults to False.

        macro_subs (Dict[str,str]): Variable-length keyword and replacement
        value string pairs to substitute during debug macro checks.

    Returns:
        int: Count of debug macro errors in the directory.
    """
    def _get_file_list(root_directory: PurePath,
                       extensions: Iterable[str]) -> List[Path]:
        """Returns a list of files recursively located within the path.

        Args:
            root_directory (PurePath): A directory Path object to the root
            folder.

            extensions (Iterable[str]): An iterable of strings that
            represent file extensions to recursively search for within
            root_directory.

        Returns:
            List[Path]: List of file Path objects to files found in the
            given directory with the given extensions.
        """
        def _show_file_discovered_message(file_count: int,
                                          elapsed_time: float) -> None:
            print(f"\rDiscovered {file_count:,} files in",
                  f"{current_start_delta:-.0f}s"
                  f"{'.' * min(int(current_start_delta), 40)}", end="\r")

        start_time = timeit.default_timer()
        previous_indicator_time = start_time

        files = []
        for file in root_directory.rglob('*'):
            if file.suffix in extensions:
                files.append(Path(file))

            # Give an indicator progress is being made
            # This has a negligible impact on overall performance
            # with print emission limited to half second intervals.
            current_time = timeit.default_timer()
            current_start_delta = current_time - start_time

            if current_time - previous_indicator_time >= 0.5:
                # Since this rewrites the line, it can be considered a form
                # of progress bar
                if show_progress_bar:
                    _show_file_discovered_message(len(files),
                                                  current_start_delta)
                previous_indicator_time = current_time

        if show_progress_bar:
            _show_file_discovered_message(len(files), current_start_delta)
            print()

        return files

    logging.info(f"Checking Debug Macros in directory: "
                 f"{directory.resolve()}\n")

    logging.info("Gathering the overall file list. This might take a"
                 "while.\n")

    start_time = timeit.default_timer()
    file_list = set(_get_file_list(directory, file_extensions))
    end_time = timeit.default_timer() - start_time

    logging.debug(f"[PERF] File search found {len(file_list):,} files in "
                  f"{end_time:.2f} seconds.")

    if ignore_git_ignore_files:
        logging.info("Getting git ignore files...")
        start_time = timeit.default_timer()
        ignored_file_paths = GitHelpers.get_git_ignored_paths(directory)
        end_time = timeit.default_timer() - start_time

        logging.debug(f"[PERF] File ignore gathering took {end_time:.2f} "
                      f"seconds.")

        logging.info("Ignoring git ignore files...")
        logging.debug(f"File list count before git ignore {len(file_list):,}")
        start_time = timeit.default_timer()
        file_list = file_list.difference(ignored_file_paths)
        end_time = timeit.default_timer() - start_time
        logging.info(f"  {len(ignored_file_paths):,} files are ignored by git")
        logging.info(f"  {len(file_list):,} files after removing "
                     f"ignored files")

        logging.debug(f"[PERF] File ignore calculation took {end_time:.2f} "
                      f"seconds.")

    if ignore_git_submodules:
        logging.info("Ignoring git submodules...")
        submodule_paths = GitHelpers.get_git_submodule_paths(directory)
        if submodule_paths:
            logging.debug(f"File list count before git submodule exclusion "
                          f"{len(file_list):,}")
            start_time = timeit.default_timer()
            file_list = [f for f in file_list
                         if not f.is_relative_to(*submodule_paths)]
            end_time = timeit.default_timer() - start_time

            for path in enumerate(submodule_paths):
                logging.debug("  {0}. {1}".format(*path))

            logging.info(f"  {len(submodule_paths):,} submodules found")
            logging.info(f"  {len(file_list):,} files will be examined after "
                         f"excluding files in submodules")

            logging.debug(f"[PERF] Submodule exclusion calculation took "
                          f"{end_time:.2f} seconds.")
        else:
            logging.warning("No submodules found")

    logging.info(f"\nStarting macro check on {len(file_list):,} files.")

    cache_progress_filter = CacheDuringProgressFilter()
    handler = next((h for h in logging.getLogger().handlers if h.get_name() ==
                   'stdout_logger_handler'), None)

    if handler is not None:
        handler.addFilter(cache_progress_filter)

    start_time = timeit.default_timer()

    failure_cnt, file_cnt = 0, 0
    for file_cnt, file in enumerate(file_list):
        file_rel_path = str(file.relative_to(directory))
        failure_cnt += check_macros_in_file(
                            file, file_rel_path, show_utf8_decode_warning,
                            **macro_subs)[0]
        if show_progress_bar:
            _show_progress(file_cnt, len(file_list),
                           f" {failure_cnt} errors" if failure_cnt > 0 else "")

    if show_progress_bar:
        _show_progress(len(file_list), len(file_list),
                       f" {failure_cnt} errors" if failure_cnt > 0 else "")
        print("\n", flush=True)

    end_time = timeit.default_timer() - start_time

    if handler is not None:
        handler.removeFilter(cache_progress_filter)

        for record in cache_progress_filter.message_cache:
            handler.emit(record)

    logging.debug(f"[PERF] The macro check operation took {end_time:.2f} "
                  f"seconds.")

    _log_failure_count(failure_cnt, file_cnt)

    return failure_cnt


def _log_failure_count(failure_count: int, file_count: int) -> None:
    """Logs the failure count.

    Args:
        failure_count (int): Count of failures to log.

        file_count (int): Count of files with failures.
    """
    if failure_count > 0:
        logging.error("\n")
        logging.error(f"{failure_count:,} debug macro errors in "
                      f"{file_count:,} files")


def _show_progress(step: int, total: int, suffix: str = '') -> None:
    """Print progress of tick to total.

    Args:
        step (int): The current step count.

        total (int): The total step count.

        suffix (str): String to print at the end of the progress bar.
    """
    global _progress_start_time

    if step == 0:
        _progress_start_time = timeit.default_timer()

    terminal_col = shutil.get_terminal_size().columns
    var_consume_len = (len("Progress|\u2588| 000.0% Complete 000s") +
                       len(suffix))
    avail_len = terminal_col - var_consume_len

    percent = f"{100 * (step / float(total)):3.1f}"
    filled = int(avail_len * step // total)
    bar = '\u2588' * filled + '-' * (avail_len - filled)
    step_time = timeit.default_timer() - _progress_start_time

    print(f'\rProgress|{bar}| {percent}% Complete {step_time:-3.0f}s'
          f'{suffix}', end='\r')


def _module_invocation_check_macros_in_directory_wrapper() -> int:
    """Provides an command-line argument wrapper for checking debug macros.

    Returns:
        int: The system exit code value.
    """
    import argparse
    import builtins

    def _check_dir_path(dir_path: str) -> bool:
        """Returns the absolute path if the path is a directory."

        Args:
            dir_path (str): A directory file system path.

        Raises:
            NotADirectoryError: The directory path given is not a directory.

        Returns:
            bool: True if the path is a directory else False.
        """
        abs_dir_path = os.path.abspath(dir_path)
        if os.path.isdir(dir_path):
            return abs_dir_path
        else:
            raise NotADirectoryError(abs_dir_path)

    def _check_file_path(file_path: str) -> bool:
        """Returns the absolute path if the path is a file."

        Args:
            file_path (str): A file path.

        Raises:
            FileExistsError: The path is not a valid file.

        Returns:
            bool: True if the path is a valid file else False.
        """
        abs_file_path = os.path.abspath(file_path)
        if os.path.isfile(file_path):
            return abs_file_path
        else:
            raise FileExistsError(file_path)

    def _quiet_print(*args, **kwargs):
        """Replaces print when quiet is requested to prevent printing messages.
        """
        pass

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)

    stdout_logger_handler = logging.StreamHandler(sys.stdout)
    stdout_logger_handler.set_name('stdout_logger_handler')
    stdout_logger_handler.setLevel(logging.INFO)
    stdout_logger_handler.setFormatter(logging.Formatter('%(message)s'))
    root_logger.addHandler(stdout_logger_handler)

    parser = argparse.ArgumentParser(
                        prog=PROGRAM_NAME,
                        description=(
                            "Checks for debug macro formatting "
                            "errors within files recursively located within "
                            "a given directory."),
                        formatter_class=RawTextHelpFormatter)

    io_req_group = parser.add_mutually_exclusive_group(required=True)
    io_opt_group = parser.add_argument_group(
                            "Optional input and output")
    git_group = parser.add_argument_group("Optional git control")

    io_req_group.add_argument('-w', '--workspace-directory',
                              type=_check_dir_path,
                              help="Directory of source files to check.\n\n")

    io_req_group.add_argument('-i', '--input-file', nargs='?',
                              type=_check_file_path,
                              help="File path for an input file to check.\n\n"
                                   "Note that some other options do not apply "
                                   "if a single file is specified such as "
                                   "the\ngit options and file extensions.\n\n")

    io_opt_group.add_argument('-l', '--log-file',
                              nargs='?',
                              default=None,
                              const='debug_macro_check.log',
                              help="File path for log output.\n"
                                   "(default: if the flag is given with no "
                                   "file path then a file called\n"
                                   "debug_macro_check.log is created and used "
                                   "in the current directory)\n\n")

    io_opt_group.add_argument('-s', '--substitution-file',
                              type=_check_file_path,
                              help="A substitution YAML file specifies string "
                                   "substitutions to perform within the debug "
                                   "macro.\n\nThis is intended to be a simple "
                                   "mechanism to expand the rare cases of pre-"
                                   "processor\nmacros without directly "
                                   "involving the pre-processor. The file "
                                   "consists of one or more\nstring value "
                                   "pairs where the key is the identifier to "
                                   "replace and the value is the value\nto "
                                   "replace it with.\n\nThis can also be used "
                                   "as a method to ignore results by "
                                   "replacing the problematic string\nwith a "
                                   "different string.\n\n")

    io_opt_group.add_argument('-v', '--verbose-log-file',
                              action='count',
                              default=0,
                              help="Set file logging verbosity level.\n"
                                   " - None:    Info & > level messages\n"
                                   " - '-v':    + Debug level messages\n"
                                   " - '-vv':   + File name and function\n"
                                   " - '-vvv':  + Line number\n"
                                   " - '-vvvv': + Timestamp\n"
                                   "(default: verbose logging is not enabled)"
                                   "\n\n")

    io_opt_group.add_argument('-n', '--no-progress-bar', action='store_true',
                              help="Disables progress bars.\n"
                                   "(default: progress bars are used in some"
                                   "places to show progress)\n\n")

    io_opt_group.add_argument('-q', '--quiet', action='store_true',
                              help="Disables console output.\n"
                                   "(default: console output is enabled)\n\n")

    io_opt_group.add_argument('-u', '--utf8w', action='store_true',
                              help="Shows warnings for file UTF-8 decode "
                                   "errors.\n"
                                   "(default: UTF-8 decode errors are not "
                                   "shown)\n\n")

    git_group.add_argument('-df', '--do-not-ignore-git-ignore-files',
                           action='store_true',
                           help="Do not ignore git ignored files.\n"
                                "(default: files in git ignore files are "
                                "ignored)\n\n")

    git_group.add_argument('-ds', '--do-not-ignore-git_submodules',
                           action='store_true',
                           help="Do not ignore files in git submodules.\n"
                                "(default: files in git submodules are "
                                "ignored)\n\n")

    parser.add_argument('-e', '--extensions', nargs='*', default=['.c'],
                        help="List of file extensions to include.\n"
                             "(default: %(default)s)")

    args = parser.parse_args()

    if args.quiet:
        # Don't print in the few places that directly print
        builtins.print = _quiet_print
    stdout_logger_handler.addFilter(QuietFilter(args.quiet))

    if args.log_file:
        file_logger_handler = logging.FileHandler(filename=args.log_file,
                                                  mode='w', encoding='utf-8')

        # In an ideal world, everyone would update to the latest Python
        # minor version (3.10) after a few weeks/months. Since that's not the
        # case, resist from using structural pattern matching in Python 3.10.
        # https://peps.python.org/pep-0636/

        if args.verbose_log_file == 0:
            file_logger_handler.setLevel(logging.INFO)
            file_logger_formatter = logging.Formatter(
                '%(levelname)-8s %(message)s')
        elif args.verbose_log_file == 1:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                '%(levelname)-8s %(message)s')
        elif args.verbose_log_file == 2:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                '[%(filename)s - %(funcName)20s() ] %(levelname)-8s '
                '%(message)s')
        elif args.verbose_log_file == 3:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                '[%(filename)s:%(lineno)s - %(funcName)20s() ] '
                '%(levelname)-8s %(message)s')
        elif args.verbose_log_file == 4:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                '%(asctime)s [%(filename)s:%(lineno)s - %(funcName)20s() ]'
                ' %(levelname)-8s %(message)s')
        else:
            file_logger_handler.setLevel(logging.DEBUG)
            file_logger_formatter = logging.Formatter(
                '%(asctime)s [%(filename)s:%(lineno)s - %(funcName)20s() ]'
                ' %(levelname)-8s %(message)s')

        file_logger_handler.addFilter(ProgressFilter())
        file_logger_handler.setFormatter(file_logger_formatter)
        root_logger.addHandler(file_logger_handler)

    logging.info(PROGRAM_NAME + "\n")

    substitution_data = {}
    if args.substitution_file:
        logging.info(f"Loading substitution file {args.substitution_file}")
        with open(args.substitution_file, 'r') as sf:
            substitution_data = yaml.safe_load(sf)

    if args.workspace_directory:
        return check_macros_in_directory(
                    Path(args.workspace_directory),
                    args.extensions,
                    not args.do_not_ignore_git_ignore_files,
                    not args.do_not_ignore_git_submodules,
                    not args.no_progress_bar,
                    args.utf8w,
                    **substitution_data)
    else:
        curr_dir = Path(__file__).parent
        input_file = Path(args.input_file)

        rel_path = str(input_file)
        if input_file.is_relative_to(curr_dir):
            rel_path = str(input_file.relative_to(curr_dir))

        logging.info(f"Checking Debug Macros in File: "
                     f"{input_file.resolve()}\n")

        start_time = timeit.default_timer()
        failure_cnt = check_macros_in_file(
                        input_file,
                        rel_path,
                        args.utf8w,
                        **substitution_data)[0]
        end_time = timeit.default_timer() - start_time

        logging.debug(f"[PERF] The file macro check operation took "
                      f"{end_time:.2f} seconds.")

        _log_failure_count(failure_cnt, 1)

        return failure_cnt


if __name__ == '__main__':
    # The exit status value is the number of macro formatting errors found.
    # Therefore, if no macro formatting errors are found, 0 is returned.
    # Some systems require the return value to be in the range 0-127, so
    # a lower maximum of 100 is enforced to allow a wide range of potential
    # values with a reasonably large maximum.
    try:
        sys.exit(max(_module_invocation_check_macros_in_directory_wrapper(),
                 100))
    except KeyboardInterrupt:
        logging.warning("Exiting due to keyboard interrupt.")
        # Actual formatting errors are only allowed to reach 100.
        # 101 signals a keyboard interrupt.
        sys.exit(101)
    except FileExistsError as e:
        # 102 signals a file not found error.
        logging.critical(f"Input file {e.args[0]} does not exist.")
        sys.exit(102)
