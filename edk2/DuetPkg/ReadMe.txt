Developer's UEFI Emulation (DUET) on Edk2

Build DUET image
=====================
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

2.2 Build BootSector
  1). run "build -p DuetPkg\DuetPkg.dsc -m DuetPkg\BootSector\BootSector.inf -a IA32"
  
2.3 Execute post build actions  
  1). enter <Workspace>\DuetPkg directory.
  2). run "PostBuild.bat IA32" for IA32 architecture platform or 
          "PostBuild.bat X64" for X64 architecture platform.

Create bootable disk
======================
  
3. Create boot disk
  The following steps are same for IA32 architecture platform or X64 arcchitecture platform.
  
3.1 Create floppy boot disk
  1). enter <Workspace>\DuetPkg directory.
  2). Insert a floppy disk to drive
  3). run "CreateBootDisk.bat floppy a: FAT12" if floppy drive is a: disk.
  
3.2 Create usb boot disk
  1). enter <Workspace>\DuetPkg directory.
  2). Plugin usb disk
  3). run "CreateBootDisk.bat usb e: FAT16" if usb drive is e: and FAT format is FAT16 or
          "CreateBootDisk.bat usb e: FAT32" if usb drive is e: and FAT format is FAT32
  4). UnPlug usb disk and plugin it again.
  5). run "CreateBootDisk.bat usb e: FAT16 step2" if usb drive is e: and FAT format is FAT16 or 
          "CreateBootDisk.bat usb e: FAT32 step2" if usb drive is e: and FAT format is FAT32.
          
          
          
   