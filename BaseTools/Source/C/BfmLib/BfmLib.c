/** @file

 Library to process EFI image.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BinFileManager.h"

#define STR_LEN_MAX_4K 4096
#define STR_LEN_MAX_1K 1024

#define EFI_TEST_FFS_ATTRIBUTES_BIT(FvbAttributes, TestAttributes, Bit) \
    ( \
      (BOOLEAN) ( \
          (FvbAttributes & EFI_FVB2_ERASE_POLARITY) ? (((~TestAttributes) & Bit) == Bit) : ((TestAttributes & Bit) == Bit) \
        ) \
    )

#ifndef __GNUC__
#define DECODE_STR           "%s -d -o \"%s\" \"%s\" > NUL"
#define ENCODE_STR           "%s -e \"%s\" -o \"%s\" > NUL"
#define GENSEC_COMPRESSION   "GenSec -s %s -c %s  \"%s\" -o \"%s\" > NUL"
#define GENSEC_GUID          "GenSec -s %s -r PROCESSING_REQUIRED -g %s \"%s\" -o \"%s\" > NUL"
#define GENSEC_STR           "GenSec -s %s \"%s\" -o \"%s\" > NUL"
#define GENSEC_ALIGN         "GenSec --sectionalign 16 \"%s\" -o \"%s\" > NUL"
#define GENFV_STR            "GenFv -i \"%s\" -o \"%s\" > NUL"
#define GENFV_FVGUID         "GenFv -i \"%s\" -o \"%s\" --FvNameGuid %s > NUL"
#define GENFV_FFS            "GenFv -f \"%s\" -g %s -o \"%s\" > NUL"
#define GENFFS_STR           "GenFfs -t %s -i \"%s\" -g %s -o \"%s\" > NUL"
#define GENFFS_FIX           "GenFfs -t %s -i \"%s\" -g %s -x -o \"%s\" > NUL"
#else
#define DECODE_STR           "%s -d -o \"%s\" \"%s\" > /dev/null"
#define ENCODE_STR           "%s -e \"%s\" -o \"%s\" > /dev/null"
#define GENSEC_COMPRESSION   "GenSec -s %s -c %s  \"%s\" -o \"%s\" > /dev/null"
#define GENSEC_GUID          "GenSec -s %s -r PROCESSING_REQUIRED -g %s \"%s\" -o \"%s\" > /dev/null"
#define GENSEC_STR           "GenSec -s %s \"%s\" -o \"%s\" > /dev/null"
#define GENSEC_ALIGN         "GenSec --sectionalign 16 \"%s\" -o \"%s\" > /dev/null"
#define GENFV_STR            "GenFv -i \"%s\" -o \"%s\" > /dev/null"
#define GENFV_FVGUID         "GenFv -i \"%s\" -o \"%s\" --FvNameGuid %s > /dev/null"
#define GENFV_FFS            "GenFv -f \"%s\" -g %s -o \"%s\" > /dev/null"
#define GENFFS_STR           "GenFfs -t %s -i \"%s\" -g %s -o \"%s\" > /dev/null"
#define GENFFS_FIX           "GenFfs -t %s -i \"%s\" -g %s -x -o \"%s\" > /dev/null"
#endif

#define DECODE_STR_ERR       "%s -d -o \"%s\" \"%s\" "
#define ENCODE_STR_ERR       "%s -e \"%s\" -o \"%s\" "

CHAR8      mFirmwareFileSystem2Guid[16] = {0x78, 0xE5, 0x8C, 0x8C, 0x3D, 0x8A, 0x1C, 0x4F, 0x99, 0x35, 0x89, 0x61, 0x85, 0xC3, 0x2D, 0xD3};

CHAR8      mFirmwareFileSystem3Guid[16] = {0x7A, 0xC0, 0x73, 0x54, 0xCB, 0x3D, 0xCA, 0x4D, 0xBD, 0x6F, 0x1E, 0x96, 0x89, 0xE7, 0x34, 0x9A };
extern     CHAR8*      mGuidToolDefinition;
UINT32     PadSizeOfBfv;

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

FV_INFORMATION *
LibInitializeFvStruct (
  FV_INFORMATION *Fv
)
{
  UINT32     Index;

  if (Fv == NULL) {
    return NULL;
  }

  memset (Fv, '\0', sizeof (FV_INFORMATION));

  for (Index = 0; Index < MAX_NUMBER_OF_FILES_IN_FV; Index ++) {
    memset (Fv->FfsAttuibutes[Index].FfsName, '\0', _MAX_PATH);
    memset (Fv->FfsAttuibutes[Index].UiName, '\0', _MAX_PATH * sizeof (CHAR16));

    Fv->FfsAttuibutes[Index].IsLeaf               = TRUE;
    Fv->FfsAttuibutes[Index].TotalSectionNum      = 0;
  }

  Fv->PatchData = NULL;
  Fv->EncapData = NULL;
  Fv->FvNext = NULL;
  Fv->FvLevel   = 0;
  Fv->IsBfvFlag = FALSE;
  Fv->IsInputFvFlag = FALSE;

  return Fv;
}

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

EFI_STATUS
LibFindFvInFd (
  IN     FILE             *InputFile,
  IN OUT FIRMWARE_DEVICE  **FdData
)
{
  FIRMWARE_DEVICE             *LocalFdData;
  UINT16                      Index;
  CHAR8                       Ffs2Guid[16];
  CHAR8                       SignatureCheck[4];
  CHAR8                       Signature[5] = "_FVH";
  FV_INFORMATION              *CurrentFv;
  FV_INFORMATION              *NewFoundFv;
  BOOLEAN                     FirstMatch;
  UINT32                      FdSize;
  UINT16                      FvCount;
  VOID                        *FdBuffer;
  VOID                        *FdBufferOri;
  UINT32                      Count;


  CurrentFv      = NULL;
  NewFoundFv     = NULL;
  FdBuffer       = NULL;
  FdBufferOri    = NULL;
  FirstMatch     = TRUE;
  Index          = 0;
  FdSize         = 0;
  FvCount        = 0;
  Count          = 0;
  LocalFdData    = NULL;

  if (InputFile == NULL) {
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
  LocalFdData     = (FIRMWARE_DEVICE *) calloc (sizeof (FIRMWARE_DEVICE), sizeof(UINT8));
  if (LocalFdData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  LocalFdData->Fv = (FV_INFORMATION *)  calloc (sizeof (FV_INFORMATION), sizeof(UINT8));
  if (LocalFdData->Fv == NULL) {
    free (LocalFdData);
    return EFI_OUT_OF_RESOURCES;
  }
  LibInitializeFvStruct (LocalFdData->Fv);

  //
  // Readout the FD file data to buffer.
  //
  FdBuffer = malloc (FdSize);

  if (FdBuffer == NULL) {
    free (LocalFdData->Fv);
    free (LocalFdData);
    return EFI_OUT_OF_RESOURCES;
  }

  if (fread (FdBuffer, 1, FdSize, InputFile) != FdSize) {
    free (LocalFdData->Fv);
    free (LocalFdData);
    free (FdBuffer);
    return EFI_ABORTED;
  }

  FdBufferOri = FdBuffer;

  for (Count=0; Count < FdSize - 4; Count++) {
    //
    // Copy 4 bytes of fd data to check the _FVH signature
    //
    memcpy (SignatureCheck, FdBuffer, 4);
    FdBuffer =(UINT8 *)FdBuffer + 4;

    if (strncmp(SignatureCheck, Signature, 4) == 0){
      //
      // Still need to determine the FileSystemGuid in EFI_FIRMWARE_VOLUME_HEADER equal to
      // EFI_FIRMWARE_FILE_SYSTEM2_GUID.
      // Turn back 28 bytes to find the GUID.
      //
      FdBuffer = (UINT8 *)FdBuffer - 28;
      memcpy (Ffs2Guid, FdBuffer, 16);

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
      // Point to the original address
      //
      FdBuffer = (UINT8 *)FdBuffer + 28;

      //
      // Here we found an FV.
      //
      if (Index == 16) {
        if (FirstMatch) {
          LocalFdData->Fv->ImageAddress = (UINTN)((UINT8 *)FdBuffer - (UINT8 *)FdBufferOri) - 0x2c;
          CurrentFv                     = LocalFdData->Fv;
          CurrentFv->FvNext             = NULL;
          //
          // Store the FV name by found sequence
          //
          sprintf(CurrentFv->FvName, "FV%d", FvCount);

          FirstMatch = FALSE;
          } else {
            NewFoundFv = (FV_INFORMATION *) malloc (sizeof (FV_INFORMATION));
            if (NULL == NewFoundFv) {
              free (LocalFdData->Fv);
              free (LocalFdData);
              free (FdBuffer);
              return EFI_OUT_OF_RESOURCES;
            }

            LibInitializeFvStruct (NewFoundFv);

            //
            // Need to turn back 0x2c bytes
            //
            NewFoundFv->ImageAddress = (UINTN)((UINT8 *)FdBuffer - (UINT8 *)FdBufferOri) - 0x2c;

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
        Index = 0;
      }

    }

    //
    // We need to turn back 3 bytes.
    //
    FdBuffer = (UINT8 *)FdBuffer - 3;
  }

  LocalFdData->Size = FdSize;

  *FdData = LocalFdData;

  free (FdBufferOri);

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

  Expands the 3 byte size commonly used in Firmware Volume data structures

  @param[in]    Size - Address of the 3 byte array representing the size

  @return       UINT32

**/
UINT32
FvBufExpand3ByteSize (
  IN VOID* Size
  )
{
  return (((UINT8*)Size)[2] << 16) +
         (((UINT8*)Size)[1] << 8) +
         ((UINT8*)Size)[0];
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
      FwVolExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) ((UINT8 *) hdr + hdr->ExtHeaderOffset);
      *Key  = (UINTN)hdr->ExtHeaderOffset + FwVolExtHeader->ExtHeaderSize;
      *Key = (UINTN)ALIGN_POINTER (*Key, 8);
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
  CHAR8                       TempBuf[_MAX_PATH];

  FfsFileSize   = 0;
  FfsFileName   = NULL;
  FfsFile       = NULL;
  TempDir       = NULL;

  TempDir = getcwd (NULL, _MAX_PATH);
  TempDir = realloc (TempDir, _MAX_PATH);

   if (strlen (TempDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    printf ("The directory is too long \n");
    return EFI_ABORTED;
  }

  strncat (TempDir, OS_SEP_STR, _MAX_PATH - strlen (TempDir) - 1);
  strncat (TempDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TempDir) - 1);
  mkdir(TempDir, S_IRWXU | S_IRWXG | S_IRWXO);

  FfsFileName = (CHAR8 *) malloc (_MAX_PATH);
  if (NULL == FfsFileName) {
    return EFI_ABORTED;
  }
  memset (FfsFileName, '\0', _MAX_PATH);
  FfsFileSize = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);

  sprintf (
    TempBuf,
    "-Num%d-%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X-Level%d",
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
  if (strlen (TempDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) + strlen(TempBuf) > _MAX_PATH - 1) {
    free(FfsFileName);
    printf ("The directory is too long \n");
    return EFI_ABORTED;
  }
  strcpy (FfsFileName, TempDir);
  strncat (FfsFileName, OS_SEP_STR, _MAX_PATH - strlen (FfsFileName) - 1);
  strncat (FfsFileName, CurrentFv->FvName, _MAX_PATH - strlen (FfsFileName) - 1);
  strncat (FfsFileName, TempBuf, _MAX_PATH - strlen (FfsFileName) - 1);

  memcpy (CurrentFv->FfsAttuibutes[*FfsCount].FfsName, FfsFileName, strlen(FfsFileName));

  //
  // Update current FFS files file state.
  //
  if (ErasePolarity) {
    CurrentFile->State = (UINT8)~(CurrentFile->State);
  }

  FfsFile = fopen (FfsFileName, "wb+");
  if (FfsFile == NULL) {
    free(FfsFileName);
    return EFI_ABORTED;
  }

  if (fwrite (CurrentFile, 1, FfsFileSize, FfsFile) != FfsFileSize) {
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


BOOLEAN
LibCheckPadFfsContainFvNameGuid (
  IN     FV_INFORMATION            *CurrentFv,
  IN     EFI_FFS_FILE_HEADER2      *CurrentFile
)
{
  UINT32                      FfsFileSize;
  UINT32                      FfsDataSize;
  EFI_GUID                    *FfsData;
  ENCAP_INFO_DATA             *LocalEncapData;

  LocalEncapData = NULL;

  FfsFileSize = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);
  FfsDataSize = FfsFileSize - GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) CurrentFile);
  FfsData     = (EFI_GUID *) ((INT8 *)CurrentFile + GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) CurrentFile));

  if (FfsDataSize == 0) {
    return TRUE;
  }

  LocalEncapData = CurrentFv->EncapData;

  do {
    if (LocalEncapData->FvExtHeader != NULL) {
      if (CompareGuid(FfsData, &LocalEncapData->FvExtHeader->FvName) == 0) {
        return TRUE;
      }
    }
    LocalEncapData = LocalEncapData->NextNode;
  } while (LocalEncapData->NextNode != NULL);

  return FALSE;
}

BOOLEAN
LibCheckPadFfsNotNull (
  IN     EFI_FFS_FILE_HEADER2       *CurrentFile
)
{
  UINT32                      FfsFileSize;
  UINT32                      FfsDataSize;
  INT8                        *FfsData;

  FfsFileSize = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);
  FfsDataSize = FfsFileSize - GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) CurrentFile);
  FfsData     = (INT8 *)CurrentFile + GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) CurrentFile);

  if (FfsDataSize == 0) {
    return FALSE;
  }

  while (FfsDataSize > 0) {
    if (((FfsData[FfsDataSize-1]) & 0xFF) != 0xFF) {
      return TRUE;
    }
    FfsDataSize--;
  }

  return FALSE;
}

/**
  Find a maximum length of free space in PAD FFS of Bfv.

  @PadFfsHeader - The header of PAD FFS file

  @return The length of free space

**/
UINT32
GetBfvMaxFreeSpace (
  IN EFI_FFS_FILE_HEADER2  *PadFfsHeader
  )
{
   UINT32    FfsSize;
   UINT32    Count;
   UINT32    Index;
   UINT32    MaxSize;
   UINT32    HeaderSize;

   Index   = 0;
   MaxSize = 0;

   if (PadFfsHeader == NULL) {
     return MaxSize;
   }

   FfsSize    = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) PadFfsHeader);
   HeaderSize = GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) PadFfsHeader);

    for (Count = HeaderSize; Count < FfsSize; Count++) {
      if (((((INT8 *)PadFfsHeader)[Count]) & 0xFF) == 0xFF) {
        Index++;
      } else {
        if (Index > MaxSize) {
          MaxSize = Index;
        }
        Index = 0;
      }
    }
    return MaxSize;
}


/**

  Get the Offset and data of PAD FFS file in FV file.
  This function should be only called for find an PAD FFS contain additional data
  (usually will contain FIT table data or reset vector.)

   BFV:
   ----------------------      <- Low
  |                      |
  |                      |
   -----------------------
  |       FFSs ...       |
   -----------------------
  |                      |
  |                      |
   -----------------------
  |     PAD FFS file     |
  |                      |
  |      reset vector    |
  |                      |
  |      FIT table       |
   -----------------------
  |      SEC CORE        |     <- High
   -----------------------

**/

EFI_STATUS
LibFindResetVectorAndFitTableData(
  IN     EFI_FIRMWARE_VOLUME_HEADER  *FvImage,
  IN     EFI_FFS_FILE_HEADER2        *CurrentFile,
  IN OUT FV_INFORMATION              *CurrentFv
)
{
  UINT32          Count1;
  UINT32          Count2;
  UINT32          FfsFileSize;
  BOOLEAN         FfsFoundFlag;
  UINT32          FfsOffset;
  UINT32          DataOffset;
  UINT32          HeaderSize;
  PATCH_DATA_PAD_FFS *LocalPatchData;

  FfsFileSize    = 0;
  Count1         = 0;
  Count2         = 0;
  FfsOffset      = 0;
  DataOffset     = 0;
  FfsFoundFlag   = FALSE;
  LocalPatchData = NULL;

  if (CurrentFv == NULL || CurrentFile == NULL || FvImage == NULL) {
    return EFI_ABORTED;
  }

  FfsFileSize = GetFfsFileLength ((EFI_FFS_FILE_HEADER *) CurrentFile);
  HeaderSize  = GetFfsHeaderLength  ((EFI_FFS_FILE_HEADER *) CurrentFile);

  for (Count1=0; Count1 < (FvImage->FvLength - FfsFileSize); Count1 ++) {
    for (Count2=0; Count2 < FfsFileSize; Count2 ++) {
      if (((INT8*)FvImage)[Count1 + Count2] != ((INT8 *) CurrentFile)[Count2]){
        break;
      }
    }
    if (Count2 == FfsFileSize) {
      FfsFoundFlag = TRUE;
      FfsOffset = Count1;
      break;
    }
  }

  if (FfsFoundFlag) {
    //
    // Find data in FFS file;
    // Will skip FFS header;
    //
    for (Count1 = HeaderSize; Count1 < FfsFileSize; Count1++) {
      if (((((INT8 *)CurrentFile)[Count1]) & 0xFF) != 0xFF) {
        DataOffset = FfsOffset + Count1;
        break;
      }
    }

    if (CurrentFv->PatchData == NULL) {
      //
      // First time found data.
      //
      CurrentFv->PatchData = (PATCH_DATA_PAD_FFS *) malloc (sizeof (PATCH_DATA_PAD_FFS));
      if (CurrentFv->PatchData == NULL) {
        return EFI_ABORTED;
      }

      CurrentFv->PatchData->Offset = DataOffset;
      CurrentFv->PatchData->Data = malloc(FfsFileSize - Count1);
      CurrentFv->PatchData->Length = FfsFileSize - Count1;
      CurrentFv->PatchData->NextNode = NULL;

      if (CurrentFv->PatchData->Data == NULL) {
        return EFI_ABORTED;
      }

      memcpy (CurrentFv->PatchData->Data, (INT8 *)CurrentFile + Count1, FfsFileSize - Count1);
    } else {
      LocalPatchData = CurrentFv->PatchData;

      while (LocalPatchData->NextNode != NULL) {
        LocalPatchData = LocalPatchData->NextNode;
      }

      LocalPatchData = (PATCH_DATA_PAD_FFS *) malloc (sizeof (PATCH_DATA_PAD_FFS));

      if (LocalPatchData == NULL) {
        return EFI_ABORTED;
      }

      LocalPatchData->Offset = DataOffset;
      LocalPatchData->Data = malloc(FfsFileSize - Count1);
      LocalPatchData->Length = FfsFileSize - Count1;
      LocalPatchData->NextNode = NULL;

      if (LocalPatchData->Data == NULL) {
        free (LocalPatchData);
        return EFI_ABORTED;
      }

      memcpy (LocalPatchData->Data, (INT8 *)CurrentFile + Count1, FfsFileSize - Count1);
      while (CurrentFv->PatchData->NextNode != NULL) {
        CurrentFv->PatchData = CurrentFv->PatchData->NextNode;
      }
      CurrentFv->PatchData->NextNode = LocalPatchData;
    }

  } else {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
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

  if (NULL == RetStr) {
    return NULL;
  }

  memset (RetStr , '\0', Count + 1);

  for (Index=0; Index <= Count -1; Index ++) {
    RetStr[Index] = ' ';
  }

  return RetStr;

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
  UINT32                 *FfsCount,
  BOOLEAN                ViewFlag,
  BOOLEAN                ErasePolarity,
  BOOLEAN                FfsGeneratedFlag
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
  CHAR8               *SystemCommand;
  UINT8               *ToolOutputBuffer;
  UINT32              ToolOutputLength;
  CHAR16              *UIName;
  UINT32              UINameSize;
  BOOLEAN             HasDepexSection;
  UINT32              NumberOfSections;
  BOOLEAN             IsFfsGenerated;
  ENCAP_INFO_DATA     *LocalEncapData;
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
  CHAR8               *UIFileName;
  CHAR8               *ToolInputFileName;
  CHAR8               *ToolOutputFileName;

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
  ToolOutputBuffer           = NULL;
  UIName                     = NULL;
  LocalEncapData             = NULL;
  BlankChar                  = NULL;
  UncompressedBuffer         = NULL;
  CompressedBuffer           = NULL;
  ScratchBuffer              = NULL;
  TempDir                    = NULL;
  ToolInputFileFullName      = NULL;
  ToolOutputFileFullName     = NULL;
  ToolInputFileName          = NULL;
  ToolOutputFileName         = NULL;
  HasDepexSection            = FALSE;
  IsFfsGenerated             = FfsGeneratedFlag;
  EncapDataNeedUpdata        = TRUE;
  LargeHeaderOffset          = 0;


  while (ParsedLength < BufferLength) {
    Ptr           = SectionBuffer + ParsedLength;

    SectionLength = FvBufExpand3ByteSize (((EFI_COMMON_SECTION_HEADER *) Ptr)->Size);
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

      //
      // Put in encapsulate data information.
      //
      LocalEncapData = CurrentFv->EncapData;
      while (LocalEncapData->NextNode != NULL) {
        if (LocalEncapData->Level == Level) {
          EncapDataNeedUpdata = FALSE;
          break;
        }
        LocalEncapData = LocalEncapData->NextNode;
      }

      if (EncapDataNeedUpdata) {
        //
        // Put in this is an FFS with FV section
        //
        LocalEncapData = CurrentFv->EncapData;
        while (LocalEncapData->NextNode != NULL) {
          LocalEncapData = LocalEncapData->NextNode;
        }

        //
        // Construct the new ENCAP_DATA
        //
        LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

        if (LocalEncapData->NextNode == NULL) {
          return EFI_ABORTED;
        }

        LocalEncapData        = LocalEncapData->NextNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = BFM_ENCAP_TREE_FV_SECTION;

        //
        // We don't need additional data for encapsulate this FFS but type.
        //
        LocalEncapData->Data        = NULL;
        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode    = NULL;
      }

      Status = LibGetFvInfo ((UINT8*)((EFI_FIRMWARE_VOLUME_IMAGE_SECTION*)Ptr + 1) + LargeHeaderOffset, CurrentFv, FvName, Level, FfsCount, ViewFlag, TRUE);
      if (EFI_ERROR (Status)) {
        return EFI_SECTION_ERROR;
      }
      break;

    case EFI_SECTION_COMPRESSION:
      Level ++;
      NumberOfSections ++;

      EncapDataNeedUpdata = TRUE;
      //
      // Put in encapsulate data information.
      //
      LocalEncapData = CurrentFv->EncapData;
      while (LocalEncapData->NextNode != NULL) {
        if (LocalEncapData->Level == Level) {
          EncapDataNeedUpdata = FALSE;
          break;
        }
        LocalEncapData = LocalEncapData->NextNode;
      }

      if (EncapDataNeedUpdata) {
        //
        // Put in this is an FFS with FV section
        //
        LocalEncapData = CurrentFv->EncapData;
        while (LocalEncapData->NextNode != NULL) {
          LocalEncapData = LocalEncapData->NextNode;
        }

        //
        // Construct the new ENCAP_DATA
        //
        LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

        if (LocalEncapData->NextNode == NULL) {
          return EFI_ABORTED;
          }

        LocalEncapData        = LocalEncapData->NextNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = BFM_ENCAP_TREE_COMPRESS_SECTION;

        //
        // Store the compress type
        //
        LocalEncapData->Data     = malloc (sizeof (UINT8));

        if (LocalEncapData->Data == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        *(UINT8 *)LocalEncapData->Data     = ((EFI_COMPRESSION_SECTION *) (Ptr + LargeHeaderOffset))->CompressionType;
        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode = NULL;
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

        if (CompressedLength != UncompressedLength) {
          printf ("Error. File is not compressed, but the compressed length does not match the uncompressed length.\n");
          return EFI_SECTION_ERROR;
        }

        UncompressedBuffer = Ptr + sizeof (EFI_COMPRESSION_SECTION) + LargeHeaderOffset;
      } else if (CompressionType == EFI_STANDARD_COMPRESSION) {
        GetInfoFunction     = EfiGetInfo;
        DecompressFunction  = EfiDecompress;

        CompressedBuffer  = Ptr + sizeof (EFI_COMPRESSION_SECTION) + LargeHeaderOffset;

        Status            = GetInfoFunction (CompressedBuffer, CompressedLength, &DstSize, &ScratchSize);
        if (EFI_ERROR (Status)) {
          return EFI_SECTION_ERROR;
        }

        if (DstSize != UncompressedLength) {
          return EFI_SECTION_ERROR;
        }

        ScratchBuffer       = malloc (ScratchSize);
        if (ScratchBuffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        UncompressedBuffer  = malloc (UncompressedLength);

        if (UncompressedBuffer == NULL) {
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
          free (UncompressedBuffer);
          return EFI_SECTION_ERROR;
        }
      } else {
        return EFI_SECTION_ERROR;
      }

      Status = LibParseSection (  UncompressedBuffer,
                                  UncompressedLength,
                                  CurrentFv,
                                  FvName,
                                  CurrentFile,
                                  Level,
                                  FfsCount,
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
      LocalEncapData = CurrentFv->EncapData;
      while (LocalEncapData->NextNode != NULL) {
        if (LocalEncapData->Level == Level) {
          EncapDataNeedUpdata = FALSE;
          break;
        }
        LocalEncapData = LocalEncapData->NextNode;
      }
      if (EncapDataNeedUpdata)  {

        //
        // Put in this is an FFS with FV section
        //
        LocalEncapData = CurrentFv->EncapData;
        while (LocalEncapData->NextNode != NULL) {
          LocalEncapData = LocalEncapData->NextNode;
          }

        //
        // Construct the new ENCAP_DATA
        //
        LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

        if (LocalEncapData->NextNode == NULL) {
          return EFI_ABORTED;
        }

        LocalEncapData        = LocalEncapData->NextNode;

        LocalEncapData->Level = Level;
        LocalEncapData->Type  = BFM_ENCAP_TREE_GUIDED_SECTION;

        //
        // We don't need additional data for encapsulate this FFS but type.
        //

        LocalEncapData->Data     = (EFI_GUID *) malloc (sizeof (EFI_GUID));

        if (LocalEncapData->Data == NULL) {
          return EFI_ABORTED;
        }

    memcpy (LocalEncapData->Data, &((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->SectionDefinitionGuid, sizeof (EFI_GUID));

        LocalEncapData->FvExtHeader = NULL;
        LocalEncapData->NextNode = NULL;
      }

      CurrentFv->FfsAttuibutes[*FfsCount].IsLeaf = FALSE;

      ExtractionTool =
        LookupGuidedSectionToolPath (
          mParsedGuidedSectionTools,
          &((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->SectionDefinitionGuid
          );

      if ((((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
        //
        // Not require process, directly gets data.
        //
        Status = LibParseSection (
          Ptr + ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset,
          SectionLength - ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset,
          CurrentFv,
          FvName,
          CurrentFile,
          Level,
          FfsCount,
          ViewFlag,
          ErasePolarity,
          IsFfsGenerated
          );
        if (ExtractionTool != NULL) {
          free (ExtractionTool);
          ExtractionTool = NULL;
        }
        if (EFI_ERROR (Status)) {
          return EFI_SECTION_ERROR;
        }
      } else if (ExtractionTool != NULL) {

        TempDir = getcwd (NULL, _MAX_PATH);
        TempDir = realloc (TempDir, _MAX_PATH);
        if (strlen (TempDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
          printf ("The directory is too long \n");
          free (ExtractionTool);
          return EFI_ABORTED;
        }
        strncat (TempDir, OS_SEP_STR, _MAX_PATH - strlen (TempDir) - 1);
        strncat (TempDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TempDir) - 1);
        mkdir(TempDir, S_IRWXU | S_IRWXG | S_IRWXO);

        ToolInputFile  = GenTempFile ();
        ToolOutputFile = GenTempFile ();
        ToolInputFileName = strrchr(ToolInputFile, OS_SEP);
        ToolOutputFileName = strrchr(ToolOutputFile, OS_SEP);

        ToolInputFileFullName   = malloc (strlen("%s%s") + strlen(TempDir) + strlen(ToolInputFileName) + 1);
        if (ToolInputFileFullName == NULL) {
          free (ExtractionTool);
          free (ToolInputFile);
          free (ToolOutputFile);
          return EFI_OUT_OF_RESOURCES;
        }
        ToolOutputFileFullName  = malloc (strlen("%s%s") + strlen(TempDir) + strlen(ToolOutputFileName) + 1);
        if (ToolOutputFileFullName == NULL) {
          free (ToolInputFileFullName);
          free (ExtractionTool);
          free (ToolInputFile);
          free (ToolOutputFile);
          return EFI_OUT_OF_RESOURCES;
        }

        sprintf (ToolInputFileFullName, "%s%s", TempDir, ToolInputFileName);
        sprintf (ToolOutputFileFullName, "%s%s", TempDir, ToolOutputFileName);

        //
        // Construction 'system' command string
        //
        SystemCommand = malloc (
          strlen (DECODE_STR) +
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
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          DECODE_STR,
          ExtractionTool,
          ToolOutputFileFullName,
          ToolInputFileFullName
          );

        Status = PutFileImage (
        ToolInputFileFullName,
        (CHAR8*) Ptr + ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset,
        SectionLength - ((EFI_GUID_DEFINED_SECTION *) (Ptr + LargeHeaderOffset))->DataOffset
        );

        if (HasDepexSection) {
          HasDepexSection = FALSE;
        }

        if (EFI_ERROR (Status)) {
          free(SystemCommand);
          free (ToolOutputFileFullName);
          free (ToolOutputFile);
          remove (ToolInputFileFullName);
          free (ToolInputFile);
          free (ToolInputFileFullName);
          return EFI_SECTION_ERROR;
        }

        if (system (SystemCommand) != EFI_SUCCESS) {
          SystemCommand = malloc (
            strlen (DECODE_STR_ERR) +
            strlen (ExtractionTool) +
            strlen (ToolInputFileFullName) +
            strlen (ToolOutputFileFullName) +
            1
            );
          if (SystemCommand == NULL) {
            free (ExtractionTool);
            LibRmDir (TempDir);
            remove (ToolInputFileFullName);
            free (ToolInputFile);
            free (ToolInputFileFullName);
            free (ToolOutputFileFullName);
            free (ToolOutputFile);
            return EFI_OUT_OF_RESOURCES;
          }
          sprintf (
            SystemCommand,
            DECODE_STR_ERR,
            ExtractionTool,
            ToolOutputFileFullName,
            ToolInputFileFullName
            );
          system (SystemCommand);
          printf("Command failed: %s\n", SystemCommand);
          free (ExtractionTool);
          ExtractionTool = NULL;
          LibRmDir (TempDir);
          free(SystemCommand);
          remove (ToolInputFileFullName);
          free (ToolInputFile);
          free (ToolInputFileFullName);
          free (ToolOutputFileFullName);
          free (ToolOutputFile);
          return EFI_ABORTED;
        }
        free (ExtractionTool);
        ExtractionTool = NULL;
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
          return EFI_SECTION_ERROR;
        }

        Status = LibParseSection (
                  ToolOutputBuffer,
                  ToolOutputLength,
                  CurrentFv,
                  FvName,
                  CurrentFile,
                  Level,
                  FfsCount,
                  ViewFlag,
                  ErasePolarity,
                  IsFfsGenerated
                  );
        if (EFI_ERROR (Status)) {
          return EFI_SECTION_ERROR;
        }
      } else {
        //
        // We don't know how to parse it now.
        //
        if (ExtractionTool != NULL) {
          free (ExtractionTool);
          ExtractionTool = NULL;
        }
        printf("  EFI_SECTION_GUID_DEFINED cannot be parsed at this time. Tool to decode this section should have been defined in %s file.\n", mGuidToolDefinition);
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
        if (!IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          IsFfsGenerated = TRUE;
        }
      }

      break;
    case EFI_SECTION_PE32:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          IsFfsGenerated = TRUE;
        }
      }

      break;
    case EFI_SECTION_PIC:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          IsFfsGenerated = TRUE;
        }
      }

      break;
    case EFI_SECTION_TE:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          IsFfsGenerated = TRUE;
        }
      }
      break;

    case EFI_SECTION_COMPATIBILITY16:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;

      if (!ViewFlag) {
        if (!IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          IsFfsGenerated = TRUE;
        }
      }
      break;

    case EFI_SECTION_FREEFORM_SUBTYPE_GUID:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        if (!IsFfsGenerated) {
          LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
          IsFfsGenerated = TRUE;
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
      break;
    case EFI_SECTION_DXE_DEPEX:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      HasDepexSection = TRUE;
      break;
    case EFI_SECTION_SMM_DEPEX:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      HasDepexSection = TRUE;
      break;

    case EFI_SECTION_USER_INTERFACE:
      NumberOfSections ++;
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;

      UiSectionLength = FvBufExpand3ByteSize (((EFI_USER_INTERFACE_SECTION *) Ptr)->CommonHeader.Size);
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
        return EFI_ABORTED;
      }

      BlankChar = LibConstructBlankChar( CurrentFv->FvLevel * 2);
      if (BlankChar == NULL) {
        free (UIName);
        return EFI_ABORTED;
      }

      if (ViewFlag) {
        UIFileName = malloc (UINameSize + 2);
        if (UIFileName == NULL) {
          free(BlankChar);
          free(UIName);
          return EFI_OUT_OF_RESOURCES;
        }
        Unicode2AsciiString (UIName, UIFileName);
        fprintf(stdout, "%sFile \"%s\"\n", BlankChar, UIFileName);
        free(UIFileName);
      }

      free (BlankChar);
      BlankChar = NULL;

      //
      // If Ffs file has been generated, then the FfsCount should decrease 1.
      //
      if (IsFfsGenerated) {
        memcpy (CurrentFv->FfsAttuibutes[*FfsCount -1].UiName, UIName, UINameSize);
      } else {
        memcpy (CurrentFv->FfsAttuibutes[*FfsCount].UiName, UIName, UINameSize);
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
    return EFI_SECTION_ERROR;
  }

  return EFI_SUCCESS;
}

/**

  Add function description

  FvImage       -  add argument description
  FileHeader    -  add argument description
  ErasePolarity -  add argument description

  EFI_SUCCESS -  Add description for return value
  EFI_ABORTED -  Add description for return value

**/
EFI_STATUS
LibGetFileInfo (
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage,
  EFI_FFS_FILE_HEADER2        *CurrentFile,
  BOOLEAN                     ErasePolarity,
  FV_INFORMATION              *CurrentFv,
  CHAR8                       *FvName,
  UINT8                       Level,
  UINT32                      *FfsCount,
  BOOLEAN                     ViewFlag
  )
{
  UINT32              FileLength;
  UINT8               FileState;
  UINT8               Checksum;
  EFI_FFS_FILE_HEADER2 BlankHeader;
  EFI_STATUS          Status;
  ENCAP_INFO_DATA     *LocalEncapData;
  BOOLEAN             EncapDataNeedUpdateFlag;
  UINT32              FfsFileHeaderSize;

  Status = EFI_SUCCESS;

  LocalEncapData  = NULL;
  EncapDataNeedUpdateFlag = TRUE;

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

  if (FileState == EFI_FILE_DATA_VALID) {
    //
    // Calculate header checksum
    //
    Checksum  = CalculateSum8 ((UINT8 *) CurrentFile, FfsFileHeaderSize);
    Checksum  = (UINT8) (Checksum - CurrentFile->IntegrityCheck.Checksum.File);
    Checksum  = (UINT8) (Checksum - CurrentFile->State);
    if (Checksum != 0) {
      return EFI_ABORTED;
    }

    if (CurrentFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      //
      // Calculate file checksum
      //
      Checksum  = CalculateSum8 ((UINT8 *) ((UINTN)CurrentFile + FfsFileHeaderSize), FileLength - FfsFileHeaderSize);
      Checksum  = Checksum + CurrentFile->IntegrityCheck.Checksum.File;
      if (Checksum != 0) {
        return EFI_ABORTED;
      }
    } else {
      if (CurrentFile->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
        return EFI_ABORTED;
      }
    }
  } else {
    return EFI_ABORTED;
  }

  Level += 1;

  if (CurrentFile->Type != EFI_FV_FILETYPE_ALL) {

    //
    // Put in encapsulate data information.
    //
    LocalEncapData = CurrentFv->EncapData;
    while (LocalEncapData->NextNode != NULL) {
      if (LocalEncapData->Level == Level) {
        EncapDataNeedUpdateFlag = FALSE;
        break;
      }
      LocalEncapData = LocalEncapData->NextNode;
    }

    if (EncapDataNeedUpdateFlag) {
      //
      // Construct the new ENCAP_DATA
      //
      LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

      if (LocalEncapData->NextNode == NULL) {
        printf ("Out of resource, memory allocation failed. \n");
        return EFI_ABORTED;
      }

      LocalEncapData        = LocalEncapData->NextNode;

      LocalEncapData->Level = Level;
      LocalEncapData->Type  = BFM_ENCAP_TREE_FFS;
      LocalEncapData->FvExtHeader = NULL;

      //
      // Store the header of FFS file.
      //
      LocalEncapData->Data     = malloc (FfsFileHeaderSize);
      if (LocalEncapData->Data == NULL) {
        printf ("Out of resource, memory allocation failed. \n");
        return EFI_ABORTED;
      }

      memcpy (LocalEncapData->Data, CurrentFile, FfsFileHeaderSize);

      LocalEncapData->NextNode = NULL;
    }

    if ( CurrentFile->Type == EFI_FV_FILETYPE_FREEFORM ){
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag) {
        LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
      }
    }else if ( CurrentFile->Type == EFI_FV_FILETYPE_RAW){
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      if (!ViewFlag){
        LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
      }
    } else if ( CurrentFile->Type == EFI_FV_FILETYPE_SECURITY_CORE){
      CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
      //
      // If an FV contain SECCORE, this FV will be considered as BFV.
      //
      CurrentFv->IsBfvFlag = TRUE;
      if (!ViewFlag){
        LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
      }
    } else if( CurrentFile->Type == EFI_FV_FILETYPE_FFS_PAD){
      //
      // First check whether the FFS file contain FvExtended FvNameGuid information.
      //
      if (!LibCheckPadFfsContainFvNameGuid (CurrentFv, CurrentFile)) {
        //
        // Then check whether the PAD file have no additional data or not.
        //
        if (LibCheckPadFfsNotNull (CurrentFile)) {
          CurrentFv->FfsAttuibutes[*FfsCount].Level = Level;
          //
          // Get the size of PAD in BFV
          //
          PadSizeOfBfv = GetBfvMaxFreeSpace (CurrentFile);
          if (!ViewFlag){
            //
            //LibGenFfsFile(CurrentFile, CurrentFv, FvName, Level, FfsCount, ErasePolarity);
            //
            Status = LibFindResetVectorAndFitTableData (FvImage, CurrentFile, CurrentFv);
            if (EFI_ERROR (Status)) {
              printf ("Find reset vector and FIT table data failed. \n");
              return EFI_ABORTED;
            }
          }
        }
      }
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
        FfsCount,
        ViewFlag,
        ErasePolarity,
        FALSE
        );
    }
    if (EFI_ERROR (Status)) {
      printf ("Error while parse the FFS file.\n");
      return EFI_ABORTED;
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
  IN     UINT32                       *FfsCount,
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

  NumberOfFiles  = 0;
  Key            = 0;
  LocalEncapData = NULL;
  CurrentFile    = NULL;

  Level += 1;
  CurrentFv->FvLevel += 1;

  Status = FvBufGetSize (Fv, &FvSize);

  ErasePolarity = (((EFI_FIRMWARE_VOLUME_HEADER*)Fv)->Attributes & EFI_FVB2_ERASE_POLARITY) ? TRUE : FALSE;

  if (!IsChildFv) {
    //
    // Write FV header information into CurrentFv struct.
    //
    CurrentFv->FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (sizeof (EFI_FIRMWARE_VOLUME_HEADER));

    if (CurrentFv->FvHeader == NULL) {
      return EFI_ABORTED;
    }

    //
    // Get the FV Header information
    //
    memcpy (CurrentFv->FvHeader, Fv, sizeof (EFI_FIRMWARE_VOLUME_HEADER));
    CurrentFv->FvExtHeader = NULL;

    //
    // Exist Extend FV header.
    //
    if (CurrentFv->FvHeader->ExtHeaderOffset != 0){
      CurrentFv->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) malloc (sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER));

      if (CurrentFv->FvExtHeader == NULL) {
        printf ("Out of resource, memory allocation failed. \n");
        return EFI_ABORTED;
      }

      //
      // Get the FV extended Header information
      //
      memcpy (CurrentFv->FvExtHeader, (VOID *)((UINTN)Fv + CurrentFv->FvHeader->ExtHeaderOffset), sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER));
      if (mFvGuidIsSet) {
        if (CompareGuid (&CurrentFv->FvExtHeader->FvName, &mFvNameGuid) == 0) {
          CurrentFv->IsInputFvFlag = TRUE;
        }
      }

    }

  }

  //
  // Put encapsulate information into structure.
  //
  if (CurrentFv->EncapData == NULL && !IsChildFv) {
    //
    // First time in, the root FV
    //
    CurrentFv->EncapData = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

    if (CurrentFv->EncapData == NULL) {
      return EFI_ABORTED;
    }
    CurrentFv->EncapData->FvExtHeader = NULL;
    CurrentFv->EncapData->Level = Level;
    CurrentFv->EncapData->Type  = BFM_ENCAP_TREE_FV;
    CurrentFv->EncapData->Data  = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (sizeof (EFI_FIRMWARE_VOLUME_HEADER));

    if (CurrentFv->EncapData->Data == NULL) {
      return EFI_ABORTED;
    }

    memcpy (CurrentFv->EncapData->Data, Fv, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

    if (((EFI_FIRMWARE_VOLUME_HEADER *)(CurrentFv->EncapData->Data))->ExtHeaderOffset != 0) {
      ExtHdrPtr = (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(CurrentFv->EncapData->Data))->ExtHeaderOffset);
      CurrentFv->EncapData->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) malloc (ExtHdrPtr->ExtHeaderSize);

      if (CurrentFv->EncapData->FvExtHeader == NULL) {
        printf ("Out of resource, memory allocation failed. \n");
        return EFI_ABORTED;
      }

      //
      // Get the FV extended Header information
      //
      memcpy(CurrentFv->EncapData->FvExtHeader, (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(CurrentFv->EncapData->Data))->ExtHeaderOffset), ExtHdrPtr->ExtHeaderSize);
      if (mFvGuidIsSet) {
        if (CompareGuid (&CurrentFv->EncapData->FvExtHeader->FvName, &mFvNameGuid) == 0) {
          CurrentFv->IsInputFvFlag = TRUE;
        }
      }

    }

    CurrentFv->EncapData->NextNode  = NULL;

  } else if (CurrentFv->EncapData == NULL) {
    return EFI_ABORTED;
  } else if (IsChildFv) {

      LocalEncapData = CurrentFv->EncapData;

      while (LocalEncapData->NextNode != NULL) {
        LocalEncapData = LocalEncapData->NextNode;
      }

      //
      // Construct the new ENCAP_DATA
      //
      LocalEncapData->NextNode = (ENCAP_INFO_DATA *) malloc (sizeof (ENCAP_INFO_DATA));

      if (LocalEncapData->NextNode == NULL) {
        return EFI_ABORTED;
      }

      LocalEncapData           = LocalEncapData->NextNode;

      LocalEncapData->Level = Level;
      LocalEncapData->Type  = BFM_ENCAP_TREE_FV;
      LocalEncapData->Data  = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (sizeof (EFI_FIRMWARE_VOLUME_HEADER));
      LocalEncapData->FvExtHeader = NULL;

      if (LocalEncapData->Data == NULL) {
        return EFI_ABORTED;
      }

      memcpy (LocalEncapData->Data, Fv, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

      if (((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset != 0) {
        ExtHdrPtr = (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset);
        LocalEncapData->FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)malloc(ExtHdrPtr->ExtHeaderSize);

        if (LocalEncapData->FvExtHeader == NULL) {
          printf ("Out of resource, memory allocation failed. \n");
          return EFI_ABORTED;
        }

        //
        // Get the FV extended Header information
        //
        memcpy(LocalEncapData->FvExtHeader, (VOID *)((UINTN)Fv + ((EFI_FIRMWARE_VOLUME_HEADER *)(LocalEncapData->Data))->ExtHeaderOffset), ExtHdrPtr->ExtHeaderSize);
      }

      LocalEncapData->NextNode  = NULL;

  }


  //
  // Get the first file
  //
  Status = FvBufFindNextFile (Fv, &Key, (VOID **) &CurrentFile);
  if (Status == EFI_NOT_FOUND) {
    CurrentFile = NULL;
  } else if (EFI_ERROR (Status)) {
    printf ("Failed to find the first file from Fv. \n");
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

    //
    // Display info about this file
    //
    Status = LibGetFileInfo (Fv, CurrentFile, ErasePolarity, CurrentFv, FvName, Level, FfsCount, ViewFlag);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Get the next file
    //
    Status = FvBufFindNextFile (Fv, &Key, (VOID **) &CurrentFile);
    if (Status == EFI_NOT_FOUND) {
      CurrentFile = NULL;
    } else if (EFI_ERROR (Status)) {
      return Status;
    }
  }

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
  TempDir = realloc (TempDir, _MAX_PATH);

  if (strlen (TempDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    printf ("The directory is too long \n");
    return EFI_ABORTED;
  }

  strncat (TempDir, OS_SEP_STR, _MAX_PATH - strlen (TempDir) - 1);
  strncat (TempDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TempDir) - 1);
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
    printf ("The directory is too long \n");
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
    printf ("The directory is too long \n");
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
  CHAR8*                 SystemCommand;

  SystemCommand             = NULL;

  if (FileIn   == NULL ||
      ToolName == NULL ||
      FileOut  == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SystemCommand = malloc (
    strlen (ENCODE_STR) +
    strlen (FileIn)  +
    strlen (ToolName)  +
    strlen (FileOut)  +
    1
    );

  if (NULL == SystemCommand) {
    return EFI_ABORTED;
  }

  sprintf (
    SystemCommand,
    ENCODE_STR,
    ToolName,
    FileIn,
    FileOut
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
    SystemCommand = malloc (
      strlen (ENCODE_STR_ERR) +
      strlen (FileIn)  +
      strlen (ToolName)  +
      strlen (FileOut)  +
      1
      );
    if (NULL == SystemCommand) {
      return EFI_ABORTED;
    }
    sprintf (
      SystemCommand,
      ENCODE_STR_ERR,
      ToolName,
      FileIn,
      FileOut
      );
    system (SystemCommand);
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
  IN     EFI_FVB_ATTRIBUTES_2   Attr,
  IN     FILE*                  InfFile
)
{
  CHAR8     *LocalStr;

  LocalStr  = NULL;

  LocalStr = (CHAR8 *) malloc (STR_LEN_MAX_4K);

  if (LocalStr == NULL) {
    printf ("Out of resource, memory allocation failed. \n");
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
    printf ("Error while write data to %p file. \n", (void*)InfFile);
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
  IN     EFI_FV_BLOCK_MAP_ENTRY  *BlockMap,
  IN     FILE*                   InfFile,
  IN     BOOLEAN                 IsRootFv
)
{
  CHAR8     *LocalStr;
  CHAR8     *BlockSize;
  CHAR8     *NumOfBlocks;

  LocalStr     = NULL;
  BlockSize    = NULL;
  NumOfBlocks  = NULL;

  if (BlockMap == NULL || InfFile  == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // This section will not over 1024 bytes and each line will never over 128 bytes.
  //
  LocalStr    = (CHAR8 *) malloc (STR_LEN_MAX_1K);
  BlockSize   = (CHAR8 *) malloc (128);
  NumOfBlocks = (CHAR8 *) malloc (128);

  if (LocalStr    == NULL ||
      BlockSize   == NULL ||
      NumOfBlocks == NULL) {
    if (LocalStr != NULL) {
      free (LocalStr);
    }
    if (BlockSize != NULL) {
      free (BlockSize);
    }
    if (NumOfBlocks != NULL) {
      free (NumOfBlocks);
    }
    return EFI_OUT_OF_RESOURCES;
  }

  memset (LocalStr, '\0', STR_LEN_MAX_1K);
  memset (BlockSize, '\0', 128);
  memset (NumOfBlocks, '\0', 128);

  strncat (LocalStr, "[options] \n", STR_LEN_MAX_1K - strlen (LocalStr) - 1);

  sprintf (BlockSize, "EFI_BLOCK_SIZE  = 0x%x \n", BlockMap->Length);
  strncat (LocalStr, BlockSize, STR_LEN_MAX_1K - strlen (LocalStr) - 1);

  if (IsRootFv) {
  sprintf (NumOfBlocks, "EFI_NUM_BLOCKS  = 0x%x \n", BlockMap->NumBlocks);
  strncat (LocalStr, NumOfBlocks, STR_LEN_MAX_1K - strlen (LocalStr) - 1);
  }

  if (fwrite (LocalStr, 1, (size_t) strlen (LocalStr), InfFile) != (size_t) strlen (LocalStr)) {
    free (LocalStr);
    free (BlockSize);
    free (NumOfBlocks);
    return EFI_ABORTED;
  }

  free (LocalStr);
  free (BlockSize);
  free (NumOfBlocks);

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
    printf ("Out of resource, memory allocation failed. \n");
    return EFI_OUT_OF_RESOURCES;
  }

  memset (LocalStr, '\0', _MAX_PATH);

  if (FirstIn) {
    sprintf (LocalStr, "[files] \nEFI_FILE_NAME = %s \n", FfsName);
  } else {
    sprintf (LocalStr, "EFI_FILE_NAME = %s \n", FfsName);
  }

  if (fwrite (LocalStr, 1, (size_t) strlen (LocalStr), InfFile) != (size_t) strlen (LocalStr)) {
    printf ("Error while write data to %p file. \n", (void*)InfFile);
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
  IN CHAR8*     InputFilePath,      OPTIONAL
  IN CHAR8*     Sections,           OPTIONAL
  IN UINT8      Type,
  IN CHAR8*     OutputFilePath,
  IN CHAR8*     UiString,           OPTIONAL
  IN CHAR8*     VerString,          OPTIONAL
  IN CHAR8*     GuidToolGuid,       OPTIONAL
  IN CHAR8*     CompressType        OPTIONAL
  )
{
  CHAR8*                 SystemCommand;
  SystemCommand             = NULL;

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
        SystemCommand = malloc (
          strlen (GENSEC_COMPRESSION) +
          strlen (mSectionTypeName[Type]) +
          strlen (CompressType) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          1
          );
        if (NULL == SystemCommand) {
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          GENSEC_COMPRESSION,
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
        SystemCommand = malloc (
          strlen (GENSEC_GUID) +
          strlen (mSectionTypeName[Type]) +
          strlen (GuidToolGuid) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          1
          );
        if (NULL == SystemCommand) {
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          GENSEC_GUID,
          mSectionTypeName[Type],
          GuidToolGuid,
          InputFilePath,
          OutputFilePath
          );
        if (system (SystemCommand) != EFI_SUCCESS) {
          free(SystemCommand);
          return EFI_ABORTED;
        }
        free(SystemCommand);
        break;

      case EFI_SECTION_FIRMWARE_VOLUME_IMAGE:
        SystemCommand = malloc (
          strlen (GENSEC_STR) +
          strlen (mSectionTypeName[Type]) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          1
          );
        if (NULL == SystemCommand) {
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          GENSEC_STR,
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

      case EFI_SECTION_RAW:
        SystemCommand = malloc (
          strlen (GENSEC_STR) +
          strlen (mSectionTypeName[Type]) +
          strlen (InputFilePath) +
          strlen (OutputFilePath) +
          1
          );
        if (NULL == SystemCommand) {
          return EFI_OUT_OF_RESOURCES;
        }
        sprintf (
          SystemCommand,
          GENSEC_STR,
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
        printf ("Please specify the section type while call GenSec tool.\n");
        return EFI_UNSUPPORTED;
    }
  } else {
    //
    // Create Dummy section.
    //
    SystemCommand = malloc (
      strlen (GENSEC_ALIGN) +
      strlen (InputFilePath) +
      strlen (OutputFilePath) +
      1
      );
    if (NULL == SystemCommand) {
      return EFI_OUT_OF_RESOURCES;
    }
    sprintf (
      SystemCommand,
      GENSEC_ALIGN,
      InputFilePath,
      OutputFilePath
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
  IN CHAR8*     FvGuidName
  )
{

  CHAR8*                 SystemCommand;
  CHAR8*                 FfsGuid = "8c8ce578-8a3d-4f1c-9935-896185c32dd3";

  SystemCommand             = NULL;

  if (OutputFilePath  == NULL ||
      InfFilePath     == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  if (InfFilePath != NULL) {
    if (FvGuidName == NULL) {
      SystemCommand = malloc (
        strlen (GENFV_STR) +
        strlen (InfFilePath)   +
        strlen (OutputFilePath)  +
        1
        );
      if (NULL == SystemCommand) {
        return EFI_OUT_OF_RESOURCES;
      }

      sprintf (
        SystemCommand,
        GENFV_STR,
        InfFilePath,          // -i
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
      SystemCommand = malloc (
        strlen (GENFV_FVGUID) +
        strlen (InfFilePath)   +
        strlen (OutputFilePath)  +
        strlen (FvGuidName) +
        1
        );
       if (NULL == SystemCommand) {
        return EFI_OUT_OF_RESOURCES;
      }

      sprintf (
        SystemCommand,
        GENFV_FVGUID,
        InfFilePath,          // -i
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

  if (InputFFSs != NULL) {
    SystemCommand = malloc (
      strlen (GENFV_FFS) +
      strlen (InputFFSs)   +
      strlen (FfsGuid)         +
      strlen (OutputFilePath)  +
      100
      );
    if (NULL == SystemCommand) {
      return EFI_OUT_OF_RESOURCES;
    }

    sprintf (
      SystemCommand,
      GENFV_FFS,
      InputFFSs,              // -f
      FfsGuid,                // -g
      OutputFilePath          // -o
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

  Convert a GUID to a string.


  @param[in]   Guid       - Pointer to GUID to print.


  @return The string after convert.

**/
CHAR8 *
LibBfmGuidToStr (
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
  IN UINT32     SectionAlign
  )
{
  CHAR8*                 SystemCommand;
  CHAR8*                 GuidStr;

  SystemCommand             = NULL;
  GuidStr                   = NULL;

  GuidStr  = LibBfmGuidToStr(&FileGuid);

  if (NULL == GuidStr) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Type == EFI_FV_FILETYPE_RAW) {
    SystemCommand = malloc (
      strlen (GENFFS_STR) +
      strlen (mFfsFileType[Type]) +
      strlen (InputFilePath) +
      strlen (GuidStr) +
      strlen (OutputFilePath) +
      1
      );
    if (NULL == SystemCommand) {
      free (GuidStr);
      return EFI_OUT_OF_RESOURCES;
    }
    sprintf (
      SystemCommand,
      GENFFS_STR,
      mFfsFileType[Type],     // -t
      InputFilePath,          // -i
      GuidStr,                // -g
      OutputFilePath          // -o
      );

    if (system (SystemCommand) != EFI_SUCCESS) {
      free(SystemCommand);
      free (GuidStr);
      return EFI_ABORTED;
    }
    free(SystemCommand);

  } else {
    //
    // -t  Type
    // -i  InputFilePath
    // -o  OutPutFilePath
    // -g  FileGuid
    // -x  Fixed
    // -n  SectionAlign
    //
    if (Fixed) {
      SystemCommand = malloc (
        strlen (GENFFS_FIX) +
        strlen (mFfsFileType[Type]) +
        strlen (InputFilePath) +
        strlen (GuidStr) +
        strlen (OutputFilePath) +
        1
        );
      if (NULL == SystemCommand) {
        free (GuidStr);
        return EFI_OUT_OF_RESOURCES;
      }
      sprintf (
        SystemCommand,
        GENFFS_FIX,
        mFfsFileType[Type],     // -t
        InputFilePath,          // -i
        GuidStr,                // -g
        OutputFilePath          // -o
        );
      if (system (SystemCommand) != EFI_SUCCESS) {
        free(SystemCommand);
        free (GuidStr);
        return EFI_ABORTED;
      }
      free(SystemCommand);
    } else {
      SystemCommand = malloc (
        strlen (GENFFS_STR) +
        strlen (mFfsFileType[Type]) +
        strlen (InputFilePath) +
        strlen (GuidStr) +
        strlen (OutputFilePath) +
        1
        );
      if (NULL == SystemCommand) {
        free (GuidStr);
        return EFI_OUT_OF_RESOURCES;
      }
      sprintf (
        SystemCommand,
        GENFFS_STR,
        mFfsFileType[Type],     // -t
        InputFilePath,          // -i
        GuidStr,                // -g
        OutputFilePath          // -o
        );
      if (system (SystemCommand) != EFI_SUCCESS) {
        free(SystemCommand);
        free (GuidStr);
        return EFI_ABORTED;
      }
      free(SystemCommand);
    }
  }
  free (GuidStr);
  return EFI_SUCCESS;
}

EFI_STATUS
LibCreateNewFdCopy(
  IN CHAR8*    OldFd,
  IN CHAR8*    NewFd
)
{
  CHAR8*                 SystemCommand;
  SystemCommand             = NULL;


  if (OldFd == NULL ||
      NewFd    == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create a copy the new file.
  //

  SystemCommand = malloc (
    strlen (COPY_STR) +
    strlen (OldFd)     +
    strlen (NewFd)  +
    1
    );
   if (NULL == SystemCommand) {
     return EFI_OUT_OF_RESOURCES;
   }

  sprintf (
    SystemCommand,
    COPY_STR,
    OldFd,
    NewFd
    );

  if (system (SystemCommand) != EFI_SUCCESS) {
    free(SystemCommand);
    return EFI_ABORTED;
  }
  free(SystemCommand);

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
  if (NULL == RetStr) {
    return NULL;
  }

  memset (RetStr, '\0', (strlen (FileName) + strlen (Dir) + strlen (Extend) + strlen("%s%s.%s") + 1));

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
  // Delete a directory and files in it
  //
  SystemCommand = malloc (
    strlen (RMDIR_STR) +
    strlen (DirName)     +
    1
    );
  if (NULL == SystemCommand) {
    return EFI_OUT_OF_RESOURCES;
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

/**
  Delete a file.

  @param[in]   FileName   Name of the file need to be deleted.

  @return EFI_INVALID_PARAMETER
  @return EFI_SUCCESS
**/
EFI_STATUS
LibBfmDeleteFile(
  IN   CHAR8    *FileName
)
{
  CHAR8*                 SystemCommand;

  SystemCommand             = NULL;


  if (FileName == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  //
  // Delete a file.
  //
  SystemCommand = malloc (
    strlen (DEL_STR) +
    strlen (FileName)     +
    1
    );
  if (NULL == SystemCommand) {
    return EFI_OUT_OF_RESOURCES;
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
LibBfmFreeFd (
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

  if (TempFv->FvHeader != NULL) {
      free (TempFv->FvHeader);
  }
    if (TempFv->FvExtHeader != NULL) {
      free (TempFv->FvExtHeader);
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
  FILE                        *UnCompressFile;
  FILE                        *CompressedFile;
  VOID                        *UnCompressedBuffer;
  VOID                        *CompressedBuffer;
  UINT32                      UnCompressedSize;
  UINT32                      CompressedSize;
  CHAR8                       *TempName;
  CHAR8                       *TemDir;
  CHAR8                       *TemString;
  EFI_STATUS                  Status;

  UnCompressFile     = NULL;
  CompressedFile     = NULL;
  UnCompressedBuffer = NULL;
  CompressedBuffer   = NULL;
  TempName           = NULL;
  TemDir             = NULL;
  TemString          = NULL;
  UnCompressedSize   = 0;
  CompressedSize     = 0;

  if ( InputFileName == NULL ||
    OutPutFileName == NULL) {
    printf ("Error while generate compressed section!\n");
    return EFI_INVALID_PARAMETER;
  }

  if (CompressionType == EFI_STANDARD_COMPRESSION) {
    UnCompressFile = fopen (InputFileName, "rb");
    if (UnCompressFile == NULL) {
      printf ("Error while open file %s \n", InputFileName);
      return EFI_ABORTED;
    }

    TemDir = getcwd (NULL, _MAX_PATH);
  TemDir = realloc (TemDir, _MAX_PATH);
    if (strlen (TemDir) + strlen (OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
      printf ("The directory is too long \n");
      fclose (UnCompressFile);
      return EFI_ABORTED;
    }
    strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen (TemDir) - 1);
    strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen (TemDir) - 1);
    TemString = GenTempFile ();
    TempName= LibFilenameStrExtended (strrchr(TemString, OS_SEP), TemDir, "comp");
    free (TemString);
    TemString = NULL;
    if (TempName == NULL) {
      fclose(UnCompressFile);
      return EFI_ABORTED;
    }

    CompressedFile = fopen (TempName, "wb+");
    if (CompressedFile == NULL) {
      printf ("Error while open file %s \n", TempName);
      fclose(UnCompressFile);
      free (TempName);
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
      printf ("Out of resource, memory allocation failed. \n");
      fclose (CompressedFile);
      fclose(UnCompressFile);
      free (TempName);
      return EFI_OUT_OF_RESOURCES;
    }

    CompressedBuffer = malloc (UnCompressedSize);

    if (CompressedBuffer == NULL) {
      printf ("Out of resource, memory allocation failed. \n");
      free (UnCompressedBuffer);
      fclose (CompressedFile);
      fclose(UnCompressFile);
      free (TempName);
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
        free (UnCompressedBuffer);
        free (CompressedBuffer);
        fclose (CompressedFile);
        fclose(UnCompressFile);
        free (TempName);
        return EFI_ABORTED;
      }

      if (CompressedSize > UnCompressedSize) {
        printf("Error while do compress operation! \n");
        free (UnCompressedBuffer);
        free (CompressedBuffer);
        fclose (CompressedFile);
        fclose(UnCompressFile);
        free (TempName);
        return EFI_ABORTED;
      }
    } else {
      printf("Error while reading file %s! \n", InputFileName);
      free (UnCompressedBuffer);
      free (CompressedBuffer);
      fclose (CompressedFile);
      fclose(UnCompressFile);
      free (TempName);
      return EFI_ABORTED;
    }

    //
    // Write the compressed data into output file
    //
    if (fwrite (CompressedBuffer, 1, (size_t) CompressedSize, CompressedFile) != (size_t) CompressedSize) {
      printf ("Error while writing %s file. \n", OutPutFileName);
      free (UnCompressedBuffer);
      free (CompressedBuffer);
      fclose(UnCompressFile);
      fclose (CompressedFile);
      free (TempName);
      return EFI_ABORTED;
    }

    fclose(UnCompressFile);
    fclose (CompressedFile);
    free (UnCompressedBuffer);
    free (CompressedBuffer);

    //
    // Call GenSec tool to generate the compressed section.
    //
    LibCreateFfsSection(TempName, NULL, EFI_SECTION_COMPRESSION, OutPutFileName, NULL, NULL, NULL, "PI_STD");
    free (TempName);
    TempName = NULL;

  } else if (CompressionType == EFI_NOT_COMPRESSED) {

    LibCreateFfsSection(InputFileName, NULL, EFI_SECTION_COMPRESSION, OutPutFileName, NULL, NULL, NULL, "PI_NONE");

  } else {
    printf ("Error while generate compressed section, unknown compression type! \n");
    return EFI_INVALID_PARAMETER;
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
          printf ( "Fail to allocate memory. \n");
          if (Tool != NULL) {
            FreeStringList (Tool);
          }
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

  FvIndex1                    = 0;
  FvIndex2                    = 0;
  FvFoundFlag                 = FALSE;

  if (FdData == NULL || FvId == NULL) {
    printf ( "Error while find FV in FD. \n");
    return EFI_INVALID_PARAMETER;
  }

  *FvInFd = FdData->Fv;

  FvIndex1 = (UINT8) atoi (FvId + 2);

  while (FvInFd != NULL) {
    if (((*FvInFd)->FvName) != NULL) {
      FvIndex2 = (UINT8) atoi ((*FvInFd)->FvName + 2);

      if ((FvIndex2 <= FvIndex1) && (((*FvInFd)->FvLevel + FvIndex2) -1 >= FvIndex1)) {
        FvFoundFlag = TRUE;
        break;
      }
      if ((*FvInFd)->FvNext == 0) {
        break;
      }
      *FvInFd = (*FvInFd)->FvNext;
    }
  }

  //
  // The specified FV id has issue, can not find the FV in FD.
  //
  if (!FvFoundFlag) {
    printf ( "Error while find FV in FD. \n");
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;

}


EFI_STATUS
LibPatchResetVectorAndFitTableData (
IN CHAR8                      *OutputFileName,
IN PATCH_DATA_PAD_FFS         *PatchData
)
{
  FILE*           NewFvFile;
  UINT64          NewFvLength;
  UINT8           *Buffer;
  UINT32          Count;


  Count           = 0;
  Buffer          = NULL;
  NewFvFile       = NULL;

  if (OutputFileName == NULL || PatchData == NULL) {
    return EFI_ABORTED;
  }

  NewFvFile = fopen (OutputFileName, "rb+");
  if (NewFvFile == NULL) {
    return EFI_ABORTED;
  }

  fseek(NewFvFile, 0, SEEK_SET);
  fseek(NewFvFile, 0, SEEK_END);

  NewFvLength = ftell(NewFvFile);

  do {

    //
    // The FV length should larger than Offset.
    //
    if (NewFvLength < PatchData->Offset) {
      fclose (NewFvFile);
      return EFI_ABORTED;
    }

    fseek(NewFvFile,PatchData->Offset,SEEK_SET);

    Buffer = (UINT8 *) malloc (PatchData->Length);

    if (Buffer == NULL) {
      fclose (NewFvFile);
      return EFI_ABORTED;
    }

    if (fread (Buffer, 1, (size_t) PatchData->Length, NewFvFile) != (size_t)  PatchData->Length) {
      fclose (NewFvFile);
      free(Buffer);
      return EFI_ABORTED;
    }

    //
    // The area used to patch data should be filled by 0xff.
    //
    for (Count = 0; Count< PatchData->Length; Count++) {
      if (Buffer[Count] != 0xff){
        fclose (NewFvFile);
        free(Buffer);
        return EFI_ABORTED;
      }
    }

    free(Buffer);

    fseek(NewFvFile,PatchData->Offset,SEEK_SET);

    if (fwrite (PatchData->Data, 1, (size_t) PatchData->Length, NewFvFile) != (size_t) PatchData->Length) {
      fclose (NewFvFile);
      return EFI_ABORTED;
    }

    PatchData = PatchData->NextNode;
  } while (PatchData != NULL);

  fclose (NewFvFile);

  return EFI_SUCCESS;
}

EFI_STATUS
LibEncapNewFvFile(
  IN     FV_INFORMATION              *FvInFd,
  IN     CHAR8                       *TemDir,
  OUT    CHAR8                       **OutputFile
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
  BOOLEAN                     FfsFoundFlag;
  UINT32                      Index;
  CHAR8                       *ExtractionTool;
  BOOLEAN                     IsLastLevelFfs;
  BOOLEAN                     IsLeafFlagIgnore;
  BOOLEAN                     FirstInFlag;
  CHAR8                       *InputFileName;
  CHAR8                       *OutputFileName;
  CHAR8                       *FvGuidName;

  Index                       = 0;
  ParentType                  = 0;
  ParentLevel                 = 0;
  Type                        = 0;
  Level                       = 0;
  FfsFoundFlag                = FALSE;
  ExtractionTool              = NULL;
  InputFileName               = NULL;
  OutputFileName              = NULL;
  IsLastLevelFfs              = TRUE;
  IsLeafFlagIgnore            = FALSE;
  FirstInFlag                 = TRUE;
  FvGuidName                  = NULL;

  //
  // Encapsulate from the lowest FFS file level.
  //
  LocalEncapData = FvInFd->EncapData;
  Level = LocalEncapData->Level;
  Type  = LocalEncapData->Type;

  //
  // Get FV Name GUID
  //

  while (LocalEncapData != NULL) {
    //
    // Has changed.
    //
    if (LocalEncapData->Level > Level) {
      if (LocalEncapData->Type == BFM_ENCAP_TREE_FFS) {
        ParentLevel = Level;
        ParentType  = Type;
      }

      Level       = LocalEncapData->Level;
      Type        = LocalEncapData->Type;
    }

    if (LocalEncapData->NextNode != NULL) {
      LocalEncapData = LocalEncapData->NextNode;
    } else {
      break;
    }
  }

  do {
    switch (ParentType) {
      case BFM_ENCAP_TREE_FV:

        //
        // Generate FV.inf attributes.
        //
        InfFileName = LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "inf");

        InfFile = fopen (InfFileName, "wt+");

        if (InfFile == NULL) {
          printf ("Could not open inf file %s to store FV information. \n", InfFileName);
          return EFI_ABORTED;
        }

        LocalEncapData = FvInFd->EncapData;
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
            printf ("Out of resource, memory allocation failed. \n");
            fclose (InfFile);
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
          Status = LibFvHeaderOptionToStr(((EFI_FIRMWARE_VOLUME_HEADER *)LocalEncapData->Data)->BlockMap, InfFile, TRUE);
        } else {
          Status = LibFvHeaderOptionToStr(((EFI_FIRMWARE_VOLUME_HEADER *)LocalEncapData->Data)->BlockMap, InfFile, FALSE);
        }


        if (EFI_ERROR (Status)) {
          printf ("Generate FV INF file [Options] section failed.\n");
          fclose (InfFile);
          if (FvGuidName != NULL) {
            free (FvGuidName);
          }
          return Status;
        }

        Status = LibFvHeaderAttributeToStr(((EFI_FIRMWARE_VOLUME_HEADER *)LocalEncapData->Data)->Attributes, InfFile);

        if (EFI_ERROR (Status)) {
          printf ("Generate FV header attribute failed.\n");
          if (FvGuidName != NULL) {
            free (FvGuidName);
          }
          fclose (InfFile);
          return Status;
        }
      if (LocalEncapData->FvExtHeader != NULL) {
        Status = LibGenExtFile(LocalEncapData->FvExtHeader, InfFile);
        if (FvGuidName != NULL) {
          free (FvGuidName);
        }
        if (EFI_ERROR(Status)) {
          printf("Generate FV EXT header failed.\n");
          fclose (InfFile);
          return Status;
        }
        FvGuidName = NULL;
      }

        //
        // Found FFSs from Fv structure.
        //
        FfsFoundFlag = FALSE;
        for (Index = 0; Index <= FvInFd->FfsNumbers; Index++) {

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
              Status = LibAddFfsFileToFvInf (FvInFd->FfsAttuibutes[Index].FfsName, InfFile, TRUE);

              if (EFI_ERROR (Status)) {
                printf ("Error while generate FV inf file [files] section. \n");
                fclose (InfFile);
                return Status;
              }

              FvInFd->FfsAttuibutes[Index].Level = 0;
              FirstInFlag = FALSE;
              } else {
                Status = LibAddFfsFileToFvInf (FvInFd->FfsAttuibutes[Index].FfsName, InfFile, FALSE);

                if (EFI_ERROR (Status)) {
                  printf ("Error while generate FV inf file [files] section. \n");
                  fclose (InfFile);
                  return Status;
                }

                FvInFd->FfsAttuibutes[Index].Level = 0;
              }
            FfsFoundFlag = TRUE;
            }
            //
            // Also add the sub FV
            //
            if (FvInFd->FfsAttuibutes[Index].Level - 1 == ParentLevel+ 1) {
              LocalEncapData = FvInFd->EncapData;
              while (LocalEncapData->NextNode != NULL) {
                if (LocalEncapData->Level == ParentLevel + 2) {
                  break;
                }
                LocalEncapData = LocalEncapData->NextNode;
              }

              if (LocalEncapData->Type == BFM_ENCAP_TREE_GUIDED_SECTION) {
                Status = LibAddFfsFileToFvInf (FvInFd->FfsAttuibutes[Index].FfsName, InfFile, FALSE);

                if (EFI_ERROR (Status)) {
                  printf ("Error while generate FV inf file [files] section.\n");
                  fclose (InfFile);
                  return Status;
                }

                FvInFd->FfsAttuibutes[Index].Level = 0;
              }

            }
          }

        IsLastLevelFfs = FALSE;
        FirstInFlag = TRUE;
        if (!FfsFoundFlag) {
          Status = LibAddFfsFileToFvInf (OutputFileName, InfFile, TRUE);
          if (EFI_ERROR (Status)) {
            printf ("Error while generate FV inf file [files] section.\n");
            fclose (InfFile);
            return Status;
          }
        }
        /*
        if (OutputFileName != NULL && FfsFoundFlag) {
          Status = LibAddFfsFileToFvInf (OutputFileName, InfFile, FALSE);

          if (EFI_ERROR (Status)) {
            //Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Generate FV inf file [files] section failed!");
            return Status;
          }
        }
        */
        //
        // Create FV
        //
        fclose (InfFile);
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "FV");
        Status = LibEncapsulateFfsToFv (InfFileName, NULL, OutputFileName, FvGuidName);
        if (FvGuidName != NULL) {
          free (FvGuidName);
        }
        if (EFI_ERROR (Status)) {
          return Status;
        }

        //
        // Patch FIT Table Data or reset vector data if exist.
        //
        if ((FvInFd->PatchData != NULL) && (1 == ParentLevel)) {
          Status = LibPatchResetVectorAndFitTableData(OutputFileName, FvInFd->PatchData);
          if (EFI_ERROR (Status)) {
            printf ("Error while patch FIT Table Data or reset vector data. \n");
            return Status;
          }
        }

        break;
      case BFM_ENCAP_TREE_FFS:
        if (OutputFileName != NULL) {
          InputFileName  = OutputFileName;
          OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "ffs");

          LocalEncapData = FvInFd->EncapData;
          while (LocalEncapData->NextNode != NULL) {
            if (LocalEncapData->Level == Level) {
              break;
            }
            LocalEncapData = LocalEncapData->NextNode;
          }

          Status = LibEncapSectionFileToFFS(EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, InputFileName, OutputFileName, ((EFI_FFS_FILE_HEADER *)LocalEncapData->Data)->Name, FALSE, 0);

          if (EFI_ERROR (Status)) {
            printf ("Error while generate FFS file. \n");
            return Status;
          }
        }
        break;
      case BFM_ENCAP_TREE_GUIDED_SECTION:
        //
        // Create the guided section original data, do compress operation.
        //
        InputFileName  = OutputFileName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "compressed");

        //
        // Use the guided section header guid to find out compress application name.
        //
        LocalEncapData = FvInFd->EncapData;
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

        Status = LibCreateGuidedSectionOriginalData (InputFileName, ExtractionTool, OutputFileName);

        if (EFI_ERROR (Status)) {
          printf ("Error while compress guided data. \n");
          return Status;
        }

        InputFileName  = OutputFileName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "guided");

        Status = LibCreateFfsSection(InputFileName, NULL, EFI_SECTION_GUID_DEFINED, OutputFileName, NULL, NULL, LibBfmGuidToStr((EFI_GUID *)LocalEncapData->Data), NULL);

        if (EFI_ERROR (Status)) {
          printf ("Error while generate guided section. \n");
          return Status;
        }

        break;
      case BFM_ENCAP_TREE_COMPRESS_SECTION:
        if (OutputFileName != NULL) {
          InputFileName  = OutputFileName;

          OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "comsec");

          LocalEncapData = FvInFd->EncapData;
          while (LocalEncapData->NextNode != NULL) {
            if (LocalEncapData->Level == ParentLevel) {
              break;
            }
            LocalEncapData = LocalEncapData->NextNode;
          }

          Status = LibGenCompressedSection (InputFileName, OutputFileName, *(UINT8 *)(LocalEncapData->Data));

          if (EFI_ERROR (Status)) {
            printf ("Error while generate compressed section. \n");
            return Status;
          }
        }
        break;
      case BFM_ENCAP_TREE_FV_SECTION:
        InputFileName  = OutputFileName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "sec");

        Status = LibCreateFfsSection(InputFileName, NULL, EFI_SECTION_FIRMWARE_VOLUME_IMAGE, OutputFileName, NULL, NULL, NULL, NULL);

        if (EFI_ERROR (Status)) {
          printf ("Error while generate FV section. \n");
          return Status;
        }

        InputFileName  = OutputFileName;
        OutputFileName= LibFilenameStrExtended (strrchr(GenTempFile (), OS_SEP), TemDir, "sec");

        //
        // Make it alignment.
        //
        Status = LibCreateFfsSection(InputFileName, NULL, 0, OutputFileName, NULL, NULL, NULL, NULL);

        if (EFI_ERROR (Status)) {
          printf ("Error while generate FV section. \n");
          return Status;
        }

        break;
      default:
        printf("Don't know how to encapsulate the FD file! \n");
        return EFI_ABORTED;
    }


    //
    // Find next level and encapsulate type
    //
    ParentLevel -= 1;
    LocalEncapData = FvInFd->EncapData;
    while (LocalEncapData->NextNode != NULL) {
      if (LocalEncapData->Level == ParentLevel) {
        ParentType = LocalEncapData->Type;
        break;
      }
      LocalEncapData = LocalEncapData->NextNode;
    }
  } while (ParentLevel != 0);


  *OutputFile = OutputFileName;

  return EFI_SUCCESS;

}

EFI_STATUS
LibLocateBfv(
  IN     FIRMWARE_DEVICE             *FdData,
  IN OUT CHAR8                       **FvId,
  IN OUT FV_INFORMATION              **FvInFd
)
{
  UINT8                       FvIndex1;
  UINT8                       FvIndex2;
  BOOLEAN                     FvFoundFlag;

  FvIndex1                    = 0;
  FvIndex2                    = 0;
  FvFoundFlag                 = FALSE;

  if (FdData == NULL || FvId == NULL || FvInFd == NULL) {
    return EFI_ABORTED;
  }

  *FvId = (*FvInFd)->FvName;

  FvIndex1 = (UINT8) atoi ((*FvInFd)->FvName + 2);

  *FvInFd = FdData->Fv;

  while (FvInFd != NULL) {
    if (((*FvInFd)->FvName) != NULL) {
      FvIndex2 = (UINT8) atoi ((*FvInFd)->FvName + 2);

      if ((FvIndex2 <= FvIndex1) && (((*FvInFd)->FvLevel + FvIndex2) -1 >= FvIndex1)) {
        FvFoundFlag = TRUE;
        break;
      }
      if ((*FvInFd)->FvNext == 0) {
        break;
      }
      *FvInFd = (*FvInFd)->FvNext;
    }
  }

  //
  // The specified FV id has issue, can not find the FV in FD.
  //
  if (!FvFoundFlag) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**

  Get the length of a file.

  @param[in]      FileName      The name of a file.

  @retval         The length of file.

**/
UINT64
GetFileSize (
  IN  CHAR8    *FileName
)
{
  FILE*        File;
  UINT64       Length;

  File = NULL;

  if (FileName == NULL) {
    return 0;
  }
  File = fopen(FileName, "r");

  if (File == NULL) {
    return 0;
  }
  fseek(File, 0L, SEEK_END);
  Length = ftell(File);
  fclose(File);

  return Length;
}

/**

  Get the length of BFV PAD file.

  @retval         The length of PAD file.

**/
UINT32
GetBfvPadSize (
  VOID
)
{
  return PadSizeOfBfv;
}

