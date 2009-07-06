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


FAQ
===
1, I fail to build UnixPkg due to "X11/extensions/XShm.h: No such file or directory"?
   The display adapter in UnixPkg is a virtual device written in X11 API. The library of x11proto-xext-dev is required
   for building.

2, I fail to build UnixPkg due to "/usr/bin/ld: cannot find -lXext"?   
   libxext-dev library is required for building.

