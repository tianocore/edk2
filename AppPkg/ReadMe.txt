                                     EADK
                  EDK II Standard Libraries and Applications
                                    ReadMe
                                 Version 1.02
                                 21 Dec. 2012


OVERVIEW
========
The EADK (uEfi Application Development Kit) provides a set of standards-based
libraries, along with utility and demonstration applications, intended to
ease development of UEFI applications based upon the EDK II Open-Source
distribution.

At this time, applications developed with the EADK are intended to reside
on, and be loaded from, storage separate from the core firmware.  This is
primarily due to size and environmental requirements.

This release of the EADK should only be used to produce UEFI Applications.  Due to the execution
environment built by the StdLib component, execution as a UEFI driver can cause system stability
issues.

This document describes the EDK II specific aspects of installing, building,
and using the Standard C Library component of the EDK II Application
Development Kit, EADK.

The EADK is comprised of three packages:
        AppPkg, StdLib, and StdLibPrivateInternalFiles.

  AppPkg   This package contains applications which demonstrate use of the
           Standard C and Sockets Libraries.
           These applications reside in AppPkg/Applications.

      Enquire  This is a program that determines many properties of the
               C compiler and the target machine that Enquire is run on.  The
               only changes required to port this 1990s era Unix program to
               EDK II were the addition of eight pragmas to enquire.c in
               order to disable some Microsoft VC++ specific warnings.

      Hello    This is a very simple EDK II native application that doesn't use
               any features of the Standard C Library.

      Main     This application is functionally identical to Hello, except that
               it uses the Standard C Library to provide a main() entry point.

      Python   A port of the Python-2.7.2 interpreter for UEFI.  Building this
               application is disabled by default.
               See the PythonReadMe.txt file, in the Python directory,
               for information on configuring and building Python.

      Lua      A port of the Lua-5.2.3 interpreter for UEFI.  This
               application is disabled by default.  Un-comment the line for
               LuaLib.inf in the [LibraryClasses] section and Lua.inf in the 
               [Components] section of AppPkg.dsc to enable building Lua.

      OrderedCollectionTest  A small Standard C Library application that
               demonstrates the use of the OrderedCollectionLib library class
               (provided by the BaseOrderedCollectionRedBlackTreeLib library
               instance in this application), and allows the user to "fuzz" the
               library with interactive or scripted API calls.

      Sockets  A collection of applications demonstrating use of the
               EDK II Socket Libraries.  These applications include:

               *   DataSink                     *   DataSource
               *   GetAddrInfo                  *   GetHostByAddr
               *   GetHostByDns                 *   GetHostByName
               *   GetNetByAddr                 *   GetNetByName
               *   GetServByName                *   GetServByPort
               *   OobRx                        *   OobTx
               *   RawIp4Rx                     *   RawIp4Tx
               *   RecvDgram                    *   SetHostName
               *   SetSockOpt                   *   TftpServer
               *   WebServer

  StdLib   The StdLib package contains the standard header files as well as
           implementations of other standards-based libraries.

           *   BsdSocketLib
                  Support routines above the sockets layer and C interface for
                  the UEFI socket library.
           *   Efi
                  Template contents for the target system's
                  \Efi\StdLib\etc directory.
           *   EfiSocketLib
                  UEFI socket implementation, may be linked into an
                  application or run as a driver.
           *   Include
                  Standard include files.
           *   LibC
                  C Standard Library implementation as per
                  ISO/IEC 9899:199409 (C95).
           *   PosixLib
                  Selected functions from the "Single Unix v4" specification.
           *   SocketDxe
                  UEFI sockets driver, includes EfiSocketLib.
           *   UseSocketDxe
                  Alternate linkage for applications that get built into the
                  firmware.  Cause application to use a common instance of the
                  sockets driver instead of including all of sockets into the
                  application.

  StdLibPrivateInternalFiles  The contents of this package are for the
           exclusive use of the library implementations in StdLib.  Please do
           not use anything from this package in your application or else
           unexpected behavior may occur.
           This package may be removed in a future release.


RELEASE NOTES
=============
  Fixes and Additions
  -------------------
Beginning with release 1.01, applications built with the StdLib package
no longer have a dependency on the TimerLib.

  Known Issues
  -----------------
This release of the EADK has some restrictions, as described below.

    1.  The target machine must be running firmware which provides the
        UEFI 2.3 HII protocol.

    2.  Applications must be launched from within the EFI Shell.

    3.  Absolute file paths may optionally be prefixed by a volume specifier
        such as "FS0:".  The volume specifier is separated from the remainder
        of the path by a single colon ':'.  The volume specifier must be one of
        the Shell's mapped volume names as shown by the "map" command.

    4.  Absolute file paths that don't begin with a volume specifier;
        e.g. paths that begin with "/", are relative to the currently selected
        volume.  When the EFI Shell first starts, there is NO selected volume.

    5.  The tmpfile(), and related, functions require that the current volume
        have a temporary directory as specified in <paths.h>.  This directory
        is specified by macro _PATH_TMP as /Efi/StdLib/tmp.

The Standard C Library provided by this package is a "hosted" implementation
conforming to the ISO/IEC 9899-1990 C Language Standard with Addendum 1. This
is commonly referred to as the "C 95" specification or ISO/IEC 9899:199409.
The following instructions assume that you have an existing EDK II or UDK 2010
source tree that has been configured to build with your tool chain.  For
convenience, it is assumed that your EDK II source tree is located at
C:\Source\Edk2.


EADK INSTALLATION
=================
The EADK is integrated within the EDK II source tree and is included with
current EDK II check-outs.  If they are missing from your tree, they may be
installed by extracting, downloading or copying them to the root of your EDK II
source tree.  The three package directories should be peers to the Conf,
MdePkg, Nt32Pkg, etc. directories.

There are some boiler-plate declarations and definitions that need to be
included in your application's INF and DSC build files.  These are described
in the CONFIGURATION section, below.

A subset of the Python 2.7.2 distribution is included as part of AppPkg.  If desired,
the full Python 2.7.2 distribution may be downloaded from python.org and used instead.
Delete or rename the existing Python-2.7.2 directory then extract the downloaded
Python-2.7.2.tgz file into the AppPkg\Applications\Python directory.  This will produce a
Python-2.7.2 directory containing the full Python distribution.  Python files that had to be
modified for EDK II are in the AppPkg\Applications\Python\PyMod-2.7.2 directory.  These
files need to be copied into the corresponding directories within the extracted Python-2.7.2
directory before Python can be built.


BUILDING
========
It is not necessary to build the libraries separately from the target
application(s). If the application references the libraries, as described in
USAGE, below; the required libraries will be built as needed.
To build the applications included in AppPkg, one would execute the following
commands within the "Visual Studio Command Prompt" window:

    > cd C:\Source\Edk2
    > .\edksetup.bat
    > build -a X64 -p AppPkg\AppPkg.dsc

This will produce the application executables: Enquire.efi, Hello.efi, and
Main.efi in the C:\Source\Edk2\Build\AppPkg\DEBUG_VS2008\X64 directory; with
the DEBUG_VS2008 component being replaced with the actual tool chain and build
type you have selected in Conf\Tools_def.txt. These executables can now be
loaded onto the target platform and executed.

If you examine the AppPkg.dsc file, you will notice that the StdLib package is
referenced in order to resolve the library classes comprising the Standard
C Library.  This, plus referencing the StdLib package in your application's
.inf file is all that is needed to link your application to the standard
libraries.

Unless explicitly stated as allowed, EADK components should not be added as
components of a DSC file which builds a platform's core firmware.  There are
incompatibilities in build flags and requirements that will conflict with the
requirements of the core firmware.  EADK components should be built using a
separate DSC file then, if absolutely necessary, included as binary components
of other DSC files.

USAGE
=====
This implementation of the Standard C Library is comprised of 16 separate
libraries in addition to the standard header files. Nine of the libraries are
associated with use of one of the standard headers; thus, if the header is used
in an application, it must be linked with the associated library.  Three
libraries are used to provide the Console and File-system device abstractions.
The libraries and associated header files are described in the following table.

 Library
  Class      Header File(s)    Notes
----------  ----------------  -------------------------------------------------
LibC        -- Use Always --  This library is always required.
LibCtype    ctype.h, wctype.h Character classification and mapping
LibLocale   locale.h          Localization types, macros, and functions
LibMath     math.h            Mathematical functions, types, and macros
LibStdio    stdio.h           Standard Input and Output functions, types, and
                              macros
LibStdLib   stdlib.h          General Utilities for numeric conversion, random
                              num., etc.
LibString   string.h          String copying, concatenation, comparison,
                              & search
LibSignal   signal.h          Functions and types for handling run-time
                              conditions
LibTime     time.h            Time and Date types, macros, and functions
LibUefi     sys/EfiSysCall.h  Provides the UEFI system interface and
                              "System Calls"
LibWchar    wchar.h           Extended multibyte and wide character utilities
LibNetUtil                    Network address and number manipulation utilities
DevConsole                    Automatically provided File I/O abstractions for
                              the UEFI Console device.  No need to list this
                              library class in your INF file(s).
DevShell    Add if desired    File I/O abstractions using UEFI shell
                              facilities.  Add this to the application's main
                              INF file if file-system access needed.
DevUtility  -- Do Not Use --  Utility functions used internally by the Device abstractions
LibGdtoa    -- Do Not Use --  This library is used internally and should not
                              need to be explicitly specified by an
                              application.  It must be defined as one of the
                              available library classes in the application's
                              DSC file.

                         Table 1:  Standard Libraries
                         ============================

The DevConsole and DevShell libraries provide device I/O functionality and are treated
specially.  DevConsole is automatically included so there is no need to reference it in your
application's DSC or INF files.  DevShell must be listed, in your application's INF file in the
[LibraryClasses] section, if your application does file I/O.

These libraries must be fully described in the [LibraryClasses] section of the
application package's DSC file. Then, each individual application needs to
specify which libraries to link to by specifying the Library Class, from the
above table, in the [LibraryClasses] section of the application's INF file. The
AppPkg.dsc, StdLib.dsc, and Enquire.inf files provide good examples of this.
More details are in the CONFIGURATION section, below.

In order to simplify this process, the [LibraryClasses] definitions, and others, are
specified in the StdLib.inc file.  If this file is included in the DSC file, usually at the
end, then other DSC file changes or additions are unnecessary.  This is further described in
the CONFIGURATION section, below.

Within the source files of the application, use of the Standard headers and
library functions follow standard C programming practices as formalized by
ISO/IEC 9899:1990, with Addendum 1, (C 95) C language specification.


BUILD CONFIGURATION
===================
DSC Files
---------

All EDK II packages which build applications that use the standard libraries
must include some "boilerplate" text in the package's .dsc file.  To make it
easier, and to reduce cut-and-paste errors, the "boilerplate" text has been
consolidated into a single file, StdLib/StdLib.inc, which can be included in
your .dsc file using the !include directive.  The provided AppPkg.dsc and
StdLib.dsc files do this on their last line.

The "boilerplate" text can be included using a !include directive in the
package's .dsc file.  The provided AppPkg.dsc and StdLib.dsc files include
the following "boilerplate" text:

  ##############################################################################
  #
  # Specify whether we are running in an emulation environment, or not.
  # Define EMULATE if we are, else keep the DEFINE commented out.
  #
  # DEFINE  EMULATE = 1

  ##############################################################################
  #
  #  Include Boilerplate text required for building with the Standard Libraries.
  #
  ##############################################################################
  !include StdLib/StdLib.inc

                      Figure 1: "Boilerplate" Inclusion
                      =================================

The EMULATE macro must be defined if one desires to do source-level debugging within one of
the emulated environments such as NT32Pkg or UnixPkg.

The final boilerplate line, in Figure 1, includes the StdLib.inc file.
Each section of StdLib/StdLib.inc is described below.

If desired, all of the Socket applications, in AppPkg, can be built by including Sockets.inc:

  !include AppPkg/Applications/Sockets/Sockets.inc

              Figure 2: Socket Applications "Boilerplate" Inclusion
              =====================================================


Descriptions of the Library Classes comprising the Standard Libraries,
as shown in Figure 3: Library Class Descriptions, are provided.

  [LibraryClasses]
    #
    # C Standard Libraries
    #
    LibC|StdLib/LibC/LibC.inf
    LibCType|StdLib/LibC/Ctype/Ctype.inf
    LibLocale|StdLib/LibC/Locale/Locale.inf
    LibMath|StdLib/LibC/Math/Math.inf
    LibSignal|StdLib/LibC/Signal/Signal.inf
    LibStdio|StdLib/LibC/Stdio/Stdio.inf
    LibStdLib|StdLib/LibC/StdLib/StdLib.inf
    LibString|StdLib/LibC/String/String.inf
    LibTime|StdLib/LibC/Time/Time.inf
    LibUefi|StdLib/LibC/Uefi/Uefi.inf
    LibWchar|StdLib/LibC/Wchar/Wchar.inf

  # Common Utilities for Networking Libraries
    LibNetUtil|StdLib/LibC/NetUtil/NetUtil.inf

  # Additional libraries for POSIX functionality.
    LibErr|StdLib/PosixLib/Err/LibErr.inf
    LibGen|StdLib/PosixLib/Gen/LibGen.inf
    LibGlob|StdLib/PosixLib/Glob/LibGlob.inf
    LibStringlist|StdLib/PosixLib/Stringlist/LibStringlist.inf

  # Libraries for device abstractions within the Standard C Library
  # Applications should not directly access any functions defined in these libraries.
    LibGdtoa|StdLib/LibC/gdtoa/gdtoa.inf
    DevConsole|StdLib/LibC/Uefi/Devices/daConsole.inf
    DevShell|StdLib/LibC/Uefi/Devices/daShell.inf
    DevUtility|StdLib/LibC/Uefi/Devices/daUtility.inf

  [LibraryClasses.ARM.UEFI_APPLICATION]
    NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

                     Figure 3: Library Class Descriptions
                     ====================================


The directives in Figure 4: Package Component Descriptions will create
instances of the BaseLib and BaseMemoryLib library classes that are built
with Link-time-Code-Generation disabled.  This is necessary when using the
Microsoft tool chains in order to allow the library's functions to be
resolved during the second pass of the linker during Link-Time-Code-Generation
of the application.

A DXE driver version of the Socket library is also built.

  [Components]
  # BaseLib and BaseMemoryLib need to be built with the /GL- switch
  # when using the Microsoft tool chains.  This is required so that
  # the library functions can be resolved during the second pass of
  # the linker during link-time-code-generation.
  #
    MdePkg/Library/BaseLib/BaseLib.inf {
      <BuildOptions>
        MSFT:*_*_*_CC_FLAGS = /X /Zc:wchar_t /GL-
    }
    MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf {
      <BuildOptions>
        MSFT:*_*_*_CC_FLAGS = /X /Zc:wchar_t /GL-
    }

  ##########
  #  Socket Layer
  ##########
    StdLib/SocketDxe/SocketDxe.inf

                    Figure 4: Package Component Descriptions
                    ========================================


Each compiler assumes, by default, that it will be used with standard libraries
and headers provided by the compiler vendor.  Many of these assumptions are
incorrect for the UEFI environment.  By including a BuildOptions section, as
shown in Figure 5: Package Build Options, these assumptions can be
tailored for compatibility with UEFI and the EDK II Standard Libraries.

Note that the set of BuildOptions used is determined by the state of the EMULATE macro.

  [BuildOptions]
  !ifndef $(EMULATE)
    # These Build Options are used when building the Standard Libraries to be run
    # on real hardware.
    INTEL:*_*_IA32_CC_FLAGS  = /Qfreestanding
     MSFT:*_*_IA32_CC_FLAGS  = /X /Zc:wchar_t
      GCC:*_*_IA32_CC_FLAGS  = -nostdinc -nostdlib

  !else
    # The Build Options, below, are only used when building the Standard Libraries
    # to be run under an emulation environment.
    # They disable optimization which facillitates debugging under the Emulation environment.
    INTEL:*_*_IA32_CC_FLAGS  = /Od
     MSFT:*_*_IA32_CC_FLAGS  = /Od
      GCC:*_*_IA32_CC_FLAGS  = -O0

                        Figure 5: Package Build Options
                        ===============================


INF Files
=========
The INF files for most modules will not require special directives in order to
support the Standard Libraries.  The two sections which require attention: LibraryClasses
and BuildOptions, are described below.

  [LibraryClasses]
    UefiLib
    LibC
    LibString
    LibStdio
    DevShell

                      Figure 6: Module Library Classes
                      ================================


Modules of type UEFI_APPLICATION that perform file I/O must include library
class DevShell.  Including this library class will allow file operations to be
handled by the UEFI Shell.  Without this class, only Console I/O is supported.


An application's INF file might need to include a [BuildOptions] section
specifying additional compiler and linker flags necessary to allow the
application to be built. Usually, this section is not needed.  When building
code from external sources, though, it may be necessary to disable some
warnings or enable/disable some compiler features.

 [BuildOptions]
  INTEL:*_*_*_CC_FLAGS          = /Qdiag-disable:181,186
   MSFT:*_*_*_CC_FLAGS          = /Oi- /wd4018 /wd4131
    GCC:*_*_IPF_SYMRENAME_FLAGS = --redefine-syms=Rename.txt

                        Figure 7: Module Build Options
                        ==============================


TARGET-SYSTEM INSTALLATION
==========================
Applications that use file system features or the Socket library depend upon
the existence of a specific directory tree structure on the same volume that
the application was loaded from.  This tree structure is described below:

    /EFI                      Root of the UEFI system area.
     |- /Tools                Directory containing applications.
     |- /Boot                 UEFI specified Boot directory.
     |- /StdLib               Root of the Standard Libraries sub-tree.
         |- /etc              Configuration files used by libraries.
         |- /tmp              Temporary files created by tmpfile(), etc.


The /Efi/StdLib/etc directory must be manually populated from the StdLib/Efi/etc source
directory.

IMPLEMENTATION-Specific Features
================================
It is very strongly recommended that applications not use the long or
unsigned long types. The size of these types varies between compilers and is one
of the less portable aspects of C. Instead, one should use the UEFI defined
types whenever possible. Use of these types, listed below for reference,
ensures that the declared objects have unambiguous, explicitly declared, sizes
and characteristics.

        UINT64   INT64     UINT32   INT32   UINT16   CHAR16
        INT16    BOOLEAN   UINT8    CHAR8   INT8
        UINTN    INTN                       PHYSICALADDRESS

There are similar types declared in sys/types.h and related files.

The types UINTN and INTN have the native width of the target processor
architecture. Thus, INTN on IA32 has a width of 32 bits while INTN on X64 and
IPF has a width of 64 bits.

For maximum portability, data objects intended to hold addresses should be
declared with type intptr_t or uintptr_t. These types, declared in
sys/stdint.h, can be used to create objects capable of holding pointers. Note
that these types will generate different sized objects on different processor
architectures.  If a constant size across all processors and compilers is
needed, use type PHYSICAL_ADDRESS.

Though not specifically required by the ISO/IEC 9899 standard, this
implementation of the Standard C Library provides the following system calls
which are declared in sys/EfiSysCall.h and/or unistd.h.

          close   creat    chmod    dup      dup2
          fcntl   fstat    getcwd   ioctl    isatty
          lseek   lstat    mkdir    open     poll
          read    rename   rmdir    stat     unlink   write

The open function will accept file names of "stdin:", "stdout:", and "stderr:"
which cause the respective streams specified in the UEFI System Table to be
opened.  Normally, these are associated with the console device.  When the
application is first started, these streams are automatically opened on File
Descriptors 0, 1, and 2 respectively.

                            # # #
