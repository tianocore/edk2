1. Introduction

   本文介绍了如何在Ubuntu下建立EDKII编译环境，使用QEMU运行EDKII BIOS.

2. 建立EDKII编译环境

       Note: EDKII (12/1/2011 SVN checkout) builds cleanly on 11.10 (with packages in proposed repo installed).

Note: EDKII is known to build correctly on Natty (11.04) but has compilation issues with the more pedantic gcc 4.6.1 ,

         so it fails to build on Oneiric (as of 20 Oct 2011). However, upsteam are aware of this and are working on a fix.

(1) Install required packages
       sudo apt-get install build-essential subversion uuid-dev iasl


       (2) Get the latest source for EDKII
     In a suitable working directory extract the latest EDKII source. Note there is no password for guest account, so hit enter.

      $ mkdir ~/src        $ cd ~/src        $ git clone git://github.com/tianocore/edk2.git

      (4) Compile base tools
    For MS Windows, prebuilt binaries of the base tools are shipped with the source;

    on Ubuntu the base tools required for building EDKII need to be built first.

      $ cd ~/src        $ make -C edk2/BaseTools
(5) Set up build environment
             You need to set EDK_TOOLS_PATH and set up the build environment by running the edksetup.sh script provided in the

      source. This script will copy template and configuration files to edk2/Conf directory.

       $ cd ~/src/edk2         $ export EDK_TOOLS_PATH=$HOME/src/edk2/BaseTools         $ . edksetup.sh BaseTools

      (6) Set up build target
           To set up the build target you need to modify the conf file Conf/target.txt.

    This will enable the firmware package to be built and set up the compiler version used.

     $ vi ~/src/edk2/Conf/target.txt
Find

 ACTIVE_PLATFORM       = Nt32Pkg/Nt32Pkg.dsc 

and replace it with

 ACTIVE_PLATFORM       = MdeModulePkg/MdeModulePkg.dsc 

Find

 TOOL_CHAIN_TAG        = MYTOOLS 

and replace it with your version on GCC here for example GCC 4.4 will be used.

 TOOL_CHAIN_TAG        = GCC44 

Find

 TARGET_ARCH           = IA32 

and replace it with 'X64' for 64bit or 'IA32 X64' to build both architectures.

 TARGET_ARCH           = X64 

3. 编译MdeModulePkg

     This will build the MdeModulePkg and helloworld program that we can use later when we launch the UEFI shell from emulator...    

    Just type build...

  $ cd ~/src/edk2/    $ build
  On a Core i5 with 4GB of RAM the total build time is two minutes. 
4. Build a full system firmware image (OVMF)
   The Open Virtual Machine Firmware (or "OVMF") can be used to enable UEFI within virtual machines. 
   It provides libraries and drivers related to virtual machines. 
   Currently OVMF support QEMU for emulating UEFI on IA32 and X86-64 based systems. 
   You could also build OVMF with source level debugging enabled. 

   Set up build target
   You can build OVMF for IA32 or X64 architechtures. 
In this example we will build OVMF for X64 architecture. 
You will need to modify Conf/target.txt and replace ACTIVE_PLATFORM with the right dsc file. 
$ vi ~/src/edk2/Conf/target.txt
Find 
 ACTIVE_PLATFORM       = MdeModulePkg/MdeModulePkg.dsc  
replace with 
 ACTIVE_PLATFORM       = OvmfPkg/OvmfPkgX64.dsc  
This will set the Target Arch to X64, PEI code to X64 and DXE/UEFI code to X64. 
Build the OvmfPkg
    $ cd ~/src/edk2      $ build
On an i5 with 4GB RAM the total build time is less than two minutes. The files built will be located 
under ~/src/Build/ 
Building the OvmfPkg with Secure Boot support
    If you wish to build OVMF with Secure Boot, you must follow the openssl installation instructions 
found in:- 
~/src/edk2/CryptoPkg/Library/OpensslLib/Patch-HOWTO.txt
and build like this instead:- 
$ cd ~/src/edk2     $ build –D SECURE_BOOT_ENABLE
If you see an error that "the required fv image size exceeds the set fv image size" consult this mailing list post. 
But note that the rest of this guide currently assumes you build WITHOUT Secure Boot. 
5. Running UEFI in QEMU

       $ sudo apt-get install qemu

Initial setup
create a directory where you will set up firmware, a directory to use as hard disk image for QEMU.

    $ mkdir ~/ovmf-qemu      $ cd ~/ovmf-qemu      $ mkdir hda-contents
Copy the firmware to your working directory.

    $ cd ~/ovmf-qemu      $ cp ~/src/edk2/Build/OvmfX64/DEBUG_GCC45/FV/OVMF.fd ./bios.bin      $ cp ~/src/edk2/Build/OvmfX64/DEBUG_GCC45/FV/OvmfVideo.rom ./vgabios-cirrus.bin
Run UEFI image in QEMU
          This will launch UEFI firmware image on QEMU and drop to UEFI shell.

   You can type exit and get to the UEFI menus or work with the shell.

   Note: to release mouse grab from QEMU hit CTRL+ALT

     $ qemu-system-x86_64 -L . -hda fat:hda-contents
6. Running HelloWorld.efi
    
As part of the MdeModule build, you also built a HelloWorld.efi. Exit any QEMU sessions you are running and copy the hello world program to your QEMU hard disk. 

$ cp ~/src/edk2/Build/MdeModule/DEBUG_GCC45/X64/HelloWorld.efi ~/ovmf-qemu/hda-contents/.
Launch UEFI firmware in QEMU, and run the hello world program. 

7. 在QEMU上利用OVMF启动Ubuntu 12.04

You can boot the Ubuntu live CD image using OVMF as your firmware. Please note that splash screen does not work, and you might not be able to get a network connection, but the OS will boot and will be fully functional. I have precise-desktop-amd64.iso downloaded to ~/Downloads/iso/12.04/, I change directory to where I copied the firmware I built and run QEMU with the following options.

$ cd ~/ovmf-qemu  $ qemu-system-x86_64 -L . -m 1024 -cdrom ~/Downloads/iso/12.04/precise-desktop-amd64.iso -vga cirrus -enable-kvm
