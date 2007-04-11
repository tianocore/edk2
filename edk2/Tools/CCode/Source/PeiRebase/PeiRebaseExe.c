/*++

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  PeiRebaseExe.c

Abstract:

  This contains all code necessary to build the PeiRebase.exe utility.
  This utility relies heavily on the PeiRebase DLL.  Definitions for both
  can be found in the PEI Rebase Utility Specification, review draft.

--*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Common/UefiBaseTypes.h>
#include <Common/FirmwareVolumeImageFormat.h>
#include <Common/FirmwareFileSystem.h>
#include <Library/PeCoffLib.h>

#include "CommonLib.h"
#include "ParseInf.h"
#include "FvLib.h"
#include "EfiUtilityMsgs.h"
#include "PeiRebaseExe.h"

EFI_STATUS
ReadHeader (
  IN FILE       *InputFile,
  OUT UINT32    *FvSize,
  OUT BOOLEAN   *ErasePolarity
  );

int
main (
  int  argc,
  char **argv
  )
/*++

Routine Description:

  This utility relocates PEI XIP PE32s in a FV.

Arguments:

  argc          - Number of command line arguments
  argv[]:
  BaseAddress     The base address to use for rebasing the FV.  The correct
                  format is a hex number preceded by 0x.
  InputFileName   The name of the input FV file.
  OutputFileName  The name of the output FV file.

  Arguments come in pair in any order.
    -I InputFileName
    -O OutputFileName
    -B BaseAddress

Returns:

  0   No error conditions detected.
  1   One or more of the input parameters is invalid.
  2   A resource required by the utility was unavailable.
      Most commonly this will be memory allocation or file creation.
  3   PeiRebase.dll could not be loaded.
  4   Error executing the PEI rebase.

--*/
{
  UINT8                       Index;
  CHAR8                       InputFileName[_MAX_PATH];
  CHAR8                       *OutputFileName;
  EFI_PHYSICAL_ADDRESS        XipBase, BsBase, RtBase;
  UINT32                      BaseTypes;
  EFI_STATUS                  Status;
  FILE                        *InputFile;
  FILE                        *OutputFile;
  FILE                        *LogFile;
  UINT64                      FvOffset;
  UINT32                      FileCount;
  int                         BytesRead;
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage;
  UINT32                      FvSize;
  EFI_FFS_FILE_HEADER         *CurrentFile;
  BOOLEAN                     ErasePolarity;
  MEMORY_FILE                 InfMemoryFile;
  CHAR8                       StringBuffer[0x100];

  ErasePolarity = FALSE;
  //
  // Set utility name for error/warning reporting purposes.
  //
  SetUtilityName (UTILITY_NAME);

  if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0) ||
      (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }

  if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
    Version();
    return STATUS_ERROR;
  }

  //
  // Verify the correct number of arguments
  //
  if (argc < MAX_ARGS) {
    Usage ();
    return STATUS_ERROR;
  }

  //
  // Initialize variables
  //
  InputFileName[0]  = '\0';
  OutputFileName    = NULL;
  XipBase = BsBase = RtBase = 0;
  BaseTypes         = 0;
  FvOffset          = 0;
  FileCount         = 0;
  ErasePolarity     = FALSE;
  InputFile         = NULL;
  OutputFile        = NULL;
  LogFile           = NULL;
  FvImage           = NULL;

  //
  // Parse the command line arguments
  //
  for (Index = 1; Index < argc; Index += 2) {
    //
    // Make sure argument pair begin with - or /
    //
    if (argv[Index][0] != '-' && argv[Index][0] != '/') {
      Usage ();
      Error (NULL, 0, 0, argv[Index], "unrecognized option");
      return STATUS_ERROR;
    }
    //
    // Make sure argument specifier is only one letter
    //
    if (argv[Index][2] != 0) {
      Usage ();
      Error (NULL, 0, 0, argv[Index], "unrecognized option");
      return STATUS_ERROR;
    }
    //
    // Determine argument to read
    //
    switch (argv[Index][1]) {
    case 'I':
    case 'i':
      if (strlen (InputFileName) == 0) {
        strcpy (InputFileName, argv[Index + 1]);
      } else {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "only one -i InputFileName may be specified");
        return STATUS_ERROR;
      }
      break;

    case 'O':
    case 'o':
      if (OutputFileName == NULL) {
        OutputFileName = argv[Index + 1];
      } else {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "only one -o OutputFileName may be specified");
        return STATUS_ERROR;
      }
      break;

    case 'F':
    case 'f':
      //
      // Load INF file into memory & initialize MEMORY_FILE structure
      //
      Status = GetFileImage (argv[Index + 1], &InfMemoryFile.FileImage, (UINT32*)&InfMemoryFile.Eof);
      InfMemoryFile.Eof = InfMemoryFile.FileImage + (UINT32)(UINTN)InfMemoryFile.Eof;
      InfMemoryFile.CurrentFilePointer = InfMemoryFile.FileImage;
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0, argv[Index + 1], "Error opening FvInfFile");
        return STATUS_ERROR;
      }

      //
      // Read BaseAddress from fv.inf file
      //
      FindToken (&InfMemoryFile, "[options]", "EFI_BASE_ADDRESS", 0, StringBuffer);

      //
      // Free INF file image
      //
      free (InfMemoryFile.FileImage);

      //
      // Point argv[Index + 1] to StringBuffer so that it could be processed as "-b"
      //
      argv[Index + 1] = StringBuffer;

    case 'B':
    case 'b':
      if (BaseTypes & 1) {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "XipBaseAddress may be specified only once by either -b or -f");
        return STATUS_ERROR;
      }

      Status = AsciiStringToUint64 (argv[Index + 1], FALSE, &XipBase);
      if (EFI_ERROR (Status)) {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "invalid hex digit given for XIP base address");
        return STATUS_ERROR;
      }

      BaseTypes |= 1;
      break;

    case 'D':
    case 'd':
      if (BaseTypes & 2) {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "-d BsBaseAddress may be specified only once");
        return STATUS_ERROR;
      }

      Status = AsciiStringToUint64 (argv[Index + 1], FALSE, &BsBase);
      if (EFI_ERROR (Status)) {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "invalid hex digit given for BS_DRIVER base address");
        return STATUS_ERROR;
      }

      BaseTypes |= 2;
      break;

    case 'R':
    case 'r':
      if (BaseTypes & 4) {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "-r RtBaseAddress may be specified only once");
        return STATUS_ERROR;
      }

      Status = AsciiStringToUint64 (argv[Index + 1], FALSE, &RtBase);
      if (EFI_ERROR (Status)) {
        Usage ();
        Error (NULL, 0, 0, argv[Index + 1], "invalid hex digit given for RT_DRIVER base address");
        return STATUS_ERROR;
      }

      BaseTypes |= 4;
      break;

    default:
      Usage ();
      Error (NULL, 0, 0, argv[Index], "unrecognized argument");
      return STATUS_ERROR;
      break;
    }
  }

  //
  // Open the file containing the FV
  //
  InputFile = fopen (InputFileName, "rb");
  if (InputFile == NULL) {
    Error (NULL, 0, 0, InputFileName, "could not open input file for reading");
    return STATUS_ERROR;
  }

  //
  // Open the log file
  //
  strcat (InputFileName, ".log");
  LogFile = fopen (InputFileName, "w");
  if (LogFile == NULL) {
    Error (NULL, 0, 0, InputFileName, "could not append to log file");
  }

  //
  // Determine size of FV
  //
  Status = ReadHeader (InputFile, &FvSize, &ErasePolarity);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "could not parse the FV header", NULL);
    goto Finish;
  }
  //
  // Allocate a buffer for the FV image
  //
  FvImage = malloc (FvSize);
  if (FvImage == NULL) {
    Error (NULL, 0, 0, "application error", "memory allocation failed");
    goto Finish;
  }
  //
  // Read the entire FV to the buffer
  //
  BytesRead = fread (FvImage, 1, FvSize, InputFile);
  fclose (InputFile);
  InputFile = NULL;
  if ((unsigned int) BytesRead != FvSize) {
    Error (NULL, 0, 0, InputFileName, "failed to read from file");
    goto Finish;
  }
  //
  // Prepare to walk the FV image
  //
  InitializeFvLib (FvImage, FvSize);
  //
  // Get the first file
  //
  Status = GetNextFile (NULL, &CurrentFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "cannot find the first file in the FV image", NULL);
    goto Finish;
  }
  //
  // Check if each file should be rebased
  //
  while (CurrentFile != NULL) {
    //
    // Rebase this file
    //
    FfsRebase (
      CurrentFile,
      BaseTypes,
      XipBase + (UINTN)CurrentFile - (UINTN)FvImage,
      &BsBase,
      &RtBase,
      LogFile
      );

    if (EFI_ERROR (Status)) {
      switch (Status) {

      case EFI_INVALID_PARAMETER:
        Error (NULL, 0, 0, "invalid parameter passed to FfsRebase", NULL);
        break;

      case EFI_ABORTED:
        Error (NULL, 0, 0, "error detected while rebasing -- aborted", NULL);
        break;

      case EFI_OUT_OF_RESOURCES:
        Error (NULL, 0, 0, "FfsRebase could not allocate required resources", NULL);
        break;

      case EFI_NOT_FOUND:
        Error (NULL, 0, 0, "FfsRebase could not locate a PE32 section", NULL);
        break;

      default:
        Error (NULL, 0, 0, "FfsRebase returned unknown status", "status=0x%08X", Status);
        break;
      }

      goto Finish;
    }
    //
    // Get the next file
    //
    Status = GetNextFile (CurrentFile, &CurrentFile);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "cannot find the next file in the FV image", NULL);
      goto Finish;
    }
  }
  //
  // Open the output file
  //
  OutputFile = fopen (OutputFileName, "wb");
  if (OutputFile == NULL) {
    Error (NULL, 0, 0, OutputFileName, "failed to open output file");
    goto Finish;
  }

  if (fwrite (FvImage, 1, FvSize, OutputFile) != FvSize) {
    Error (NULL, 0, 0, "failed to write to output file", 0);
    goto Finish;
  }

Finish:
  if (InputFile != NULL) {
    fclose (InputFile);
  }
  //
  // If we created an output file, and there was an error, remove it so
  // subsequent builds will rebuild it.
  //
  if (OutputFile != NULL) {
    if (GetUtilityStatus () == STATUS_ERROR) {
      remove (OutputFileName);
    }

    fclose (OutputFile);
  }

  if (LogFile != NULL) {
    fclose (LogFile);
  }

  if (FvImage != NULL) {
    free (FvImage);
  }

  return GetUtilityStatus ();
}

EFI_STATUS
ReadHeader (
  IN FILE       *InputFile,
  OUT UINT32    *FvSize,
  OUT BOOLEAN   *ErasePolarity
  )
/*++

Routine Description:

  This function determines the size of the FV and the erase polarity.  The
  erase polarity is the FALSE value for file state.

Arguments:

  InputFile       The file that contains the FV image.
  FvSize          The size of the FV.
  ErasePolarity   The FV erase polarity.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_INVALID_PARAMETER   A required parameter was NULL or is out of range.
  EFI_ABORTED             The function encountered an error.

--*/
{
  EFI_FIRMWARE_VOLUME_HEADER  VolumeHeader;
  EFI_FV_BLOCK_MAP_ENTRY      BlockMap;
  UINTN                       Signature[2];
  UINTN                       BytesRead;
  UINT32                      Size;

  BytesRead = 0;
  Size      = 0;
  //
  // Check input parameters
  //
  if ((InputFile == NULL) || (FvSize == NULL) || (ErasePolarity == NULL)) {
    Error (NULL, 0, 0, "ReadHeader()", "invalid input parameter");
    return EFI_INVALID_PARAMETER;
  }
  //
  // Read the header
  //
  fread (&VolumeHeader, sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
  BytesRead     = sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  Signature[0]  = VolumeHeader.Signature;
  Signature[1]  = 0;

  //
  // Get erase polarity
  //
  if (VolumeHeader.Attributes & EFI_FVB_ERASE_POLARITY) {
    *ErasePolarity = TRUE;
  }

  do {
    fread (&BlockMap, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
    BytesRead += sizeof (EFI_FV_BLOCK_MAP_ENTRY);

    if (BlockMap.NumBlocks != 0) {
      Size += BlockMap.NumBlocks * BlockMap.BlockLength;
    }

  } while (!(BlockMap.NumBlocks == 0 && BlockMap.BlockLength == 0));

  if (VolumeHeader.FvLength != Size) {
    Error (NULL, 0, 0, "volume size not consistant with block maps", NULL);
    return EFI_ABORTED;
  }

  *FvSize = Size;

  rewind (InputFile);

  return EFI_SUCCESS;
}

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
  printf ("%s v%d.%d -PEI Rebase Utility.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2007 Intel Corporation. All rights reserved.\n");
}

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
  Version();

  printf (
    "Usage: %s -I InputFileName -O OutputFileName -B BaseAddress [-F InputFvInfName]\n",
    UTILITY_NAME
    );
  printf ("  [-D BootDriverBaseAddress] [-R RuntimeDriverBaseAddress]\n");
  printf ("  Where:\n");
  printf ("      InputFileName is the name of the EFI FV file to rebase.\n");
  printf ("      OutputFileName is the desired output file name.\n");
  printf ("      BaseAddress is the rebase address for all drivers run in Flash.\n");
  printf ("      InputFvInfName is the Fv.inf file that contains this FV base address to rebase against.\n");
  printf ("      BootDriverBaseAddress is the rebase address for all boot drivers in this fv image.\n");
  printf ("      RuntimeDriverBaseAddress is the rebase address for all runtime drivers in this fv image.\n");
  printf ("  Argument pair may be in any order.\n\n");
}

EFI_STATUS
FfsRebase (
  IN OUT  EFI_FFS_FILE_HEADER       *FfsFile,
  IN      UINT32                    Flags,
  IN OUT  EFI_PHYSICAL_ADDRESS      XipBase,
  IN OUT  EFI_PHYSICAL_ADDRESS      *BsBase,
  IN OUT  EFI_PHYSICAL_ADDRESS      *RtBase,
  OUT     FILE                      *LogFile
  )
/*++

Routine Description:

  This function determines if a file is XIP and should be rebased.  It will
  rebase any PE32 sections found in the file using the base address.

Arguments:

  FfsFile           A pointer to Ffs file image.
  BaseAddress       The base address to use for rebasing the file image.

Returns:

  EFI_SUCCESS             The image was properly rebased.
  EFI_INVALID_PARAMETER   An input parameter is invalid.
  EFI_ABORTED             An error occurred while rebasing the input file image.
  EFI_OUT_OF_RESOURCES    Could not allocate a required resource.
  EFI_NOT_FOUND           No compressed sections could be found.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  EFI_PHYSICAL_ADDRESS                  NewPe32BaseAddress;
  UINTN                                 Index;
  EFI_FILE_SECTION_POINTER              CurrentPe32Section;
  EFI_FFS_FILE_STATE                    SavedState;
  EFI_IMAGE_NT_HEADERS32                *PeHdr;
  EFI_TE_IMAGE_HEADER                   *TEImageHeader;
  UINT8                                 FileGuidString[80];
  UINT32                                TailSize;
  EFI_FFS_FILE_TAIL                     TailValue;
  EFI_PHYSICAL_ADDRESS                  *BaseToUpdate;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY       *DebugEntry;


  //
  // Verify input parameters
  //
  if (FfsFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Convert the GUID to a string so we can at least report which file
  // if we find an error.
  //
  PrintGuidToBuffer (&FfsFile->Name, FileGuidString, sizeof (FileGuidString), TRUE);
  if (FfsFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
    TailSize = sizeof (EFI_FFS_FILE_TAIL);
  } else {
    TailSize = 0;
  }
  //
  // Do some cursory checks on the FFS file contents
  //
  Status = VerifyFfsFile (FfsFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "file does not appear to be a valid FFS file, cannot be rebased", FileGuidString);
    return EFI_INVALID_PARAMETER;
  }

  //
  // We only process files potentially containing PE32 sections.
  //
  switch (FfsFile->Type) {
    case EFI_FV_FILETYPE_SECURITY_CORE:
    case EFI_FV_FILETYPE_PEI_CORE:
    case EFI_FV_FILETYPE_PEIM:
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
    case EFI_FV_FILETYPE_DRIVER:
    case EFI_FV_FILETYPE_DXE_CORE:
      break;
    default:
      return EFI_SUCCESS;
  }

  //
  // Rebase each PE32 section
  //
  Status      = EFI_SUCCESS;
  for (Index = 1;; Index++) {
    Status = GetSectionByType (FfsFile, EFI_SECTION_PE32, Index, &CurrentPe32Section);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Initialize context
    //
    memset (&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle     = (VOID *) ((UINTN) CurrentPe32Section.Pe32Section + sizeof (EFI_PE32_SECTION));
    ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) FfsRebaseImageRead;
    Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "GetImageInfo() call failed on rebase", FileGuidString);
      return Status;
    }

    //
    // Don't Load PeImage, only to relocate current image.
    //
    ImageContext.ImageAddress = (UINTN) CurrentPe32Section.Pe32Section + sizeof (EFI_PE32_SECTION);

    //
    // Check if section-alignment and file-alignment match or not
    //
    PeHdr = (EFI_IMAGE_NT_HEADERS *)((UINTN)ImageContext.ImageAddress + ImageContext.PeCoffHeaderOffset);
    if (PeHdr->OptionalHeader.SectionAlignment != PeHdr->OptionalHeader.FileAlignment) {
      //
      // Nor XIP module can be ignored.
      //
      if ((Flags & 1) == 0) {
        continue;
      }
      Error (NULL, 0, 0, "Section-Alignment and File-Alignment does not match", FileGuidString);
      return EFI_ABORTED;
    }

    //
    // Update CodeView and PdbPointer in ImageContext
    //
    DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)(UINTN)(
                    ImageContext.ImageAddress +
                    ImageContext.DebugDirectoryEntryRva
                    );
    ImageContext.CodeView = (VOID *)(UINTN)(
                              ImageContext.ImageAddress +
                              DebugEntry->RVA
                              );
    switch (*(UINT32 *) ImageContext.CodeView) {
    case CODEVIEW_SIGNATURE_NB10:
      ImageContext.PdbPointer = (CHAR8 *) ImageContext.CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
      break;

    case CODEVIEW_SIGNATURE_RSDS:
      ImageContext.PdbPointer = (CHAR8 *) ImageContext.CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
      break;

    default:
      break;
    }

    //
    // Calculate the PE32 base address, based on file type
    //
    switch (FfsFile->Type) {
      case EFI_FV_FILETYPE_SECURITY_CORE:
      case EFI_FV_FILETYPE_PEI_CORE:
      case EFI_FV_FILETYPE_PEIM:
      case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
        if ((Flags & 1) == 0) {
          //
          // We aren't relocating XIP code, so skip it.
          //
          return EFI_SUCCESS;
        }

        NewPe32BaseAddress =
          XipBase + (UINTN)ImageContext.ImageAddress - (UINTN)FfsFile;
        BaseToUpdate = &XipBase;
        break;

      case EFI_FV_FILETYPE_DRIVER:
        PeHdr = (EFI_IMAGE_NT_HEADERS32*)(ImageContext.ImageAddress + ImageContext.PeCoffHeaderOffset);
        switch (PeHdr->OptionalHeader.Subsystem) {
          case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
            if ((Flags & 4) == 0) {
              //
              // RT drivers aren't supposed to be relocated
              //
              continue;
            }

            NewPe32BaseAddress = *RtBase;
            BaseToUpdate = RtBase;
            break;

          default:
            //
            // We treat all other subsystems the same as BS_DRIVER
            //
            if ((Flags & 2) == 0) {
              //
              // Skip all BS_DRIVER's
              //
              continue;
            }

            NewPe32BaseAddress = *BsBase;
            BaseToUpdate = BsBase;
            break;
        }
        break;

      case EFI_FV_FILETYPE_DXE_CORE:
        if ((Flags & 2) == 0) {
          //
          // Skip DXE core
          //
          return EFI_SUCCESS;
        }

        NewPe32BaseAddress = *BsBase;
        BaseToUpdate = BsBase;
        break;

      default:
        //
        // Not supported file type
        //
        return EFI_SUCCESS;
    }

    ImageContext.DestinationAddress = NewPe32BaseAddress;
    Status                          = PeCoffLoaderRelocateImage (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "RelocateImage() call failed on rebase", FileGuidString);
      return Status;
    }

    //
    // Update BASE address
    //
    fprintf (
      LogFile,
      "%s %016I64X\n",
      FileGuidString,
      ImageContext.DestinationAddress
      );
    *BaseToUpdate += EFI_SIZE_TO_PAGES (ImageContext.ImageSize) * EFI_PAGE_SIZE;

    //
    // Now update file checksum
    //
    if (FfsFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      TailSize = sizeof (EFI_FFS_FILE_TAIL);
    } else {
      TailSize = 0;
    }

    if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      SavedState  = FfsFile->State;
      FfsFile->IntegrityCheck.Checksum.File = 0;
      FfsFile->State                        = 0;
      if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
        FfsFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                  (UINT8 *) FfsFile,
                                                  GetLength (FfsFile->Size) - TailSize
                                                  );
      } else {
        FfsFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
      }

      FfsFile->State = SavedState;
    }
    //
    // Update tail if present
    //
    if (FfsFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      TailValue = (EFI_FFS_FILE_TAIL) (~(FfsFile->IntegrityCheck.TailReference));
      *(EFI_FFS_FILE_TAIL *) (((UINTN) FfsFile + GetLength (FfsFile->Size) - sizeof (EFI_FFS_FILE_TAIL))) = TailValue;
    }
  }

  if ((Flags & 1) == 0 || (
      FfsFile->Type != EFI_FV_FILETYPE_SECURITY_CORE &&
      FfsFile->Type != EFI_FV_FILETYPE_PEI_CORE &&

      FfsFile->Type != EFI_FV_FILETYPE_PEIM &&
      FfsFile->Type != EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER
      )) {
    //
    // Only XIP code may have a TE section
    //
    return EFI_SUCCESS;
  }

  //
  // Now process TE sections
  //
  for (Index = 1;; Index++) {
    Status = GetSectionByType (FfsFile, EFI_SECTION_TE, Index, &CurrentPe32Section);
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Calculate the TE base address, the FFS file base plus the offset of the TE section less the size stripped off
    // by GenTEImage
    //
    TEImageHeader = (EFI_TE_IMAGE_HEADER *) ((UINT8 *) CurrentPe32Section.Pe32Section + sizeof (EFI_COMMON_SECTION_HEADER));

    //
    // Initialize context, load image info.
    //
    memset (&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle     = (VOID *) TEImageHeader;
    ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) FfsRebaseImageRead;

    Status                  = PeCoffLoaderGetImageInfo (&ImageContext);

    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "GetImageInfo() call failed on rebase of TE image", FileGuidString);
      return Status;
    }
    //
    // Don't reload TeImage
    //
    ImageContext.ImageAddress = (UINTN) TEImageHeader;

    //
    // Update CodeView and PdbPointer in ImageContext
    //
    DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)(UINTN)(
                   ImageContext.ImageAddress +
                   ImageContext.DebugDirectoryEntryRva +
                   sizeof(EFI_TE_IMAGE_HEADER) -
                   TEImageHeader->StrippedSize
                   );

    ImageContext.CodeView = (VOID *)(UINTN)(
                              ImageContext.ImageAddress +
                              DebugEntry->RVA +
                              sizeof(EFI_TE_IMAGE_HEADER) -
                              TEImageHeader->StrippedSize
                              );

    switch (*(UINT32 *) ImageContext.CodeView) {
    case CODEVIEW_SIGNATURE_NB10:
      ImageContext.PdbPointer = (CHAR8 *) ImageContext.CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
      break;

    case CODEVIEW_SIGNATURE_RSDS:
      ImageContext.PdbPointer = (CHAR8 *) ImageContext.CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
      break;

    default:
      break;
    }

    //
    // Reloacate TeImage
    //
    ImageContext.DestinationAddress = XipBase + (UINTN) TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) \
                                      - TEImageHeader->StrippedSize - (UINTN) FfsFile;
    Status                          = PeCoffLoaderRelocateImage (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0, "RelocateImage() call failed on rebase of TE image", FileGuidString);
      return Status;
    }

    if (FfsFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      TailSize = sizeof (EFI_FFS_FILE_TAIL);
    } else {
      TailSize = 0;
    }
    //
    // Now update file checksum
    //
    if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      SavedState  = FfsFile->State;
      FfsFile->IntegrityCheck.Checksum.File = 0;
      FfsFile->State                        = 0;
      if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
        FfsFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                  (UINT8 *) FfsFile,
                                                  GetLength (FfsFile->Size) - TailSize
                                                  );
      } else {
        FfsFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
      }

      FfsFile->State = SavedState;
    }
    //
    // Update tail if present
    //
    if (FfsFile->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
      TailValue = (EFI_FFS_FILE_TAIL) (~(FfsFile->IntegrityCheck.TailReference));
      *(EFI_FFS_FILE_TAIL *) (((UINTN) FfsFile + GetLength (FfsFile->Size) - sizeof (EFI_FFS_FILE_TAIL))) = TailValue;
    }

    fprintf (
      LogFile,
      "%s %016I64X\n",
      FileGuidString,
      ImageContext.DestinationAddress
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
FfsRebaseImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINT32  *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file

  FileOffset - The offset, in bytes, into the file to read

  ReadSize   - The number of bytes to read from the file starting at FileOffset

  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8   *Destination8;
  CHAR8   *Source8;
  UINT32  Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}
