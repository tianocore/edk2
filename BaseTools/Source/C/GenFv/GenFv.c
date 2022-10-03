/** @file
  This contains all code necessary to build the GenFvImage.exe utility.
  This utility relies heavily on the GenFvImage Lib.  Definitions for both
  can be found in the Tiano Firmware Volume Generation Utility
  Specification, review draft.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// File included in build
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "GenFvInternalLib.h"

//
// Utility Name
//
#define UTILITY_NAME  "GenFv"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

EFI_GUID  mEfiFirmwareFileSystem2Guid = EFI_FIRMWARE_FILE_SYSTEM2_GUID;
EFI_GUID  mEfiFirmwareFileSystem3Guid = EFI_FIRMWARE_FILE_SYSTEM3_GUID;

STATIC
VOID
Version (
  VOID
)
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

STATIC
VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "\nUsage: %s [options]\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -o FileName, --outputfile FileName\n\
                        File is the FvImage or CapImage to be created.\n");
  fprintf (stdout, "  -i FileName, --inputfile FileName\n\
                        File is the input FV.inf or Cap.inf to specify\n\
                        how to construct FvImage or CapImage.\n");
  fprintf (stdout, "  -b BlockSize, --blocksize BlockSize\n\
                        BlockSize is one HEX or DEC format value\n\
                        BlockSize is required by Fv Image.\n");
  fprintf (stdout, "  -n NumberBlock, --numberblock NumberBlock\n\
                        NumberBlock is one HEX or DEC format value\n\
                        NumberBlock is one optional parameter.\n");
  fprintf (stdout, "  -f FfsFile, --ffsfile FfsFile\n\
                        FfsFile is placed into Fv Image\n\
                        multi files can input one by one\n");
  fprintf (stdout, "  -s FileTakenSize, --filetakensize FileTakenSize\n\
                        FileTakenSize specifies the size of the required\n\
                        space that the input file is placed in Fvimage.\n\
                        It is specified together with the input file.\n");
  fprintf (stdout, "  -r Address, --baseaddr Address\n\
                        Address is the rebase start address for drivers that\n\
                        run in Flash. It supports DEC or HEX digital format.\n\
                        If it is set to zero, no rebase action will be taken\n");
  fprintf (stdout, "  -F ForceRebase, --force-rebase ForceRebase\n\
                        If value is TRUE, will always take rebase action\n\
                        If value is FALSE, will always not take reabse action\n\
                        If not specified, will take rebase action if rebase address greater than zero, \n\
                        will not take rebase action if rebase address is zero.\n");
  fprintf (stdout, "  -a AddressFile, --addrfile AddressFile\n\
                        AddressFile is one file used to record the child\n\
                        FV base address when current FV base address is set.\n");
  fprintf (stdout, "  -m logfile, --map logfile\n\
                        Logfile is the output fv map file name. if it is not\n\
                        given, the FvName.map will be the default map file name\n");
  fprintf (stdout, "  -g Guid, --guid Guid\n\
                        GuidValue is one specific capsule guid value\n\
                        or fv file system guid value.\n\
                        Its format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n");
  fprintf (stdout, "  --FvNameGuid Guid     Guid is used to specify Fv Name.\n\
                        Its format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n");
  fprintf (stdout, "  --capflag CapFlag     Capsule Reset Flag can be PersistAcrossReset,\n\
                        or PopulateSystemTable or InitiateReset or not set\n");
  fprintf (stdout, "  --capoemflag CapOEMFlag\n\
                        Capsule OEM Flag is an integer between 0x0000 and 0xffff\n");
  fprintf (stdout, "  --capheadsize HeadSize\n\
                        HeadSize is one HEX or DEC format value\n\
                        HeadSize is required by Capsule Image.\n");
  fprintf (stdout, "  -c, --capsule         Create Capsule Image.\n");
  fprintf (stdout, "  -p, --dump            Dump Capsule Image header.\n");
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  -d, --debug level     Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version             Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit.\n");
}

UINT32 mFvTotalSize;
UINT32 mFvTakenSize;

int
main (
  IN int   argc,
  IN char  **argv
  )
/*++

Routine Description:

  This utility uses GenFvImage.Lib to build a firmware volume image.

Arguments:

  FvInfFileName      The name of an FV image description file or Capsule Image.

  Arguments come in pair in any order.
    -I FvInfFileName

Returns:

  EFI_SUCCESS            No error conditions detected.
  EFI_INVALID_PARAMETER  One or more of the input parameters is invalid.
  EFI_OUT_OF_RESOURCES   A resource required by the utility was unavailable.
                         Most commonly this will be memory allocation
                         or file creation.
  EFI_LOAD_ERROR         GenFvImage.lib could not be loaded.
  EFI_ABORTED            Error executing the GenFvImage lib.

--*/
{
  EFI_STATUS            Status;
  CHAR8                 *InfFileName;
  CHAR8                 *AddrFileName;
  CHAR8                 *MapFileName;
  CHAR8                 *InfFileImage;
  UINT32                InfFileSize;
  CHAR8                 *OutFileName;
  BOOLEAN               CapsuleFlag;
  BOOLEAN               DumpCapsule;
  FILE                  *FpFile;
  EFI_CAPSULE_HEADER    *CapsuleHeader;
  UINT64                LogLevel, TempNumber;
  UINT32                Index;

  InfFileName   = NULL;
  AddrFileName  = NULL;
  InfFileImage  = NULL;
  OutFileName   = NULL;
  MapFileName   = NULL;
  InfFileSize   = 0;
  CapsuleFlag   = FALSE;
  DumpCapsule   = FALSE;
  FpFile        = NULL;
  CapsuleHeader = NULL;
  LogLevel      = 0;
  TempNumber    = 0;
  Index         = 0;
  mFvTotalSize  = 0;
  mFvTakenSize  = 0;
  Status        = EFI_SUCCESS;

  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No input options specified.");
    Usage ();
    return STATUS_ERROR;
  }

  //
  // Init global data to Zero
  //
  memset (&mFvDataInfo, 0, sizeof (FV_INFO));
  memset (&mCapDataInfo, 0, sizeof (CAP_INFO));
  //
  // Set the default FvGuid
  //
  memcpy (&mFvDataInfo.FvFileSystemGuid, &mEfiFirmwareFileSystem2Guid, sizeof (EFI_GUID));
  mFvDataInfo.ForceRebase = -1;

  //
  // Parse command line
  //
  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Version ();
    Usage ();
    return STATUS_SUCCESS;
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version ();
    return STATUS_SUCCESS;
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-i") == 0) || (stricmp (argv[0], "--inputfile") == 0)) {
      InfFileName = argv[1];
      if (InfFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Input file can't be null");
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-a") == 0) || (stricmp (argv[0], "--addrfile") == 0)) {
      AddrFileName = argv[1];
      if (AddrFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Address file can't be null");
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--outputfile") == 0)) {
      OutFileName = argv[1];
      if (OutFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Output file can't be null");
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-r") == 0) || (stricmp (argv[0], "--baseaddr") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempNumber);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      mFvDataInfo.BaseAddress    = TempNumber;
      mFvDataInfo.BaseAddressSet = TRUE;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-b") == 0) || (stricmp (argv[0], "--blocksize") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempNumber);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      if (TempNumber == 0) {
        Error (NULL, 0, 1003, "Invalid option value", "Fv block size can't be set to zero");
        return STATUS_ERROR;
      }
      mFvDataInfo.FvBlocks[0].Length = (UINT32) TempNumber;
      DebugMsg (NULL, 0, 9, "FV Block Size", "%s = 0x%llx", EFI_BLOCK_SIZE_STRING, (unsigned long long) TempNumber);
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-n") == 0) || (stricmp (argv[0], "--numberblock") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempNumber);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      if (TempNumber == 0) {
        Error (NULL, 0, 1003, "Invalid option value", "Fv block number can't be set to zero");
        return STATUS_ERROR;
      }
      mFvDataInfo.FvBlocks[0].NumBlocks = (UINT32) TempNumber;
      DebugMsg (NULL, 0, 9, "FV Number Block", "%s = 0x%llx", EFI_NUM_BLOCKS_STRING, (unsigned long long) TempNumber);
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((strcmp (argv[0], "-f") == 0) || (stricmp (argv[0], "--ffsfile") == 0)) {
      if (argv[1] == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Input Ffsfile can't be null");
        return STATUS_ERROR;
      }
      if (strlen (argv[1]) > MAX_LONG_FILE_PATH - 1) {
        Error (NULL, 0, 1003, "Invalid option value", "Input Ffsfile name %s is too long!", argv[1]);
        return STATUS_ERROR;
      }
      strncpy (mFvDataInfo.FvFiles[Index], argv[1], MAX_LONG_FILE_PATH - 1);
      mFvDataInfo.FvFiles[Index][MAX_LONG_FILE_PATH - 1] = 0;
      DebugMsg (NULL, 0, 9, "FV component file", "the %uth name is %s", (unsigned) Index + 1, argv[1]);
      argc -= 2;
      argv += 2;

      if (argc > 0) {
        if ((stricmp (argv[0], "-s") == 0) || (stricmp (argv[0], "--filetakensize") == 0)) {
          if (argv[1] == NULL) {
            Error (NULL, 0, 1003, "Invalid option value", "Ffsfile Size can't be null");
            return STATUS_ERROR;
          }
          Status = AsciiStringToUint64 (argv[1], FALSE, &TempNumber);
          if (EFI_ERROR (Status)) {
            Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
            return STATUS_ERROR;
          }
          mFvDataInfo.SizeofFvFiles[Index] = (UINT32) TempNumber;
          DebugMsg (NULL, 0, 9, "FV component file size", "the %uth size is %s", (unsigned) Index + 1, argv[1]);
          argc -= 2;
          argv += 2;
        }
      }
      Index ++;
      continue;
    }

    if ((stricmp (argv[0], "-s") == 0) || (stricmp (argv[0], "--filetakensize") == 0)) {
      Error (NULL, 0, 1003, "Invalid option", "It must be specified together with -f option to specify the file size.");
      return STATUS_ERROR;
    }

    if ((stricmp (argv[0], "-c") == 0) || (stricmp (argv[0], "--capsule") == 0)) {
      CapsuleFlag = TRUE;
      argc --;
      argv ++;
      continue;
    }

    if ((strcmp (argv[0], "-F") == 0) || (stricmp (argv[0], "--force-rebase") == 0)) {
      if (argv[1] == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Froce rebase flag can't be null");
        return STATUS_ERROR;
      }

      if (stricmp (argv[1], "TRUE") == 0) {
        mFvDataInfo.ForceRebase = 1;
      } else if (stricmp (argv[1], "FALSE") == 0) {
        mFvDataInfo.ForceRebase = 0;
      } else {
        Error (NULL, 0, 1003, "Invalid option value", "froce rebase flag value must be \"TRUE\" or \"FALSE\"");
        return STATUS_ERROR;
      }

      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "--capheadsize") == 0) {
      //
      // Get Capsule Image Header Size
      //
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempNumber);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      mCapDataInfo.HeaderSize = (UINT32) TempNumber;
      DebugMsg (NULL, 0, 9, "Capsule Header size", "%s = 0x%llx", EFI_CAPSULE_HEADER_SIZE_STRING, (unsigned long long) TempNumber);
      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "--capflag") == 0) {
      //
      // Get Capsule Header
      //
      if (argv[1] == NULL) {
        Error (NULL, 0, 1003, "Option value is not set", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      if (strcmp (argv[1], "PopulateSystemTable") == 0) {
        mCapDataInfo.Flags |= CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE;
      } else if (strcmp (argv[1], "PersistAcrossReset") == 0) {
        mCapDataInfo.Flags |= CAPSULE_FLAGS_PERSIST_ACROSS_RESET;
      } else if (strcmp (argv[1], "InitiateReset") == 0) {
        mCapDataInfo.Flags |= CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET;
      } else {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      DebugMsg (NULL, 0, 9, "Capsule Flag", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "--capoemflag") == 0) {
      if (argv[1] == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Capsule OEM flag can't be null");
      }
      Status = AsciiStringToUint64(argv[1], FALSE, &TempNumber);
      if (EFI_ERROR (Status) || TempNumber > 0xffff) {
        Error (NULL, 0, 1003, "Invalid option value", "Capsule OEM flag value must be integer value between 0x0000 and 0xffff");
        return STATUS_ERROR;
      }
      mCapDataInfo.Flags |= TempNumber;
      DebugMsg( NULL, 0, 9, "Capsule OEM Flags", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "--capguid") == 0) {
      //
      // Get the Capsule Guid
      //
      Status = StringToGuid (argv[1], &mCapDataInfo.CapGuid);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", EFI_CAPSULE_GUID_STRING, argv[1]);
        return STATUS_ERROR;
      }
      DebugMsg (NULL, 0, 9, "Capsule Guid", "%s = %s", EFI_CAPSULE_GUID_STRING, argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-g") == 0) || (stricmp (argv[0], "--guid") == 0)) {
      //
      // Get the Capsule or Fv Guid
      //
      Status = StringToGuid (argv[1], &mCapDataInfo.CapGuid);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", EFI_GUID_STRING, argv[1]);
        return STATUS_ERROR;
      }
      memcpy (&mFvDataInfo.FvFileSystemGuid, &mCapDataInfo.CapGuid, sizeof (EFI_GUID));
      mFvDataInfo.FvFileSystemGuidSet = TRUE;
      DebugMsg (NULL, 0, 9, "Capsule Guid", "%s = %s", EFI_CAPSULE_GUID_STRING, argv[1]);
      DebugMsg (NULL, 0, 9, "FV Guid", "%s = %s", EFI_FV_FILESYSTEMGUID_STRING, argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    if (stricmp (argv[0], "--FvNameGuid") == 0) {
      //
      // Get Fv Name Guid
      //
      Status = StringToGuid (argv[1], &mFvDataInfo.FvNameGuid);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", EFI_GUID_STRING, argv[1]);
        return STATUS_ERROR;
      }
      mFvDataInfo.FvNameGuidSet = TRUE;
      DebugMsg (NULL, 0, 9, "FV Name Guid", "%s = %s", EFI_FV_NAMEGUID_STRING, argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-p") == 0) || (stricmp (argv[0], "--dump") == 0)) {
      DumpCapsule = TRUE;
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-m") == 0) || (stricmp (argv[0], "--map") == 0)) {
      MapFileName = argv[1];
      if (MapFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Map file can't be null");
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-v") == 0) || (stricmp (argv[0], "--verbose") == 0)) {
      SetPrintLevel (VERBOSE_LOG_LEVEL);
      VerboseMsg ("Verbose output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      SetPrintLevel (KEY_LOG_LEVEL);
      KeyMsg ("Quiet output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &LogLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, current input level is %d", (int) LogLevel);
        return STATUS_ERROR;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    //
    // Don't recognize the parameter.
    //
    Error (NULL, 0, 1000, "Unknown option", "%s", argv[0]);
    return STATUS_ERROR;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  //
  // check input parameter, InfFileName can be NULL
  //
  if (InfFileName == NULL && DumpCapsule) {
    Error (NULL, 0, 1001, "Missing option", "Input Capsule Image");
    return STATUS_ERROR;
  }
  VerboseMsg ("the input FvInf or CapInf file name is %s", InfFileName);

  if (!DumpCapsule && OutFileName == NULL) {
    Error (NULL, 0, 1001, "Missing option", "Output File");
    return STATUS_ERROR;
  }
  if (OutFileName != NULL) {
    VerboseMsg ("the output file name is %s", OutFileName);
  }

  //
  // Read the INF file image
  //
  if (InfFileName != NULL) {
    Status = GetFileImage (InfFileName, &InfFileImage, &InfFileSize);
    if (EFI_ERROR (Status)) {
      return STATUS_ERROR;
    }
  }

  if (DumpCapsule) {
    VerboseMsg ("Dump the capsule header information for the input capsule image %s", InfFileName);
    //
    // Dump Capsule Image Header Information
    //
    CapsuleHeader = (EFI_CAPSULE_HEADER *) InfFileImage;
    if (OutFileName == NULL) {
      FpFile = stdout;
    } else {
      FpFile = fopen (LongFilePath (OutFileName), "w");
      if (FpFile == NULL) {
        Error (NULL, 0, 0001, "Error opening file", OutFileName);
        return STATUS_ERROR;
      }
    }
    if (FpFile != NULL) {
      fprintf (FpFile, "Capsule %s Image Header Information\n", InfFileName);
      fprintf (FpFile, "  GUID                  %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
                      (unsigned) CapsuleHeader->CapsuleGuid.Data1,
                      (unsigned) CapsuleHeader->CapsuleGuid.Data2,
                      (unsigned) CapsuleHeader->CapsuleGuid.Data3,
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[0],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[1],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[2],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[3],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[4],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[5],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[6],
                      (unsigned) CapsuleHeader->CapsuleGuid.Data4[7]);
      fprintf (FpFile, "  Header size           0x%08X\n", (unsigned) CapsuleHeader->HeaderSize);
      fprintf (FpFile, "  Flags                 0x%08X\n", (unsigned) CapsuleHeader->Flags);
      fprintf (FpFile, "  Capsule image size    0x%08X\n", (unsigned) CapsuleHeader->CapsuleImageSize);
      fclose (FpFile);
    }
  } else if (CapsuleFlag) {
    VerboseMsg ("Create capsule image");
    //
    // Call the GenerateCapImage to generate Capsule Image
    //
    for (Index = 0; mFvDataInfo.FvFiles[Index][0] != '\0'; Index ++) {
      strcpy (mCapDataInfo.CapFiles[Index], mFvDataInfo.FvFiles[Index]);
    }

    Status = GenerateCapImage (
              InfFileImage,
              InfFileSize,
              OutFileName
              );
  } else {
    VerboseMsg ("Create Fv image and its map file");
    //
    // Will take rebase action at below situation:
    // 1. ForceRebase Flag specified to TRUE;
    // 2. ForceRebase Flag not specified, BaseAddress greater than zero.
    //
    if (((mFvDataInfo.BaseAddress > 0) && (mFvDataInfo.ForceRebase == -1)) || (mFvDataInfo.ForceRebase == 1)) {
      VerboseMsg ("FvImage Rebase Address is 0x%llX", (unsigned long long) mFvDataInfo.BaseAddress);
    }
    //
    // Call the GenerateFvImage to generate Fv Image
    //
    Status = GenerateFvImage (
              InfFileImage,
              InfFileSize,
              OutFileName,
              MapFileName
              );
  }

  //
  // free InfFileImage memory
  //
  if (InfFileImage != NULL) {
    free (InfFileImage);
  }

  //
  //  update boot driver address and runtime driver address in address file
  //
  if (Status == EFI_SUCCESS && AddrFileName != NULL && mFvBaseAddressNumber > 0) {
    FpFile = fopen (LongFilePath (AddrFileName), "w");
    if (FpFile == NULL) {
      Error (NULL, 0, 0001, "Error opening file", AddrFileName);
      return STATUS_ERROR;
    }
    fprintf (FpFile, FV_BASE_ADDRESS_STRING);
    fprintf (FpFile, "\n");
    for (Index = 0; Index < mFvBaseAddressNumber; Index ++) {
      fprintf (
        FpFile,
        "0x%llx\n",
        (unsigned long long)mFvBaseAddress[Index]
        );
    }
    fflush (FpFile);
    fclose (FpFile);
  }

  if (Status == EFI_SUCCESS) {
    DebugMsg (NULL, 0, 9, "The Total Fv Size", "%s = 0x%x", EFI_FV_TOTAL_SIZE_STRING, (unsigned) mFvTotalSize);
    DebugMsg (NULL, 0, 9, "The used Fv Size", "%s = 0x%x", EFI_FV_TAKEN_SIZE_STRING, (unsigned) mFvTakenSize);
    DebugMsg (NULL, 0, 9, "The space Fv size", "%s = 0x%x", EFI_FV_SPACE_SIZE_STRING, (unsigned) (mFvTotalSize - mFvTakenSize));
  }

  VerboseMsg ("%s tool done with return code is 0x%x.", UTILITY_NAME, GetUtilityStatus ());

  return GetUtilityStatus ();
}
