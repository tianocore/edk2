This directory contains the next generation of EDK II build tools and template files.
Templates are located in the Conf directory, while the tools executables for
Microsoft Windows 32-bit Operating Systems are located in the Bin\Win32 directory, other
directory contains tools source.

1. Build step to generate the binary tools.

=== Windows/Visual Studio Notes ===

To build the BaseTools, you should run the standard vsvars32.bat script
from your preferred Visual Studio installation or you can run get_vsvars.bat
to use latest automatically detected version.

In addition to this, you should set the following environment variables:

 * EDK_TOOLS_PATH - Path to the BaseTools sub directory under the edk2 tree
 * BASE_TOOLS_PATH - The directory where the BaseTools source is located.
   (It is the same directory where this README.txt is located.)

After this, you can run the toolsetup.bat file, which is in the same
directory as this file.  It should setup the remainder of the environment,
and build the tools if necessary.

Please also refer to the 'BuildNotes.txt' file for more information on
building under Windows.

=== Unix-like operating systems ===

To build on Unix-like operating systems, you only need to type 'make' in
the base directory of the project.

=== Ubuntu Notes ===

On Ubuntu, the following command should install all the necessary build
packages to build all the C BaseTools:

  sudo apt-get install build-essential uuid-dev

=== Python sqlite3 module ===
On Windows, the cx_freeze will not copy the sqlite3.dll to the frozen
binary directory (the same directory as build.exe and GenFds.exe).
Please copy it manually from <PythonHome>\DLLs.

The Python distributed with most recent Linux will have sqlite3 module
built in. If not, please install sqlit3 package separately.

=== CMake Project Generation ===
Create a build folder, and invoke CMake selecting your prefered project
type.

Examples:

On Windows:
cd %WORKSPACE%\BaseTools
mkdir build && cd build

REM VS Enviromental Variables
cmake ..
cmake --build . --target INSTALL --config MinSizeRel
cmake --build . --target CHECK --config MinSizeRel
* installs to BaseTools/Bin/Windows-x64/

REM Force Win32
cmake .. -G"Visual Studio 16 2019" -AWin32
cmake --build . --target INSTALL --config MinSizeRel
cmake --build . --target CHECK --config MinSizeRel
* installs to BaseTools/Bin/Windows-Win32/

REM Force ARM64
cmake .. -G"Visual Studio 16 2019" -AARM64


Supported Visual Studio Generators
* Visual Studio 16 2019        = Generates Visual Studio 2019 project files.
                                 Use -A option to specify architecture.
  Visual Studio 15 2017 [arch] = Generates Visual Studio 2017 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 14 2015 [arch] = Generates Visual Studio 2015 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 12 2013 [arch] = Generates Visual Studio 2013 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 11 2012 [arch] = Generates Visual Studio 2012 project files.
                                 Optional [arch] can be "Win64" or "ARM".
  Visual Studio 10 2010 [arch] = Generates Visual Studio 2010 project files.
                                 Optional [arch] can be "Win64" or "IA64".
  Visual Studio 9 2008 [arch]  = Generates Visual Studio 2008 project files.
                                 Optional [arch] can be "Win64" or "IA64".

On Darwin
cd $WORKSPACE/BaseTools
mkdir build && cd build
cmake ..
make install
make check


On Linux

cd $WORKSPACE/BaseTools
mkdir build && cd build
cmake ..
make install -j32
make check

cd $WORKSPACE/BaseTools
mkdir build && cd build
CXX=/usr/bin/clang++ CC=/usr/bin/clang cmake ..
make install -j32
make check

cd $WORKSPACE/BaseTools
mkdir build && cd build
cmake .. -GNinja
autoninja install
ninja check


17-NOV-2019
