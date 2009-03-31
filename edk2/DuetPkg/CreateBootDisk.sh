#! /bin/sh

# Set up environment at fisrt.
export BUILD_DIR=$WORKSPACE/Build/DuetPkg/DEBUG_UNIXGCC
export BASETOOLS_DIR=$WORKSPACE/Conf/BaseToolsSource/Source/C/bin
export DISK_LABEL=DUET
export PROCESS_MARK=TRUE

if [ \
     -z "$*" -o \
     "$*" = "-?" -o \
     "$*" = "-h" -o \
     "$*" = "--help" \
   ]
then
	echo "Usage: CreateBootDisk [usb|floppy|ide] MediaPath DevicePath [FAT12|FAT16|FAT32]"
	echo "e.g. : CreateBootDisk floppy /media/floppy0 /dev/fd0 FAT12 "
	PROCESS_MARK=FALSE
fi

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
				$BASETOOLS_DIR/BootSectImage -g FDBs.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/bootsect.com -f
				$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_DEVICE -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/bootsect.com
	
				cp $BUILD_DIR/FV/Efildr $EFI_BOOT_MEDIA
	
				mkdir -p $EFI_BOOT_MEDIA/efi
				mkdir -p $EFI_BOOT_MEDIA/efi/boot

				cp $WORKSPACE/EdkShellBinPkg/Bin/Ia32/Shell.efi $EFI_BOOT_MEDIA/efi/boot/bootia32.efi 
	
				echo Done.
			else
				echo "Wrong FAT type $4 for floppy!"
			fi
			;;

		file) # CreateFile
			if [ "$4" = FAT12 ]
				then
				echo "Start to create file boot disk ..."
				echo Create boot sector ...	
	
				## Linux version of GenBootSector has not pass build yet.
				$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_MEDIA -o FDBs.com
				$BASETOOLS_DIR/BootSectImage -g FDBs.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bootsect.com -f
				$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_MEDIA -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bootsect.com
				echo Done.
			else
				echo "Wrong FAT type" $4 "for floppy!"
			fi
			;;

		usb) # CreateUsb

			if [ "$4" = FAT16 ]
			then
				if [ "$5" = step2 ]
				then
					cp $BUILD_DIR/FV/Efildr16 $EFI_BOOT_MEDIA
					mkdir $EFI_BOOT_MEDIA/efi/boot
					cp $WORKSPACE/EdkShellBinPkg/bin/ia32/Shell.efi $EFI_BOOT_MEDIA/efi/boot/bootia32.efi
					echo "step2 Done!"
				else
					echo Format $EFI_BOOT_DEVICE ...
					#Do format command.
					echo Create boot sector ...
					## Linux version of GenBootSector & Bootsectimage has not pass build yet.
					$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_DEVICE -o UsbBs16.com
					$BASETOOLS_DIR/BootSectImage -g UsbBs16.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs16.com -f
					$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_DEVICE -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs16.com
					$BASETOOLS_DIR/GnuGenBootSector -m -o $EFI_BOOT_DEVICE -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Mbr.com
					echo Done.
					echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN TO DO STEP2!
				fi
			elif [ "$4" = FAT32 ]
			then 
				if [ "$5" = step2 ]
				then
					cp $BUILD_DIR/FV/Efildr20 $EFI_BOOT_MEDIA
					mkdir $EFI_BOOT_MEDIA/efi/boot
					cp $WORKSPACE/EdkShellBinPkg/bin/ia32/Shell.efi $EFI_BOOT_MEDIA/efi/boot/bootia32.efi
					echo "step2 Done!"
				else
					echo Format $EFI_BOOT_DEVICE ...
					#Do format command.
					echo Create boot sector ...
					## Linux version of GenBootSector & Bootsectimage has not pass build yet.
					$BASETOOLS_DIR/GnuGenBootSector -i $EFI_BOOT_DEVICE -o UsbBs32.com
					$BASETOOLS_DIR/BootSectImage -g UsbBs32.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs32.com -f
					$BASETOOLS_DIR/GnuGenBootSector -o $EFI_BOOT_DEVICE -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs32.com
					$BASETOOLS_DIR/GnuGenBootSector -m -o $EFI_BOOT_DEVICE -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Mbr.com
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
