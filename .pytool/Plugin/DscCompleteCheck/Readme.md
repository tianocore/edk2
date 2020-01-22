# Dsc Complete Check Plugin

This CiBuildPlugin scans all INF files from a package and confirms they are
listed in the package level DSC file. The test considers it an error if any INF
does not appear in the `Components` section of the package-level DSC (indicating
that it would not be built if the package were built). This is critical because
much of the CI infrastructure assumes that all modules will be listed in the DSC
and compiled.

This test will ignore INFs in the following cases:

1. When MODULE_TYPE = HOST_APPLICATION
2. When a Library instance **only** supports the HOST_APPLICATION environment

## Configuration

The plugin has a few configuration options to support the UEFI codebase.

``` yaml
"DscCompleteCheck": {
        "DscPath": "",   # Path to dsc from root of package
        "IgnoreInf": []  # Ignore INF if found in filesystem but not dsc
    }
```

### DscPath

Path to DSC to consider platform dsc

### IgnoreInf

Ignore error if Inf file is not listed in DSC file
