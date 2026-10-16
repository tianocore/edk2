# Markdown Lint Plugin

This CiBuildPlugin scans all the markdown files in a given package and checks for linter errors.

## Requirements

The test case in this plugin will be skipped if the requirements are not met.

1. NodeJs installed and on your path
2. `markdownlint-cli` NodeJs package installed
3. A `.markdownlint.yaml` config file either at your repository root or package root.

- NodeJS: <https://nodejs.org/en/>
- markdownlint-cli: <https://www.npmjs.com/package/markdownlint-cli>
  - Source repository: <https://github.com/igorshubovych/markdownlint-cli>

## Configuration

It is desired to use standard configuration methods so that both local editors and CI process leverage the same
configuration. This mostly works but for ignoring files there is currently a small discrepancy.

First there is/can be a `.markdownlintignore` file at root of the repository. This file much like a `.gitignore` is
great for broadly ignoring files with patterns. This works for both usage in a local editor and CI.

For the CI plugin, you can use the `IgnoreFiles` configuration option described in the Plugin Configuration.

## Plugin Configuration

The plugin has only minimal configuration options to support the UEFI codebase.

``` yaml
  "MarkdownLintCheck": {
    "AuditOnly": False,          # If True, log all errors and then mark as skipped
    "IgnoreFiles": []            # Package root relative file, folder, or glob pattern to ignore
  }
```

### AuditOnly

- `Boolean` - Default is `False`.

If `True` run the test in an Audit only mode which will log all errors but instead of failing the build it will set the
test as skipped. This allows visibility into the failures without breaking the build.

### IgnoreFiles

This supports package relative files, folders, and glob patterns to ignore. These are passed to the markdownlint-cli
tool as quoted `-i` parameters.

## Linter Configuration

All configuration options available to the linter can be set in `.markdownlint.yaml`. This includes customizing
rule options and enforcement.

- Markdownlint configuration options: <https://github.com/DavidAnson/markdownlint#configuration>
- Linter rules: <https://github.com/DavidAnson/markdownlint/blob/main/doc/Rules.md>

## Rule Overrides

There are times when a certain rule should not apply to part of a markdown file. Markdownlint has numerous ways
to configure this. See the in-file configuration options described in the links above.
