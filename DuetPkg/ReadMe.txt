Developer's UEFI Emulation (DUET) on Edk2

A. Build DUET image on Windows Platform
========================================
1. Tools preparation

  To build DUET image, Visual Studio is required:
  1). Base on below link to create Visual Studio build environment.
      https://github.com/tianocore/tianocore.github.io/wiki/Windows-systems

2. Build Duet Platform module

  1). run cmd.exe to open command line window.
  2). enter workspace root directory such as c:\edk2_tree
  3). set the environment variable EDK_TOOLS_BIN to point at the BaseTools binaries directory
      i.e., "set EDK_TOOLS_BIN=c:\edk2-BaseTools-win32"
  4). run "edksetup.bat"
  5). run "build -p DuetPkg\DuetPkgIa32.dsc -a IA32 -t VS2015x86" for IA32 architecture platform (using 64-bit VS2015 for example) or
          "build -p DuetPkg\DuetPkgX64.dsc -a X64 -t VS2015x86" for X64 architecture platform.

  NOTE: The post build script 'PostBuild.sh' will be automatically called after the build command.


Create bootable disk
======================

3. Create boot disk
  The following steps are same for IA32 architecture platform or X64 architecture platform.

3.1 Create floppy boot disk
  1). enter <Workspace>\DuetPkg directory.
  2). Insert a floppy disk to drive
  3). run "CreateBootDisk.bat floppy a: FAT12 IA32" if floppy drive is a: disk and Arch to boot is IA32.
      or
      run "CreateBootDisk.bat floppy a: FAT12 X64" if floppy drive is a: disk and Arch to boot is X64.
3.2 Create usb boot disk
  1). enter <Workspace>\DuetPkg directory.
  2). Plugin usb disk
  3). run "CreateBootDisk.bat usb e: FAT16 IA32" if usb drive is e: and FAT format is FAT16 and Arch to boot is IA32.
      or "CreateBootDisk.bat usb e: FAT16 X64" if usb drive is e: and FAT format is FAT16 and Arch to boot is X64.
      or "CreateBootDisk.bat usb e: FAT32 IA32" if usb drive is e: and FAT format is FAT32 and Arch to boot is IA32.
      or "CreateBootDisk.bat usb e: FAT32 X64" if usb drive is e: and FAT format is FAT32 and Arch to boot is X64.
  4). UnPlug usb disk and plugin it again.
  5). run "CreateBootDisk.bat usb e: FAT16 IA32 step2" if usb drive is e: and FAT format is FAT16 and Arch to boot is IA32.
      or "CreateBootDisk.bat usb e: FAT16 X64 step2" if usb drive is e: and FAT format is FAT16 and Arch to boot is X64.
      or "CreateBootDisk.bat usb e: FAT32 IA32 step2" if usb drive is e: and FAT format is FAT32 and Arch to boot is IA32.
      or "CreateBootDisk.bat usb e: FAT32 X64 step2" if usb drive is e: and FAT format is FAT32 and Arch to boot is X64.

B. Build DUET image on Linux Platform
======================================
1. Tools preparation

  To build DUET image, GCC installation (4.4+) is required:
  1). Base on below link to create GCC build environment.
      https://github.com/tianocore/tianocore.github.io/wiki/Using-EDK-II-with-Native-GCC

2. Build Duet Platform module

  1). Open the terminal.
  2). enter workspace root directory such as /edk2_tree
  3). run ". edksetup.sh BaseTools"
  4). run "build -p DuetPkg/DuetPkgIa32.dsc -a IA32 -t GCC49" for IA32 architecture platform (using GCC 4.9 for example) or
          "build -p DuetPkg/DuetPkgX64.dsc -a X64 -t GCC49" for X64 architecture platform.

  NOTE: The post build script 'PostBuild.sh' will be automatically called after the build command.
        After post build action, you should check the size of EfiLdr at $WORKSPACE/Build/DuetPkgIA32(DuetPkgX64)/DEBUG_GCC49 directory, it must less than 470k.
        If not, you should manually remove some unnecessary drivers at DuetPkg.fdf file.

3. Create bootable disk
   The following steps are same for IA32 architecture platform or X64 architecture platform.

3.1 Create floppy boot disk
  1). enter /edk2_tree/DuetPkg directory.
  2). Insert a floppy disk to drive
  3). run "CreateBootDisk.sh" to build floppy drive
      such as "./CreateBootDisk.sh floppy /media/floppy0 /dev/fd0 FAT12 IA32"

3.2 Create usb boot disk
  1). enter /edk2_tree/DuetPkg directory.
  2). Plugin usb disk
  3). run "CreateBootDisk.sh" to build usb drive
      such as "./CreateBootDisk.sh usb /media/usb0 /dev/sdb0 FAT16 IA32"
  4). UnPlug usb disk and plugin it again.
  5). run "./CreateBootDisk.sh usb /media/usb0 /dev/sdb0 FAT16 IA32 step2"
