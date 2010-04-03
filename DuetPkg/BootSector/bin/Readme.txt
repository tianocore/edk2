These binaries are used to make the bootable floppy or usb disk. 
The binaries of boot sector are built from DuetPkg\Bootsector\BootSector.inf at r8617 with following steps:
1) enter edk2 workspace directory from command line windows.
2) run "edksetup.bat"
3) run "build -p DuetPkg/DuetPkg.dsc -a IA32 -m DuetPkg/BootSector/BootSector.inf"



