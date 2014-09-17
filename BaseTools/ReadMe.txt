This directory contains the next generation of EDK II build tools and template files.
Templates are located in the Conf directory, while the tools executables for
Microsoft Windows 32-bit Operating Systems are located in the Bin\Win32 directory, other 
directory contatins tools source.

1. Build step to generate the binary tools.

=== Windows/Visual Studio Notes ===

To build the BaseTools, you should run the standard vsvars32.bat script.

In addition to this, you should set the following environment variables:

 * EDK_TOOLS_PATH - Path to the BaseTools sub directory under the edk2 tree
 * BASE_TOOLS_PATH - The directory where the BaseTools source is located.
   (It is the same directory where this README.txt is located.)
 * PYTHON_FREEZER_PATH - Path to where the python freezer tool is installed

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

26-OCT-2011
