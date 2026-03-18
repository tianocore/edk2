# Host UnitTest Compiler Plugin

A CiBuildPlugin that compiles the dsc for host based unit test apps.
An IUefiBuildPlugin may be attached to this plugin that will run the unit tests and collect the results after successful compilation.

To run the unit tests and collect the results after successful compilation, The
host UnitTest Compliler Plugin will execute any IUefiBuildPlugin that has the
scope 'host-based-test'.

## Configuration

The package relative path of the DSC file to build.

``` yaml
"HostUnitTestCompilerPlugin": {
    "DscPath": "<path to dsc from root of pkg>"
}
```

### DscPath

Package relative path to the DSC file to build.

## Running a single test

It is possible to compile and run only a single test. This is useful when writing a test or running a test that is failing.
Below is an example command to build and run only a single test

`stuart_ci_build -c .pytool/CISettings.py -p MdePkg BUILDMODULE=MdePkg/Test/GoogleTest/Library/StackCheckLib/GoogleTestStackCheckLib.inf -d HostUnitTestCompilerPlugin=run`

The important parts to note is:

1. `BUILDMODULE=<PATH>` is package path relative
2. `-d` is used to disable all CI plugins
3. `HostUnitTestCompilerPlugin=run` is used to re-enable this plugin.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
