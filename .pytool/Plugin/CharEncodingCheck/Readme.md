# Character Encoding Check Plugin

This CiBuildPlugin scans all the files in a package to make sure each file is
correctly encoded and all characters can be read.  Improper encoding causes
tools to fail in some situations especially in different locals.

## Configuration

The plugin can be configured to ignore certain files.

``` yaml
"CharEncodingCheck": {
    "IgnoreFiles": []
}
```
### IgnoreFiles

OPTIONAL List of file to ignore.
