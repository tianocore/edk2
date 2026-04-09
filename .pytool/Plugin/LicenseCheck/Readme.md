# License Check Plugin

This CiBuildPlugin scans all new added files in a package to make sure code
is contributed under BSD-2-Clause-Patent.

## Configuration

The plugin can be configured to ignore certain files.

``` yaml
"LicenseCheck": {
    "IgnoreFiles": []
}
```
### IgnoreFiles

OPTIONAL List of file to ignore.
