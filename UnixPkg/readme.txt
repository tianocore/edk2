Deprecation Notice
==================
UnixPkg is deprecated. Please see UnixPkg/Deprecated.txt for more information.

Unix Simulation Platform
========================

UnixPkg is one of platform package which can be built to a platform's firmware from UnixPkg.dsc file.
And this package provide a simulation platform under *INUX environment in IA32 architecture.

- Same points between real platform and simulation platform:
  1) Unix simulation platform also run with firmware device image built from UnixPkg
  2) The working flow of simulation platform also contains SEC/PEI/DXE phase.

- Different points between real platform and simulation platform:
  1) The SEC phase in simulation platform in fact is a *INUX native application which can be run from *INUX shell;
  2) The device in simulation platform is not real hardware but simulation component in API level;
  3) The ThunkBus driver in simulation platform will get virtual device's desription from PCD and create virtual Device

Build
=====
UnixPkg is built with following command:
  build -p UnixPkg/UnixPkg.dsc -a IA32 -t ELFGCC
  Notes: ELFGCC is defined in <Workspace>/Conf/tools_def.txt file. This tool chain use native gcc/binutil instead of 
         cross-compiler like UNIXGCC tool chain.

On Mac OS X you can cd into UnixPkg directory and execute ./build.sh to build. This does not require
setting up the environment like running the build command. Note Snow Leopard or later is required. 
This script should also work for any *INUX, but has not been tested. 

./build.sh run will lanuch the emulator in gdb so you can source level debug via gdb. 


Notes:
=====
On Mac OS X Snow Leopard you can use Xcode 3.2 as a GUI debugger. 
Launch Xcode and open UnixPkg/Xcode/xcode_project/xcode_project.xcodeproj
Under the build menu chose build and debug. shift-cmd-B shows the build results.

Under most *INUX the EFI executables are placed in the emulated EFI memory by the EFI PE/COFF loader
but dlopen() is used to also load the image into the process to support source level debug. 
The entry point for the image is moved from the EFI emulator memory into the dlopen() image. This
is not the case for Mac OS X. On Mac OS X a debugger script is used and the real EFI images in
the emulator are the ones being debugged. 

Also on Mac OS X the stack alignment requirements for IA-32 are 16 bytes and this is more strict
than the EFI ABI. To work around this gasket code was introduced to ensure the stack is always
16 byte aligned when making any POSIX call on Mac OS X. 

To build PE/COFF images with Xcode 3.2 and extra tool call mtoc is required to convert Mach-O 
images into PE/COFF images. The tool only supports EFI PE/COFF images and the instructions on 
how to download it are on the edk2 website.

FAQ
===
1, I fail to build UnixPkg due to "X11/extensions/XShm.h: No such file or directory"?
   The display adapter in UnixPkg is a virtual device written in X11 API. The library of x11proto-xext-dev is required
   for building.

2, I fail to build UnixPkg due to "/usr/bin/ld: cannot find -lXext"?   
   libxext-dev library is required for building.

