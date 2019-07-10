/** @file

 Library to parse and generate FV image.

 Copyright (c)  2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FirmwareModuleManagement.h"

#define STR_LEN_MAX_4K 4096
#define STR_LEN_MAX_1K 1024

#define EFI_TEST_FFS_ATTRIBUTES_BIT(FvbAttributes, TestAttributes, Bit) \
    ( \
      (BOOLEAN) ( \
          (FvbAttributes & EFI_FVB2_ERASE_POLARITY) ? (((~TestAttributes) & Bit) == Bit) : ((TestAttributes & Bit) == Bit) \
        ) \
    )

CHAR8      mFirmwareFileSystem2Guid[16] = {0x78, 0xE5, 0x8C, 0x8C, 0x3D, 0x8A, 0x1C, 0x4F, 0x99, 0x35, 0x89, 0x61, 0x85, 0xC3, 0x2D, 0xD3};

CHAR8      mFirmwareFileSystem3Guid[16] = {0x7A, 0xC0, 0x73, 0x54, 0xCB, 0x3D, 0xCA, 0x4D, 0xBD, 0x6F, 0x1E, 0x96, 0x89, 0xE7, 0x34, 0x9A };

EFI_GUID   mEfiCrc32GuidedSectionExtractionProtocolGuid = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;
extern     CHAR8*      mGuidToolDefinition;

static CHAR8 *mSectionTypeName[] = {
  NULL,                                 // 0x00 - reserved
  "EFI_SECTION_COMPRESSION",            // 0x01
  "EFI_SECTION_GUID_DEFINED",           // 0x02
  NULL,                                 // 0x03 - reserved
  NULL,                                 // 0x04 - reserved
  NULL,                                 // 0x05 - reserved
  NULL,                                 // 0x06 - reserved
  NULL,                                 // 0x07 - reserved
  NULL,                                 // 0x08 - reserved
  NULL,                                 // 0x09 - reserved
  NULL,                                 // 0x0A - reserved
  NULL,                                 // 0x0B - reserved
  NULL,                                 // 0x0C - reserved
  NULL,                                 // 0x0D - reserved
  NULL,                                 // 0x0E - reserved
  NULL,                                 // 0x0F - reserved
  "EFI_SECTION_PE32",                   // 0x10
  "EFI_SECTION_PIC",                    // 0x11
  "EFI_SECTION_TE",                     // 0x12
  "EFI_SECTION_DXE_DEPEX",              // 0x13
  "EFI_SECTION_VERSION",                // 0x14
  "EFI_SECTION_USER_INTERFACE",         // 0x15
  "EFI_SECTION_COMPATIBILITY16",        // 0x16
  "EFI_SECTION_FIRMWARE_VOLUME_IMAGE",  // 0x17
  "EFI_SECTION_FREEFORM_SUBTYPE_GUID",  // 0x18
  "EFI_SECTION_RAW",                    // 0x19
  NULL,                                 // 0x1A
  "EFI_SECTION_PEI_DEPEX",              // 0x1B
  "EFI_SECTION_SMM_DEPEX"               // 0x1C
};


static CHAR8 *mFfsFileType[] = {
  NULL,                                   // 0x00
  "EFI_FV_FILETYPE_RAW",                  // 0x01
  "EFI_FV_FILETYPE_FREEFORM",             // 0x02
  "EFI_FV_FILETYPE_SECURITY_CORE",        // 0x03
  "EFI_FV_FILETYPE_PEI_CORE",             // 0x04
  "EFI_FV_FILETYPE_DXE_CORE",             // 0x05
  "EFI_FV_FILETYPE_PEIM",                 // 0x06
  "EFI_FV_FILETYPE_DRIVER",               // 0x07
  "EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER", // 0x08
  "EFI_FV_FILETYPE_APPLICATION",          // 0x09
  "EFI_FV_FILETYPE_SMM",                  // 0x0A
  "EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE",// 0x0B
  "EFI_FV_FILETYPE_COMBINED_SMM_DXE",     // 0x0C
  "EFI_FV_FILETYPE_SMM_CORE"              // 0x0D
 };

static CHAR8 *mGuidSectionAttr[] = {
  "NONE",                                 // 0x00
  "PROCESSING_REQUIRED",                  // 0x01
  "AUTH_STATUS_VALID"                     // 0x02
};

static EFI_GUID mFvUiGuid = {
  0xA67DF1FA, 0x8DE8, 0x4E98, {
    0xAF, 0x09, 0x4B, 0xDF, 0x2E, 0xFF, 0xBC, 0x7C
  }
};


/**
  Generate the unique template filename.
**/
CHAR8 *
GenTempFile (
 VOID
 )
{
  CHAR8   *TemString;
  TemString = NULL;
#ifndef __GNUC__
  TemString = CloneString (tmpnam (NULL));
#else
  CHAR8 tmp[] = "/tmp/fileXXXXXX";
  UINTN Fdtmp;
  Fdtmp = mkstemp(tmp);
  TemString = CloneString(tmp);
  close(Fdtmp);
#endif
  return TemString;
}

static
EFI_STATUS
LibExtractFvUiName(CONST EFI_FIRMWARE_VOLUME_EXT_HEADER *FvExtHeader, CHAR8 **FvUiName)
{
  UINT8 *ExtEnd;
  UINT32 ExtDataSize;
  EFI_FIRMWARE_VOLUME_EXT_ENTRY *ExtEntry;
  EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE *GuidEntry;


  ExtEnd = (UINT8 *)FvExtHeader + FvExtHeader->ExtHeaderSize;
  ExtEntry = (EFI_FIRMWARE_VOLUME_EXT_ENTRY *)(FvExtHeader + 1);
  while ((UINT8 *)ExtEntry < ExtEnd) {
    //
    // GUID type EXT
    //
    if (ExtEntry->ExtEntryType == 0x0002) {
      GuidEntry = (EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE *)ExtEntry;
      if (memcmp(&GuidEntry->FormatType, &mFvUiGuid, sizeof(EFI_GUID)) == 0) {
        ExtDataSize = ExtEntry->ExtEntrySize - (sizeof(EFI_GUID)+sizeof(*ExtEntry));
        *FvUiName = malloc(ExtDataSize + 1);
        if (*FvUiName != NULL) {
          memcpy(*FvUiName, (UINT8 *)GuidEntry + sizeof(EFI_GUID)+sizeof(*ExtEntry), ExtDataSize);
          (*FvUiName)[ExtDataSize] = '\0';
          return EFI_SUCCESS;
        }
      }
    }

    ExtEntry = (EFI_FIRMWARE_VOLUME_EXT_ENTRY *)((UINT8 *)ExtEntry + ExtEntry->ExtEntrySize);
  }
  return EFI_NOT_FOUND;
}

FV_INFORMATION *
LibInitializeFvStruct (
  FV_INFORMATION *Fv
)
{
  UINT32     Index;

  if (Fv == NULL) {
    return NULL;
  }

  for (Index = 0; Index < MAX_NUMBER_OF_FILES_IN_FV; Index ++) {
    memset (Fv->FfsAttuibutes[Index].FfsName, '\0', _MAX_PATH);
    memset (Fv->FfsAttuibutes[Index].UiName, '\0', _MAX_PATH * sizeof (CHAR16));
    memset (&Fv->FfsAttuibutes[Index].GuidName, '\0', sizeof(EFI_GUID));
    Fv->FfsAttuibutes[Index].UiNameSize           = 0;
    Fv->FfsAttuibutes[Index].IsLeaf               = TRUE;
    Fv->FfsAttuibutes[Index].Level                = 0xFF;
    Fv->FfsAttuibutes[Index].TotalSectionNum      = 0;
    Fv->FfsAttuibutes[Index].Depex                = NULL;
    Fv->FfsAttuibutes[Index].DepexLen             = 0;
    Fv->FfsAttuibutes[Index].IsHandle             = FALSE;
    Fv->FfsAttuibutes[Index].IsFvStart            = FALSE;
    Fv->FfsAttuibutes[Index].IsFvEnd              = FALSE;
  }

  Fv->EncapData = NULL;
  Fv->FvNext = NULL;
  Fv->ChildFvFFS = NULL;
  Fv->FvLevel   = 0;
  Fv->MulFvLevel = 1;
  strcpy(Fv->AlignmentStr,"8");
  return Fv;
}


EFI_STATUS
LibFindFvInFd (
  IN     FILE             *InputFile,
  IN OUT FIRMWARE_DEVICE  **FdData
)
{
  FIRMWARE_DEVICE             *LocalFdData;
  UINT16                      Index;
  CHAR8                       Ffs2Guid[16];
  CHAR8                       SignatureCheck[5] = "";
  CHAR8                       Signature[5] = "_FVH";
  FV_INFORMATION              *CurrentFv;
  FV_INFORMATION              *NewFoundFv;
  BOOLEAN                     FirstMatch;
  UINT32                      FdSize;
  UINT16                      FvCount;
  UINT8                       *FdBuffer;
  UINT8                       *FdBufferEnd;
  UINT8                       *FdBufferOri;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;

  CurrentFv      = NULL;
  NewFoundFv     = NULL;
  FdBuffer       = NULL;
  FdBufferOri    = NULL;
  FirstMatch     = TRUE;
  Index          = 0;
  FdSize         = 0;
  FvCount        = 0;
  LocalFdData    = NULL;

  if (InputFile == NULL) {
    Error ("FMMT", 0, 0001, "Error opening the input file", "");
    return EFI_ABORTED;
  }

  //
  // Find each FVs in the FD
  //

  fseek(InputFile,0,SEEK_SET);
  fseek(InputFile,0,SEEK_END);

  FdSize = ftell(InputFile);

  fseek(InputFile,0,SEEK_SET);
  //
  // Create an FD structure to store useful information.
  //
  LocalFdData     = (FIRMWARE_DEVICE *) malloc (sizeof (FIRMWARE_DEVICE));
  if (LocalFdData == NULL) {
    Error ("FMMT", 0, 0002, "Error searching FVs in the input fd", "Allocate memory error");
    return EFI_OUT_OF_RESOURCES;
  }
  LocalFdData->Fv = (FV_INFORMATION *)  malloc (sizeof (FV_INFORMATION));
  if (LocalFdData->Fv == NULL) {
    Error ("FMMT", 0, 0002, "Error searching FVs in the input fd", "Allocate memory error");
    free (LocalFdData);
    return EFI_OUT_OF_RESOURCES;
  }

  LibInitializeFvStruct (LocalFdData->Fv);

  //
  // Readout the FD file data to buffer.
  //
  FdBuffer = malloc (FdSize);

  if (FdBuffer == NULL) {
    Error ("FMMT", 0, 0002, "Error searching FVs in the input fd", "Allocate memory error");
    free (LocalFdData->Fv);
    free (LocalFdData);
    return EFI_OUT_OF_RESOURCES;
  }

  if (fread (FdBuffer, 1, FdSize, InputFile) != FdSize) {
    Error ("FMMT", 0, 0002, "Error searching FVs in the input fd", "Read FD file error!");
    free (LocalFdData->Fv);
    free (LocalFdData);
    free (FdBuffer);
    return EFI_ABORTED;
  }

  FdBufferOri = FdBuffer;
  FdBufferEnd = FdBuffer + FdSize;

  while (FdBuffer <= FdBufferEnd - sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) FdBuffer;
    //
    // Copy 4 bytes of fd data to check the _FVH signature
    //
    memcpy (SignatureCheck, &FvHeader->Signature, 4);

    if (strncmp(SignatureCheck, Signature, 4) == 0){
      //
      // Still need to determine the FileSystemGuid in EFI_FIRMWARE_VOLUME_HEADER equal to
      // EFI_FIRMWARE_FILE_SYSTEM2_GUID or EFI_FIRMWARE_FILE_SYSTEM3_GUID.
      // Turn back 28 bytes to find the GUID.
      //
      memcpy (Ffs2Guid, &FvHeader->FileSystemGuid, 16);

      //
      // Compare GUID.
      //
      for (Index = 0; Index < 16; Index ++) {
        if (Ffs2Guid[Index] != mFirmwareFileSystem2Guid[Index]) {
          break;
        }
      }
    if (Index != 16) {
      for (Index = 0; Index < 16; Index ++) {
          if (Ffs2Guid[Index] != mFirmwareFileSystem3Guid[Index]) {
            break;
          }
        }
    }

      //
      // Here we found an FV.
      //
      if ((Index == 16) && ((FdBuffer + FvHeader->FvLength) <= FdBufferEnd)) {
        if (FirstMatch) {
          LocalFdData->Fv->ImageAddress = (UINTN)((UINT8 *)FdBuffer - (UINT8 *)FdBufferOri);
          CurrentFv                     = LocalFdData->Fv;
          CurrentFv->FvNext             = NULL;
          //
          // Store the FV name by found sequence
          //
          sprintf(CurrentFv->FvName, "FV%d", FvCount);

          FirstMatch = FALSE;
          } else {
            NewFoundFv = (FV_INFORMATION *) malloc (sizeof (FV_INFORMATION));
            if (NewFoundFv == NULL) {
              Error ("FMMT", 0, 0002, "Error searching FVs in the input fd", "Allocate memory error");
              free (LocalFdData->Fv);
              free (LocalFdData);
              free (FdBuffer);
              return EFI_OUT_OF_RESOURCES;
            }

            LibInitializeFvStruct (NewFoundFv);

            //
            // Need to turn back 0x2c bytes
            //
            NewFoundFv->ImageAddress = (UINTN)((UINT8 *)FdBuffer - (UINT8 *)FdBufferOri);

            //
            // Store the FV name by found sequence
            //
            sprintf(NewFoundFv->FvName, "FV%d", FvCount);

            //
            // Value it to NULL for found FV usage.
            //
            NewFoundFv->FvNext       = NULL;
            CurrentFv->FvNext        = NewFoundFv;

            //
            // Make the CurrentFv point to next FV.
            //
            CurrentFv                = CurrentFv->FvNext;
          }

        FvCount ++;
        FdBuffer = FdBuffer + FvHeader->FvLength;
      } else {
        FdBuffer ++;
      }

    } else {
      FdBuffer ++;
    }
  }

  LocalFdData->Size = FdSize;

  *FdData = LocalFdData;

  free (FdBufferOri);

  return EFI_SUCCESS;
}

UINTN
GetFreeOffset (
  IN   VOID   *InputFv
)
{
  UINTN FreeOffset;
  UINTN Offset;
  EFI_STATUS Status;
  EFI_FFS_FILE_HEADER2 *CurrentFile;

  Offset = 0;
  CurrentFile = NULL;
  FreeOffset = 0;
  do {
    FreeOffset = (UINTN)ALIGN_POINTER(Offset, 8);
    Status = FvBufFindNextFile(InputFv, &Offset, (VOID **)&CurrentFile);
    if (Status == EFI_NOT_FOUND) {
      CurrentFile = NULL;
      break;
    }
    else if (EFI_ERROR(Status)) {
      return Status;
    }
  } while (CurrentFile != NULL);

  return FreeOffset;
}

/*
  Construct a set of blank chars based on the number.

  @param[in]   Count The number of blank chars.

  @return      A string contained the blank chars.

*/
CHAR8 *
LibConstructBlankChar (
  IN UINT8    Count
)
{
  CHAR8    *RetStr;
  UINT8    Index;

  Index  = 0;
  RetStr = NULL;

  RetStr = (CHAR8 *) malloc (Count +1);

  if (RetStr == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return NULL;
  }

  memset (RetStr , '\0', Count + 1);

  for (Index=0; Index <= Count -1; Index ++) {
    RetStr[Index] = ' ';
  }

  return RetStr;

}

/**

  This function determines the size of the FV and the erase polarity.  The
  erase polarity is the FALSE value for file state.


  @param[in ]   InputFile       The file that contains the FV image.
  @param[out]   FvSize          The size of the FV.
  @param[out]   ErasePolarity   The FV erase polarity.

  @return EFI_SUCCESS             Function completed successfully.
  @return EFI_INVALID_PARAMETER   A required parameter was NULL or is out of range.
  @return EFI_ABORTED             The function encountered an error.

**/
EFI_STATUS
LibReadFvHeader (
  IN   VOID                       *InputFv,
  IN   BOOLEAN                    ViewFlag,
  IN   UINT8                      FvLevel,
  IN   UINT8                      FvCount,
  IN   CHAR8                      *FvName
  )
{
  EFI_FIRMWARE_VOLUME_HEADER     *VolumeHeader;
  CHAR8                          *BlankSpace;
  CHAR8                          *FvUiName;
  EFI_FIRMWARE_VOLUME_EXT_HEADER *FvExtHeader;

  BlankSpace = NULL;
  FvUiName = NULL;

  //
  // Check input parameters
  //
  if (InputFv == NULL) {
    Error (__FILE__, __LINE__, 0, "FMMT application error", "invalid parameter to function");
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the header
  //
  VolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *) InputFv;


  BlankSpace = LibConstructBlankChar((FvLevel)*2);

  if (BlankSpace == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return EFI_OUT_OF_RESOURCES;
  }


  if (ViewFlag) {
    if ((FvLevel -1) == 0) {
      printf ("\n%s :\n", FvName);
    } else {
      printf ("%sChild FV named FV%d of %s\n", BlankSpace, FvCount, FvName);
    }
  }

  //
  // Print FV header information
  //
  if (ViewFlag) {
    printf ("\n%sAttributes:            %X\n", BlankSpace, (unsigned) VolumeHeader->Attributes);
    printf ("%sTotal Volume Size:     0x%08X\n", BlankSpace, (unsigned) VolumeHeader->FvLength);
    printf ("%sFree Volume Size:      0x%08X\n", BlankSpace, (unsigned) (VolumeHeader->FvLength - GetFreeOffset(InputFv)));
  }

  if (ViewFlag && VolumeHeader->ExtHeaderOffset != 0) {
    FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)((UINT8 *)VolumeHeader + VolumeHeader->ExtHeaderOffset);
    printf("%sFvNameGuid:            %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
           BlankSpace,
           FvExtHeader->FvName.Data1,
           FvExtHeader->FvName.Data2,
           FvExtHeader->FvName.Data3,
           FvExtHeader->FvName.Data4[0],
           FvExtHeader->FvName.Data4[1],
           FvExtHeader->FvName.Data4[2],
           FvExtHeader->FvName.Data4[3],
           FvExtHeader->FvName.Data4[4],
           FvExtHeader->FvName.Data4[5],
           FvExtHeader->FvName.Data4[6],
           FvExtHeader->FvName.Data4[7]);
    LibExtractFvUiName(FvExtHeader, &FvUiName);
    if (FvUiName != NULL && FvLevel == 1) {
      printf("%sFV UI Name:            %s\n\n", BlankSpace, FvUiName);
    }
    free(FvUiName);
  }
  free (BlankSpace);
  return EFI_SUCCESS;
}

/*
  Get size info from FV file.

  @param[in]
  @param[out]

  @retval

*/
EFI_STATUS
LibGetFvSize (
  IN   FILE                       *InputFile,
  OUT  UINT32                     *FvSize
  )
{

  UINTN                          BytesRead;
  UINT32                         Size;
  EFI_FV_BLOCK_MAP_ENTRY         BlockMap;

  BytesRead = 0;
  Size      = 0;

  if (InputFile == NULL || FvSize == NULL) {
    Error (__FILE__, __LINE__, 0, "FMMT application error", "invalid parameter to function");
    return EFI_INVALID_PARAMETER;
  }

  fseek (InputFile, sizeof (EFI_FIRMWARE_VOLUME_HEADER) - sizeof (EFI_FV_BLOCK_MAP_ENTRY), SEEK_CUR);
  do {
    fread (&BlockMap, sizeof (EFI_FV_BLOCK_MAP_ENTRY), 1, InputFile);
    BytesRead += sizeof (EFI_FV_BLOCK_MAP_ENTRY);

    if (BlockMap.NumBlocks != 0) {
      Size += BlockMap.NumBlocks * BlockMap.Length;
    }
  } while (!(BlockMap.NumBlocks == 0 && BlockMap.Length == 0));


  *FvSize = Size;

  return EFI_SUCCESS;
}

/**

  Clears out all files from the Fv buffer in memory

  @param[in]    Fv - Address of the Fv in memory

  @return       EFI_STATUS

**/
EFI_STATUS
FvBufGetSize (
  IN  VOID   *Fv,
  OUT UINTN  *Size
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr;
  EFI_FV_BLOCK_MAP_ENTRY     *blk;

  *Size = 0;
  hdr   = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;
  blk   = hdr->BlockMap;

  while (blk->Length != 0 || blk->NumBlocks != 0) {
    *Size = *Size + (blk->Length * blk->NumBlocks);
    if (*Size >= 0x40000000) {
    //
      // If size is greater than 1GB, then assume it is corrupted
      //
      return EFI_VOLUME_CORRUPTED;
    }
    blk++;
  }

  if (*Size == 0) {
    //
    // If size is 0, then assume the volume is corrupted
    //
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}

/*
  Generate the leaf FFS files.

*/
EFI_STATUS
LibGenFfsFile (
  EFI_FFS_FILE_HEADER2   *CurrentFile,
  FV_INFORMATION         *CurrentFv,
  CHAR8                  *FvName,
  UINT8                  Level,
  UINT32                 *FfsCount,
  BOOLEAN                ErasePolarity
)
{
  UINT32                      FfsFileSize;
  CHAR8                       *FfsFileName;
  FILE                        *FfsFile;
  CHAR8                       *TempDir;


  FfsFileSize   = 0;
  FfsFileName   = NULL;
  FfsFile       = NULL;
  TempDir       = NULL;

  TempDir = getcwd (NULL, _MAX_PATH);
  if (strlen (TempDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    Error("FMMT", 0, 1001,  "The directory is too long.", "");
    return EFI_ABORTED;
  }
  strncat (TempDir, OS_SEP_STR, _MAX_PATH - strlen(TempDir) - 1);
  strncat (TempDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TempDir) - 1);

  mkdir(TempDir, S_IRWXU | S_IRWXG | S_IRWXO);

  FfsFileName = (CHAR8 *) malloc (_MAX_PATH);
  if (FfsFileName == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return EFI_ABORTED;
  }
  memset (FfsFileName, '\0', _MAX_PATH);
  FfsFileSize = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);
  sprintf (
    (CHAR8 *)FfsFileName,
    "%s%cNum%d-%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X-Level%d",
    TempDir,
    OS_SEP,
    *FfsCount,
    (unsigned) CurrentFile->Name.Data1,
    CurrentFile->Name.Data2,
    CurrentFile->Name.Data3,
    CurrentFile->Name.Data4[0],
    CurrentFile->Name.Data4[1],
    CurrentFile->Name.Data4[2],
    CurrentFile->Name.Data4[3],
    CurrentFile->Name.Data4[4],
    CurrentFile->Name.Data4[5],
    CurrentFile->Name.Data4[6],
    CurrentFile->Name.Data4[7],
    Level
  );

  memcpy (CurrentFv->FfsAttuibutes[*FfsCount].FfsName, FfsFileName, strlen(FfsFileName));
  memcpy (&CurrentFv->FfsAttuibutes[*FfsCount].GuidName, &CurrentFile->Name, sizeof(EFI_GUID));
  //
  // Update current FFS files file state.
  //
  if (ErasePolarity) {
    CurrentFile->State = (UINT8)~(CurrentFile->State);
  }

  FfsFile = fopen (FfsFileName, "wb+");
  if (FfsFile == NULL) {
    Error ("FMMT", 0, 0003, "error writing FFS file", "cannot Create a new ffs file.");
    free(FfsFileName);
    return EFI_ABORTED;
  }

  if (fwrite (CurrentFile, 1, FfsFileSize, FfsFile) != FfsFileSize) {
     Error ("FMMT", 0, 0004, "error writing FFS file", "cannot Create a new ffs file.");
     fclose(FfsFile);
     free(FfsFileName);
     return EFI_ABORTED;
  }

  fclose(FfsFile);
  free(FfsFileName);
  FfsFileName = NULL;

  CurrentFv->FfsNumbers  = *FfsCount;

  *FfsCount += 1;

  if (ErasePolarity) {
    CurrentFile->State = (UINT8)~(CurrentFile->State);
  }

  return EFI_SUCCESS;
}

VOID
Unicode2AsciiString (
  IN  CHAR16 *Source,
  OUT CHAR8  *Destination
  )
  /*++

  Routine Description:

  Convert a null-terminated unicode string to a null-terminated ascii string.

  Arguments:

    Source      - The pointer to the null-terminated input unicode string.
    Destination - The pointer to the null-terminated output ascii string.

  Returns:

    N/A

  --*/
{
  while (*Source != '\0') {
    *(Destination++) = (CHAR8) *(Source++);
  }
  //
  // End the ascii with a NULL.
  //
  *Destination = '\0';
}


/**

  Parses EFI Sections, if the view flag turn on, then will collect FFS section information
  and extract FFS files.

  @param[in]      SectionBuffer - Buffer containing the section to parse.
  @param[in]      BufferLength  - Length of SectionBuffer
  @param[in, out] CurrentFv
  @param[in]      FvName
  @param[in]      CurrentFile
  @param[in]      Level
  @param[in, out] FfsCount
  @param[in]      ViewFlag
  @param[in]      ErasePolarity

  @retval       EFI_SECTION_ERROR - Problem with section parsing.
                      (a) compression errors
                      (b) unrecognized section
  @retval       EFI_UNSUPPORTED - Do not know how to parse the section.
  @retval       EFI_SUCCESS - Section successfully parsed.
  @retval       EFI_OUT_OF_RESOURCES - Memory allocation failed.

--*/
EFI_STATUS
LibParseSection (
  UINT8                  *SectionBuffer,
  UINT32                 BufferLength,
  FV_INFORMATION         *CurrentFv,
  CHAR8                  *FvName,
  EFI_FFS_FILE_HEADER2   *CurrentFile,
  UINT8                  Level,
  ENCAP_INFO_DATA        **CurrentFvEncapData,
  UINT8                  FfsLevel,
  UINT32                 *FfsCount,
  UINT8                  *FvCount,
  BOOLEAN                ViewFlag,
  BOOLEAN                ErasePolarity,
  BOOLEAN                *IsFfsGenerated
  )
{
  UINT32              ParsedLength;
  UINT8               *Ptr;
  UINT32              SectionLength;
  UINT32              UiSectionLength;
  EFI_SECTION_TYPE    Type;
  EFI_STATUS          Status;
  CHAR8               *ExtractionTool;
  CHAR8               *ToolInputFile;
  CHAR8               *ToolOutputFile;
  CHAR8               *SystemCommandFormatString;
  CHAR8               *SystemCommand;
  UINT8               *ToolOutputBuffer;
  UINT32              ToolOutputLength;
  CHAR16              *UIName;
  UINT32              UINameSize;
  BOOLEAN             HasDepexSection;
  UINT32              NumberOfSections;
  ENCAP_INFO_DATA     *LocalEncapData;
  ENCAP_INFO_DATA     *LocalEncapDataTemp;
  CHAR8               *BlankChar;
  UINT8               *UncompressedBuffer;
  UINT32              UncompressedLength;
  UINT8               *CompressedBuffer;
  UINT32              CompressedLength;
  UINT8               CompressionType;
  DECOMPRESS_FUNCTION DecompressFunction;
  GETINFO_FUNCTION    GetInfoFunction;
  UINT32              DstSize;
  UINT32              ScratchSize;
  UINT8               *ScratchBuffer;
  BOOLEAN             EncapDataNeedUpdata;
  CHAR8               *TempDir;
  CHAR8               *ToolInputFileFullName;
  CHAR8               *ToolOutputFileFullName;
  UINT8               LargeHeaderOffset;
  UINT16              GuidAttr;
  UINT16              DataOffset;
  CHAR8               *UIFileName;
  CHAR8               *ToolInputFileName;
  CHAR8               *ToolOutputFileName;

  DataOffset                 = 0;
  GuidAttr                   = 0;
  ParsedLength               = 0;
  ToolOutputLength           = 0;
  UINameSize                 = 0;
  NumberOfSections           = 0;
  UncompressedLength         = 0;
  CompressedLength           = 0;
  CompressionType            = 0;
  DstSize                    = 0;
  ScratchSize                = 0;
  Ptr                        = NULL;
  ExtractionTool             = NULL;
  ToolInputFile              = NULL;
  ToolOutputFile             = NULL;
  SystemCommand              = NULL;
  SystemCommandFormatString  = NULL;
  ToolOutputBuffer           = NULL;
  UIName                     = NULL;
  LocalEncapData             = NULL;
  LocalEncapDataTemp         = NULL;
  BlankChar                  = NULL;
  UncompressedBuffer         = NULL;
  CompressedBuffer           = NULL;
  ScratchBuffer              = NULL;
  TempDir                    = NULL;
  ToolInputFileFullName      = NULL;
  ToolOutputFileFullName     = NULL;
  ToolInputFileName          = NULL;
  ToolOutputFileFullName     = NULL;
  HasDepexSection            = FALSE;
  EncapDataNeedUpdata        = TRUE;
  LargeHeaderOffset          = 0;


  while (ParsedLength < BufferLength) {
    Ptr           = SectionBuffer + ParsedLength;

    SectionLength = GetLength (((EFI_COMMON_SECTION_HEADER *) Ptr)->Size);
    Type          = ((EFI_COMMON_SECTION_HEADER *) Ptr)->Type;

    //
    // This is sort of an odd check, but is necessary because FFS files are
    // padded to a QWORD boundary, meaning there is potentially a whole section
    // header worth of 0xFF bytes.
    //
    if (SectionLength == 0xffffff && Type == 0xff) {
      ParsedLength += 4;
      continue;
    }
  //
  //If Size is 0xFFFFFF then ExtendedSize contains the size of the section.
  //
    if (SectionLength == 0xffffff) {
    SectionLength     = ((EFI_COMMON_SECTION_HEADER2 *) Ptr)->ExtendedSize;
    LargeHeaderOffset = sizeof (EFI_COMMON_SECTION_HEADER2) - sizeof (EFI_COMMON_SECTION_HEADER);
  }

    switch (Type) {

    case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:

      EncapDataNeedUpdata = TRUE;

      Level ++;
      NumberOfSections ++;

      CurrentFv->FfsAttuibutes[*FfsCount].IsLeaf = FALSE;
      CurrentFv->FfsAttuibutes[*FfsCount].IsFvStart = TRUE;
      //
      // Put in encapsulate data information.
      //
      LocalEncapData = *CurrentFvEncapData;
      if (LocalEncapData->NextNode != NULL) {
        LocalEncapData = LocalEncapData->NextNode;
        while (LocalEncapData->RightNode != NULL) {
          LocalEncapData = LocalEncapData->RightNode;
        }
      }

      if (EncapDataNeedUpdata) {
        //
        // Put in this is an FFS with FV section
        //

        //
        // Construct the new ENCAP_DATA
        //
        LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

        if (LocalEncapData->NextNode == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }

        LocalEncapData        = LocalEncapData->NextNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = FMMT_ENCAP_TREE_FV_SECTION;

        //
        // We don't need additional data for encapsulate this FFS but type.
        //
        LocalEncapData->Data        = NULL;
        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode    = NULL;
    LocalEncapData->RightNode = NULL;
        LocalEncapData->Depex = NULL;
        LocalEncapData->DepexLen = 0;
        LocalEncapData->UiNameSize = 0;
        LocalEncapData->FvId  = *FvCount;
      }

     //
     //save parent level FFS file's GUID name
     //
     LocalEncapDataTemp = CurrentFv->EncapData;
     while (LocalEncapDataTemp->NextNode != NULL) {
         if (LocalEncapDataTemp->Level == FfsLevel) {
           while (LocalEncapDataTemp->RightNode != NULL) {
             LocalEncapDataTemp = LocalEncapDataTemp->RightNode;
           }
             if (LocalEncapDataTemp != NULL && LocalEncapDataTemp->FvExtHeader == NULL) {
                 LocalEncapDataTemp->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)malloc(sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER));
                 if (LocalEncapDataTemp->FvExtHeader == NULL) {
                     Error(NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
                     return EFI_ABORTED;
                 }

                 if (*FfsCount >= 1) {
                    if ((memcmp(&CurrentFv->FfsAttuibutes[*FfsCount - 1].GuidName, &(LocalEncapDataTemp->FvExtHeader->FvName), sizeof(EFI_GUID)) == 0)) {
                        memcpy(LocalEncapDataTemp->UiName, CurrentFv->FfsAttuibutes[*FfsCount - 1].UiName, _MAX_PATH);
                        LocalEncapDataTemp->UiNameSize = CurrentFv->FfsAttuibutes[*FfsCount - 1].UiNameSize;
                        LocalEncapDataTemp->DepexLen = CurrentFv->FfsAttuibutes[*FfsCount - 1].DepexLen;
                        LocalEncapDataTemp->Depex = malloc (LocalEncapDataTemp->DepexLen);
                        if (LocalEncapDataTemp->Depex == NULL) {
                            Error(NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
                            return EFI_ABORTED;
                        }
                        memcpy(LocalEncapDataTemp->Depex, CurrentFv->FfsAttuibutes[*FfsCount - 1].Depex, LocalEncapDataTemp->DepexLen);
                    }
                 }
             }
             break;
         }
         LocalEncapDataTemp = LocalEncapDataTemp->NextNode;
     }

      Status = LibGetFvInfo ((UINT8*)((EFI_FIRMWARE_VOLUME_IMAGE_SECTION*)Ptr + 1) + LargeHeaderOffset, CurrentFv, FvName, Level, &LocalEncapData, FfsCount, FvCount, ViewFlag, TRUE);
      if (EFI_ERROR (Status)) {
        Error ("FMMT", 0, 0003, "printing of FV section contents failed", NULL);
        return EFI_SECTION_ERROR;
      }
      if (*FfsCount >= 1) {
        CurrentFv->FfsAttuibutes[*FfsCount -1].IsFvEnd = TRUE;
      }
      break;

    case EFI_SECTION_COMPRESSION:
      Level ++;
      NumberOfSections ++;

      EncapDataNeedUpdata = TRUE;
      //
      // Put in encapsulate data information.
      //
      LocalEncapData = *CurrentFvEncapData;
      if (LocalEncapData->NextNode != NULL) {
        EncapDataNeedUpdata = FALSE;
        while (LocalEncapData->RightNode != NULL) {
          LocalEncapData = LocalEncapData->RightNode;
        }
      }

      if (EncapDataNeedUpdata) {
        //
        // Put in this is an FFS with FV section
        //

        //
        // Construct the new ENCAP_DATA
        //
        LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

        if (LocalEncapData->NextNode == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
          }

        LocalEncapData        = LocalEncapData->NextNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = FMMT_ENCAP_TREE_COMPRESS_SECTION;

        //
        // Store the compress type
        //
        LocalEncapData->Data     = malloc (sizeof (UINT8));

        if (LocalEncapData->Data == NULL) {
          Error ("FMMT", 0, 0003, "Allocate memory failed", NULL);
          return EFI_OUT_OF_RESOURCES;
        }

        *(UINT8 *)LocalEncapData->Data     = ((EFI_COMPRESSION_SECTION *) (Ptr + LargeHeaderOffset))->CompressionType;
        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode = NULL;
    LocalEncapData->RightNode = NULL;
      } else {
        LocalEncapData->RightNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));
        if (LocalEncapData->RightNode == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }
        LocalEncapData        = LocalEncapData->RightNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = FMMT_ENCAP_TREE_COMPRESS_SECTION;

        //
        // Store the compress type
        //
        LocalEncapData->Data     = malloc (sizeof (UINT8));

        if (LocalEncapData->Data == NULL) {
          Error ("FMMT", 0, 0003, "Allocate memory failed", NULL);
          return EFI_OUT_OF_RESOURCES;
        }

        *(UINT8 *)LocalEncapData->Data     = ((EFI_COMPRESSION_SECTION *) (Ptr + LargeHeaderOffset))->CompressionType;
        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode = NULL;
        LocalEncapData->RightNode = NULL;

      }

      //
      // Process compressed section
      //
      CurrentFv->FfsAttuibutes[*FfsCount].IsLeaf = FALSE;

      UncompressedBuffer  = NULL;
      CompressedLength    = SectionLength - sizeof (EFI_COMPRESSION_SECTION) - LargeHeaderOffset;
      UncompressedLength  = ((EFI_COMPRESSION_SECTION *) (Ptr + LargeHeaderOffset))->UncompressedLength;
      CompressionType     = ((EFI_COMPRESSION_SECTION *) (Ptr + LargeHeaderOffset))->CompressionType;

      if (CompressionType == EFI_NOT_COMPRESSED) {
        //printf ("  Compression Type:  EFI_NOT_COMPRESSED\n");
        if (CompressedLength != UncompressedLength) {
          Error ("FMMT", 0, 0, "file is not compressed, but the compressed length does not match the uncompressed length", NULL);
          return EFI_SECTION_ERROR;
        }

        UncompressedBuffer = Ptr + sizeof (EFI_COMPRESSION_SECTION) + LargeHeaderOffset;
      } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
        GetInfoFunction     = EfiGetInfo;
        DecompressFunction  = EfiDecompress;

        CompressedBuffer  = Ptr + sizeof (EFI_COMPRESSION_SECTION) + LargeHeaderOffset;

        Status            = GetInfoFunction (CompressedBuffer, CompressedLength, &DstSize, &ScratchSize);
        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0003, "error getting compression info from compression section", NULL);
          return EFI_SECTION_ERROR;
        }

        if (DstSize != UncompressedLength) {
          Error ("FMMT", 0, 0003, "compression error in the compression section", NULL);
          return EFI_SECTION_ERROR;
        }

        ScratchBuffer       = malloc (ScratchSize);
        if (ScratchBuffer == NULL) {
          Error ("FMMT", 0, 0003, "Allocate memory failed", NULL);
          return EFI_OUT_OF_RESOURCES;
        }
        UncompressedBuffer  = malloc (UncompressedLength);
        if (UncompressedBuffer == NULL) {
          Error ("FMMT", 0, 0003, "Allocate memory failed", NULL);
          free (ScratchBuffer);
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // Decompress the section.
        //
        Status = DecompressFunction (
                  CompressedBuffer,
                  CompressedLength,
                  UncompressedBuffer,
                  UncompressedLength,
                  ScratchBuffer,
                  ScratchSize
                  );
        free (ScratchBuffer);
        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0003, "decompress failed", NULL);
          free (UncompressedBuffer);
          return EFI_SECTION_ERROR;
        }
      } else {
        Error ("FMMT", 0, 0003, "unrecognized compression type", "type 0x%X", CompressionType);
        return EFI_SECTION_ERROR;
      }

      Status = LibParseSection (  UncompressedBuffer,
                                  UncompressedLength,
                                  CurrentFv,
                                  FvName,
                                  CurrentFile,
                                  Level,
                                  &LocalEncapData,
                                  FfsLevel,
                                  FfsCount,
                                  FvCount,
                                  ViewFlag,
                                  ErasePolarity,
                                  IsFfsGenerated);

      if (CompressionType == EFI_STANDARD_COMPRESSION) {
        //
        // We need to deallocate Buffer
        //
        free (UncompressedBuffer);
      }

      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 0003, "failed to parse section", NULL);
        return EFI_SECTION_ERROR;
      }

      break;

    case EFI_SECTION_GUID_DEFINED:
      //
      // Process GUID defined
      // looks up the appropriate tool to use for extracting
      // a GUID defined FV section.
      //
      Level ++;
      NumberOfSections++;
      EncapDataNeedUpdata = TRUE;
      //
      // Put in encapsulate data information.
      //
      LocalEncapData = *CurrentFvEncapData;
      if (LocalEncapData->NextNode != NULL) {
        EncapDataNeedUpdata = FALSE;
        while (LocalEncapData->RightNode != NULL) {
          LocalEncapData = LocalEncapData->RightNode;
        }
      }
      GuidAttr = ((EFI_GUID_DEFINED_SECTION *)(Ptr + LargeHeaderOffset))->Attributes;
      DataOffset = ((EFI_GUID_DEFINED_SECTION *)(Ptr + LargeHeaderOffset))->DataOffset;

      if ((ViewFlag) && ((GuidAttr & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0)) {
        ToolOutputBuffer = Ptr + DataOffset;
        ToolOutputLength = SectionLength - DataOffset;
        Status = LibParseSection(
                ToolOutputBuffer,
                ToolOutputLength,
                CurrentFv,
                FvName,
                CurrentFile,
                Level,
                &LocalEncapData,
                FfsLevel,
                FfsCount,
                FvCount,
                ViewFlag,
                ErasePolarity,
                IsFfsGenerated
                );
        if (EFI_ERROR(Status)) {
          Error(NULL, 0, 0003, "parse of decoded GUIDED section failed", NULL);
          return EFI_SECTION_ERROR;
        }
        break;
      }

      if (EncapDataNeedUpdata)  {

        //
        // Put in this is an FFS with FV section
        //

        //
        // Construct the new ENCAP_DATA
        //
        LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

        if (LocalEncapData->NextNode == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }

        LocalEncapData        = LocalEncapData->NextNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = FMMT_ENCAP_TREE_GUIDED_SECTION;
        LocalEncapData->Depex = NULL;
        LocalEncapData->DepexLen = 0;
        LocalEncapData->UiNameSize = 0;
        //
        // We don't need additional data for encapsulate this FFS but type.
        // include DataOffset + Attributes
        //

        LocalEncapData->Data     = (EFI_GUID *) malloc (sizeof (EFI_GUID) + 4);

        if (LocalEncapData->Data == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }

        //
        // include guid attribute and dataoffset
        //
        memcpy (LocalEncapData->Data, Ptr + LargeHeaderOffset + OFFSET_OF (EFI_GUID_DEFINED_SECTION, SectionDefinitionGuid), sizeof (EFI_GUID) + 4);

        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode = NULL;
    LocalEncapData->RightNode = NULL;
      } else {
        LocalEncapData->RightNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));
        if (LocalEncapData->RightNode == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
          }
        LocalEncapData        = LocalEncapData->RightNode;
        LocalEncapData->Level = Level;
        LocalEncapData->Type  = FMMT_ENCAP_TREE_GUIDED_SECTION;
        LocalEncapData->Depex = NULL;
        LocalEncapData->DepexLen = 0;
        LocalEncapData->UiNameSize = 0;
        //
        // We don't need additional data for encapsulate this FFS but type.
        // include DataOffset + Attributes
        //

        LocalEncapData->Data     = (EFI_GUID *) malloc (sizeof (EFI_GUID) + 4);

        if (LocalEncapData->Data == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }

        //
        // include guid attribute and dataoffset
        //
        memcpy (LocalEncapData->Data, Ptr + LargeHeaderOffset + OFFSET_OF (EFI_GUID_DEFINED_SECTION, SectionDefinitionGuid), sizeof (EFI_GUID) + 4);

        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode = NULL;
    LocalEncapData->RightNode = NULL;
      }

      CurrentFv->FfsAttuibutes[*FfsCount].IsLeaf = FALSE;

      ExtractionTool =
        LookupGuidedSectionToolPath (
          mParsedGuidedSectionTools,
          &((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->SectionDefinitionGuid
          );

      if (ExtractionTool != NULL && ((GuidAttr & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) != 0)) {

        TempDir = getcwd (NULL, _MAX_PATH);
        if (strlen (TempDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
          Error("FMMT", 0, 1001, "The directory is too long.", "");
          free (ExtractionTool);
          return EFI_SECTION_ERROR;
        }
        strncat (TempDir, OS_SEP_STR, _MAX_PATH - strlen(TempDir) - 1);
        strncat (TempDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TempDir) - 1);
        mkdir(TempDir, S_IRWXU | S_IRWXG | S_IRWXO);
        ToolInputFile  = GenTempFile ();
        if (ToolInputFile == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          free (ExtractionTool);
          return EFI_OUT_OF_RESOURCES;
        }
        ToolOutputFile = GenTempFile ();
        if (ToolOutputFile == NULL) {
          free (ToolInputFile);
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          free (ExtractionTool);
          return EFI_OUT_OF_RESOURCES;
        }
        ToolInputFileName = strrchr(ToolInputFile, OS_SEP);
        if (ToolInputFileName == NULL) {
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ExtractionTool);
          return EFI_ABORTED;
        }
        ToolOutputFileName = strrchr(ToolOutputFile, OS_SEP);
        if (ToolOutputFileName == NULL) {
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ExtractionTool);
          return EFI_ABORTED;
        }

        ToolInputFileFullName   = malloc (strlen("%s%s") + strlen(TempDir) + strlen(ToolInputFileName) + 1);
        if (ToolInputFileFullName == NULL) {
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ExtractionTool);
          Error ("FMMT", 0, 0003, "Allocate memory failed", NULL);
          return EFI_OUT_OF_RESOURCES;
        }
        ToolOutputFileFullName  = malloc (strlen("%s%s") + strlen(TempDir) + strlen(ToolOutputFileName) + 1);

        if (ToolOutputFileFullName == NULL) {
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ToolInputFileFullName);
          free (ExtractionTool);
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_OUT_OF_RESOURCES;
        }

        sprintf (ToolInputFileFullName, "%s%s", TempDir, ToolInputFileName);
        sprintf (ToolOutputFileFullName, "%s%s", TempDir, ToolOutputFileName);

        //
        // Construction 'system' command string
        //
        SystemCommandFormatString = "%s -d -o \"%s\" \"%s\"";
        SystemCommand = malloc (
          strlen (SystemCommandFormatString) +
          strlen (ExtractionTool) +
          strlen (ToolInputFileFullName) +
          strlen (ToolOutputFileFullName) +
          1
          );
        if (SystemCommand == NULL) {
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ToolInputFileFullName);
          free (ToolOutputFileFullName);
          free (ExtractionTool);
          return EFI_ABORTED;
        }
        sprintf (
          SystemCommand,
          "%s -d -o \"%s\" \"%s\"",
          ExtractionTool,
          ToolOutputFileFullName,
          ToolInputFileFullName
          );
        free (ExtractionTool);
        ExtractionTool = NULL;

        Status = PutFileImage (
        ToolInputFileFullName,
        (CHAR8*) Ptr + ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset,
        SectionLength - ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset
        );

        if (HasDepexSection) {
          HasDepexSection = FALSE;
        }

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "unable to decoded GUIDED section", NULL);
          free (SystemCommand);
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ToolOutputFileFullName);
          remove (ToolInputFileFullName);
          free (ToolInputFileFullName);
          return EFI_SECTION_ERROR;
        }

        if (system (SystemCommand) != EFI_SUCCESS) {
          printf("Command failed: %s\n", SystemCommand);
          free (SystemCommand);
          free (ToolInputFile);
          free (ToolOutputFile);
          free (ToolOutputFileFullName);
          remove (ToolInputFileFullName);
          free (ToolInputFileFullName);
          return EFI_ABORTED;
        }
        free (SystemCommand);
        remove (ToolInputFileFullName);
        free (ToolInputFile);
        free (ToolInputFileFullName);
        ToolInputFile = NULL;
        ToolInputFileFullName = NULL;


        Status = GetFileImage (
                   ToolOutputFileFullName,
                   (CHAR8 **)&ToolOutputBuffer,
                   &ToolOutputLength
                   );
        remove (ToolOutputFileFullName);
        free (ToolOutputFile);
        free (ToolOutputFileFullName);
        ToolOutputFile = NULL;
        ToolOutputFileFullName = NULL;

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "unable to read decoded GUIDED section", NULL);
          return EFI_SECTION_ERROR;
        }

        Status = LibParseSection (
                  ToolOutputBuffer,
                  ToolOutputLength,
                  CurrentFv,
                  FvName,
                  CurrentFile,
                  Level,
                  &LocalEncapData,
                  FfsLevel,
                  FfsCount,
                  FvCount,
                  ViewFlag,
                  ErasePolarity,
                  IsFfsGenerated
                  );
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 0003, "parse of decoded GUIDED section failed", NULL);
          return EFI_SECTION_ERROR;
        }
      } else if ((GuidAttr & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0){
          Status = LibParseSection (
            Ptr + ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset,
            SectionLength - ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset,
            CurrentFv,
            FvName,
            CurrentFile,
            Level,
            &LocalEncapData,
            FfsLevel,
            FfsCount,
            FvCount,
            ViewFlag,
            ErasePolarity,
            IsFfsGenerated
            );
          if (ExtractionTool != NULL) {
            free (ExtractionTool);
            ExtractionTool = NULL;
          }
          if (EFI_ERROR (Status)) {
            Error (NULL, 0, 0003, "parse of decoded GUIDED section failed", NULL);
            return EFI_SECTION_ERROR;
          }
      }else {
        //
        // We don't know how to parse it now.
        //
        if (ExtractionTool != NULL) {
          free (ExtractionTool);
          ExtractionTool = NULL;
        }
        Error ("FMMT", 0, 0003, "Error parsing section", \
                              "EFI_SECTION_GUID_DEFINED cannot be parsed at this time. Tool to decode this section should have been defined in %s file.", mGuidToolDefinition);
        printf("  Its GUID is: ");
        PrintGuid(&(((EFI_GUID_DEFINED_SECTION *)(Ptr + LargeHeaderOffset))->SectionDefinitionGuid));
        return EFI_UNSUPPORTED;
      }
      break;

    //
    //Leaf sections
    //
    case EFI_SECTION_RAW:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!*IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          *IsFfsGenerated = TRUE;
        }
      }

      break;
    case EFI_SECTION_PE32:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!*IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          *IsFfsGenerated = TRUE;
        }
      }

      break;
    case EFI_SECTION_PIC:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!*IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          *IsFfsGenerated = TRUE;
        }
      }

      break;
    case EFI_SECTION_TE:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!*IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          *IsFfsGenerated = TRUE;
        }
      }
      break;

    case EFI_SECTION_COMPATIBILITY16:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!*IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          *IsFfsGenerated = TRUE;
        }
      }
      break;

    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!*IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          *IsFfsGenerated = TRUE;
        }
      }
      break;

    case EFI_SECTION_VERSION:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      break;
    case EFI_SECTION_PEI_DEPEX:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      HasDepexSection = TRUE;
      CurrentFv->FfsAttuibutes[*FfsCount].Depex = malloc (SectionLength);
      memcpy(CurrentFv->FfsAttuibutes[*FfsCount].Depex, Ptr, SectionLength);
      CurrentFv->FfsAttuibutes[*FfsCount].DepexLen = SectionLength;
      break;
    case EFI_SECTION_DXE_DEPEX:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      HasDepexSection = TRUE;
      CurrentFv->FfsAttuibutes[*FfsCount].Depex = malloc (SectionLength);
      memcpy(CurrentFv->FfsAttuibutes[*FfsCount].Depex, Ptr, SectionLength);
      CurrentFv->FfsAttuibutes[*FfsCount].DepexLen = SectionLength;
      break;
    case EFI_SECTION_SMM_DEPEX:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      HasDepexSection = TRUE;
      CurrentFv->FfsAttuibutes[*FfsCount].Depex = malloc (SectionLength);
      memcpy(CurrentFv->FfsAttuibutes[*FfsCount].Depex, Ptr, SectionLength);
      CurrentFv->FfsAttuibutes[*FfsCount].DepexLen = SectionLength;
      break;

    case EFI_SECTION_USER_INTERFACE:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;

      UiSectionLength = GetLength (((EFI_USER_INTERFACE_SECTION *) Ptr)->CommonHeader.Size);
    if (UiSectionLength == 0xffffff) {
      UiSectionLength   = ((EFI_USER_INTERFACE_SECTION2 *) Ptr)->CommonHeader.ExtendedSize;
    UINameSize        = UiSectionLength - sizeof(EFI_COMMON_SECTION_HEADER2);
    } else {
      UINameSize = UiSectionLength - sizeof(EFI_COMMON_SECTION_HEADER);
    }

      UIName     = (CHAR16 *) malloc (UINameSize + 2);
      if (UIName != NULL) {
        memset (UIName, '\0', UINameSize + 2);
        if (UiSectionLength >= 0xffffff) {
          memcpy(UIName, ((EFI_USER_INTERFACE_SECTION2 *) Ptr)->FileNameString, UINameSize);
        } else {
          memcpy(UIName, ((EFI_USER_INTERFACE_SECTION *) Ptr)->FileNameString, UINameSize);
        }
      } else {
        Error ("FMMT", 0, 0001, "Memory allocate error!", NULL);
        return EFI_OUT_OF_RESOURCES;
      }

      BlankChar = LibConstructBlankChar( CurrentFv->FvLevel * 2);
      if (BlankChar == NULL) {
        free(UIName);
        return EFI_OUT_OF_RESOURCES;
      }

      if (ViewFlag) {
        UIFileName = malloc (UINameSize + 2);
        if (UIFileName == NULL) {
          Error ("FMMT", 0, 4001, "Memory allocation fail!", NULL);
          free (UIName);
          free (BlankChar);
          return EFI_OUT_OF_RESOURCES;
        }
        Unicode2AsciiString (UIName, UIFileName);
        fprintf(stdout, "%sFile \"%s\"\n", BlankChar, UIFileName);
        free(UIFileName);
      }
      free (BlankChar);

      //
      // If Ffs file has been generated, then the FfsCount should decrease 1.
      //
      if (*IsFfsGenerated) {
        memcpy (CurrentFv->FfsAttuibutes[*FfsCount -1].UiName, UIName, UINameSize);
        CurrentFv->FfsAttuibutes[*FfsCount -1].UiNameSize = UINameSize;
      } else {
        memcpy (CurrentFv->FfsAttuibutes[*FfsCount].UiName, UIName, UINameSize);
        CurrentFv->FfsAttuibutes[*FfsCount].UiNameSize = UINameSize;
      }

      HasDepexSection = FALSE;
    free(UIName);
    UINameSize = 0;

      break;
    default:
      break;
    }

    ParsedLength += SectionLength;
    //
    // We make then next section begin on a 4-byte boundary
    //
    ParsedLength = GetOccupiedSize (ParsedLength, 4);
  }

  if (ParsedLength < BufferLength) {
    Error ("FMMT", 0, 0003, "sections do not completely fill the sectioned buffer being parsed", NULL);
    return EFI_SECTION_ERROR;
  }


  return EFI_SUCCESS;
}

/**

  Iterates through the files contained within the firmware volume

  @param[in]    Fv  - Address of the Fv in memory
  @param[in]    Key - Should be 0 to get the first file.  After that, it should be
                      passed back in without modifying it's contents to retrieve
                      subsequent files.
  @param[in]    File- Output file pointer
                      File == NULL - invalid parameter
                      otherwise - *File will be update to the location of the file

  @return       EFI_STATUS
                EFI_NOT_FOUND
                EFI_VOLUME_CORRUPTED

**/
EFI_STATUS
FvBufFindNextFile (
  IN     VOID      *Fv,
  IN OUT UINTN     *Key,
  OUT    VOID      **File
  )
{
  EFI_FIRMWARE_VOLUME_HEADER *hdr;
  EFI_FFS_FILE_HEADER        *fhdr;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FwVolExtHeader;
  EFI_FVB_ATTRIBUTES_2       FvbAttributes;
  UINTN                      fsize;
  EFI_STATUS                 Status;
  UINTN                      fvSize;

  hdr = (EFI_FIRMWARE_VOLUME_HEADER*)Fv;
  fhdr = NULL;

  if (Fv == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FvBufGetSize (Fv, &fvSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (*Key == 0) {
    if (hdr->ExtHeaderOffset != 0) {
      //
      // Searching for files starts on an 8 byte aligned boundary after the end of the Extended Header if it exists.
      //
      FwVolExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)((UINT8 *)hdr + hdr->ExtHeaderOffset);
      *Key = (UINTN)hdr->ExtHeaderOffset + FwVolExtHeader->ExtHeaderSize;
      *Key = (UINTN)ALIGN_POINTER(*Key, 8);
    } else {
      *Key = hdr->HeaderLength;
    }
  }

  FvbAttributes = hdr->Attributes;

  for(
      *Key = (UINTN)ALIGN_POINTER (*Key, 8);
      (*Key + sizeof (*fhdr)) < fvSize;
      *Key = (UINTN)ALIGN_POINTER (*Key, 8)
    ) {
    fhdr = (EFI_FFS_FILE_HEADER*) ((UINT8*)hdr + *Key);
    fsize = GetFfsFileLength (fhdr);
    if (!EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_HEADER_VALID
        ) ||
        EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_HEADER_INVALID
        )
      ) {
      *Key = *Key + 1;
      continue;
    } else if(
        EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_MARKED_FOR_UPDATE
        ) ||
        EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_DELETED
        )
      ) {
      *Key = *Key + fsize;
      continue;
    } else if (EFI_TEST_FFS_ATTRIBUTES_BIT(
          FvbAttributes,
          fhdr->State,
          EFI_FILE_DATA_VALID
        )
      ) {
      *File = (UINT8*)hdr + *Key;
      *Key = *Key + fsize;
      return EFI_SUCCESS;
    }

    *Key = *Key + 1;
  }

  return EFI_NOT_FOUND;
}

/**

  TODO: Add function description

  FvImage       - TODO: add argument description
  FileHeader    - TODO: add argument description
  ErasePolarity - TODO: add argument description

  EFI_SUCCESS - TODO: Add description for return value
  EFI_ABORTED - TODO: Add description for return value

**/
EFI_STATUS
LibGetFileInfo (
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage,
  EFI_FFS_FILE_HEADER2        *CurrentFile,
  BOOLEAN                     ErasePolarity,
  FV_INFORMATION              *CurrentFv,
  CHAR8                       *FvName,
  UINT8                       Level,
  ENCAP_INFO_DATA             **CurrentFvEncapData,
  UINT32                      *FfsCount,
  UINT8                       *FvCount,
  BOOLEAN                     ViewFlag
  )
{
  UINT32              FileLength;
  UINT8               FileState;
  UINT8               Checksum;
  EFI_FFS_FILE_HEADER2 BlankHeader;
  EFI_STATUS          Status;
  UINT8               GuidBuffer[PRINTED_GUID_BUFFER_SIZE];
  ENCAP_INFO_DATA     *LocalEncapData;
  BOOLEAN             EncapDataNeedUpdateFlag;
  BOOLEAN             IsGeneratedFfs;
  UINT32              FfsFileHeaderSize;

  Status = EFI_SUCCESS;

  LocalEncapData  = NULL;
  EncapDataNeedUpdateFlag = TRUE;
  IsGeneratedFfs   = FALSE;

  FfsFileHeaderSize = GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) CurrentFile);
  FileLength        = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);

  //
  // Check if we have free space
  //
  if (ErasePolarity) {
    memset (&BlankHeader, -1, FfsFileHeaderSize);
  } else {
    memset (&BlankHeader, 0, FfsFileHeaderSize);
  }

  //
  // Is this FV blank?
  //
  if (memcmp (&BlankHeader, CurrentFile, FfsFileHeaderSize) == 0) {
    return EFI_SUCCESS;
  }

  //
  // Print file information.
  //
  FileState = GetFileState (ErasePolarity, (EFI_FFS_FILE_HEADER *)CurrentFile);
  PrintGuidToBuffer (&(CurrentFile->Name), GuidBuffer, PRINTED_GUID_BUFFER_SIZE, FALSE);
  if (FileState == EFI_FILE_DATA_VALID) {
    //
    // Calculate header checksum
    //
    Checksum  = CalculateSum8 ((UINT8 *) CurrentFile, FfsFileHeaderSize);
    Checksum  = (UINT8) (Checksum - CurrentFile->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - CurrentFile->State);
    if (Checksum != 0) {
      Error ("FMMT", 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid header checksum", GuidBuffer);
      return EFI_ABORTED;
    }

    if (CurrentFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      //
      // Calculate file checksum
      //
      Checksum  = CalculateSum8 ((UINT8 *) ((UINTN)CurrentFile + FfsFileHeaderSize), FileLength - FfsFileHeaderSize);
      Checksum  = Checksum + CurrentFile->IntegrityCheck.Checksum.File;
      if (Checksum != 0) {
        Error ("FMMT", 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid file checksum", GuidBuffer);
        return EFI_ABORTED;
      }
    } else {
      if (CurrentFile->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
        Error ("FMMT", 0, 0003, "error parsing FFS file", "FFS file with Guid %s has invalid header checksum -- not set to fixed value of 0xAA", GuidBuffer);
        return EFI_ABORTED;
      }
    }
  } else {
    Error ("FMMT", 0, 0003, "error parsing FFS file", "FFS file with Guid %s has the invalid/unrecognized file state bits", GuidBuffer);
    return EFI_ABORTED;
  }

  Level += 1;

  if ((CurrentFile->Type != EFI_FV_FILETYPE_ALL) && (CurrentFile->Type != EFI_FV_FILETYPE_FFS_PAD)) {

    //
    // Put in encapsulate data information.
    //
    LocalEncapData = *CurrentFvEncapData;
    if (LocalEncapData->NextNode != NULL) {
      LocalEncapData = LocalEncapData->NextNode;
      EncapDataNeedUpdateFlag = FALSE;
      while (LocalEncapData->RightNode != NULL) {
        LocalEncapData = LocalEncapData->RightNode;
      }
    }

    if (EncapDataNeedUpdateFlag) {
      //
      // Construct the new ENCAP_DATA
      //
      LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

      if (LocalEncapData->NextNode == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      LocalEncapData        = LocalEncapData->NextNode;

      LocalEncapData->Level = Level;
      LocalEncapData->Type  = FMMT_ENCAP_TREE_FFS;
      LocalEncapData->FvExtHeader = NULL;
      LocalEncapData->Depex = NULL;
      LocalEncapData->DepexLen = 0;
      LocalEncapData->UiNameSize = 0;
      //
      // Store the header of FFS file.
      //
      LocalEncapData->Data     = malloc (FfsFileHeaderSize);
      if (LocalEncapData->Data == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      memcpy (LocalEncapData->Data, CurrentFile, FfsFileHeaderSize);
      LocalEncapData->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)malloc(sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER));
      if (LocalEncapData->FvExtHeader == NULL) {
        Error(NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
        }
      LocalEncapData->FvExtHeader->FvName.Data1 = CurrentFile->Name.Data1;
      LocalEncapData->FvExtHeader->FvName.Data2 = CurrentFile->Name.Data2;
      LocalEncapData->FvExtHeader->FvName.Data3 = CurrentFile->Name.Data3;
      LocalEncapData->FvExtHeader->FvName.Data4[0] = CurrentFile->Name.Data4[0];
      LocalEncapData->FvExtHeader->FvName.Data4[1] = CurrentFile->Name.Data4[1];
      LocalEncapData->FvExtHeader->FvName.Data4[2] = CurrentFile->Name.Data4[2];
      LocalEncapData->FvExtHeader->FvName.Data4[3] = CurrentFile->Name.Data4[3];
      LocalEncapData->FvExtHeader->FvName.Data4[4] = CurrentFile->Name.Data4[4];
      LocalEncapData->FvExtHeader->FvName.Data4[5] = CurrentFile->Name.Data4[5];
      LocalEncapData->FvExtHeader->FvName.Data4[6] = CurrentFile->Name.Data4[6];
      LocalEncapData->FvExtHeader->FvName.Data4[7] = CurrentFile->Name.Data4[7];
      LocalEncapData->NextNode = NULL;
    LocalEncapData->RightNode = NULL;
    }else{
      //
      // Construct the new ENCAP_DATA
      //
      LocalEncapData->RightNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

      if (LocalEncapData->RightNode == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      LocalEncapData        = LocalEncapData->RightNode;

      LocalEncapData->Level = Level;
      LocalEncapData->Type  = FMMT_ENCAP_TREE_FFS;
    LocalEncapData->FvExtHeader = NULL;
      LocalEncapData->Depex = NULL;
      LocalEncapData->DepexLen = 0;
      LocalEncapData->UiNameSize = 0;
      //
      // Store the header of FFS file.
      //
      LocalEncapData->Data     = malloc (FfsFileHeaderSize);
      if (LocalEncapData->Data == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      memcpy (LocalEncapData->Data, CurrentFile, FfsFileHeaderSize);
      LocalEncapData->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)malloc(sizeof(EFI_FIRMWARE_VOLUME_EXT_HEADER));
      if (LocalEncapData->FvExtHeader == NULL) {
        Error(NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }
      LocalEncapData->FvExtHeader->FvName.Data1 = CurrentFile->Name.Data1;
      LocalEncapData->FvExtHeader->FvName.Data2 = CurrentFile->Name.Data2;
      LocalEncapData->FvExtHeader->FvName.Data3 = CurrentFile->Name.Data3;
      LocalEncapData->FvExtHeader->FvName.Data4[0] = CurrentFile->Name.Data4[0];
      LocalEncapData->FvExtHeader->FvName.Data4[1] = CurrentFile->Name.Data4[1];
      LocalEncapData->FvExtHeader->FvName.Data4[2] = CurrentFile->Name.Data4[2];
      LocalEncapData->FvExtHeader->FvName.Data4[3] = CurrentFile->Name.Data4[3];
      LocalEncapData->FvExtHeader->FvName.Data4[4] = CurrentFile->Name.Data4[4];
      LocalEncapData->FvExtHeader->FvName.Data4[5] = CurrentFile->Name.Data4[5];
      LocalEncapData->FvExtHeader->FvName.Data4[6] = CurrentFile->Name.Data4[6];
      LocalEncapData->FvExtHeader->FvName.Data4[7] = CurrentFile->Name.Data4[7];
      LocalEncapData->RightNode = NULL;
      LocalEncapData->NextNode = NULL;
    }

   if ( CurrentFile->Type == EFI_FV_FILETYPE_RAW){
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag){
        LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
      }
    } else if( CurrentFile->Type == EFI_FV_FILETYPE_FFS_PAD){
      //EFI_FV_FILETYPE_FFS_PAD
    } else {
    //
    // All other files have sections
    //
    Status = LibParseSection (
      (UINT8 *) ((UINTN) CurrentFile + FfsFileHeaderSize),
      FileLength - FfsFileHeaderSize,
      CurrentFv,
      FvName,
      CurrentFile,
      Level,
      &LocalEncapData,
      Level,
      FfsCount,
      FvCount,
      ViewFlag,
      ErasePolarity,
      &IsGeneratedFfs
      );
    }
    if (EFI_ERROR (Status)) {
      printf ("ERROR: Parsing the FFS file.\n");
      return Status;
    }
  }


  return EFI_SUCCESS;
}

/**

  Get firmware information. Including the FV headers,

  @param[in]    Fv            - Firmware Volume to get information from

  @return       EFI_STATUS

**/
EFI_STATUS
LibGetFvInfo (
  IN     VOID                         *Fv,
  IN OUT FV_INFORMATION               *CurrentFv,
  IN     CHAR8                        *FvName,
  IN     UINT8                        Level,
  IN     ENCAP_INFO_DATA              **CurrentFvEncapData,
  IN     UINT32                       *FfsCount,
  IN OUT UINT8                        *FvCount,
  IN     BOOLEAN                      ViewFlag,
  IN     BOOLEAN                      IsChildFv
  )
{
  EFI_STATUS                  Status;
  UINTN                       NumberOfFiles;
  BOOLEAN                     ErasePolarity;
  UINTN                       FvSize;
  EFI_FFS_FILE_HEADER2        *CurrentFile;
  UINTN                       Key;
  ENCAP_INFO_DATA             *LocalEncapData;
  EFI_FIRMWARE_VOLUME_EXT_HEADER *ExtHdrPtr;
  EFI_FIRMWARE_VOLUME_HEADER *FvHdr;

  NumberOfFiles  = 0;
  Key            = 0;
  LocalEncapData = NULL;
  CurrentFile    = NULL;
  FvHdr = (EFI_FIRMWARE_VOLUME_HEADER *)Fv;


  Level += 1;
  *FvCount += 1;
  CurrentFv->FvLevel += 1;

  Status = FvBufGetSize (Fv, &FvSize);

  ErasePolarity = (((EFI_FIRMWARE_VOLUME_HEADER*)Fv)->Attributes & EFI_FVB2_ERASE_POLARITY) ? TRUE : FALSE;

  Status = LibReadFvHeader (Fv, ViewFlag, CurrentFv->FvLevel, *FvCount - 1, CurrentFv->FvName);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0003, "error parsing FV image", "Header is invalid");
    return EFI_ABORTED;
  }

  if (!IsChildFv) {
    //
    // Write FV header information into CurrentFv struct.
    //
    CurrentFv->FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (FvHdr->HeaderLength);

    if (CurrentFv->FvHeader == NULL) {
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      return EFI_ABORTED;
    }

    //
    // Get the FV Header information
    //
    memcpy(CurrentFv->FvHeader, Fv, FvHdr->HeaderLength);
    CurrentFv->FvExtHeader = NULL;
    CurrentFv->FvUiName = NULL;

    //
    // Exist Extend FV header.
    //
    if (CurrentFv->FvHeader->ExtHeaderOffset != 0){
      ExtHdrPtr = (VOID *)((UINTN)Fv + CurrentFv->FvHeader->ExtHeaderOffset);
      CurrentFv->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) malloc (ExtHdrPtr->ExtHeaderSize);

      if (CurrentFv->FvExtHeader == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      //
      // Get the FV extended Header information
      //
      memcpy (CurrentFv->FvExtHeader, (VOID *)((UINTN)Fv + CurrentFv->FvHeader->ExtHeaderOffset), ExtHdrPtr->ExtHeaderSize);
      LibExtractFvUiName(CurrentFv->FvExtHeader, &CurrentFv->FvUiName);
    }
  }

  //
  // Put encapsulate information into structure.
  //
  LocalEncapData = *CurrentFvEncapData;
  if (LocalEncapData == NULL && !IsChildFv) {
    //
    // First time in, the root FV
    //
    LocalEncapData = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));
    CurrentFv->EncapData = LocalEncapData;
    if (CurrentFv->EncapData == NULL) {
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      return EFI_ABORTED;
    }

    CurrentFv->EncapData->FvExtHeader = NULL;
    CurrentFv->EncapData->Depex = NULL;
    CurrentFv->EncapData->DepexLen = 0;
    CurrentFv->EncapData->UiNameSize = 0;
    CurrentFv->EncapData->Level = Level;
    CurrentFv->EncapData->Type  = FMMT_ENCAP_TREE_FV;
    CurrentFv->EncapData->Data  = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (sizeof (EFI_FIRMWARE_VOLUME_HEADER));
    CurrentFv->EncapData->FvId  = *FvCount;

    if (CurrentFv->EncapData->Data == NULL) {
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      return EFI_ABORTED;
    }

    memcpy (CurrentFv->EncapData->Data, Fv, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

    if (((EFI_FIRMWARE_VOLUME_HEADER *)(CurrentFv->EncapData->Data))->ExtHeaderOffset != 0) {
      ExtHdrPtr = (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(CurrentFv->EncapData->Data))->ExtHeaderOffset);
      CurrentFv->EncapData->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) malloc (ExtHdrPtr->ExtHeaderSize);

      if (CurrentFv->EncapData->FvExtHeader == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      //
      // Get the FV extended Header information
      //
      memcpy(CurrentFv->EncapData->FvExtHeader, (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(CurrentFv->EncapData->Data))->ExtHeaderOffset), ExtHdrPtr->ExtHeaderSize);
    }

    CurrentFv->EncapData->NextNode  = NULL;
  CurrentFv->EncapData->RightNode  = NULL;
  } else if (LocalEncapData == NULL) {
    return EFI_ABORTED;
  } else if (IsChildFv) {

      LocalEncapData = *CurrentFvEncapData;
      while (LocalEncapData->NextNode != NULL) {
        LocalEncapData = LocalEncapData->NextNode;
      }

      //
      // Construct the new ENCAP_DATA
      //
      LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

      if (LocalEncapData->NextNode == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      LocalEncapData           = LocalEncapData->NextNode;

      LocalEncapData->Level = Level;
      LocalEncapData->Type  = FMMT_ENCAP_TREE_FV;
      LocalEncapData->Data  = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (sizeof (EFI_FIRMWARE_VOLUME_HEADER));
      LocalEncapData->FvExtHeader = NULL;
      LocalEncapData->Depex = NULL;
      LocalEncapData->DepexLen = 0;
      LocalEncapData->UiNameSize = 0;
      LocalEncapData->FvId  = *FvCount;
      if (LocalEncapData->Data == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }

      memcpy (LocalEncapData->Data, Fv, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

      if (((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset != 0) {
        ExtHdrPtr = (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset);
        LocalEncapData->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)malloc(ExtHdrPtr->ExtHeaderSize);

        if (LocalEncapData->FvExtHeader == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }

        //
        // Get the FV extended Header information
        //
        memcpy(LocalEncapData->FvExtHeader, (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset), ExtHdrPtr->ExtHeaderSize);
      }

      LocalEncapData->NextNode  = NULL;
      LocalEncapData->RightNode  = NULL;

  }


  //
  // Get the first file
  //
  Status = FvBufFindNextFile (Fv, &Key, (VOID **) &CurrentFile);
  if (Status == EFI_NOT_FOUND) {
    CurrentFile = NULL;
  } else if (EFI_ERROR (Status)) {
    Error ("FMMT", 0, 0003, "error parsing FV image", "cannot find the first file in the FV image");
    return Status;
  }

  while (CurrentFile != NULL) {

    //
    // Increment the number of files counter
    //
    NumberOfFiles++;

    //
    // Store FFS file Header information
    //
    CurrentFv->FfsHeader[*FfsCount].Attributes       = CurrentFile->Attributes;
    CurrentFv->FfsHeader[*FfsCount].IntegrityCheck   = CurrentFile->IntegrityCheck;
    CurrentFv->FfsHeader[*FfsCount].Name             = CurrentFile->Name;
    CurrentFv->FfsHeader[*FfsCount].Size[0]          = CurrentFile->Size[0];
    CurrentFv->FfsHeader[*FfsCount].Size[1]          = CurrentFile->Size[1];
    CurrentFv->FfsHeader[*FfsCount].Size[2]          = CurrentFile->Size[2];
    CurrentFv->FfsHeader[*FfsCount].State            = CurrentFile->State;
    CurrentFv->FfsHeader[*FfsCount].Type             = CurrentFile->Type;
    CurrentFv->FfsHeader[*FfsCount].ExtendedSize     = CurrentFile->ExtendedSize;
    CurrentFv->FfsAttuibutes[*FfsCount].Offset = Key - GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);

    CurrentFv->FfsAttuibutes[*FfsCount].FvLevel = CurrentFv->FvLevel;
     if (CurrentFv->FvLevel > CurrentFv->MulFvLevel) {
      CurrentFv->MulFvLevel = CurrentFv->FvLevel;
   }
    //
    // Display info about this file
    //
    Status = LibGetFileInfo (Fv, CurrentFile, ErasePolarity, CurrentFv, FvName, Level, &LocalEncapData, FfsCount, FvCount, ViewFlag);
    if (EFI_ERROR (Status)) {
      Error ("FMMT", 0, 0003, "error parsing FV image", "failed to parse a file in the FV");
      return Status;
    }

    //
    // Get the next file
    //
    Status = FvBufFindNextFile (Fv, &Key, (VOID **) &CurrentFile);
    if (Status == EFI_NOT_FOUND) {
      CurrentFile = NULL;
    } else if (EFI_ERROR (Status)) {
      Error ("FMMT", 0, 0003, "error parsing FV image", "cannot find the next file in the FV image");
      return Status;
    }
  }

  if (IsChildFv) {
      if (CurrentFv->FvLevel > 1) {
          CurrentFv->FvLevel -= 1;
      }
  }
  return EFI_SUCCESS;
}

/**

  This function returns the next larger size that meets the alignment
  requirement specified.

  @param[in]      ActualSize      The size.
  @param[in]      Alignment       The desired alignment.

  @retval         EFI_SUCCESS     Function completed successfully.
  @retval         EFI_ABORTED     The function encountered an error.

**/
UINT32
GetOccupiedSize (
  IN UINT32  ActualSize,
  IN UINT32  Alignment
  )
{
  UINT32  OccupiedSize;

  OccupiedSize = ActualSize;
  while ((OccupiedSize & (Alignment - 1)) != 0) {
    OccupiedSize++;
  }

  return OccupiedSize;
}



EFI_STATUS
LibDeleteAndRenameFfs(
  IN CHAR8*    DeleteFile,
  IN CHAR8*    NewFile
)
{
  CHAR8*                 SystemCommand;
  CHAR8*                 TemDir;
  SystemCommand             = NULL;

  if (DeleteFile == NULL ||
      NewFile    == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete the file of the specified extract FFS file.
  //
  SystemCommand = malloc (
    strlen (DEL_STR) +
    strlen (DeleteFile)  +
    1
    );
  if (SystemCommand == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return EFI_ABORTED;
  }

  sprintf (
    SystemCommand,
    DEL_STR,
    DeleteFile
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
    free(SystemCommand);
    return EFI_ABORTED;
  }
  free(SystemCommand);

  TemDir = getcwd (NULL, _MAX_PATH);
  if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    Error("FMMT", 0, 1001, "The directory is too long.", "");
     return EFI_ABORTED;
  }
  strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
  strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);

  mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);
  //
  // Create a copy the new file.
  //

  SystemCommand = malloc (
    strlen (COPY_STR) +
    strlen (NewFile)    +
    strlen (DeleteFile) +
    1
    );
  if (SystemCommand == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return EFI_ABORTED;
  }

  sprintf (
    SystemCommand,
    COPY_STR,
    NewFile,
    DeleteFile
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
  free(SystemCommand);
    return EFI_ABORTED;
  }
  free(SystemCommand);

  return EFI_SUCCESS;

}

/**
  Converts ASCII characters to Unicode.
  Assumes that the Unicode characters are only these defined in the ASCII set.

  String      - Pointer to string that is written to FILE.
  UniString   - Pointer to unicode string

  The address to the ASCII string - same as AsciiStr.

**/
VOID
LibAscii2Unicode (
  IN   CHAR8          *String,
  OUT  CHAR16         *UniString
  )
{
  while (*String != '\0') {
    *(UniString++) = (CHAR16) *(String++);
    }
  //
  // End the UniString with a NULL.
  //
  *UniString = '\0';
}


EFI_STATUS
LibCreateGuidedSectionOriginalData(
  IN CHAR8*    FileIn,
  IN CHAR8*    ToolName,
  IN CHAR8*    FileOut
)
{
  CHAR8*                 SystemCommandFormatString;
  CHAR8*                 SystemCommand;

  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;

  if (FileIn   == NULL ||
      ToolName == NULL ||
      FileOut  == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete the file of the specified extract FFS file.
  //
  SystemCommandFormatString = "%s -e \"%s\" -o \"%s\"";

  SystemCommand = malloc (
    strlen (SystemCommandFormatString) +
    strlen (FileIn)  +
    strlen (ToolName)  +
    strlen (FileOut)  +
    1
    );
  if (SystemCommand == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return EFI_ABORTED;
  }

  sprintf (
    SystemCommand,
    "%s -e \"%s\" -o \"%s\"",
    ToolName,
    FileIn,
    FileOut
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
    printf("Command failed: %s\n", SystemCommand);
  free(SystemCommand);
    return EFI_ABORTED;
  }
  free(SystemCommand);

  return EFI_SUCCESS;
}

/**

   This function convert the FV header's attribute to a string. The converted string
   will be put into an INF file as the input of GenFV.

   @param[in]      Attr       FV header's attribute.
   @param[out]     InfFile    InfFile contain FV header attribute information.

   @retval         EFI_SUCCESS.
   @retval         EFI_INVLID_PARAMETER
   @retval         EFI_OUT_OF_RESOURCES

**/
EFI_STATUS
LibFvHeaderAttributeToStr (
  IN     EFI_FVB_ATTRIBUTES_2     Attr,
  IN     FILE*                  InfFile
)
{
  CHAR8     *LocalStr;

  LocalStr  = NULL;

  LocalStr = (CHAR8 *) malloc (STR_LEN_MAX_4K);

  if (LocalStr == NULL) {
    printf ("Memory allocate error!\n");
    return EFI_OUT_OF_RESOURCES;
  }

  memset (LocalStr, '\0', STR_LEN_MAX_4K);

  if (Attr == 0 || InfFile  == NULL) {
    free (LocalStr);
    return EFI_INVALID_PARAMETER;
  }

  strncat (LocalStr, "[attributes] \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);

  if (Attr & EFI_FVB2_READ_DISABLED_CAP) {
    strncat (LocalStr, "EFI_READ_DISABLED_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_READ_ENABLED_CAP) {
    strncat (LocalStr, "EFI_READ_ENABLED_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_READ_STATUS) {
    strncat (LocalStr, "EFI_READ_STATUS = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_WRITE_DISABLED_CAP) {
    strncat (LocalStr, "EFI_WRITE_DISABLED_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_WRITE_ENABLED_CAP) {
    strncat (LocalStr, "EFI_WRITE_ENABLED_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_WRITE_STATUS) {
    strncat (LocalStr, "EFI_WRITE_STATUS = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_LOCK_CAP) {
    strncat (LocalStr, "EFI_LOCK_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_LOCK_STATUS) {
    strncat (LocalStr, "EFI_LOCK_STATUS = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_STICKY_WRITE) {
    strncat (LocalStr, "EFI_STICKY_WRITE = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_MEMORY_MAPPED) {
    strncat (LocalStr, "EFI_MEMORY_MAPPED = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_ERASE_POLARITY) {
    strncat (LocalStr, "EFI_ERASE_POLARITY = 1 \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_READ_LOCK_CAP) {
    strncat (LocalStr, "EFI_READ_LOCK_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_READ_LOCK_STATUS) {
    strncat (LocalStr, "EFI_READ_LOCK_STATUS = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_WRITE_LOCK_CAP) {
    strncat (LocalStr, "EFI_WRITE_LOCK_CAP = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_WRITE_LOCK_STATUS) {
    strncat (LocalStr, "EFI_WRITE_LOCK_STATUS = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (Attr & EFI_FVB2_LOCK_STATUS) {
    strncat (LocalStr, "EFI_READ_LOCK_STATUS = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  //
  // Alignment
  //
  if (Attr & EFI_FVB2_ALIGNMENT_1) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_1 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_2) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_2 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_4) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_4 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_8) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_8 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_16) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_16 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_32) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_32 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_64) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_64 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_128) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_128 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_256) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_256 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_512) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_512 = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_1K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_1K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_2K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_2K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_4K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_4K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_8K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_8K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_16K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_16K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_32K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_32K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_64K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_64K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_128K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_128K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_256K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_256K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_512K) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_512K = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_1M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_1M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_2M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_2M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_4M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_4M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_8M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_8M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_16M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_16M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_32M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_32M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_64M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_64M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_128M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_128M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_256M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_256M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_512M) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_512M = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_1G) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_1G = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  } else if (Attr & EFI_FVB2_ALIGNMENT_2G) {
    strncat (LocalStr, "EFI_FVB2_ALIGNMENT_2G = TRUE \n", STR_LEN_MAX_4K - strlen (LocalStr) - 1);
  }

  if (fwrite (LocalStr, 1, (size_t) strlen (LocalStr), InfFile) != (size_t) strlen (LocalStr)) {
    printf ("Error while writing data to %p file.", (void*)InfFile);
    free (LocalStr);
    return EFI_ABORTED;
  }

  free (LocalStr);

  return EFI_SUCCESS;
}


/**
   This function fill the FV inf files option field.

   @param[in]      BlockMap       FV header's attribute.
   @param[out]     InfFile    InfFile contain FV header attribute information.

   @retval         EFI_SUCCESS.
   @retval         EFI_INVLID_PARAMETER

**/
EFI_STATUS
LibFvHeaderOptionToStr (
  IN     EFI_FIRMWARE_VOLUME_HEADER  *FvHeader,
  IN     FILE*                   InfFile,
  IN     BOOLEAN                 IsRootFv
)
{
  CHAR8     *LocalStr;
  CHAR8     *TempStr;
  EFI_FV_BLOCK_MAP_ENTRY  *BlockMap;

  LocalStr     = NULL;
  TempStr      = NULL;

  if (FvHeader == NULL || InfFile  == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // This section will not over 1024 bytes and each line will never over 128 bytes.
  //
  LocalStr    = (CHAR8 *) malloc (STR_LEN_MAX_1K);
  TempStr     = (CHAR8 *) malloc (128);

  if (LocalStr    == NULL ||
     TempStr   == NULL ) {
      if (LocalStr != NULL) {
        free (LocalStr);
      }
      if (TempStr != NULL) {
        free (TempStr);
      }
    printf ("Memory allocate error! \n");
    return EFI_OUT_OF_RESOURCES;
  }

  BlockMap = FvHeader->BlockMap;
  memset (LocalStr, '\0', STR_LEN_MAX_1K);
  memset (TempStr, '\0', 128);

  strncat (LocalStr, "[options] \n", STR_LEN_MAX_1K - strlen (LocalStr) - 1);


  snprintf (TempStr, 128, "EFI_BLOCK_SIZE  = 0x%x \n", BlockMap->Length);
  strncat (LocalStr, TempStr, STR_LEN_MAX_1K - strlen (LocalStr) - 1);

  if (IsRootFv) {
  snprintf (TempStr, 128, "EFI_NUM_BLOCKS  = 0x%x \n", BlockMap->NumBlocks);
  strncat (LocalStr, TempStr, STR_LEN_MAX_1K - strlen (LocalStr) - 1);
  }

  if (fwrite (LocalStr, 1, (size_t) strlen (LocalStr), InfFile) != (size_t) strlen (LocalStr)) {
    printf ("Error while writing data to %p file.", (void*)InfFile);
    free (LocalStr);
    free (TempStr);
    return EFI_ABORTED;
  }

  free (LocalStr);
  free (TempStr);

  return EFI_SUCCESS;
}

/**
   This function fill the FV inf files option field.

   @param[in]      FfsName    Ffs file path/name.
   @param[out]     InfFile    InfFile contain FV header attribute information
   @param[in]      FirstIn    Is the first time call this function? If yes, should create [files] section.

   @retval         EFI_SUCCESS.
   @retval         EFI_INVLID_PARAMETER

**/
EFI_STATUS
LibAddFfsFileToFvInf (
  IN     CHAR8                   *FfsName,
  IN     FILE*                   InfFile,
  IN     BOOLEAN                 FirstIn
)
{

  CHAR8     *LocalStr;

  LocalStr     = NULL;

  if (FfsName == NULL || InfFile  == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (strlen(FfsName) == 0) {
    return EFI_SUCCESS;
  }

  LocalStr    = (CHAR8 *) malloc (_MAX_PATH);

  if (LocalStr == NULL) {
    printf ("Memory allocate error! \n");
    return EFI_OUT_OF_RESOURCES;
  }

  memset (LocalStr, '\0', _MAX_PATH);

  if (FirstIn) {
    sprintf (LocalStr, "[files] \nEFI_FILE_NAME = %s \n", FfsName);
  } else {
    sprintf (LocalStr, "EFI_FILE_NAME = %s \n", FfsName);
  }

  if (fwrite (LocalStr, 1, (size_t) strlen (LocalStr), InfFile) != (size_t) strlen (LocalStr)) {
    printf ("Error while writing data to %p file.", (void*)InfFile);
    free (LocalStr);
    return EFI_ABORTED;
  }

  free (LocalStr);

  return EFI_SUCCESS;
}


/**
  Convert EFI file to PE or TE section

  @param[in]   InputFilePath   .efi file, it's optional unless process PE/TE section.
  @param[in]   Type            PE or TE and UI/Version
  @param[in]   OutputFilePath  .te or .pe file
  @param[in]   UiString        String for generate UI section usage, this parameter is optional
                               unless Type is EFI_SECTION_USER_INTERFACE.
  @param[in]   VerString       String for generate Version section usage, this parameter is optional
                               unless Type is EFI_SECTION_VERSION.

  @retval EFI_SUCCESS

**/
EFI_STATUS
LibCreateFfsSection (
  IN FV_INFORMATION   *FvInFd,      OPTIONAL
  IN CHAR8*     InputFilePath,      OPTIONAL
  IN CHAR8*     Sections,           OPTIONAL
  IN UINT8      Type,
  IN CHAR8*     OutputFilePath,
  IN CHAR8*     UiString,           OPTIONAL
  IN CHAR8*     VerString,          OPTIONAL
  IN CHAR8*     GuidToolGuid,       OPTIONAL
  IN UINT16     GuidHeaderLength,
  IN UINT16     GuidAttr,
  IN CHAR8*     CompressType       OPTIONAL
  )
{
  //EFI_STATUS             Status;
  CHAR8*                 SystemCommandFormatString;
  CHAR8*                 SystemCommand;
  FILE                   *file;
  UINT8                  Buffer[4];
  int                    BitNum;
  int                    Position;
  UINT32                 AlignmentValue;
  //
  // Workaround for static code checkers.
  // Ensures the size of 'AlignmentStr' can hold all the digits of an
  // unsigned 32-bit integer plus the size unit character.
  //
  char                   AlignmentStr[16];

  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;
  strcpy(AlignmentStr,"1");
  //
  // Call GenSec tool to generate FFS section.
  //

  //
  // -s SectionType.
  //
  if (Type != 0) {
    switch (Type) {
      //
      // Process compression section
      //
      case EFI_SECTION_COMPRESSION:
        SystemCommandFormatString = "GenSec -s %s -c %s  \"%s\" -o \"%s\"";
        SystemCommand = malloc (
          strlen (SystemCommandFormatString) +
          strlen (mSectionTypeName[Type]) +
          strlen (CompressType) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          1
          );
        if (SystemCommand == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }
        sprintf (
          SystemCommand,
          "GenSec -s %s -c %s  \"%s\" -o \"%s\"",
          mSectionTypeName[Type],
          CompressType,
          InputFilePath,
          OutputFilePath
          );

        if (system (SystemCommand) != EFI_SUCCESS) {
          free(SystemCommand);
          return EFI_ABORTED;
        }
        free(SystemCommand);
        break;

      //
      // Process GUID defined section
      //
      case EFI_SECTION_GUID_DEFINED:
        SystemCommandFormatString = "GenSec -s %s -g %s \"%s\" -o \"%s\" -r %s -r %s -l %d";
        SystemCommand = malloc (
          strlen (SystemCommandFormatString) +
          strlen (mSectionTypeName[Type]) +
          strlen (GuidToolGuid) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          strlen (mGuidSectionAttr[GuidAttr&0x01]) +
          strlen (mGuidSectionAttr[GuidAttr&0x02]) +
          4 +
          1
          );
        if (SystemCommand == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }
        sprintf (
          SystemCommand,
          "GenSec -s %s -g %s \"%s\" -o \"%s\" -r %s -r %s -l %d",
          mSectionTypeName[Type],
          GuidToolGuid,
          InputFilePath,
          OutputFilePath,
          mGuidSectionAttr[GuidAttr&0x01],
          mGuidSectionAttr[GuidAttr&0x02],
          GuidHeaderLength
          );

        if (system (SystemCommand) != EFI_SUCCESS) {
          free(SystemCommand);
          return EFI_ABORTED;
        }
        free(SystemCommand);
        break;

      case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:

        SystemCommandFormatString = "GenSec -s %s \"%s\" -o \"%s\"";
        SystemCommand = malloc (
          strlen (SystemCommandFormatString) +
          strlen (mSectionTypeName[Type]) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          1
          );
        if (SystemCommand == NULL) {
          Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
          return EFI_ABORTED;
        }
        sprintf (
          SystemCommand,
          "GenSec -s %s \"%s\" -o \"%s\"",
          mSectionTypeName[Type],
          InputFilePath,
          OutputFilePath
          );

        if (system (SystemCommand) != EFI_SUCCESS) {
          free(SystemCommand);
          return EFI_ABORTED;
        }
        free(SystemCommand);
        break;

      default:
        Error ("FMMT", 0, 0003, "Please specify the section type while call GenSec tool.", NULL);
        return EFI_UNSUPPORTED;
    }
  } else {
    //
    // Create Dummy section.
    //
    file = fopen(InputFilePath, "rb");
    if (file == NULL) {
      Error(NULL, 0, 0001, "Error opening the file", InputFilePath);
      return EFI_INVALID_PARAMETER;
    }
    // The Section Struct, 3 bits for Size, then 1 bit for Type
    if (fread(Buffer, 1, (size_t)(4), file) != (size_t)(4)) {
      fclose(file);
      return EFI_ABORTED;
    }
    if (*(Buffer + 3) == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
      // The Section Struct, if size is not 0xFFFF, the length is 4
      Position = 4;
      // If Size is 0xFFFFFF then ExtendedSize contains the size of the section
      if ((*Buffer == 0xFF) && (*(Buffer + 1) == 0xFF) && (*(Buffer + 2) == 0xFF)) {
        Position = 8;
      }
      //Per EFI_FIRMWARE_VOLUME_HEADER struct, 0x2E bit is EFI_FVB_ATTRIBUTES_2 attr
      fseek(file, 0x2E + Position, SEEK_SET);
      BitNum = fgetc(file);
      AlignmentValue = 1 << (BitNum & 0x1F);
      if (AlignmentValue >= 0x400){
        if (AlignmentValue >= 0x10000){
          strcpy(AlignmentStr,"64K");
        }
        else{
          sprintf(AlignmentStr, "%d", AlignmentValue/0x400);
          strcat(AlignmentStr, "K");
        }
      }
      else{
        sprintf(AlignmentStr, "%d", AlignmentValue);
      }
      strcpy(FvInFd->AlignmentStr, AlignmentStr);
    }
    fclose(file);
    SystemCommandFormatString = "GenSec \"%s\" -o \"%s\" --sectionalign %s";
    SystemCommand = malloc (
      strlen (SystemCommandFormatString) +
      strlen (InputFilePath) +
      strlen (OutputFilePath) +
      4 +                                 // Alignment maximum length
      1
      );
    if (SystemCommand == NULL) {
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      return EFI_ABORTED;
    }
    sprintf (
      SystemCommand,
      "GenSec \"%s\" -o \"%s\" --sectionalign %s",
      InputFilePath,
      OutputFilePath,
      AlignmentStr
      );

    if (system (SystemCommand) != EFI_SUCCESS) {
      free(SystemCommand);
      return EFI_ABORTED;
    }
    free(SystemCommand);

  }

  return EFI_SUCCESS;
}

/**
  Encapsulate FFSs to FV

  @param[in]   InputFilePath   Section file will be read into this FFS file. This option is required.
  @param[in]   OutputFilePath  The created PI firmware file name. This option is required.
  @param[in]   BlockSize       BlockSize is one HEX or DEC format value required by FV image.
  @param[in]   FileTakeSize

  @retval EFI_SUCCESS

**/
EFI_STATUS
LibEncapsulateFfsToFv (
  IN CHAR8*     InfFilePath,
  IN CHAR8*     InputFFSs,
  IN CHAR8*     OutputFilePath,
  IN CHAR8*     FvGuidName,
  IN BOOLEAN    IsLargeFile
  )
{

  CHAR8*                 SystemCommandFormatString;
  CHAR8*                 SystemCommand;
  CHAR8*                 FfsGuid = "8c8ce578-8a3d-4f1c-9935-896185c32dd3";

  if (IsLargeFile == TRUE) {
    FfsGuid = "5473c07a-3dcb-4dca-bd6f-1e9689e7349a";
  }

  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;

  if (OutputFilePath  == NULL ||
      InfFilePath     == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  if (InfFilePath != NULL) {
    if (FvGuidName == NULL) {
      SystemCommandFormatString = "GenFv -i \"%s\" -g %s -o \"%s\"";

      SystemCommand = malloc (
        strlen (SystemCommandFormatString) +
        strlen (InfFilePath)   +
        strlen (FfsGuid) +
        strlen (OutputFilePath)  +
        1
        );
      if (SystemCommand == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }
      sprintf (
        SystemCommand,
        "GenFv -i \"%s\" -g %s -o \"%s\"",
        InfFilePath,          // -i
        FfsGuid,              // -g
        OutputFilePath        // -o
        );

      if (system (SystemCommand) != EFI_SUCCESS) {
        free(SystemCommand);
        return EFI_ABORTED;
      }

      free(SystemCommand);
    } else {
      //
      // Have FvGuidName in it.
      //
      SystemCommandFormatString = "GenFv -i \"%s\" -g %s -o \"%s\" --FvNameGuid %s";

      SystemCommand = malloc (
        strlen (SystemCommandFormatString) +
        strlen (InfFilePath)   +
        strlen (FfsGuid)  +
        strlen (OutputFilePath)  +
        strlen (FvGuidName) +
        1
        );
      if (SystemCommand == NULL) {
        Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
        return EFI_ABORTED;
      }
      sprintf (
        SystemCommand,
        "GenFv -i \"%s\" -g %s -o \"%s\" --FvNameGuid %s",
        InfFilePath,          // -i
        FfsGuid,              // -g
        OutputFilePath,       // -o
        FvGuidName            // FvNameGuid
        );

      if (system (SystemCommand) != EFI_SUCCESS) {
        free(SystemCommand);
        return EFI_ABORTED;
      }
      free(SystemCommand);

    }
  }

  return EFI_SUCCESS;
}


/**

  Convert a GUID to a string.


  @param[in]   Guid       - Pointer to GUID to print.


  @return The string after convert.

**/
CHAR8 *
LibFmmtGuidToStr (
  IN  EFI_GUID  *Guid
)
{
  CHAR8 * Buffer;

  Buffer = NULL;

  if (Guid == NULL) {
    printf ("The guid is NULL while convert guid to string! \n");
    return NULL;
  }

  Buffer = (CHAR8 *) malloc (36 + 1);

  if (Buffer == NULL) {
    printf ("Error while allocate resource! \n");
    return NULL;
  }
  memset (Buffer, '\0', 36 + 1);

  sprintf (
      Buffer,
      "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      Guid->Data1,
      Guid->Data2,
      Guid->Data3,
      Guid->Data4[0],
      Guid->Data4[1],
      Guid->Data4[2],
      Guid->Data4[3],
      Guid->Data4[4],
      Guid->Data4[5],
      Guid->Data4[6],
      Guid->Data4[7]
      );

  return Buffer;
}


/**
  Encapsulate an FFS section file to an FFS file.

  @param[in]   Type            Type is one FV file type defined in PI spec, which is one type of EFI_FV_FILETYPE_RAW, EFI_FV_FILETYPE_FREEFORM,
                               EFI_FV_FILETYPE_SECURITY_CORE, EFI_FV_FILETYPE_PEIM, EFI_FV_FILETYPE_PEI_CORE, EFI_FV_FILETYPE_DXE_CORE,
                               EFI_FV_FILETYPE_DRIVER, EFI_FV_FILETYPE_APPLICATION, EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER,
                               EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE. This option is required.
  @param[in]   InputFilePath   Section file will be read into this FFS file. This option is required.
  @param[in]   OutputFilePath  The created PI firmware file name. This option is required.
  @param[in]   FileGuid        FileGuid is the unique identifier for this FFS file. This option is required.
  @param[in]   Fixed           Set fixed attribute in FFS file header to indicate that the file may not be moved from its present location.
  @param[in]   SectionAlign    FileAlign specifies FFS file alignment, which only support the following alignment: 8,16,128,512,1K,4K,32K,64K.

  @retval EFI_SUCCESS

**/
EFI_STATUS
LibEncapSectionFileToFFS (
  IN UINT8      Type,
  IN CHAR8*     InputFilePath,
  IN CHAR8*     OutputFilePath,
  IN EFI_GUID   FileGuid,
  IN BOOLEAN    Fixed,
  IN CHAR8*     SectionAlign
  )
{
  CHAR8*                 SystemCommandFormatString;
  CHAR8*                 SystemCommand;
  CHAR8*                 GuidStr;


  SystemCommandFormatString = NULL;
  SystemCommand             = NULL;
  GuidStr                   = NULL;

  GuidStr  = LibFmmtGuidToStr(&FileGuid);

  if (GuidStr == NULL) {
    return EFI_ABORTED;
  }


  //
  // -t  Type
  // -i  InputFilePath
  // -o  OutPutFilePath
  // -g  FileGuid
  // -x  Fixed
  // -n  SectionAlign
  //

  if (Fixed) {
    SystemCommandFormatString = "GenFfs -t %s -i \"%s\" -g %s -x -o \"%s\" -a %s";
    SystemCommand = malloc (
      strlen (SystemCommandFormatString) +
      strlen (mFfsFileType[Type]) +
      strlen (InputFilePath) +
      strlen (GuidStr) +
      strlen (OutputFilePath) +
      4 +
      1
      );
    if (SystemCommand == NULL) {
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      free (GuidStr);
      return EFI_ABORTED;
    }
    sprintf (
      SystemCommand,
      "GenFfs -t %s -i \"%s\" -g %s -x -o \"%s\" -a %s",
      mFfsFileType[Type],     // -t
      InputFilePath,          // -i
      GuidStr,                // -g
      OutputFilePath,         // -o
      SectionAlign
      );

    free (GuidStr);
    if (system (SystemCommand) != EFI_SUCCESS) {
      free(SystemCommand);
      return EFI_ABORTED;
    }
    free(SystemCommand);
  } else {
    SystemCommandFormatString = "GenFfs -t %s -i \"%s\" -g %s -o \"%s\" -a %s";
    SystemCommand = malloc (
      strlen (SystemCommandFormatString) +
      strlen (mFfsFileType[Type]) +
      strlen (InputFilePath) +
      strlen (GuidStr) +
      strlen (OutputFilePath) +
      4 +
      1
      );
    if (SystemCommand == NULL) {
      free (GuidStr);
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      return EFI_ABORTED;
    }
    sprintf (
      SystemCommand,
      "GenFfs -t %s -i \"%s\" -g %s -o \"%s\" -a %s",
      mFfsFileType[Type],     // -t
      InputFilePath,          // -i
      GuidStr,                // -g
      OutputFilePath,         // -o
      SectionAlign
      );

    free (GuidStr);
    if (system (SystemCommand) != EFI_SUCCESS) {
      free(SystemCommand);
      return EFI_ABORTED;
    }
    free(SystemCommand);
  }


  return EFI_SUCCESS;
}

EFI_STATUS
LibCreateNewFdCopy(
  IN CHAR8*    OldFd,
  IN CHAR8*    NewFd
)
{

  FILE*                       NewFdFile;
  FILE*                       OldFdFile;
  CHAR8                       *NewFdDir;
  CHAR8                       *OldFdDir;
  UINT64                      FdLength;
  UINT32                      Count;
  BOOLEAN                     UseNewDirFlag;
  CHAR8                       *Buffer;

  NewFdFile = NULL;
  OldFdFile = NULL;
  NewFdDir  = NULL;
  OldFdDir  = NULL;
  Count     = 0;
  UseNewDirFlag = FALSE;

  if (OldFd == NULL ||
    NewFd    == NULL) {
      return EFI_INVALID_PARAMETER;
  }


  NewFdDir = getcwd (NULL, _MAX_PATH);

  Count = strlen(NewFdDir);

  if (strlen(NewFd) > Count) {

    do {
      if (NewFdDir[Count-1] == NewFd[Count-1]) {
        Count--;
      } else {
        if (strlen(NewFdDir) + strlen (OS_SEP_STR) + strlen (NewFd) > _MAX_PATH -1) {
          return EFI_ABORTED;
        }
        strncat (NewFdDir,OS_SEP_STR, _MAX_PATH - strlen (NewFdDir) -1);
        strncat (NewFdDir,NewFd, _MAX_PATH - strlen (NewFdDir) -1);
        UseNewDirFlag = TRUE;
        break;
      }

    } while (Count != 1);

  }else {
    if (strlen(NewFdDir) + strlen (OS_SEP_STR) + strlen (NewFd) > _MAX_PATH -1) {
      return EFI_ABORTED;
    }
    strncat (NewFdDir,OS_SEP_STR, _MAX_PATH - strlen (NewFdDir) -1);
    strncat (NewFdDir,NewFd, _MAX_PATH - strlen (NewFdDir) -1);
    UseNewDirFlag = TRUE;
  }

  if (UseNewDirFlag) {
    NewFdFile = fopen (NewFdDir, "wb+");
    if (NewFdFile == NULL) {
      NewFdFile = fopen (NewFd, "wb+");
    }
  } else {
    NewFdFile = fopen (NewFd, "wb+");
  }
  // support network path file
  if (OldFd[0] == '\\' && OldFd[1] == '\\') {
    OldFdFile = fopen (OldFd, "rb");
  } else {
  UseNewDirFlag = FALSE;

  OldFdDir = getcwd (NULL, _MAX_PATH);

  Count = strlen(OldFdDir);

  if (strlen(OldFd) > Count) {

    do {
      if (OldFdDir[Count-1] == OldFd[Count-1]) {
        Count--;
      } else {
        if (strlen(OldFdDir) + strlen (OS_SEP_STR) + strlen (OldFd) > _MAX_PATH -1) {
          if (NewFdFile != NULL) {
            fclose(NewFdFile);
          }
          return EFI_ABORTED;
        }
        strncat (OldFdDir,OS_SEP_STR, _MAX_PATH - strlen (OldFdDir) -1);
        strncat (OldFdDir,OldFd, _MAX_PATH - strlen (OldFdDir) -1);
        UseNewDirFlag = TRUE;
        break;
      }

    } while (Count != 1);

  }else {
    if (strlen(OldFdDir) + strlen (OS_SEP_STR) + strlen (OldFd) > _MAX_PATH -1) {
      if (NewFdFile != NULL) {
        fclose(NewFdFile);
      }
      return EFI_ABORTED;
    }
    strncat (OldFdDir,OS_SEP_STR, _MAX_PATH - strlen (OldFdDir) -1);
    strncat (OldFdDir,OldFd, _MAX_PATH - strlen (OldFdDir) -1);
    UseNewDirFlag = TRUE;
  }

  if (UseNewDirFlag) {
    OldFdFile = fopen (OldFdDir, "rb+");
    if (OldFdFile == NULL) {
      OldFdFile = fopen (OldFd, "rb+");
    }
  } else {
    OldFdFile = fopen (OldFd, "rb+");
  }
  }

  if (NewFdFile == NULL) {
    Error ("FMMT", 0, 0003, "error Open FD file", "cannot Create a new FD file.");
    if (OldFdFile != NULL) {
      fclose (OldFdFile);
    }
    return EFI_ABORTED;
  }

  if (OldFdFile == NULL) {
    Error ("FMMT", 0, 0003, "error Open FD file", "cannot Create a new FD file.");
    if (NewFdFile != NULL) {
      fclose (NewFdFile);
    }
    return EFI_ABORTED;
  }


  fseek(OldFdFile,0,SEEK_SET);
  fseek(OldFdFile,0,SEEK_END);

  FdLength = ftell(OldFdFile);

  fseek(OldFdFile,0,SEEK_SET);
  fseek(NewFdFile,0,SEEK_SET);

  Buffer = malloc ((size_t)FdLength);

  if (Buffer == NULL)  {
    fclose(OldFdFile);
    fclose(NewFdFile);
    return EFI_ABORTED;
  }

  if (fread (Buffer, 1, (size_t) FdLength, OldFdFile) != (size_t) FdLength) {
    Error ("FMMT", 0, 0003, "error reading FD file %s", OldFd);
    free (Buffer);
    fclose(OldFdFile);
    fclose(NewFdFile);
    return EFI_ABORTED;
  }

  if (fwrite (Buffer, 1, (size_t) FdLength, NewFdFile) != (size_t) FdLength) {
    Error ("FMMT", 0, 0004, "error writing FD file", "cannot Create a new FD file.");
    free (Buffer);
    fclose(OldFdFile);
    fclose(NewFdFile);
    return EFI_ABORTED;
  }
  free (Buffer);
  fclose(OldFdFile);
  fclose (NewFdFile);

  return EFI_SUCCESS;
}


/**
  This function will assemble the filename, directory and extend and return the combined string.
  Like FileName = file1, Dir = c:\temp extend = txt, the output string will be:
  c:\temp\file1.txt.

  @param[in]
  @param[in]
  @param[in]

  @retrun     A string contain all the input information.

**/
CHAR8 *
LibFilenameStrExtended (
  IN CHAR8      *FileName,
  IN CHAR8      *Dir,
  IN CHAR8      *Extend
)
{
  CHAR8 *RetStr;

  RetStr = NULL;

  if (FileName == NULL) {
    return NULL;
  }

  if (Dir == NULL || Extend == NULL) {
    return FileName;
  }

  RetStr = (CHAR8 *) malloc (strlen (FileName) +
                             strlen (Dir) +
                             strlen (Extend) +
                             strlen ("%s%s.%s") +
                             1);
  if (RetStr == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    return NULL;
  }

  memset (RetStr, '\0', (strlen (FileName) + strlen (Dir) + strlen (Extend) + strlen ("%s%s.%s") + 1));

  sprintf (RetStr, "%s%s.%s", Dir, FileName, Extend);

  return RetStr;
}

/**
  Delete a directory and files in it.

  @param[in]   DirName   Name of the directory need to be deleted.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
LibRmDir (
  IN  CHAR8*  DirName
)
{
  CHAR8*                 SystemCommand;

  SystemCommand             = NULL;


  if (DirName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (access (DirName, 0) == -1){
    return EFI_SUCCESS;
  }

  //
  // Delete a directory and files in it.
  //

  SystemCommand = malloc (
    strlen (RMDIR_STR) +
    strlen (DirName)     +
    1
    );
   if (SystemCommand == NULL) {
     Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
     return EFI_ABORTED;
   }

  sprintf (
    SystemCommand,
    RMDIR_STR,
    DirName
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
    free(SystemCommand);
    return EFI_ABORTED;
  }
  free(SystemCommand);

  return EFI_SUCCESS;
}

EFI_STATUS
LibGenExtFile(
CONST EFI_FIRMWARE_VOLUME_EXT_HEADER *ExtPtr,
FILE *InfFile
)
{
  CHAR8   *TempDir;
  FILE    *ExtFile;
  CHAR8   OutputExtFile[_MAX_PATH];
  CHAR8   Line[512];
  size_t  Len;

  TempDir = NULL;

  TempDir = getcwd(NULL, _MAX_PATH);
  if (strlen (TempDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    Error("FMMT", 0, 1001,  "The directory is too long.", "");
    return EFI_ABORTED;
  }
  strncat (TempDir, OS_SEP_STR, _MAX_PATH - strlen (TempDir) -1);
  strncat (TempDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TempDir) -1);

  mkdir(TempDir, S_IRWXU | S_IRWXG | S_IRWXO);

  sprintf(
    Line,
    "%c%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X%d.ext",
    OS_SEP,
    (unsigned)ExtPtr->FvName.Data1,
    ExtPtr->FvName.Data2,
    ExtPtr->FvName.Data3,
    ExtPtr->FvName.Data4[0],
    ExtPtr->FvName.Data4[1],
    ExtPtr->FvName.Data4[2],
    ExtPtr->FvName.Data4[3],
    ExtPtr->FvName.Data4[4],
    ExtPtr->FvName.Data4[5],
    ExtPtr->FvName.Data4[6],
    ExtPtr->FvName.Data4[7],
    ExtPtr->ExtHeaderSize
    );
  if (strlen (TempDir) + strlen (Line) > _MAX_PATH - 1) {
    Error("FMMT", 0, 1001,  "The directory is too long.", "");
    return EFI_ABORTED;
  }
  strncpy (OutputExtFile, TempDir, _MAX_PATH - 1);
  OutputExtFile[_MAX_PATH - 1] = 0;
  strncat (OutputExtFile, Line, _MAX_PATH - strlen (OutputExtFile) - 1);


  ExtFile = fopen(OutputExtFile, "wb+");
  if (ExtFile == NULL) {
    return EFI_ABORTED;
  }

  if (fwrite(ExtPtr, 1, ExtPtr->ExtHeaderSize, ExtFile) != ExtPtr->ExtHeaderSize) {
    fclose(ExtFile);
    return EFI_ABORTED;
  }

  fclose(ExtFile);

  strcpy (Line, "EFI_FV_EXT_HEADER_FILE_NAME = ");
  if (strlen (Line) + strlen (OutputExtFile) + 1 > sizeof(Line) / sizeof (CHAR8) - 1) {
    Error("FMMT", 0, 1001,  "The directory is too long.", "");
    return EFI_ABORTED;
  }
  strncat (Line, OutputExtFile, sizeof(Line) / sizeof (CHAR8) - strlen (Line) - 1);
  strncat (Line, "\n", sizeof(Line) / sizeof (CHAR8) - strlen (Line) - 1);
  Len = strlen(Line);
  if (fwrite(Line, 1, Len, InfFile) != Len) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Delete a file.

  @param[in]   FileName   Name of the file need to be deleted.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
LibFmmtDeleteFile(
  IN   CHAR8    *FileName
)
{
  CHAR8*                 SystemCommand;
  CHAR8                  *TemDir;

  SystemCommand             = NULL;
  TemDir                    = NULL;


  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // if the FileName is not in TemDir, we don't need to delete.
  TemDir = getcwd (NULL, _MAX_PATH);
  if (*(TemDir + strlen(TemDir) - 1) == OS_SEP) {
    *(TemDir + strlen(TemDir) - 1) = '\0';
  }
  if (strlen (TemDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    Error (NULL, 0, 2000, "Path: The current path is too long.", NULL);
    return EFI_ABORTED;
  }
  strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen (TemDir) - 1);
  strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TemDir) - 1);
  if (strstr(FileName, TemDir) == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Delete a file
  //

  SystemCommand = malloc (
    strlen (DEL_STR) +
    strlen (FileName)     +
    1
    );
   if (SystemCommand == NULL) {
     Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
     return EFI_ABORTED;
   }

  sprintf (
    SystemCommand,
    DEL_STR,
    FileName
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
    free(SystemCommand);
    return EFI_ABORTED;
  }
  free(SystemCommand);

  return EFI_SUCCESS;

}


/**

  Free the whole Fd data structure.

  @param[in]  Fd  The pointer point to the Fd data structure.

**/
VOID
LibFmmtFreeFd (
  FIRMWARE_DEVICE *Fd
  )
{
  FV_INFORMATION   *CurrentFv;
  FV_INFORMATION   *TempFv;
  ENCAP_INFO_DATA  *EncapData1;
  ENCAP_INFO_DATA  *EncapData2;

  CurrentFv        = NULL;
  TempFv           = NULL;
  EncapData1       = NULL;
  EncapData2       = NULL;

  if (Fd == NULL) {
    return;
  }

  CurrentFv = Fd->Fv;

  do {
    TempFv = CurrentFv;
    CurrentFv = CurrentFv->FvNext;

    free (TempFv->FvHeader);

    if (TempFv->FvExtHeader != NULL) {
      free (TempFv->FvExtHeader);
    }
    if (TempFv->FvUiName) {
      free(TempFv->FvUiName);
    }

    //
    // Free encapsulate data;
    //
    EncapData1 = TempFv->EncapData;

    while (EncapData1 != NULL) {

      EncapData2 = EncapData1;
      EncapData1 = EncapData1->NextNode;

      if (EncapData2->Data != NULL) {
        free (EncapData2->Data);
      }
    if (EncapData2->FvExtHeader != NULL) {
      free(EncapData2->FvExtHeader);
    }
      free (EncapData2);
      EncapData2 = NULL;
    }

    EncapData1 = NULL;

    free (TempFv);
    TempFv = NULL;

  } while (CurrentFv != NULL);

  CurrentFv = NULL;
  free (Fd);
  Fd = NULL;

  return;
}

/**
  Generate the compressed section with specific type.
  Type could be EFI_STANDARD_COMPRESSION or EFI_NOT_COMPRESSED

  @param[in]  InputFileName    File name of the raw data.
  @param[in]  OutPutFileName   File name of the sectioned data.
  @param[in]  CompressionType  The compression type.

  @return  EFI_INVALID_PARAMETER
  @return  EFI_ABORTED
  @return  EFI_OUT_OF_RESOURCES
  @return  EFI_SUCCESS

**/
EFI_STATUS
LibGenCompressedSection (
  CHAR8         *InputFileName,
  CHAR8         *OutPutFileName,
  UINT8         CompressionType
)
{
  //FILE                        *UnCompressFile;
  //FILE                        *CompressedFile;
  //VOID                        *UnCompressedBuffer;
  //VOID                        *CompressedBuffer;
  //UINT32                      UnCompressedSize;
  //UINT32                      CompressedSize;
  //CHAR8                       *TempName;
  //CHAR8                       *TemDir;
  //EFI_STATUS                  Status;

  //UnCompressFile     = NULL;
  //CompressedFile     = NULL;
  //UnCompressedBuffer = NULL;
  //CompressedBuffer   = NULL;
  //TempName           = NULL;
  //TemDir             = NULL;
  //UnCompressedSize   = 0;
  //CompressedSize     = 0;

  if ( InputFileName == NULL ||
       OutPutFileName == NULL) {
    printf ("Error while generate compressed section!\n");
    return EFI_INVALID_PARAMETER;
  }

  if (CompressionType == EFI_STANDARD_COMPRESSION) {
    /*

    UnCompressFile = fopen (InputFileName, "rb");
    if (UnCompressFile == NULL) {
      printf ("Error while open file %s \n", InputFileName);
      return EFI_ABORTED;
    }

    TemDir = _getcwd (NULL, _MAX_PATH);
    sprintf(TemDir, "%s\\%s", TemDir, TEMP_DIR_NAME);

    TempName= LibFilenameStrExtended (strrchr(CloneString (tmpnam (NULL)),'\\'), TemDir, "comp");

    CompressedFile = fopen (TempName, "wb+");
    if (CompressedFile == NULL) {
      printf ("Error while open file %s \n", TempName);
      return EFI_ABORTED;
    }

    //
    // Get the original file size;
    //
    fseek(UnCompressFile,0,SEEK_SET);
    fseek(UnCompressFile,0,SEEK_END);

    UnCompressedSize = ftell(UnCompressFile);

    fseek(UnCompressFile,0,SEEK_SET);

    UnCompressedBuffer = malloc (UnCompressedSize);

    if (UnCompressedBuffer == NULL) {
      printf("Error while allocate memory! \n");
      return EFI_OUT_OF_RESOURCES;
    }

    CompressedBuffer = malloc (UnCompressedSize);

    if (CompressedBuffer == NULL) {
      printf("Error while allocate memory! \n");
      return EFI_OUT_OF_RESOURCES;
    }

    if (fread (UnCompressedBuffer, 1, (size_t) UnCompressedSize, UnCompressFile) == (size_t) UnCompressedSize) {
      CompressedSize = UnCompressedSize;

      Status = EfiCompress ( UnCompressedBuffer,
                             UnCompressedSize,
                             CompressedBuffer,
                             &CompressedSize);

      if (EFI_ERROR(Status)) {
        printf("Error while do compress operation! \n");
        return EFI_ABORTED;
      }

      if (CompressedSize > UnCompressedSize) {
        printf("Error while do compress operation! \n");
        return EFI_ABORTED;
      }
    } else {
      printf("Error while reading file %s! \n", InputFileName);
      return EFI_ABORTED;
    }

    //
    // Write the compressed data into output file
    //
    if (fwrite (CompressedBuffer, 1, (size_t) CompressedSize, CompressedFile) != (size_t) CompressedSize) {
      Error ("FMMT", 0, 0004, "error writing %s file", OutPutFileName);
      fclose(UnCompressFile);
      fclose (CompressedFile);
      return EFI_ABORTED;
    }

    fclose(UnCompressFile);
    fclose (CompressedFile);
    */

    //
    // Call GenSec tool to generate the compressed section.
    //
    LibCreateFfsSection(NULL, InputFileName, NULL, EFI_SECTION_COMPRESSION, OutPutFileName, NULL, NULL, NULL, 0, 0, "PI_STD");

  } else if (CompressionType == EFI_NOT_COMPRESSED) {

      LibCreateFfsSection(NULL, InputFileName, NULL, EFI_SECTION_COMPRESSION, OutPutFileName, NULL, NULL, NULL, 0, 0, "PI_NONE");

  } else {
    printf ("Error while generate compressed section, unknown compression type! \n");
    return EFI_INVALID_PARAMETER;
  }


  return EFI_SUCCESS;
}

EFI_STATUS
LibEncapNewFvFile(
  IN  FV_INFORMATION   *FvInFd,
  IN  CHAR8            *TemDir,
  IN  ENCAP_INFO_DATA  *CurrentEncapData,
  IN  UINT32           Level_Break,
  OUT FFS_INFORMATION  **OutputFile
)
{
  EFI_STATUS                  Status;
  UINT32                      ParentType;
  UINT8                       ParentLevel;
  UINT32                      Type;
  UINT8                       Level;
  CHAR8                       *InfFileName;
  FILE                        *InfFile;
  ENCAP_INFO_DATA             *LocalEncapData;
  ENCAP_INFO_DATA             *LocalEncapDataTemp;
  ENCAP_INFO_DATA             *LocalEncapDataNext;
  BOOLEAN                     FfsFoundFlag;
  UINT32                      Index;
  UINT32                      OuterIndex;
  CHAR8                       *ExtractionTool;
  BOOLEAN                     IsLastLevelFfs;
  BOOLEAN                     IsLeafFlagIgnore;
  BOOLEAN                     FirstInFlag;
  BOOLEAN                     OutputFileNameListFlag;
  CHAR8                       *InputFileName;
  CHAR8                       *OutputFileName;
  FFS_INFORMATION             *OutputFileNameList;
  FFS_INFORMATION             *ChildFileNameList;
  FFS_INFORMATION             *NewFileNameList;
  CHAR8                       *FvGuidName;
  UINT16                      GuidAttributes;
  UINT16                      GuidDataOffset;
  BOOLEAN                     IsRootFv;
  BOOLEAN                     IsLargeFile;
  UINT32                      EncapFvStart;
  UINT32                      EncapFvIndex;
  CHAR8                       *TmpFileName;
  FILE                        *TmpFile;
  FILE                        *InputFile;
  FILE                        *OutFile;
  UINT32                      InputFileSize;
  UINT32                      OutputFileSize;
  UINT32                      LargeFileSize;
  UINT8                       *Buffer = NULL;
  UINT8                       SectionHeader[4] = { 0x00, 0x00, 0x00, 0x00 };
  UINT32                      Id;
  UINT32                      SubFvId;
  UINT32                      header;
  UINT8                       AlignN;
  UINT8                       AlignV[1] = {0xFF};
  AlignN                      = 0;
  Id                          = 0;
  InputFileSize               = 0;
  EncapFvIndex                = 0;
  Index                       = 0;
  OuterIndex                  = 0;
  ParentType                  = 0;
  ParentLevel                 = 0;
  Type                        = 0;
  Level                       = 0;
  SubFvId                     = 0;
  FfsFoundFlag                = FALSE;
  LocalEncapDataTemp          = NULL;
  LocalEncapDataNext          = NULL;
  ExtractionTool              = NULL;
  InputFileName               = NULL;
  OutputFileName              = NULL;
  IsLastLevelFfs              = TRUE;
  IsLeafFlagIgnore            = FALSE;
  FirstInFlag                 = TRUE;
  FvGuidName                  = NULL;
  OutputFileNameListFlag      = TRUE;
  IsLargeFile                 = FALSE;
  OutputFileSize              = 0;
  LargeFileSize               = 0x1000000;


  OutputFileNameList = (FFS_INFORMATION *)malloc(sizeof(FV_INFORMATION));
  if (OutputFileNameList == NULL) {
    Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
    return EFI_OUT_OF_RESOURCES;
  }
  OutputFileNameList->FFSName = NULL;
  OutputFileNameList->InFvId = 0;
  OutputFileNameList->IsFFS = FALSE;
  OutputFileNameList->ParentLevel = 0;
  OutputFileNameList->Next = NULL;
  OutputFileNameList->UiNameSize = 0;
  OutputFileNameList->Depex = NULL;
  OutputFileNameList->DepexLen = 0;
  OutputFileNameList->FfsFoundFlag = FALSE;

  ChildFileNameList = (FFS_INFORMATION *)malloc(sizeof(FV_INFORMATION));
  if (ChildFileNameList == NULL) {
    Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
    return EFI_OUT_OF_RESOURCES;
  }
  ChildFileNameList->FFSName = NULL;
  ChildFileNameList->InFvId = 0;
  ChildFileNameList->ParentLevel = 0;
  ChildFileNameList->Next = NULL;
  ChildFileNameList->IsFFS = FALSE;
  ChildFileNameList->UiNameSize = 0;
  ChildFileNameList->Depex = NULL;
  ChildFileNameList->DepexLen = 0;
  ChildFileNameList->FfsFoundFlag = FALSE;
  //
  // Encapsulate from the lowest FFS file level.
  //
    LocalEncapData = CurrentEncapData;
    if (LocalEncapData == NULL) {
        LocalEncapData = FvInFd->EncapData;
    }
    Level = LocalEncapData->Level;
    Type = LocalEncapData->Type;

  if (CurrentEncapData == NULL) {
    LocalEncapData = FvInFd->EncapData;
    while (LocalEncapData != NULL) {
      if (LocalEncapData->Type == FMMT_ENCAP_TREE_FFS) {
        LocalEncapDataTemp = LocalEncapData->RightNode;
        while (LocalEncapDataTemp != NULL) {
            LocalEncapDataNext = LocalEncapDataTemp->NextNode;
            if (LocalEncapDataNext != NULL && LocalEncapDataNext->NextNode != NULL) {

                LibEncapNewFvFile(FvInFd, TemDir, LocalEncapDataTemp, 1, &ChildFileNameList);
                ChildFileNameList->ParentLevel = LocalEncapDataTemp->Level -1;
                if (FvInFd->ChildFvFFS == NULL) {
                    FvInFd->ChildFvFFS = ChildFileNameList;
                } else {
                    NewFileNameList = FvInFd->ChildFvFFS;
                    while (NewFileNameList->Next != NULL) {
                        NewFileNameList = NewFileNameList->Next;
                    }
                    NewFileNameList->Next = ChildFileNameList;
                }
            }
            LocalEncapDataTemp = LocalEncapDataTemp->RightNode;
        }
      }

      if (LocalEncapData->Level > Level) {
        if (LocalEncapData->Type == FMMT_ENCAP_TREE_FFS) {
            ParentLevel = Level;
            ParentType  = Type;
        }
        Level       = LocalEncapData->Level;
        Type        = LocalEncapData->Type;
      }
      LocalEncapData = LocalEncapData->NextNode;
    }
  } else {
    LocalEncapData = CurrentEncapData;
    while (LocalEncapData != NULL) {
        if (LocalEncapData->Level > Level) {
            if (LocalEncapData->Type == FMMT_ENCAP_TREE_FFS) {
                ParentLevel = Level;
                ParentType  = Type;
            }
            Level       = LocalEncapData->Level;
            Type        = LocalEncapData->Type;
        }
        LocalEncapData = LocalEncapData->NextNode;
    }
  }

  do {
    switch (ParentType) {
      case FMMT_ENCAP_TREE_FV:
        OutputFileNameListFlag = TRUE;
        EncapFvStart = 0;
    for(OuterIndex=0;OutputFileNameListFlag;OuterIndex++){
        //
        // Generate FV.inf attributes.
        //
        InfFileName = LibFilenameStrExtended (strrchr(GenTempFile (),OS_SEP), TemDir, "inf");
    FirstInFlag = TRUE;

        InfFile = fopen (InfFileName, "wt+");

        if (InfFile == NULL) {
          Error ("FMMT", 0, 0004, "Could not open inf file %s to store FV information! \n", "");
          free (OutputFileNameList);
          free (ChildFileNameList);
          return EFI_ABORTED;
        }

        if (CurrentEncapData == NULL) {
           LocalEncapData = FvInFd->EncapData;
        } else {
            LocalEncapData = CurrentEncapData;
        }

        while (LocalEncapData->NextNode != NULL) {
          if (LocalEncapData->Level == ParentLevel) {
             break;
          }
          LocalEncapData = LocalEncapData->NextNode;
        }

        if (((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset != 0) {
          //
          // FV GUID Name memory allocation
          //
          FvGuidName = (CHAR8 *) malloc (255);

          if (FvGuidName == NULL) {
            Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
            fclose (InfFile);
            free (OutputFileNameList);
            free (ChildFileNameList);
            return EFI_ABORTED;
          }

          memset(FvGuidName, '\0', 255);

          sprintf(
            FvGuidName,
            "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
            LocalEncapData->FvExtHeader->FvName.Data1,
            LocalEncapData->FvExtHeader->FvName.Data2,
            LocalEncapData->FvExtHeader->FvName.Data3,
            LocalEncapData->FvExtHeader->FvName.Data4[0],
            LocalEncapData->FvExtHeader->FvName.Data4[1],
            LocalEncapData->FvExtHeader->FvName.Data4[2],
            LocalEncapData->FvExtHeader->FvName.Data4[3],
            LocalEncapData->FvExtHeader->FvName.Data4[4],
            LocalEncapData->FvExtHeader->FvName.Data4[5],
            LocalEncapData->FvExtHeader->FvName.Data4[6],
            LocalEncapData->FvExtHeader->FvName.Data4[7]
          );

        } else {
          FvGuidName = NULL;
        }


        if (ParentLevel == 1) {
          Status = LibFvHeaderOptionToStr((EFI_FIRMWARE_VOLUME_HEADER *)LocalEncapData->Data, InfFile, TRUE);
        } else {
          Status = LibFvHeaderOptionToStr((EFI_FIRMWARE_VOLUME_HEADER *)LocalEncapData->Data, InfFile, FALSE);
        }


        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "generate FV INF file [Options] section failed.");
          fclose (InfFile);
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }

        Status = LibFvHeaderAttributeToStr(((EFI_FIRMWARE_VOLUME_HEADER *)LocalEncapData->Data)->Attributes, InfFile);

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV header attribute failed");
          fclose (InfFile);
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }
        if (LocalEncapData->FvExtHeader != NULL) {
          Status = LibGenExtFile(LocalEncapData->FvExtHeader, InfFile);
          if (EFI_ERROR(Status)) {
            Error("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV EXT header failed");
            fclose (InfFile);
            free (OutputFileNameList);
            free (ChildFileNameList);
            return Status;
          }
          FvGuidName = NULL;
        }

        if (CurrentEncapData != NULL) {
            for (Index = 0; Index <= FvInFd->FfsNumbers; Index++) {
                if ((memcmp(&FvInFd->FfsAttuibutes[Index].GuidName, &(CurrentEncapData->FvExtHeader->FvName), sizeof(EFI_GUID)) == 0)) {
                    SubFvId = Index;
                    break;
                }
            }
        }
        //
        // Found FFSs from Fv structure.
        //
        FfsFoundFlag = FALSE;
        IsRootFv = FALSE;
        for (Index=0; Index <= FvInFd->FfsNumbers; Index++) {
            if (OutputFileNameList != NULL && OutputFileNameList->FFSName != NULL && OutputFileNameList->IsFFS == FALSE){
                break;
            }
            if (OutputFileNameList != NULL && OutputFileNameList->FFSName != NULL && OutputFileNameList->IsFFS == TRUE){
                if (Index == EncapFvIndex) {
                    if (FirstInFlag) {
                            Status = LibAddFfsFileToFvInf (OutputFileNameList->FFSName, InfFile, TRUE);
                            FirstInFlag = FALSE;
                        } else {
                            Status = LibAddFfsFileToFvInf (OutputFileNameList->FFSName, InfFile, FALSE);
                        }
                        if (EFI_ERROR (Status)) {
                            Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV inf file [files] section failed!");
                            fclose (InfFile);
                            free (OutputFileNameList);
                            free (ChildFileNameList);
                            return Status;
                        }
                }
            }

            NewFileNameList = FvInFd->ChildFvFFS;
            while (NewFileNameList != NULL && NewFileNameList -> FFSName != NULL) {
                if (NewFileNameList -> ParentLevel == ParentLevel && Index == NewFileNameList->InFvId && NewFileNameList->FfsFoundFlag==TRUE) {
                    if (FirstInFlag) {
                        Status = LibAddFfsFileToFvInf (NewFileNameList->FFSName, InfFile, TRUE);
                        FirstInFlag = FALSE;
                    } else {
                        Status = LibAddFfsFileToFvInf (NewFileNameList->FFSName, InfFile, FALSE);
                    }
                    if (EFI_ERROR (Status)) {
                        Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV inf file [files] section failed!");
                        fclose (InfFile);
                        free (OutputFileNameList);
                        free (ChildFileNameList);
                        return Status;
                    }
                }
                NewFileNameList = NewFileNameList->Next;
            }

            if (FvInFd->FfsAttuibutes[Index].IsHandle==TRUE) {
                continue;
            }
            if (SubFvId > 0 && Index < SubFvId) {
                continue;
            }

          //
          // For the last level FFS, the level below FFSs we should not care the IsLeaf Flag.
          //
          if (IsLastLevelFfs) {
            IsLeafFlagIgnore = TRUE;
            } else {
              IsLeafFlagIgnore = FvInFd->FfsAttuibutes[Index].IsLeaf;
            }

          if (FvInFd->FfsAttuibutes[Index].Level >= ParentLevel + 1 && IsLeafFlagIgnore) {
            if (FirstInFlag) {
        if (FvInFd->FfsAttuibutes[Index].Level < 0xFF) {
          FfsFoundFlag = TRUE;
          Status = LibAddFfsFileToFvInf (FvInFd->FfsAttuibutes[Index].FfsName, InfFile, TRUE);
          FirstInFlag = FALSE;
          FvInFd->FfsAttuibutes[Index].IsHandle=TRUE;
          EncapFvStart = Index;
        }

              if (EFI_ERROR (Status)) {
                  Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV inf file [files] section failed!");
                  fclose (InfFile);
                  free (OutputFileNameList);
                  free (ChildFileNameList);
                  return Status;
              }
              if (Index == 0) {
                // Root FV need to include all FFS files.
                IsRootFv = TRUE;
              }
            } else {
          if (FvInFd->FfsAttuibutes[Index].Level < 0xFF) {
          FfsFoundFlag = TRUE;
          Status = LibAddFfsFileToFvInf (FvInFd->FfsAttuibutes[Index].FfsName, InfFile, FALSE);
          FvInFd->FfsAttuibutes[Index].IsHandle=TRUE;
        }

                if (EFI_ERROR (Status)) {
                  Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV inf file [files] section failed!");
                  fclose (InfFile);
                  free (OutputFileNameList);
                  free (ChildFileNameList);
                  return Status;
                }
                if (Index == 0) {
                  // Root FV need to include all FFS files.
                  IsRootFv = TRUE;
                }
              }


      //avoid a FV contain too many ffs files
           if ((!IsRootFv) && (FvInFd->FfsAttuibutes[Index].FvLevel <= FvInFd->MulFvLevel) && (FvInFd->FfsAttuibutes[Index+1].FvLevel <= FvInFd->MulFvLevel) &&
               (FvInFd->FfsAttuibutes[Index].FvLevel != FvInFd->FfsAttuibutes[Index+1].FvLevel) && (ParentLevel != 1) && (FvInFd->FfsAttuibutes[Index].Level != FvInFd->FfsAttuibutes[Index+1].Level) &&
               FvInFd->FfsAttuibutes[Index].Level != 0xFF && FvInFd->FfsAttuibutes[Index+1].Level != 0xFF && FvInFd->FfsAttuibutes[Index+1].Level != 0x0){
        FvInFd->FfsAttuibutes[Index].Level = 0;
        break;
      }else{
        if (FvInFd->FfsAttuibutes[Index].Level != 0xFF){
          FvInFd->FfsAttuibutes[Index].Level = 0;
        }
      }

            }
          }
       // The Fv may has multiple level (> 2), when it is in the FvLevel == 2, we set the IsLastLevelFfs Flag
       if (Index <=FvInFd->FfsNumbers && FvInFd->FfsAttuibutes[Index].FvLevel <= FvInFd->MulFvLevel) {
           if (FvInFd->FfsAttuibutes[Index].FvLevel == 2) {
               IsLastLevelFfs = FALSE;
           }
       }
    if (!FfsFoundFlag){
      OutputFileNameListFlag = FALSE;
      if (OuterIndex > 0){
        fclose (InfFile);
        break;
      }
    }
        //
        // Create FV
        //
        fclose (InfFile);

        EncapFvIndex = EncapFvStart;

        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "FV");

        Status = LibEncapsulateFfsToFv (InfFileName, NULL, OutputFileName, FvGuidName, IsLargeFile);

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV failed!");
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }

    OutputFileNameList->FFSName = (char *)malloc(strlen(OutputFileName)+1);
    if (OutputFileNameList->FFSName == NULL) {
      Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
      return EFI_OUT_OF_RESOURCES;
    }
    memcpy((char *)OutputFileNameList->FFSName, (char *)OutputFileName, strlen(OutputFileName)+1);
      if (CurrentEncapData != NULL) {
        OutputFileNameList->InFvId = EncapFvIndex;
        if (EncapFvIndex > 0) {
            memcpy(OutputFileNameList->UiName,FvInFd->FfsAttuibutes[EncapFvIndex - 1].UiName, FvInFd->FfsAttuibutes[EncapFvIndex - 1].UiNameSize);
            OutputFileNameList->UiNameSize = FvInFd->FfsAttuibutes[EncapFvIndex - 1].UiNameSize;
            OutputFileNameList->Depex = FvInFd->FfsAttuibutes[EncapFvIndex - 1].Depex;
            OutputFileNameList->DepexLen = FvInFd->FfsAttuibutes[EncapFvIndex - 1].DepexLen;
            OutputFileNameList->FfsFoundFlag = FfsFoundFlag;
        }
      }
    }
        break;
      case FMMT_ENCAP_TREE_FFS:

        while(OutputFileNameList!= NULL && OutputFileNameList->FFSName != NULL){
          InputFileName  = OutputFileNameList->FFSName;
          OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "ffs");
          LocalEncapData = CurrentEncapData;
          if (LocalEncapData == NULL) {
            LocalEncapData = FvInFd->EncapData;
          }
          while (LocalEncapData->NextNode != NULL) {
            if (LocalEncapData->Level == ParentLevel) {
        for(;LocalEncapData->NextNode != NULL;) {
          if(LocalEncapData->FvExtHeader != NULL) {
            break;
          }
          LocalEncapData = LocalEncapData->NextNode;
        }
                break;
            }
            LocalEncapData = LocalEncapData->NextNode;
          }

          if (LocalEncapData->FvExtHeader == NULL) {
            Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FFS file failed!");
            free (OutputFileNameList);
            free (ChildFileNameList);
            return EFI_ABORTED;
          }

          if (OutputFileNameList->UiNameSize > 0) {
              TmpFileName = LibFilenameStrExtended(strrchr(GenTempFile (), OS_SEP), TemDir, "tmp");
              TmpFile = fopen(TmpFileName, "wb+");
              if (TmpFile == NULL) {
                 Error("FMMT", 0, 0004, "Could not open tmp file %s to store UI section information! \n", "");
                 free (OutputFileNameList);
                 free (ChildFileNameList);
                 return EFI_ABORTED;
              }
             header = (OutputFileNameList->UiNameSize+4) | (EFI_SECTION_USER_INTERFACE << 24);
             Index = 0;
             while (header) {
                 SectionHeader[Index] = header % 0x100;
                 header /= 0x100;
                 Index ++;
             }
             InputFile = fopen(InputFileName, "rb+");
             if (InputFile == NULL) {
               Error("FMMT", 0, 0004, "Could not open input file %s! \n", "");
               fclose(TmpFile);
               free (OutputFileNameList);
               free (ChildFileNameList);
               return EFI_ABORTED;
             }
             fseek(InputFile, 0, SEEK_SET);
             fseek(InputFile, 0, SEEK_END);
             InputFileSize = ftell(InputFile);
             fseek(InputFile, 0, SEEK_SET);

             Buffer = malloc(InputFileSize+OutputFileNameList->UiNameSize+4);
             memcpy(Buffer, (CHAR16 *)SectionHeader, 4);
             memcpy(Buffer + 4, (CHAR16 *)(OutputFileNameList->UiName), OutputFileNameList->UiNameSize);
             if (fread(Buffer+4+OutputFileNameList->UiNameSize, 1, InputFileSize, InputFile) != InputFileSize) {
               Error("FMMT", 0, 0004, "Could not open sec file %s to add UI section information! \n", "");
               fclose(TmpFile);
               fclose(InputFile);
               free(Buffer);
               free (OutputFileNameList);
               free (ChildFileNameList);
               return EFI_ABORTED;
             }
             fwrite(Buffer, 1, InputFileSize + OutputFileNameList->UiNameSize + 4, TmpFile);
             free(Buffer);
             fclose(TmpFile);
             fclose(InputFile);
             InputFileName = TmpFileName;
          }
          if (OutputFileNameList->DepexLen > 0) {
              TmpFileName = LibFilenameStrExtended(strrchr(GenTempFile (), OS_SEP), TemDir, "tmp");
              TmpFile = fopen(TmpFileName, "wb+");
              if (TmpFile == NULL) {
                  Error("FMMT", 0, 0004, "Could not open tmp file %s to store Depex section information! \n", "");
                  free (OutputFileNameList);
                  free (ChildFileNameList);
                  return EFI_ABORTED;
              }
              InputFile = fopen(InputFileName, "rb+");
              if (InputFile == NULL) {
                Error("FMMT", 0, 0004, "Could not open input file %s! \n", "");
                fclose(TmpFile);
                free (OutputFileNameList);
                free (ChildFileNameList);
                return EFI_ABORTED;
              }
              fseek(InputFile, 0, SEEK_SET);
              fseek(InputFile, 0, SEEK_END);
              InputFileSize = ftell(InputFile);
              fseek(InputFile, 0, SEEK_SET);
              // make sure the section is 4 byte align
              if (OutputFileNameList->DepexLen % 4 != 0) {
                  AlignN = 4 - OutputFileNameList->DepexLen % 4;
              }
              Buffer = malloc(InputFileSize + OutputFileNameList->DepexLen + AlignN);
              memcpy(Buffer, OutputFileNameList->Depex, OutputFileNameList->DepexLen);
              if (AlignN != 0) {
                  for (Index = 0; Index < AlignN; Index ++) {
                      memcpy(Buffer + OutputFileNameList->DepexLen + Index, AlignV, 1);
                  }
              }
              if (fread(Buffer + OutputFileNameList->DepexLen + AlignN, 1, InputFileSize, InputFile) != InputFileSize) {
                  Error("FMMT", 0, 0004, "Could not open sec file %s to add Depex section information! \n", "");
                  fclose(TmpFile);
                  fclose(InputFile);
                  free(Buffer);
                  free (OutputFileNameList);
                  free (ChildFileNameList);
                  return EFI_ABORTED;
              }
              fwrite(Buffer, 1, InputFileSize + OutputFileNameList->DepexLen + AlignN, TmpFile);
              free(Buffer);
              fclose(TmpFile);
              fclose(InputFile);
              InputFileName = TmpFileName;
         }
         for (Id = FvInFd->FfsNumbers; Id <= FvInFd->FfsNumbers; Id--) {
             if ((memcmp(&FvInFd->FfsAttuibutes[Id].GuidName, &(LocalEncapData->FvExtHeader->FvName), sizeof(EFI_GUID)) == 0)){
                 if (access(FvInFd->FfsAttuibutes[Id].FfsName, 0) != -1) {
                     Status = LibFmmtDeleteFile(FvInFd->FfsAttuibutes[Id].FfsName);
                     if (EFI_ERROR(Status)) {
                         Error("FMMT", 0, 0004, "error while encapsulate FD Image", "Delete the specified file failed!");
                         free (OutputFileNameList);
                         free (ChildFileNameList);
                         return Status;
                     }
                     memset(FvInFd->FfsAttuibutes[Id].FfsName, '\0', _MAX_PATH);
                     FvInFd->FfsAttuibutes[Id].Level = 0xFF;
                     break;
                 }
             }
         }
         if (LocalEncapData->NextNode != NULL) {
             LocalEncapDataTemp = LocalEncapData->NextNode;
             if ((LocalEncapDataTemp->Type == FMMT_ENCAP_TREE_GUIDED_SECTION) || (LocalEncapDataTemp->Type == FMMT_ENCAP_TREE_COMPRESS_SECTION)) {
                 Status = LibEncapSectionFileToFFS(EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, InputFileName, OutputFileName, LocalEncapData->FvExtHeader->FvName, FALSE, "1");
             }
             else{
                 Status = LibEncapSectionFileToFFS(EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, InputFileName, OutputFileName, LocalEncapData->FvExtHeader->FvName, FALSE, FvInFd->AlignmentStr);
             }
         }
         else{
             Status = LibEncapSectionFileToFFS(EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, InputFileName, OutputFileName, LocalEncapData->FvExtHeader->FvName, FALSE, FvInFd->AlignmentStr);
         }
      if (EFI_ERROR (Status)) {
        Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FFS file failed!");
        free (OutputFileNameList);
        free (ChildFileNameList);
        return Status;
      }
      free(LocalEncapData->FvExtHeader);
      LocalEncapData->FvExtHeader = NULL;
      OutputFileNameList->FFSName = (char *)malloc(strlen(OutputFileName)+1);
      if (OutputFileNameList->FFSName == NULL) {
          Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
          return EFI_OUT_OF_RESOURCES;
      }
      memcpy((char *)OutputFileNameList->FFSName, (char *)OutputFileName, strlen(OutputFileName)+1);
      OutputFileNameList->IsFFS = TRUE;
      if (OutputFileNameList->Next == NULL){
          break;
      }
      OutputFileNameList = OutputFileNameList->Next;
    }
        break;
      case FMMT_ENCAP_TREE_GUIDED_SECTION:
      while(OutputFileNameList!= NULL && OutputFileNameList->FFSName != NULL){
        //
        // Create the guided section original data, do compress operation.
        //
        InputFileName  = OutputFileNameList->FFSName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "compressed");

        //
        // Use the guided section header guid to find out compress application name.
        //
        LocalEncapData = CurrentEncapData;
          if (LocalEncapData == NULL) {
            LocalEncapData = FvInFd->EncapData;
        }
        while (LocalEncapData->NextNode != NULL) {
          if (LocalEncapData->Level == ParentLevel) {
            break;
          }
          LocalEncapData = LocalEncapData->NextNode;
        }

        ExtractionTool =
          LookupGuidedSectionToolPath (
            mParsedGuidedSectionTools,
            (EFI_GUID *)LocalEncapData->Data
            );
        GuidDataOffset = *(UINT16 *) ((UINT8 *) LocalEncapData->Data + sizeof (EFI_GUID));
        GuidAttributes = *(UINT16 *) ((UINT8 *) LocalEncapData->Data + sizeof (EFI_GUID) + sizeof (UINT16));

        Status = LibCreateGuidedSectionOriginalData (InputFileName, ExtractionTool, OutputFileName);

        if (EFI_ERROR (Status) || GuidDataOffset < sizeof (EFI_GUID_DEFINED_SECTION)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Compress guided data failed!");
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }

        GuidDataOffset = GuidDataOffset - sizeof (EFI_GUID_DEFINED_SECTION);
        InputFileName  = OutputFileName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "guided");

        Status = LibCreateFfsSection(NULL, InputFileName, NULL, EFI_SECTION_GUID_DEFINED, OutputFileName, NULL, NULL, LibFmmtGuidToStr((EFI_GUID *)LocalEncapData->Data), GuidDataOffset, GuidAttributes, NULL);
        OutFile = fopen(OutputFileName, "rb+");
        if (OutFile == NULL) {
            Error("FMMT", 0, 0004, "Could not open the file %s! \n", "");
            free (OutputFileNameList);
            free (ChildFileNameList);
            return EFI_ABORTED;
        }
        fseek(OutFile, 0, SEEK_SET);
        fseek(OutFile, 0, SEEK_END);
        OutputFileSize = ftell(OutFile);
        fclose(OutFile);
        if (OutputFileSize > LargeFileSize) {
            IsLargeFile = TRUE;
        }
    OutputFileNameList->FFSName = (char *)malloc(strlen(OutputFileName)+1);
    if (OutputFileNameList->FFSName == NULL) {
      Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
      return EFI_OUT_OF_RESOURCES;
    }
    memcpy((char *)OutputFileNameList->FFSName, (char *)OutputFileName, strlen(OutputFileName)+1);

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate guided section failed!");
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }
        if (OutputFileNameList->Next == NULL){
          break;
       }
       OutputFileNameList = OutputFileNameList->Next;
    }
        break;
      case FMMT_ENCAP_TREE_COMPRESS_SECTION:
        while(OutputFileNameList!= NULL && OutputFileNameList->FFSName != NULL){
          InputFileName  = OutputFileNameList->FFSName;

          OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "comsec");
          LocalEncapData = CurrentEncapData;
          if (LocalEncapData == NULL) {
            LocalEncapData = FvInFd->EncapData;
          }
          while (LocalEncapData->NextNode != NULL) {
            if (LocalEncapData->Level == ParentLevel) {
              break;
            }
            LocalEncapData = LocalEncapData->NextNode;
          }

          Status = LibGenCompressedSection (InputFileName, OutputFileName, *(UINT8 *)(LocalEncapData->Data));
      OutputFileNameList->FFSName = (char *)malloc(strlen(OutputFileName)+1);
      if (OutputFileNameList->FFSName == NULL) {
        Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
        return EFI_OUT_OF_RESOURCES;
      }
      memcpy((char *)OutputFileNameList->FFSName, (char *)OutputFileName, strlen(OutputFileName)+1);

          if (EFI_ERROR (Status)) {
            Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate compressed section failed!");
            free (OutputFileNameList);
            free (ChildFileNameList);
            return Status;
          }
          if (OutputFileNameList->Next == NULL){
            break;
          }
          OutputFileNameList = OutputFileNameList->Next;
        }
        break;
      case FMMT_ENCAP_TREE_FV_SECTION:
        while(OutputFileNameList!= NULL && OutputFileNameList->FFSName != NULL){
        InputFileName  = OutputFileNameList->FFSName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "sec");

        Status = LibCreateFfsSection(NULL, InputFileName, NULL, EFI_SECTION_FIRMWARE_VOLUME_IMAGE, OutputFileName, NULL, NULL, NULL, 0, 0, NULL);

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV section failed!");
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }

        InputFileName  = OutputFileName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "sec");

        //
        // Make it alignment.
        //
        Status = LibCreateFfsSection(FvInFd, InputFileName, NULL, 0, OutputFileName, NULL, NULL, NULL, 0, 0, NULL);
        OutFile = fopen(OutputFileName, "rb+");
        if (OutFile == NULL) {
            Error("FMMT", 0, 0004, "Could not open the file %s! \n", "");
            free (OutputFileNameList);
            free (ChildFileNameList);
            return EFI_ABORTED;
        }
        fseek(OutFile, 0, SEEK_SET);
        fseek(OutFile, 0, SEEK_END);
        OutputFileSize = ftell(OutFile);
        fclose(OutFile);
        if (OutputFileSize > LargeFileSize) {
            IsLargeFile = TRUE;
        }

    OutputFileNameList->FFSName = (char *)malloc(strlen(OutputFileName)+1);
    if (OutputFileNameList->FFSName == NULL) {
      Error ("FMMT", 0, 0004, "Out of resource, memory allocation failed! \n", "");
      return EFI_OUT_OF_RESOURCES;
    }
    memcpy((char *)OutputFileNameList->FFSName, (char *)OutputFileName, strlen(OutputFileName)+1);

        if (EFI_ERROR (Status)) {
          Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV section failed!");
          free (OutputFileNameList);
          free (ChildFileNameList);
          return Status;
        }
        if (OutputFileNameList->Next == NULL){
           break;
        }
        OutputFileNameList = OutputFileNameList->Next;
    }
        break;
      default:
        for (Id = FvInFd->FfsNumbers; Id <= FvInFd->FfsNumbers; Id--) {
            if ((memcmp(&FvInFd->FfsAttuibutes[Id].GuidName, &(CurrentEncapData->FvExtHeader->FvName), sizeof(EFI_GUID)) == 0)){
                FvInFd->FfsAttuibutes[Id].IsHandle = TRUE;
                memcpy(OutputFileNameList->UiName, FvInFd->FfsAttuibutes[Id].UiName, FvInFd->FfsAttuibutes[Id].UiNameSize);
                OutputFileNameList->UiNameSize = FvInFd->FfsAttuibutes[Id].UiNameSize;
                OutputFileNameList->FFSName = FvInFd->FfsAttuibutes[Id].FfsName;
                OutputFileNameList->Depex = FvInFd->FfsAttuibutes[Id].Depex;
                OutputFileNameList->DepexLen = FvInFd->FfsAttuibutes[Id].DepexLen;
                OutputFileNameList->FfsFoundFlag = TRUE;
                OutputFileNameList->IsFFS = TRUE;
                OutputFileNameList->InFvId = Id;
                *OutputFile = OutputFileNameList;
                return EFI_SUCCESS;
            }
        }
    }

    if (CurrentEncapData == NULL) {
       LocalEncapData = FvInFd->EncapData;
    } else {
        if (OutputFileNameList != NULL && OutputFileNameList->FFSName != NULL && OutputFileNameList->IsFFS == TRUE) {
          *OutputFile = OutputFileNameList;
          return EFI_SUCCESS;
        }
        LocalEncapData = CurrentEncapData;
    }
    ParentLevel -= 1;

    while (LocalEncapData->NextNode != NULL) {
      if (LocalEncapData->Level == ParentLevel) {
       LocalEncapDataTemp = LocalEncapData->NextNode;
       if ((LocalEncapDataTemp != NULL) && (LocalEncapDataTemp->Level == ParentLevel)) {
           ParentType = LocalEncapDataTemp->Type;
           break;
        }
        ParentType = LocalEncapData->Type;
        break;
      }
      LocalEncapData = LocalEncapData->NextNode;
    }
  } while (ParentLevel != Level_Break);

  *OutputFile = OutputFileNameList;
  return EFI_SUCCESS;

}

EFI_STATUS
LibFindFvInEncapData (
  ENCAP_INFO_DATA *EncapData,
  UINT8 *Index
)
{
  ENCAP_INFO_DATA *LocalEncapData;
  LocalEncapData = EncapData;
  if (LocalEncapData == NULL) {
    Error("FMMT", 0, 0005, "error while find FV in Encapulate buffer", "Invalid parameters.");
    return EFI_INVALID_PARAMETER;
  }
  while (LocalEncapData != NULL) {
    if (LocalEncapData->RightNode != NULL) {
      LibFindFvInEncapData (LocalEncapData->RightNode, Index);
    }
    if (LocalEncapData->Type == FMMT_ENCAP_TREE_FV) {
      (*Index)++;
    }
    LocalEncapData = LocalEncapData->NextNode;
  }
    return EFI_SUCCESS;
}

EFI_STATUS
LibLocateFvViaFvId (
  IN     FIRMWARE_DEVICE     *FdData,
  IN     CHAR8               *FvId,
  IN OUT FV_INFORMATION      **FvInFd
)
{
  UINT8                       FvIndex1;
  UINT8                       FvIndex2;
  BOOLEAN                     FvFoundFlag;
  CHAR8*                      FvGuidName;
  ENCAP_INFO_DATA             *LocalEncapData;
  ENCAP_INFO_DATA             *LocalEncapDataRight;
  ENCAP_INFO_DATA             *LocalEncapDataNext;
  FvIndex1                    = 0;
  FvIndex2                    = 0;
  FvFoundFlag                 = FALSE;
  FvGuidName                  = NULL;
  LocalEncapDataNext          = NULL;
  LocalEncapDataRight         = NULL;

  if (FdData == NULL || FvId == NULL || FvInFd == NULL || FdData->Fv == NULL) {
    Error ("FMMT", 0, 0005, "error while find FV in FD", "Invalid parameters.");
    return EFI_INVALID_PARAMETER;
  }

  *FvInFd = FdData->Fv;

  if (strlen(FvId) < 3) {
    Error ("FMMT", 0, 0005, "error while find FV in FD", "Invalid FvId, please double check the FvId. You can use view operate to get the FvId information!");
    return EFI_ABORTED;
  }

  FvGuidName = (CHAR8 *) malloc (255);
  if (FvGuidName == NULL) {
    Error ("FMMT", 0, 0005, "Resource: Memory can't be allocated", NULL);
    return EFI_ABORTED;
  }
  memset(FvGuidName, '\0', 255);
  LocalEncapData = NULL;

  if (strlen(FvId) == 36) {
    while (FvInFd != NULL) {
      if (((*FvInFd)->FvExtHeader) != NULL) {
        sprintf(
          FvGuidName,
          "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
          (*FvInFd)->FvExtHeader->FvName.Data1,
          (*FvInFd)->FvExtHeader->FvName.Data2,
          (*FvInFd)->FvExtHeader->FvName.Data3,
          (*FvInFd)->FvExtHeader->FvName.Data4[0],
          (*FvInFd)->FvExtHeader->FvName.Data4[1],
          (*FvInFd)->FvExtHeader->FvName.Data4[2],
          (*FvInFd)->FvExtHeader->FvName.Data4[3],
          (*FvInFd)->FvExtHeader->FvName.Data4[4],
          (*FvInFd)->FvExtHeader->FvName.Data4[5],
          (*FvInFd)->FvExtHeader->FvName.Data4[6],
          (*FvInFd)->FvExtHeader->FvName.Data4[7]);
        if (strcmp(FvGuidName, FvId) == 0) {
          FvId = (*FvInFd)->FvName;
          break;
        }
      }
      if ((*FvInFd)->MulFvLevel > 1) {
        LocalEncapData = (*FvInFd) -> EncapData;
        LocalEncapData = LocalEncapData->NextNode;
        while (LocalEncapData != NULL) {
          if (LocalEncapData->RightNode != NULL) {
            LocalEncapDataRight = LocalEncapData->RightNode;
            while (LocalEncapDataRight !=NULL) {
              if (LocalEncapDataRight->NextNode != NULL) {
                LocalEncapDataNext = LocalEncapDataRight->NextNode;
                while (LocalEncapDataNext != NULL) {
                  if (LocalEncapDataNext->Type == FMMT_ENCAP_TREE_FV) {
                    if (LocalEncapDataNext->FvExtHeader != NULL) {
                      sprintf(
                          FvGuidName,
                          "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                          LocalEncapDataNext->FvExtHeader->FvName.Data1,
                          LocalEncapDataNext->FvExtHeader->FvName.Data2,
                          LocalEncapDataNext->FvExtHeader->FvName.Data3,
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[0],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[1],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[2],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[3],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[4],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[5],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[6],
                          LocalEncapDataNext->FvExtHeader->FvName.Data4[7]);
                          if (strcmp(FvGuidName, FvId) == 0)
                          {
                            sprintf(FvId, "%s%d", "FV", LocalEncapDataNext->FvId - 1);
                            break;
                          }

                    }
                  }
                  LocalEncapDataNext = LocalEncapDataNext->NextNode;
                }
              }
              LocalEncapDataRight = LocalEncapDataRight->RightNode;
            }
          }
          if (LocalEncapData->Type == FMMT_ENCAP_TREE_FV) {
            if (LocalEncapData->FvExtHeader != NULL) {
              sprintf(
                FvGuidName,
                "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                LocalEncapData->FvExtHeader->FvName.Data1,
                LocalEncapData->FvExtHeader->FvName.Data2,
                LocalEncapData->FvExtHeader->FvName.Data3,
                LocalEncapData->FvExtHeader->FvName.Data4[0],
                LocalEncapData->FvExtHeader->FvName.Data4[1],
                LocalEncapData->FvExtHeader->FvName.Data4[2],
                LocalEncapData->FvExtHeader->FvName.Data4[3],
                LocalEncapData->FvExtHeader->FvName.Data4[4],
                LocalEncapData->FvExtHeader->FvName.Data4[5],
                LocalEncapData->FvExtHeader->FvName.Data4[6],
                LocalEncapData->FvExtHeader->FvName.Data4[7]);

                if (strcmp(FvGuidName, FvId) == 0) {
                  sprintf(FvId, "%s%d", "FV", LocalEncapData->FvId - 1);
                  break;
                }
            }
          }
          LocalEncapData = LocalEncapData->NextNode;
        }
      }
      if ((*FvInFd)->FvNext == 0) {
        break;
      }
      *FvInFd = (*FvInFd)->FvNext;
    }
  }
  *FvInFd = FdData->Fv;
  FvIndex1 = (UINT8) atoi (FvId + 2);

  while (FvInFd != NULL) {
    if (((*FvInFd)->FvName) != NULL) {
      FvIndex2 = (UINT8) atoi ((*FvInFd)->FvName + 2);
      LocalEncapData = (*FvInFd)->EncapData;
      LibFindFvInEncapData (LocalEncapData, &FvIndex2);

      if ((FvIndex2 - 1 >= FvIndex1)) {
        FvFoundFlag = TRUE;
        break;
      }
      if ((*FvInFd)->FvNext == 0) {
        break;
      }
    }
    *FvInFd = (*FvInFd)->FvNext;
  }
  if (FvGuidName != NULL) {
    free (FvGuidName);
  }
  //
  // The specified FV id has issue, can not find the FV in FD.
  //
  if (!FvFoundFlag) {
    Error ("FMMT", 0, 0005, "error while find FV in FD", "Invalid FvId, please double check the FvId. You can use view operate to get the FvId information!");
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;

}

#define BUILD_IN_TOOL_COUNT 4

EFI_HANDLE
LibPreDefinedGuidedTools (
  VOID
)
{
  EFI_GUID            Guid;
  STRING_LIST         *Tool;
  GUID_SEC_TOOL_ENTRY *FirstGuidTool;
  GUID_SEC_TOOL_ENTRY *LastGuidTool;
  GUID_SEC_TOOL_ENTRY *NewGuidTool;
  UINT8               Index;
  EFI_STATUS          Status;

  CHAR8 PreDefinedGuidedTool[BUILD_IN_TOOL_COUNT][255] = {
    "a31280ad-481e-41b6-95e8-127f4c984779 TIANO TianoCompress",
    "ee4e5898-3914-4259-9d6e-dc7bd79403cf LZMA LzmaCompress",
    "fc1bcdb0-7d31-49aa-936a-a4600d9dd083 CRC32 GenCrc32",
    "3d532050-5cda-4fd0-879e-0f7f630d5afb BROTLI BrotliCompress"
  };

  Tool            = NULL;
  FirstGuidTool   = NULL;
  LastGuidTool    = NULL;
  NewGuidTool     = NULL;
  Index           = 0;

  for (Index = 0; Index < BUILD_IN_TOOL_COUNT; Index++) {
    Tool = SplitStringByWhitespace (PreDefinedGuidedTool[Index]);
    if ((Tool != NULL) &&
        (Tool->Count == 3)
       ) {
      Status = StringToGuid (Tool->Strings[0], &Guid);
      if (!EFI_ERROR (Status)) {
        NewGuidTool = malloc (sizeof (GUID_SEC_TOOL_ENTRY));
        if (NewGuidTool != NULL) {
          memcpy (&(NewGuidTool->Guid), &Guid, sizeof (Guid));
          NewGuidTool->Name = CloneString(Tool->Strings[1]);
          NewGuidTool->Path = CloneString(Tool->Strings[2]);
          NewGuidTool->Next = NULL;
        } else {
          printf ("Error while allocate resource! \n");
          FreeStringList (Tool);
          return NULL;
        }
        if (FirstGuidTool == NULL) {
          FirstGuidTool = NewGuidTool;
        } else {
          LastGuidTool->Next = NewGuidTool;
        }
        LastGuidTool = NewGuidTool;
      }
    } else {
      fprintf (stdout, "Error");
    }
    if (Tool != NULL) {
      FreeStringList (Tool);
      Tool = NULL;
    }
  }
  return FirstGuidTool;
}
