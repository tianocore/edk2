# CodeQL Plugin

The set of CodeQL plugins provided include two main plugins that seamlessly integrate into a Stuart build environment:

1. `CodeQlBuildPlugin` - Used to produce a CodeQL database from a build.
2. `CodeQlAnalyzePlugin` - Used to analyze a CodeQL database.

While CodeQL can be run in a CI environment with other approaches. This plugin offers the following advantages:

1. Provides exactly the same results locally as on a CI server.
2. Integrates very well into VS Code.
3. Very simple to use - just use normal Stuart update and build commands.
4. Very simple to understand - minimally wraps the official CodeQL CLI.
5. Very simple to integrate - works like any other Stuart build plugin.
   - Integration is usually just a few lines of code.
6. Portable - not tied to Azure DevOps specific, GitHub specific, or other host infrastructure.
7. Versioned - the query and filters are versioned in source control so easy to find and track.

It is very important to read the Integration Instructions in this file and determine how to best integrate the
CodeQL plugin into your environment.

Due to the total size of dependencies required to run CodeQL and the flexibility needed by a platform to determine what
CodeQL queries to run and how to interpret results, a number of configuration options are provided to allow a high
degree of flexibility during platform integration.

This document is focused on those setting up the CodeQL plugin in their environment. Once setup, end users simply need
to use their normal build commands and process and CodeQL will be integrated with it. The most relevant section for
such users is [Local Development Tips](#local-development-tips).

## Table of Contents

1. [Database and Analysis Result Locations](#database-and-analysis-result-locations)
2. [Global Configuration](#global-configuration)
3. [Package-Specific Configuration](#package-specific-configuration)
4. [Filter Patterns](#filter-patterns)
5. [Integration Instructions](#integration-instructions)
   - [Integration Step 1 - Choose Scopes](#integration-step-1---choose-scopes)
     - [Scopes Available](#scopes-available)
   - [Integration Step 2 - Choose CodeQL Queries](#integration-step-2---choose-codeql-queries)
   - [Integration Step 3 - Determine Global Configuration Values](#integration-step-3---determine-global-configuration-values)
   - [Integration Step 4 - Determine Package-Specific Configuration Values](#integration-step-4---determine-package-specific-configuration-values)
   - [Integration Step 5 - Testing](#integration-step-5---testing)
   - [Integration Step 6 - Define Inclusion and Exclusion Filter Patterns](#integration-step-6---define-inclusion-and-exclusion-filter-patterns)
6. [High-Level Operation](#high-level-operation)
   - [CodeQlBuildPlugin](#codeqlbuildplugin)
   - [CodeQlAnalyzePlugin](#codeqlanalyzeplugin)
7. [Local Development Tips](#local-development-tips)
8. [Resolution Guidelines](#resolution-guidelines)

## Database and Analysis Result Locations

The CodeQL database is written to a directory unique to the package and target being built:

  `Build/codeql-db-<package>-<target>-<instance>`

For example: `Build/codeql-db-mdemodulepkg-debug-0`

The plugin does not delete or overwrite existing databases, the instance value is simply increased. This is
because databases are large, take a long time to generate, and are important for reproducing analysis results. The user
is responsible for deleting database directories when they are no longer needed.

Similarly, analysis results are written to a directory unique to the package and target. For analysis, results are
stored in individual files so those files are stored in a single directory.

For example, all analysis results for the above package and target will be stored in:
  `codeql-analysis-mdemodulepkg-debug`

CodeQL results are stored in [SARIF](https://sarifweb.azurewebsites.net/) (Static Analysis Results Interchange Format)
([CodeQL SARIF documentation](https://codeql.github.com/docs/codeql-cli/sarif-output/)) files. Each SARIF file
corresponding to a database will be stored in a file with an instance matching the database instance.

For example, the analysis result file for the above database would be stored in this file:
  `codeql-analysis-mdemodulepkg-debug/codeql-db-mdemodulepkg-debug-0.sarif`

Result files are overwritten. This is because result files are quick to generate and need to represent the latest
results for the last analysis operation performed. The user is responsible for backing up SARIF result files if they
need to saved.

## Global Configuration

Global configuration values are specified with build environment variables.

These values are all optional. They provide a convenient mechanism for a build script to set the value for all packages
built by the script.

- `STUART_CODEQL_AUDIT_ONLY` - If `true` (case insensitive), `CodeQlAnalyzePlugin` will be in audit-only mode. In this
  mode all CodeQL failures are ignored.
- `STUART_CODEQL_PATH` - The path to the CodeQL CLI application to use.
- `STUART_CODEQL_QUERY_SPECIFIERS` - The CodeQL CLI query specifiers to use. See [Running codeql database analyze](https://codeql.github.com/docs/codeql-cli/analyzing-databases-with-the-codeql-cli/#running-codeql-database-analyze)
  for possible options.
- `STUART_CODEQL_FILTER_FILES` - The path to "filter" files that contains filter patterns as described in
  [Filter Patterns](#filter-patterns).
  - More than one file may be specified by separating each absolute file path with a comma.
    - This might be useful to reference a global filter file from an upstream repo and also include a global filter
      file for the local repo.
    - Filters are concatenated in the order of files in the variable. Patterns in later files can override patterns
      in earlier files.
  - The file only needs to contain a list of filter pattern strings under a `"Filters"` key. For example:

    ```yaml
      {
        "Filters": [
          "<pattern-line-1>",
          "<pattern-line-2>"
        ]
      }
      ...
    ```

    Comments are allowed in the filter files and begin with `#` (like a normal YAML file).

## Package-Specific Configuration

Package-specific configuration values reuse existing package-level configuration approaches to simplify adjusting
CodeQL plugin behavior per package.

These values are all optional. They provide a convenient mechanism for a package owner to adjust settings specific to
the package.

``` yaml
  "CodeQlAnalyze": {
      "AuditOnly": False,         # Don't fail the build if there are errors. Just log them.
      "QuerySpecifiers": ""       # Query specifiers to pass to CodeQL CLI.
      "Filters": ""               # Inclusion/exclusion filters
  }
```

> _NOTE:_ If a global filter set is provided via `STUART_CODEQL_FILTER_FILES` and a package has a package-specific
> list, then the package-specific filter list (in a package CI YAML file) is appended onto the global filter list and
> may be used to override settings in the global list.

The format used to specify items in `"Filters"` is specified in [Filter Patterns](#filter-patterns).

## Filter Patterns

As you inspect results, you may want to include or exclude certain sets of results. For example, exclude some files by
file path entirely or adjust the CodeQL rule applied to a certain file. This plugin reuses logic from a popular
GitHub Action called [`filter-sarif`](https://github.com/advanced-security/filter-sarif) to allow filtering as part of
the plugin analysis process.

If any results are excluded using filters, the results are removed from the SARIF file. This allows the exclude results
seen locally to exactly match the results on the CI server.

Read the ["Patterns"](https://github.com/advanced-security/filter-sarif#patterns) section there for more details. The
patterns section is also copied below with some updates to make the information more relevant for an edk2 codebase
for convenience.

Each pattern line is of the form:

```plaintext
[+/-]<file pattern>[:<rule pattern>]
```

For example:

```yaml
-**/*Test*.c:**             # exclusion pattern: remove all alerts from all test files
-**/*Test*.c                # ditto, short form of the line above
+**/*.c:cpp/infiniteloop    # inclusion pattern: This line has precedence over the first two
                            # and thus "allow lists" alerts of type "cpp/infiniteloop"
**/*.c:cpp/infiniteloop     # ditto, the "+" in inclusion patterns is optional
**                          # allow all alerts in all files (reverses all previous lines)
```

- The path separator character in patterns is always `/`, independent of the platform the code is running on and
  independent of the paths in the SARIF file.
- `*` matches any character, except a path separator
- `**` matches any character and is only allowed between path separators, e.g. `/**/file.txt`, `**/file.txt` or `**`.
  NOT allowed: `**.txt`, `/etc**`
- The rule pattern is optional. If omitted, it will apply to alerts of all types.
- Subsequent lines override earlier ones. By default all alerts are included.
- If you need to use the literals `+`, `-`, `\` or `:` in your pattern, you can escape them with `\`, e.g.
  `\-this/is/an/inclusion/file/pattern\:with-a-semicolon:and/a/rule/pattern/with/a/\\/backslash`. For `+` and `-`, this
  is only necessary if they appear at the beginning of the pattern line.

## Integration Instructions

First, note that most CodeQL CLI operations will take a long time the first time they are run. This is due to:

1. Downloads - Downloading the CodeQL CLI binary (during `stuart_update`) and downloading CodeQL queries during
   CodeQL plugin execution
2. Cache not established - CodeQL CLI caches data as it performs analysis. The first time analysis is performed will
   take more time than in the future.

Second, these are build plugins. This means a build needs to take place for the plugins to run. This typically happens
in the following two scenarios:

1. `stuart_build` - A single package is built and the build process is started by the stuart tools.
2. `stuart_ci_build` - A number of packages may be built and the build process is started by the `CompilerPlugin`.

In any case, each time a package is built, the CodeQL plugins will be run if their scopes are active.

### Integration Step 1 - Choose Scopes

Decide which scopes need to be enabled in your platform, see [Scopes Available](#scopes-available).

Consider using a build profile to enable CodeQL so developers and pipelines can use the profile when they are
interested in CodeQL results but in other cases they can easily work without CodeQL in the way.

Furthermore, build-script specific command-line parameters might be useful to control CodeQL scopes and other
behavior.

#### Scopes Available

This CodeQL plugin leverages scopes to control major pieces of functionality. Any combination of scopes can be
returned from the `GetActiveScopes()` function in the platform settings manager to add and remove functionality.

Plugin scopes:

- `codeql-analyze` - Activate `CodeQlAnalyzePlugin` to perform post-build analysis of the last generated database for
  the package and target specified.
- `codeql-build` - Activate `CodeQlBuildPlugin` to hook the firmware build in pre-build such that the build will
  generate a CodeQL database during build.

In most cases, to perform a full CodeQL run, `codeql-build` should be enabled so a new CodeQL database is generated
during build and `codeql-analyze` should be be enabled so analysis of that database is performed after the build is
completed.

External dependency scopes:

- `codeql-ext-dep` - Downloads the cross-platform CodeQL CLI as an external dependency.
- `codeql-linux-ext-dep` - Downloads the Linux CodeQL CLI as an external dependency.
- `codeql-windows-ext-dep` - Downloads the Windows CodeQL CLI as an external dependency.

Note, that the CodeQL CLI is large in size. Sizes as of the [v2.11.2 release](https://github.com/github/codeql-cli-binaries/releases/tag/v2.11.2).

| Cross-platform |  Linux | Windows |
|:--------------:|:------:|:-------:|
|     934 MB     | 415 MB |  290 MB |

Therefore, the following is recommended:

1. **Ideal** - Create container images for build agents and install the CodeQL CLI for the container OS into the
   container.
2. Leverage host-OS detection (e.g. [`GetHostInfo()`](https://github.com/tianocore/edk2-pytool-library/blob/42ad6561af73ba34564f1577f64f7dbaf1d0a5a2/edk2toollib/utility_functions.py#L112))
to set the scope for the appropriate operating system. This will download the much smaller OS-specific application.

> _NOTE:_ You should never have more than one CodeQL external dependency scope enabled at a time.

### Integration Step 2 - Choose CodeQL Queries

Determine which queries need to be run against packages in your repo. In most cases, the same set of queries will be
run against all packages. It is also possible to customize the queries run at the package level.

The default set of Project Mu CodeQL queries is specified in the `MuCodeQlQueries.qls` file in this plugin.

> _NOTE:_ The queries in `MuCodeQlQueries.qls` may change at any time. If you do not want these changes to impact
> your platform, do not relay on option (3).

The plugin decides what queries to run based on the following, in order of preference:

1. Package CI YAML file query specifier
2. Build environment variable query specifier
3. Plugin default query set file

For details on how to set (1) and (2), see the Package CI Configuration and Environment Variable sections respectively.

> _NOTE:_ The value specified is directly passed as a `query specifier` to CodeQL CLI. Therefore, the arguments
> allowed by the `<query-specifiers>` argument of CodeQL CLI are allowed here. See
> [Running codeql database analyze](https://codeql.github.com/docs/codeql-cli/analyzing-databases-with-the-codeql-cli/#running-codeql-database-analyze).

A likely scenario is that a platform needs to run local/closed source queries in addition to the open-source queries.
There's various ways to handle that:

1. Create a query specifier that includes all the queries needed, both public and private and use that query specifier,
   either globally or at package-level.

   For example, at the global level - `STUART_CODEQL_QUERY_SPECIFIERS` = _"Absolute_path_to_AllMyQueries.qls"_

2. Specify a query specifier that includes the closed sources queries and reuse the public query list provided by
   this plugin.

   For example, at the global level - `STUART_CODEQL_QUERY_SPECIFIERS` = _"Absolute_path_to_MuCodeQlQueries.qls
   Absolute_path_to_ClosedSourceQueries.qls"_

Refer to the CodeQL documentation noted above on query specifiers to devise other options.

### Integration Step 3 - Determine Global Configuration Values

Review the Environment Variable section to determine which, if any, global values need to be set in your build script.

### Integration Step 4 - Determine Package-Specific Configuration Values

Review the Package CI Configuration section to determine which, if any, global values need to be set in your
package's CI YAML file.

### Integration Step 5 - Testing

Verify a `stuart_update` and `stuart_build` (or `stuart_ci_build`) command work.

### Integration Step 6 - Define Inclusion and Exclusion Filter Patterns

After reviewing the test results from Step 5, determine if you need to apply any filters as described in
[Filter Patterns](#filter-patterns).

## High-Level Operation

This section summarizes the complete CodeQL plugin flow. This is to help developers understand basic theory of
operation behind the plugin and can be skipped by anyone not interested in those details.

### CodeQlBuildPlugin

1. Register a pre-build hook
2. Determine the package and target being built
3. Determine the best CodeQL CLI path to use
   - First choice, the `STUART_CODEQL_PATH` environment variable
     - Note: This is set by the CodeQL CLI external dependency if that is used
   - Second choice, `codeql` as found on the system path
4. Determine the directory name for the CodeQL database
   - Format: `Build/codeql-db-<package>-<target>-<instance>`
5. Clean the build directory of the active platform and target
   - CodeQL database generation only works on clean builds
6. Ensure the "build" step is not skipped as a build is needed to generate a CodeQL database
7. Build a CodeQL file that wraps around the edk2 build
   - Written to the package build directory
     - Example: `Build/MdeModulePkg/VS2022/codeql_build_command.bat`
8. Set the variables necessary for stuart to call CodeQL CLI during the build phase
   - Sets `EDK_BUILD_CMD` and `EDK_BUILD_PARAMS`

### CodeQlAnalyzePlugin

1. Register a post-build hook
2. Determine the package and target being built
3. Determine the best CodeQL CLI path to use
   - First choice, the `STUART_CODEQL_PATH` environment variable
     - Note: This is set by the CodeQL CLI external dependency if that is used
   - Second choice, `codeql` as found on the system path
4. Determine the directory name for the most recent CodeQL database
   - Format: `Build/codeql-db-<package>-<target>-<instance>`
5. Determine plugin audit status for the given package and target
   - Check if `AuditOnly` is enabled either globally or for the package
6. Determine the CodeQL query specifiers to use for the given package and target
   - First choice, the package CI YAML file value
   - Second choice, the `STUART_CODEQL_QUERY_SPECIFIERS`
   - Third choice, use `CodeQlQueries.qls` (in the plugin directory)
7. Run CodeQL CLI to perform database analysis
8. Parse the analysis SARIF file to determine the number of CodeQL failures
9. Return the number of failures (or zero if `AuditOnly` is enabled)

## Local Development Tips

This section contains helpful tips to expedite common scenarios when working with CodeQL locally.

1. Pre-build, Build, and Post-Build

   Generating a database requires the pre-build and build steps. Analyzing a database requires the post-build step.

   Therefore, if you are making tweaks that don't affect the build, such as modifying the CodeQL queries used or level
   of severity reported, you can save time by skipping pre-build and post-build (e.g. `--skipprebuild` and
   `--skipbuild`).

2. Scopes

   Similar to (1), add/remove `codeql-build` and `codeql-analyze` from the active scopes to save time depending on what
   you are trying to do.

   If you are focusing on coding, remove the code CodeQL scopes if they are active. If you are ready to check your
   changes against CodeQL, simply add the scopes back. It is recommended to use build profiles to do this more
   conveniently.

   If you already have CodeQL CLI enabled, you can remove the `codeql-ext-dep` scope locally. The build will use the
   `codeql` command on your path.

3. CodeQL Output is in the CI Build Log

   To see exactly which queries CodeQL ran or why it might be taking longer than expected, look in the CI build log
   (i.e. `Build/CI_BUILDLOG.txt`) where the CodeQL CLI application output is written.

   Search for the text you see in the progress output (e.g. "Analyzing _MdeModulePkg_ (_DEBUG_) CodeQL database at")
   to jump to the section of the log just before the CodeQL CLI is invoked.

4. Use a SARIF Viewer to Read Results

The [SARIF Viewer extension for VS Code](https://marketplace.visualstudio.com/items?itemName=MS-SarifVSCode.sarif-viewer)
can open the .sarif file generated by this plugin and allow you to click links directly to the problem area in source
files.

## Resolution Guidelines

This section captures brief guidelines to keep in mind while resolving CodeQL issues.

1. Look at surrounding code. Changes should always take into account the context of nearby code. The new logic may
   need to account conditions not immediately obvious based on the issue alone. It is easy to focus only on the line
   of code highlighted by CodeQL and miss the code's role in the big picture.
2. A CodeQL alert may be benign but the code can be refactored to prevent the alert. Often refactoring the code makes
   the code intention clearer and avoids an unnecessary exception.
3. Consider adding unit tests while making CodeQL fixes especially for commonly used code and code with a high volume
   of CodeQL alerts.
