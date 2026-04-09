# Host Unit Test Dsc Complete Check Plugin

This CiBuildPlugin scans all INF files from a package for those related to host
based unit tests confirms they are listed in the unit test DSC file for the package.
The test considers it an error if any INF meeting the requirements does not appear
in the `Components` section of the unit test DSC. This is critical because
much of the CI infrastructure assumes that  modules will be listed in the DSC
and compiled.

This test will only require INFs in the following cases:

1. When MODULE_TYPE = HOST_APPLICATION
2. When a Library instance supports the HOST_APPLICATION environment

## Configuration

The plugin has a few configuration options to support the UEFI codebase.

``` yaml
"HostUnitTestDscCompleteCheck": {
    "DscPath": "", # Path to Host based unit test DSC file
    "IgnoreInf": []  # Ignore INF if found in filesystem but not dsc
}
```

### DscPath

Path to DSC to consider platform dsc

### IgnoreInf

Ignore error if Inf file is not listed in DSC file
