
# Set up environment at fisrt.
export BASETOOLS_DIR=$WORKSPACE_TOOLS_PATH/Bin/Win32
export BUILD_DIR=$WORKSPACE/Conf/BaseToolsSource/Source/C/bin
export DISK_LABEL=DUET

if [ \
     -z "$*" -o \
     "$*" = "-?" -o \
     "$*" = "-h" -o \
     "$*" = "--help" \
   ]
then
	echo "Usage: CreateBootDisk [usb|floppy|ide] DiskNumber [FAT12|FAT16|FAT32]"
	exit 1
fi

export EFI_BOOT_DISK=$2

case "$1" in
	floppy)
		if [ "$3" = FAT12 ]
		then
			echo Start to create floppy boot disk ...
			echo Format $EFI_BOOT_DISK ...
			## Do some format things , not done yet.
			echo Create boot sector ...
			## Linux version of GenBootSector has not pass build yet.
			$BASETOOLS_DIR/Genbootsector -i $EFI_BOOT_DISK -o FDBs.com
			$BASETOOLS_DIR/Bootsectimage -g FDBs.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bootsect.com -f
			$BASETOOLS_DIR/Genbootsector -o $EFI_BOOT_DISK -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bootsect.com
	
			cp $BUILD_DIR/FV/EfiLdr $EFI_BOOT_DISK
			cat $WORKSPACE/EdkShellBinPkg/bin/ia32/Shell.efi > $EFI_BOOT_DISK/efi/boot/bootia32.efi 
	
			echo Done.
		else
			echo "Wrong FAT type $3 for floppy!"
			exit
		fi
		;;

	file) # CreateFile
		if [ "$3" = FAT12 ]
			then
			echo "Start to create file boot disk ..."
			echo Create boot sector ...	
	
			## Linux version of GenBootSector has not pass build yet.
			$BASETOOLS_DIR/Genbootsector -i $EFI_BOOT_DISK -o FDBs.com
			$BASETOOLS_DIR/Bootsectimage -g FDBs.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bootsect.com -f
			$BASETOOLS_DIR/Genbootsector -o $EFI_BOOT_DISK -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bootsect.com
			echo Done.
		else
			echo "Wrong FAT type" $3 "for floppy!"
			exit
		fi
		;;

	usb) # CreateUsb

		if [ "$3" = FAT16 ]
		then
			if [ "$4" = step2 ]
			then
				cp $BUILD_DIR/FV/EfiLdr16 $EFI_BOOT_DISK
				mkdir $EFI_BOOT_DISK/efi/boot
				cp $WORKSPACE/EdkShellBinPkg/bin/ia32/Shell.efi $EFI_BOOT_DISK/efi/boot/bootia32.efi
				echo "step2 Done!"
			else
				echo Format $EFI_BOOT_DISK ...
				#Do format command.
				echo Create boot sector ...
				## Linux version of GenBootSector & Bootsectimage has not pass build yet.
				$BASETOOLS_DIR/Genbootsector -i $EFI_BOOT_DISK -o UsbBs16.com
				$BASETOOLS_DIR/Bootsectimage -g UsbBs16.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs16.com -f
				$BASETOOLS_DIR/Genbootsector -o $EFI_BOOT_DISK -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs16.com
				$BASETOOLS_DIR/Genbootsector -m -o $EFI_BOOT_DISK -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Mbr.com
				echo Done.
				echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN TO DO STEP2!
				exit 1
			fi
		elif [ "$3" = FAT32 ]
		then 
			if [ "$4" = step2 ]
			then
				cp $BUILD_DIR/FV/EfiLdr20 $EFI_BOOT_DISK
				mkdir $EFI_BOOT_DISK/efi/boot
				cp $WORKSPACE/EdkShellBinPkg/bin/ia32/Shell.efi $EFI_BOOT_DISK/efi/boot/bootia32.efi
				echo "step2 Done!"
			else
				echo Format $EFI_BOOT_DISK ...
				#Do format command.
				echo Create boot sector ...
				## Linux version of GenBootSector & Bootsectimage has not pass build yet.
				$BASETOOLS_DIR/Genbootsector -i $EFI_BOOT_DISK -o UsbBs32.com
				$BASETOOLS_DIR/Bootsectimage -g UsbBs32.com $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs32.com -f
				$BASETOOLS_DIR/Genbootsector -o $EFI_BOOT_DISK -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Bs32.com
				$BASETOOLS_DIR/Genbootsector -m -o $EFI_BOOT_DISK -i $BUILD_DIR/IA32/DuetPkg/BootSector/BootSector/OUTPUT/Mbr.com
				echo Done.
				echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN TO DO STEP2!
				exit 1
			fi			
		else
			echo "Wrong FAT type $1 for floppy!"
		fi
	
		;;

	ide) # CreateIde
	exit 1
	;;
	*)
	echo "Arg1 should be [floopy | file | usb | ide] !"
	exit 1

esac


































