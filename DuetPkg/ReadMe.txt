Developer's UEFI Emulation (DUET) on Edk2

A. Build DUET image on Windows Platform
========================================
1. Tools preparation

To build DUET image, following tools are required:

  1). *Visual Studio 2005*
      Assume installed at <VS_PATH>, 
      e.g.: C:\Program Files\Microsoft Visual Studio .NET 2003\.
  2). WinDDK
      Assume installed at <WIN_DDK_PATH>, e.g.: C:\WINDDK\3790.1830\.
      
2. Build steps

2.1 Build Duet Platform module   

  1). run cmd.exe to open command line window.
  2). enter workspace root directory such as c:\edk2_tree
  2). run "edksetup.bat"
  3). run "build -p DuetPkg\DuetPkg.dsc -a IA32" for IA32 architecture platform or 
          "build -p DuetPkg\DuetPkg.dsc -a X64" for X64 architecture platform.
 
2.2 Execute post build actions  
  1). enter <Workspace>\DuetPkg directory.
  2). run "PostBuild.bat IA32" for IA32 architecture platform or 
          "PostBuild.bat X64" for X64 architecture platform.

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

  To build DUET image, GCC44 is required:
  1). Base on below link to create GCC44 build environment.
      http://tianocore.sourceforge.net/wiki/Using_EDK_II_with_Native_GCC

2. Build steps

2.1 Build Duet Platform module   

  1). Open the terminal.
  2). enter workspace root directory such as /edk2_tree
  3). run ". edksetup.sh BaseTools"
  4). run "build -p DuetPkg/DuetPkg.dsc -a IA32 -t GCC44" for IA32 architecture platform or 
          "build -p DuetPkg/DuetPkg.dsc -a X64 -t GCC44" for X64 architecture platform.
  
2.2 Execute post build actions  
  1). enter /edk2_tree/DuetPkg directory.
  2). run "./PostBuild.sh IA32 GCC44" for IA32 architecture platform or 
          "./PostBuild.sh X64 GCC44" for X64 architecture platform.

 NOTE: After post build action, you should check the size of EfiLdr at $WORKSPACE/Build/DuetPkg/DEBUG_GCC44 directory, it must less than 470k.
       If not, you should manually remove some unnecessary drivers at DuetPkg.fdf file.
 
3. Create bootable disk
   The following steps are same for IA32 architecture platform or X64 architecture platform.
   Now only support floopy.
   
   3.1 Create floppy boot disk
	  1). enter /edk2_tree/DuetPkg directory.
	  2). Insert a floppy disk to drive
	  3). run "CreateBootDisk.sh" to build floppy drive
		  such as "./CreateBootDisk.sh floppy /media/floppy0 /dev/fd0 FAT12 IA32"