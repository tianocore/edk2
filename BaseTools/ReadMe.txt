This directory contains the EDK II build tools and template files.
Templates are located in the Conf directory, while the tools executables for
Microsoft Windows Operating Systems are located in the Bin\\Win32 directory, other
directory contains tools source.

Build step to generate the binary tools
---------------------------------------

Windows/Visual Studio Notes
===========================

To build the BaseTools, you should run the standard vsvars32.bat script
from your preferred Visual Studio installation or you can run get_vsvars.bat
to use latest automatically detected version.

In addition to this, you should set the following environment variables::

 * EDK_TOOLS_PATH - Path to the BaseTools sub directory under the edk2 tree
 * BASE_TOOLS_PATH - The directory where the BaseTools source is located.
   (It is the same directory where this README.rst is located.)

After this, you can run the toolsetup.bat file, which is in the same
directory as this file.  It should setup the remainder of the environment,
and build the tools if necessary.

Please also refer to the ``BuildNotes.txt`` file for more information on
building under Windows.

Unix-like operating systems
===========================

To build on Unix-like operating systems, you only need to type ``make`` in
the base directory of the project.

Ubuntu Notes
============

On Ubuntu, the following command should install all the necessary build
packages to build all the C BaseTools::

 sudo apt install build-essential uuid-dev
