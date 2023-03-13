\page mipi_syst_building_page Building SyS-T

[TOC]

Building the SyS-T Library                  {#mipi_syst_building}
=========================================================================

The SyS-T library includes CMake build support scripts. CMake is an
open-source, cross-platform build environment that is available for all major
platforms. The CMake tooling is available from https://cmake.org.

Requirements for building SyS-T {#mipi_syst_build_req}
=========================================================================
The SyS-T Library requires the following tooling for building

Component              |Description
-----------------------|--------------------------
CMake (https://cmake.org)|Build and packaging system
C-Compiler             | Needed to build library from its C-Source code
C++-Compiler           | Needed for building the unit test suite
Google Test (https://github.com/google/googletest)|Needed for building the unit test suite
Doxygen(www.doxygen.org)| Needed for building the HTML documentaion

Configuring the build {#mipi_syst_build_conf}
=========================================================================
CMake based projects require configuration for generating the platform native
build support files from the CMake scripts. Configuring the SyS-T CMake scripts
follows CMake standard methodologies. Use the CMake documentation to familiarize
yourself with the CMake build environment.

The SyS-T project uses the following configuration options:

Configuration             |Description
--------------------------|--------------------------------
SYST_CFG_CONFORMANCE_LEVEL| The library conformance level setting (10=minimal, 20=cpu low overhead, 30=complete).
SYST_BUILD_PLATFORM_NAME  | The name of the platform adaptation code directory (see also: @ref mipi_syst_adapting_platform).
SYST_BUILD_GTEST_DIR      | File system location of the Google Test framework source code. This code is needed for the library unit tests only.
SYST_BUILD_TEST           | Option to enable/disable generation of unit tests.
SYST_BUILD_DOC            | Option to enable/disable generation of the HTML documentation (requires Doxygen)
CMAKE_INSTALL_PREFIX      | File System location for installing the SyS-T build.

Configuring using the CMake Gui
------------------------------------------------------------------------------
The CMake distribution includs a GUI front end for setting up a configuration.
The GUI front end is started using the cmake-gui command.

Perform the following steps inside the CMake Gui:

- Point the "Where is the source code:" field to the library directory inside
  the SyS-T distribution.
- Point the "Where to build the binaries:" field to a temporary location. This
  directory will hold build artifacts like object files. Note: This is not the
  installation location for the build result.
- Press "Configure".
  A new dialog appears for selecting the desired native platform build tooling.
  Select the desired tooling and press "Finish". CMake now parses the build
  scripts and populates the variable table with build configuration options.
- Update the path for the variable ``CMAKE_INSTALL_PREFIX`` to point to your
  desired install location.
- To build the unit tests, update the ``SYST_BUILD_GTEST_DIR`` variable to point
  to the Google Test sources and check the ``SYST_BUILD_TEST`` field.
- To build the HTML documentation, verify that doxygen was found by CMake and
  check the ``SYST_BUILD_DOC`` option.
- Press "Configure" again to activate the changed settings.
- Press "Generate" to generate the native build configuration files.

Configuring using the CMake command line
------------------------------------------------------------------------------
The SyS-T project can be configured with command line option, following
CMake standard methodologies. Follow these steps to setup a project using the
command line.

- Create a new directory for holding the build artifact files and enter this
  directory

        $ mkdir syst_build
        $ cd syst_build

- Run the configuration using the cmake tool. The following call shows an
 example with all options set:

 ```
 $ cmake \
     -DSYST_BUILD_GTEST_DIR=/home/username/googletest \
     -DSYST_BUILD_TEST=ON \
     -DSYST_BUILD_DOC=ON \
     -DSYST_BUILD_PLATFORM=example \
     -DCMAKE_INSTALL_PREFIX=/opt/mipi_syst \
     /home/jdoe/src/sys_t
```

Building and Installing {#mipi_syst_building_deploy}
=========================================================================
The build system provides various targets for building and installing the
SyS-T library, header files and documentation. The following table shows which
targets exist:

Target Name     |Description
----------------|--------------------------------
all             | Build binaries and, if enabled, the unit tests
doc             | Build HTML documentation
syst_unittest   | Build unit tests
install         | Install to CMAKE_INSTALL_PREFIX location, implies 'all'
clean           | Remove build artifacts
RUN_TEST        | Run the tests (not showing test output)
RUN_TEST_VERBOSE| Run the tests (showing test output)

Integration Build and Test Script
=========================================================================
A bash script in ``examples/scripts/bldall.sh`` can be used to run an
integration test using the different projects. The script builds all projects
sequentially using the example library platform. It then runs their components
tests and finally calls the printer tool to format the output of
the ``hello`` example application. The following transcript shows how to
run execute the script. The BLD_ROOT variable sets the location of the build
folder. If unset, the script creates a local build folder inside the scripts
folder.

```
$ cd sys-t/examples/scripts
$ BLD_ROOT=/tmp/sys_t_test_bld ./bldall.sh
```
