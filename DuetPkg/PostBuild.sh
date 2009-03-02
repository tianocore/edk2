#
#  Currently, Build system does not provide post build mechanism for module 
#  and platform building, so just use a sh file to do post build commands.
#  Originally, following post building command is for EfiLoader module.
#
export BUILD_DIR=$WORKSPACE/Build/DuetPkg/DEBUG_UNIXGCC
export BASETOOLS_DIR=$WORKSPACE/Conf/BaseToolsSource/Source/C/bin
export PROCESSOR=""
if [ \
     -z "$1" -o \
     "$1" = "-?" -o \
     "$1" = "-h" -o \
     "$1" = "--help" \
   ]
then
	echo Error! Please specific the architecture. 
	echo Usage: ". PostBuild.sh [IA32|X64]"
fi

if [ "$1" = IA32 ]
then
	export PROCESSOR=IA32
fi

if [ "$1" = X64 ]
then
	export PROCESSOR=X64
fi

export OUTPUT_DIR=$BUILD_DIR/$PROCESSOR/DuetPkg/BootSector/BootSector/OUTPUT

echo Compressing DUETEFIMainFv.FV ...
$BASETOOLS_DIR/TianoCompress -e -o $BUILD_DIR/FV/DUETEFIMAINFV.z $BUILD_DIR/FV/DUETEFIMAINFV.Fv

echo Compressing DxeMain.efi ...
$BASETOOLS_DIR/TianoCompress -e -o $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/$PROCESSOR/DxeCore.efi

echo Compressing DxeIpl.efi ...
$BASETOOLS_DIR/TianoCompress -e -o $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/$PROCESSOR/DxeIpl.efi	

echo Generate Loader Image ...

if [ $PROCESSOR = IA32 ]
then
	$BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr32 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z
	mkdir -p $BUILD_DIR/FV/Efildr
	mkdir -p $BUILD_DIR/FV/Efildr16
	mkdir -p $BUILD_DIR/FV/Efildr20

	cp $OUTPUT_DIR/start.com $OUTPUT_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 $BUILD_DIR/FV/Efildr
	cp $OUTPUT_DIR/start16.com $OUTPUT_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 $BUILD_DIR/FV/Efildr16
	cp $OUTPUT_DIR/start32.com $OUTPUT_DIR/efi32.com2 $BUILD_DIR/FV/Efildr32 $BUILD_DIR/FV/Efildr20	
	echo Done!
fi

if [ $PROCESSOR = X64 ]
then
	$BASETOOLS_DIR/EfiLdrImage -o $BUILD_DIR/FV/Efildr64 $BUILD_DIR/$PROCESSOR/EfiLoader.efi $BUILD_DIR/FV/DxeIpl.z $BUILD_DIR/FV/DxeMain.z $BUILD_DIR/FV/DUETEFIMAINFV.z
	mkdir -p $BUILD_DIR/FV/EfildrPure
	mkdir -p $BUILD_DIR/FV/Efildr16Pure
	mkdir -p $BUILD_DIR/FV/Efildr20Pure

	cp $OUTPUT_DIR/start64.com $OUTPUT_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 $BUILD_DIR/FV/EfildrPure
	$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/EfildrPure -o $BUILD_DIR/FV/Efildr
	cp $OUTPUT_DIR/st16_64.com $OUTPUT_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 $BUILD_DIR/FV/Efildr16Pure
	$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/Efildr16Pure -o $BUILD_DIR/FV/Efildr16
	cp $OUTPUT_DIR/st32_64.com $OUTPUT_DIR/efi64.com2 $BUILD_DIR/FV/Efildr64 $BUILD_DIR/FV/Efildr20Pure
	$BASETOOLS_DIR/GenPage $BUILD_DIR/FV/Efildr20Pure -o $BUILD_DIR/FV/Efildr20
	
	echo Done!
fi

