# Debug Macro Check

This Python application scans all files in a build package for debug macro formatting issues. It is intended to be a
fundamental build-time check that is part of a normal developer build process to catch errors right away.

As a build plugin, it is capable of finding these errors early in the development process after code is initially
written to ensure that all code tested is free of debug macro formatting errors. These errors often creep into debug
prints in error conditions that are not frequently executed making debug even more difficult and confusing when they
are encountered. In other cases, debug macros with these errors in the main code path can lead to unexpected behavior
when executed. As a standalone script, it can be easily run manually or integrated into other CI processes.

The plugin is part of a set of debug macro check scripts meant to be relatively portable so they can be applied to
additional code bases with minimal effort.

## 1. BuildPlugin/DebugMacroCheckBuildPlugin.py

This is the build plugin. It is discovered within the Stuart Self-Describing Environment (SDE) due to the accompanying
file `DebugMacroCheck_plugin_in.yaml`.

Since macro errors are considered a coding bug that should be found and fixed during the build phase of the developer
process (before debug and testing), this plugin is run in pre-build. It will run within the scope of the package
being compiled. For a platform build, this means it will run against the package being built. In a CI build, it will
run in pre-build for each package as each package is built.

The build plugin has the following attributes:

  1. Registered at `global` scope. This means it will always run.

  2. Called only on compilable build targets (i.e. does nothing on `"NO-TARGET"`).

  3. Runs as a pre-build step. This means it gives results right away to ensure compilation follows on a clean slate.
     This also means it runs in platform build and CI. It is run in CI as a pre-build step when the `CompilerPlugin`
     compiles code. This ensures even if the plugin was not run locally, all code submissions have been checked.

  4. Reports any errors in the build log and fails the build upon error making it easy to discover problems.

  5. Supports two methods of configuration via "substitution strings":

     1. By setting a build variable called `DEBUG_MACRO_CHECK_SUB_FILE` with the name of a substitution YAML file to
        use.

        **Example:**

        ```python
        shell_environment.GetBuildVars().SetValue(
                                            "DEBUG_MACRO_CHECK_SUB_FILE",
                                            os.path.join(self.GetWorkspaceRoot(), "DebugMacroCheckSub.yaml"),
                                            "Set in CISettings.py")
        ```

        **Substitution File Content Example:**

        ```yaml
        ---
        # OvmfPkg/CpuHotplugSmm/ApicId.h
        # Reason: Substitute with macro value
        FMT_APIC_ID: 0x%08x

        # DynamicTablesPkg/Include/ConfigurationManagerObject.h
        # Reason: Substitute with macro value
        FMT_CM_OBJECT_ID: 0x%lx

        # OvmfPkg/IntelTdx/TdTcg2Dxe/TdTcg2Dxe.c
        # Reason: Acknowledging use of two format specifiers in string with one argument
        #         Replace ternary operator in debug string with single specifier
        'Index == COLUME_SIZE/2 ? " | %02x" : " %02x"': "%d"

        # DynamicTablesPkg/Library/Common/TableHelperLib/ConfigurationManagerObjectParser.c
        # ShellPkg/Library/UefiShellAcpiViewCommandLib/AcpiParser.c
        # Reason: Acknowledge that string *should* expand to one specifier
        #         Replace variable with expected number of specifiers (1)
        Parser[Index].Format: "%d"
        ```

     2. By entering the string substitutions directory into a dictionary called `StringSubstitutions` in a
        `DebugMacroCheck` section of the package CI YAML file.

        **Example:**

        ```yaml
        "DebugMacroCheck": {
          "StringSubstitutions": {
            "SUB_A": "%Lx"
          }
        }
        ```

### Debug Macro Check Build Plugin: Simple Disable

The build plugin can simply be disabled by setting an environment variable named `"DISABLE_DEBUG_MACRO_CHECK"`. The
plugin is disabled on existence of the variable. The contents of the variable are not inspected at this time.

## 2. DebugMacroCheck.py

This is the main Python module containing the implementation logic. The build plugin simply wraps around it.

When first running debug macro check against a new, large code base, it is recommended to first run this standalone
script and address all of the issues and then enable the build plugin.

The module supports a number of configuration parameters to ease debug of errors and to provide flexibility for
different build environments.

### EDK 2 PyTool Library Dependency

This script has minimal library dependencies. However, it has one dependency you might not be familiar with on the
Tianocore EDK 2 PyTool Library (edk2toollib):

```py
from edk2toollib.utility_functions import RunCmd
```

You simply need to install the following pip module to use this library: `edk2-pytool-library`
(e.g. `pip install edk2-pytool-library`)

More information is available here:

- PyPI page: [edk2-pytool-library](https://pypi.org/project/edk2-pytool-library/)
- GitHub repo: [tianocore/edk2-pytool-library](https://github.com/tianocore/edk2-pytool-library)

If you strongly prefer not including this additional dependency, the functionality imported here is relatively
simple to substitute with the Python [`subprocess`](https://docs.python.org/3/library/subprocess.html) built-in
module.

### Examples

Simple run against current directory:

`> python DebugMacroCheck.py -w .`

Simple run against a single file:

`> python DebugMacroCheck.py -i filename.c`

Run against a directory with output placed into a file called "debug_macro_check.log":

`> python DebugMacroCheck.py -w . -l`

Run against a directory with output placed into a file called "custom.log" and debug log messages enabled:

`> python DebugMacroCheck.py -w . -l custom.log -v`

Run against a directory with output placed into a file called "custom.log", with debug log messages enabled including
python script function and line number, use a substitution file called "file_sub.yaml", do not show the progress bar,
and run against .c and .h files:

`> python DebugMacroCheck.py -w . -l custom.log -vv -s file_sub.yaml -n -e .c .h`

> **Note**: It is normally not recommended to run against .h files as they and many other non-.c files normally do
  not have full `DEBUG` macro prints.

```plaintext
usage: Debug Macro Checker [-h] (-w WORKSPACE_DIRECTORY | -i [INPUT_FILE]) [-l [LOG_FILE]] [-s SUBSTITUTION_FILE] [-v] [-n] [-q] [-u]
                           [-df] [-ds] [-e [EXTENSIONS ...]]

Checks for debug macro formatting errors within files recursively located within a given directory.

options:
  -h, --help            show this help message and exit
  -w WORKSPACE_DIRECTORY, --workspace-directory WORKSPACE_DIRECTORY
                        Directory of source files to check.

  -i [INPUT_FILE], --input-file [INPUT_FILE]
                        File path for an input file to check.

                        Note that some other options do not apply if a single file is specified such as the
                        git options and file extensions.

  -e [EXTENSIONS ...], --extensions [EXTENSIONS ...]
                        List of file extensions to include.
                        (default: ['.c'])

Optional input and output:
  -l [LOG_FILE], --log-file [LOG_FILE]
                        File path for log output.
                        (default: if the flag is given with no file path then a file called
                        debug_macro_check.log is created and used in the current directory)

  -s SUBSTITUTION_FILE, --substitution-file SUBSTITUTION_FILE
                        A substitution YAML file specifies string substitutions to perform within the debug macro.

                        This is intended to be a simple mechanism to expand the rare cases of pre-processor
                        macros without directly involving the pre-processor. The file consists of one or more
                        string value pairs where the key is the identifier to replace and the value is the value
                        to replace it with.

                        This can also be used as a method to ignore results by replacing the problematic string
                        with a different string.

  -v, --verbose-log-file
                        Set file logging verbosity level.
                         - None:    Info & > level messages
                         - '-v':    + Debug level messages
                         - '-vv':   + File name and function
                         - '-vvv':  + Line number
                         - '-vvvv': + Timestamp
                        (default: verbose logging is not enabled)

  -n, --no-progress-bar
                        Disables progress bars.
                        (default: progress bars are used in some places to show progress)

  -q, --quiet           Disables console output.
                        (default: console output is enabled)

  -u, --utf8w           Shows warnings for file UTF-8 decode errors.
                        (default: UTF-8 decode errors are not shown)


Optional git control:
  -df, --do-not-ignore-git-ignore-files
                        Do not ignore git ignored files.
                        (default: files in git ignore files are ignored)

  -ds, --do-not-ignore-git_submodules
                        Do not ignore files in git submodules.
                        (default: files in git submodules are ignored)
```

## String Substitutions

`DebugMacroCheck` currently runs separate from the compiler toolchain. This has the advantage that it is very portable
and can run early in the build process, but it also means pre-processor macro expansion does not happen when it is
invoked.

In practice, it has been very rare that this is an issue for how most debug macros are written. In case it is, a
substitution file can be used to inform `DebugMacroCheck` about the string substitution the pre-processor would
perform.

This pattern should be taken as a warning. It is just as difficult for humans to keep debug macro specifiers and
arguments balanced as it is for `DebugMacroCheck` pre-processor macro substitution is used. By separating the string
from the actual arguments provided, it is more likely for developers to make mistakes matching print specifiers in
the string to the arguments. If usage is reasonable, a string substitution can be used as needed.

### Ignoring Errors

Since substitution files perform a straight textual substitution in macros discovered, it can be used to replace
problematic text with text that passes allowing errors to be ignored.

## Python Version Required (3.10)

This script is written to take advantage of new Python language features in Python 3.10. If you are not using Python
3.10 or later, you can:

  1. Upgrade to Python 3.10 or greater
  2. Run this script in a [virtual environment](https://docs.python.org/3/tutorial/venv.html) with Python 3.10
     or greater
  3. Customize the script for compatibility with your Python version

These are listed in order of recommendation. **(1)** is the simplest option and will upgrade your environment to a
newer, safer, and better Python experience. **(2)** is the simplest approach to isolate dependencies to what is needed
to run this script without impacting the rest of your system environment. **(3)** creates a one-off fork of the script
that, by nature, has a limited lifespan and will make accepting future updates difficult but can be done with relatively
minimal effort back to recent Python 3 releases.
