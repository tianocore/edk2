# UncrustifyCheck Plugin

This CiBuildPlugin scans all the files in a given package and checks for coding standard compliance issues.

This plugin is enabled by default. If a package would like to prevent the plugin from reporting errors, it can do
so by enabling [`AuditOnly`](#auditonly) mode.

This plugin requires the directory containing the Uncrustify executable that should be used for this plugin to
be specified in an environment variable named `UNCRUSTIFY_CI_PATH`. This unique variable name is used to avoid confusion
with other paths to Uncrustify which might not be the expected build for use by this plugin.

By default, an Uncrustify configuration file named "uncrustify.cfg" located in the same directory as the plugin is
used. The value can be overridden to a package-specific path with the `ConfigFilePath` configuration file option.

* Uncrustify source code and documentation: https://github.com/uncrustify/uncrustify
* Project Mu Uncrustify fork source code and documentation: https://dev.azure.com/projectmu/Uncrustify

## Files Checked in a Package

By default, this plugin will discover all files in the package with the following default paths:

```python
[
# C source
"*.c",
"*.h"
]
```

From this list of files, any files ignored by Git or residing in a Git submodule will be removed. If Git is not
found, submodules are not found, or ignored files are not found no changes are made to the list of discovered files.

To control the paths checked in a given package, review the configuration options described in this file.

## Configuration

The plugin can be configured with a few optional configuration options.

``` yaml
  "UncrustifyCheck": {
      "AdditionalIncludePaths": [], # Additional paths to check formatting (wildcards supported).
      "AuditOnly": False,           # Don't fail the build if there are errors.  Just log them.
      "ConfigFilePath": "",         # Custom path to an Uncrustify config file.
      "IgnoreFiles": [],            # A list of file patterns to ignore.
      "IgnoreStandardPaths": [],    # Standard Plugin defined paths that should be ignored.
      "OutputFileDiffs": True,      # Output chunks of formatting diffs in the test case log.
                                    # This can significantly slow down the plugin on very large packages.
      "SkipGitExclusions": False    # Don't exclude git ignored files and files in git submodules.
  }
```

### `AdditionalIncludePaths`

A package configuration file can specify any additional paths to be included with this option.

At this time, it is recommended all files run against the plugin be written in the C or C++ language.

### `AuditOnly`

`Boolean` - Default is `False`.

If `True`, run the test in an "audit only mode" which will log all errors but instead of failing the build, it will set
the test as skipped. This allows visibility into the failures without breaking the build.

### `ConfigFilePath`

`String` - Default is `"uncrustify.cfg"`

When specified in the config file, this is a package relative path to the Uncrustify configuration file.

### `IgnoreFiles`

This option supports .gitignore file and folder matching strings including wildcards.

The files specified by this configuration option will not be processed by Uncrustify.

### `IgnoreStandardPaths`

This plugin by default will check the below standard paths. A package configuration file can specify any of these paths
to be ignored.

```python
[
# C source
"*.c",
"*.h"
]
```

### `OutputFileDiffs`

`Boolean` - Default is `True`.

If `True`, output diffs of formatting changes into the test case log. This is helpful to exactly understand what changes
need to be made to the source code in order to fix a coding standard compliance issue.

Note that calculating the file diffs on a very large set of of results (e.g. >100 files) can significantly slow down
plugin execution.

### `SkipGitExclusions`

`Boolean` - Default is `False`.

By default, files in paths matched in a .gitignore file or a recognized git submodule are excluded. If this option
is `True`, the plugin will not attempt to recognize these files and exclude them.

## High-Level Plugin Operation

This plugin generates two main sets of temporary files:

  1. A working directory in the directory `Build/.pytool/Plugin/Uncrustify`
  2. For each source file with formatting errors, a sibling file with the `.uncrustify_plugin` extension

The working directory contains temporary files unique to operation of the plugin. All of these files are removed on
exit of the plugin including successful or unsuccessful execution (such as a Python exception occurring). If for any
reason, any files in the package exist prior to running the plugin with the `.uncrustify_plugin` extension, the plugin
will inform the user to remove these files and exit before running Uncrustify. This is to ensure the accuracy of the
results reported from each execution instance of the plugin.

The plugin determines the list of relevant files to check with Uncrustify and then invokes Uncrustify with that file
list. For any files not compliant to the configuration file provided, Uncrustify will generate a corresponding file
with the `.uncrustify_plugin` extension. The plugin discovers all of these files. If any such files are present, this
indicates a formatting issue was found and the test is marked failed (unless `AuditOnly` mode is enabled).

The test case log will contain a report of which files failed to format properly, allowing the user to run Uncrustify
against the file locally to fix the issue. If the `OutputFileDiffs` configuration option is set to `True`, the plugin
will output diff chunks for all code formatting issues in the test case log.
