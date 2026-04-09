# Library Class Check Plugin

This CiBuildPlugin scans at all library header files found in the `Library`
folders in all of the package's declared include directories and ensures that
all files have a matching LibraryClass declaration in the DEC file for the
package. Any missing declarations will cause a failure.

## Configuration

The plugin has a few configuration options to support the UEFI codebase.

``` yaml
"LibraryClassCheck": {
    IgnoreHeaderFile: [],  # Ignore a file found on disk
    IgnoreLibraryClass: [] # Ignore a declaration found in dec file
}
```

### IgnoreHeaderFile

Ignore a file found on disk

### IgnoreLibraryClass

Ignore a declaration found in dec file
