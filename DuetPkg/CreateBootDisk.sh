#! /bin/sh

## @file
#
#  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#
##

# Set up environment at fisrt.

if [ -z "$EDK_TOOLS_PATH" ]
then
export BASETOOLS_DIR=$WORKSPACE/Conf/BaseToolsSource/Source/C/bin
else
export BASETOOLS_DIR=$EDK_TOOLS_PATH/Source/C/bin
fi

export BOOTSECTOR_BIN_DIR=$WORKSPACE/DuetPkg/BootSector/bin
export DISK_LABEL=DUET
export PROCESS_MARK=TRUE

if [ \
     -z "$*" -o \
     "$*" = "-?" -o \
     "$*" = "-h" -o \
     "$*" = "--help" \
   ]
then
	echo "Usage: CreateBootDisk [usb|floppy|ide|file] MediaPath DevicePath [FAT12|FAT16|FAT32] [IA32|X64] [GCC44|UNIXGCC]"
	echo "e.g. : CreateBootDisk floppy /media/floppy0 /dev/fd0 FAT12 IA32"
	PROCESS_MARK=FALSE
fi

case "$5" in
   IA32)
     export PROCESSOR=IA32
     ;;
   X64)
     export PROCESSOR=X64
     ;;
   *)
     echo Invalid Architecture string, should be only IA32 or X64
     return 1
esac

if [ -z "$6" ]
then
  TOOLCHAIN=GCC44
else
  TOOLCHAIN=$6
fi

export BUILD_DIR=$WORKSPACE/Build/DuetPkg$PROCESSOR/DEBUG_$TOOLCHAIN


export EFI_BOOT_MEDIA=$2
export EFI_BOOT_DEVICE=$3

if [ "$PROCESS_MARK" = TRUE ]
then
	case "$1" in
		floppy)
			if [ "$4" = FAT12 ]
			then
				echo Start to create floppy boot disk ...
				echo Format $EFI_BOOT_MEDIA ...
				## Format floppy disk
				umount $EFI_BOOT_MEDIA
				mkfs.msdos $EFI_BOOT_DEVICE
				mount $EFI_BOOT_DEVICE $EFI_BOOT_MEDIA
				echo Create boot sector ...
				## Linux version of GenBootSector has not pass build yet.
				$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_DEVICE -o FDBs.com
				cp $BOOTSECTOR_BIN_DIR/bootsect.com FDBs-1.com
				$BASETOOLS_DIR/BootSectImage -g FDBs.com FDBs-1.com -f
				$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_DEVICE -i FDBs-1.com
				rm FDBs-1.com
				cp $BUILD_DIR/FV/Efildr $EFI_BOOT_MEDIA
	
				mkdir -p $EFI_BOOT_MEDIA/efi
				mkdir -p $EFI_BOOT_MEDIA/efi/boot
				if [ "$5" = IA32 ]
				then
					cp $WORKSPACE/ShellBinPkg/UefiShell/Ia32/Shell.efi $EFI_BOOT_MEDIA/efi/boot/boot$5.efi
				else
					if [ "$5" = X64 ]
					then
						cp $WORKSPACE/ShellBinPkg/UefiShell/X64/Shell.efi $EFI_BOOT_MEDIA/efi/boot/boot$5.efi
					else
						echo Wrong Arch!
					fi
				fi
				echo Done.
			else
				echo "Wrong FAT type $4 for floppy!"
			fi
			;;

		file) # CreateFile
			if [ "$4" = FAT12 ]
				then
				echo "Start to create file boot disk ..."
				dd bs=512 count=2880 if=/dev/zero of=$EFI_BOOT_MEDIA
				mkfs.msdos -F 12 $EFI_BOOT_MEDIA

				mcopy -i $EFI_BOOT_MEDIA $BUILD_DIR/FV/Efildr ::/Efildr
				mmd -i $EFI_BOOT_MEDIA ::/efi ::/efi/boot
				if [ "$5" = IA32 ]
				then
					mcopy -i $EFI_BOOT_MEDIA $WORKSPACE/ShellBinPkg/UefiShell/Ia32/Shell.efi ::/efi/boot/boot$5.efi
				elif [ "$5" = X64 ]
				then
					mcopy -i $EFI_BOOT_MEDIA $WORKSPACE/ShellBinPkg/UefiShell/X64/Shell.efi ::/efi/boot/boot$5.efi
				else
					echo Wrong Arch!
				fi
				mdir -i $EFI_BOOT_MEDIA -s ::

				## Linux version of GenBootSector has not pass build yet.
				$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_MEDIA -o $EFI_BOOT_MEDIA.bs0
				cp $BOOTSECTOR_BIN_DIR/bootsect.com $EFI_BOOT_MEDIA.bs1
				$BASETOOLS_DIR/BootSectImage -g $EFI_BOOT_MEDIA.bs0 $EFI_BOOT_MEDIA.bs1
				$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_MEDIA -i $EFI_BOOT_MEDIA.bs1
				rm $EFI_BOOT_MEDIA.bs[0-1]
				echo Done.
			else
				echo "Wrong FAT type" $4 "for floppy!"
			fi
			;;

		usb) # CreateUsb

			if [ "$4" = FAT16 ]
			then
				if [ "$6" = step2 ]
				then
					cp $BUILD_DIR/FV/Efildr16 $EFI_BOOT_MEDIA
					mkdir $EFI_BOOT_MEDIA/efi/boot
					if [ "$5" = IA32 ]
					then
						cp $WORKSPACE/ShellBinPkg/UefiShell/Ia32/Shell.efi $EFI_BOOT_MEDIA/efi/boot/boot$5.efi
					else
						if [ "$5" = X64 ]
						then
							cp $WORKSPACE/ShellBinPkg/UefiShell/X64/Shell.efi $EFI_BOOT_MEDIA/efi/boot/boot$5.efi
						else
							echo Wrong Arch!
						fi
					fi
					echo "step2 Done!"
				else
					echo Format $EFI_BOOT_DEVICE ...
					#Do format command.
					echo Create boot sector ...
					## Linux version of GenBootSector & Bootsectimage has not pass build yet.
					$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_DEVICE -o UsbBs16.com
					cp $BOOTSECTOR_BIN_DIR/bs16.com Bs16-1.com
					$BASETOOLS_DIR/BootSectImage -g UsbBs16.com Bs16-1.com -f
					$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_DEVICE -i Bs16-1.com
					rm Bs16-1.com
					$BASETOOLS_DIR/GnuGenBootSector -m -o $EFI_BOOT_DEVICE -i $BOOTSECTOR_BIN_DIR/Mbr.com
					echo Done.
					echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN TO DO STEP2!
				fi
			elif [ "$4" = FAT32 ]
			then 
				if [ "$6" = step2 ]
				then
					cp $BUILD_DIR/FV/Efildr20 $EFI_BOOT_MEDIA
					mkdir $EFI_BOOT_MEDIA/efi/boot
					if [ "$5" = IA32 ]
					then
						cp $WORKSPACE/ShellBinPkg/UefiShell/Ia32/Shell.efi $EFI_BOOT_MEDIA/efi/boot/boot$5.efi
					else
						if [ "$5" = X64 ]
						then
							cp $WORKSPACE/ShellBinPkg/UefiShell/X64/Shell.efi $EFI_BOOT_MEDIA/efi/boot/boot$5.efi
						else
							echo Wrong Arch!
						fi
					fi
					echo "step2 Done!"
				else
					echo Format $EFI_BOOT_DEVICE ...
					#Do format command.
					echo Create boot sector ...
					## Linux version of GenBootSector & Bootsectimage has not pass build yet.
					$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_DEVICE -o UsbBs32.com
					cp $BOOTSECTOR_BIN_DIR/bs32.com Bs32-1.com
					$BASETOOLS_DIR/BootSectImage -g UsbBs32.com Bs32-1.com -f
					$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_DEVICE -i Bs32-1.com
					rm Bs32-1.com
					$BASETOOLS_DIR/GnuGenBootSector -m -o $EFI_BOOT_DEVICE -i $BOOTSECTOR_BIN_DIR/Mbr.com
					echo Done.
					echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN TO DO STEP2!
				fi			
			else
				echo "Wrong FAT type $1 for floppy!"
			fi
	
			;;

		ide) # CreateIde
		echo "Not support yet!"
		;;
		*)
		echo "Arg1 should be [floppy | file | usb | ide] !"

	esac
fi
