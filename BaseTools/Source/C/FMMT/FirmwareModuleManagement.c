/** @file

 FMMT main routine.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FirmwareModuleManagement.h"
#include "Rebase.h"
#include <stdlib.h>
#include <wchar.h>

CHAR8*      mGuidToolDefinition     = "FmmtConf.ini";
extern EFI_FIRMWARE_VOLUME_HEADER  *mFvHeader;
extern UINT32                      mFvLength;

//
// Store GUIDed Section guid->tool mapping
//
EFI_HANDLE mParsedGuidedSectionTools = NULL;
#define EFI_FFS_VOLUME_TOP_FILE_GUID \
{ \
  0x1BA0062E, 0xC779, 0x4582, { 0x85, 0x66, 0x33, 0x6A, 0xE8, 0xF7, 0x8F, 0x09 } \
}
#define FSP_FFS_INFORMATION_FILE_GUID \
{ 0x912740be, 0x2284, 0x4734, { 0xb9, 0x71, 0x84, 0xb0, 0x27, 0x35, 0x3f, 0x0c }};

static EFI_GUID mVTFGuid = EFI_FFS_VOLUME_TOP_FILE_GUID;
static EFI_GUID mFSPGuid = FSP_FFS_INFORMATION_FILE_GUID;


/**

Routine Description:

  The Usage of FMMT tool.

Arguments:

  None

Returns:

  None

**/
VOID
Usage (
  VOID
  )
{
  //
  // Summary usage
  //
  fprintf (stdout, "Usage: %s [options] \n\n", UTILITY_SHORT_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2011 - 2019, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");

  //
  // Command Line for View
  //
  fprintf (stdout, "  -v <input-binary-file>\n\
            View each FV and the named files within each FV.\n");

  //
  // Command Line for Delete entire FV
  //
  fprintf (stdout, "  -d <input-binary-file> <FV-id> <output-binary-file>\n\
            Delete the entire FV in an FD binary\n");

  //
  // Command Line for Delete file from FV
  //
  fprintf (stdout, "  -d <input-binary-file> <FV-id> <File-Name> [<FV-id> <File-Name> ...] <output-binary-file>\n\
            Delete a file (or files) from the firmware volume in an FD binary\n");

  //
  // Command Line for Add
  //
  fprintf (stdout, "  -a <input-binary-file> <FV-id> <NewFilePath> [<FV-id> <NewFilePath> ...] <output-binary-file>\n\
            Add a file (or files) to the firmware volume in an FD binary\n");

  //
  // Command Line for Replace
  //
  fprintf (stdout, "  -r <input-binary-file> <FV-id> <File-Name> <NewFilePath> [<FV-id> <File-Name> <NewFilePath> ...] <output-binary-file>\n\
            The replace command combines the functionality of remove and add into a single operation.\n");

  fprintf (stdout, "\nNote:\n");
  fprintf (stdout, "  <FV-id> is the sequence of the firmware volume included in the FD image, it both support the sequentially format like FV0, FV1 and the FV's file guid value format.\n");
  return;
}


BOOLEAN
IsVtf(EFI_FFS_FILE_HEADER2* ffs) {
  if (!memcmp(&ffs->Name, &mVTFGuid, sizeof (EFI_GUID))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

BOOLEAN
IsFsp(EFI_FFS_FILE_HEADER2* ffs) {
  if (!memcmp(&ffs->Name, &mFSPGuid, sizeof (EFI_GUID))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

static
EFI_STATUS
GetBaseAddrFromVtf(FIRMWARE_DEVICE *FdData, CHAR8 *FvId, UINT64 *BaseAddr) {
  EFI_STATUS Status;
  FV_INFORMATION *CurrentFv;
  FV_INFORMATION *FvInFd;

  Status = LibLocateFvViaFvId(FdData, FvId, &FvInFd);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Get bottom FV
  //
  CurrentFv = FdData->Fv;
  while (CurrentFv->FvNext) {
    CurrentFv = CurrentFv->FvNext;
  }
  if (CurrentFv->FfsNumbers > 0 && IsVtf(&CurrentFv->FfsHeader[CurrentFv->FfsNumbers])) {
    //
    // Found VTF at the top of FV
    // Assume 4G address
    //
    *BaseAddr = 0x100000000 - (FdData->Size - FvInFd->ImageAddress);
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

static
EFI_STATUS
GetBaseAddrFromFsp(FIRMWARE_DEVICE *FdData, CONST UINT8* FdBuffer, CHAR8 *FvId, UINT64 *BaseAddr)
{
  EFI_STATUS Status;
  FV_INFORMATION *FvInFd;
  FV_INFORMATION *CurrentFv;
  FV_INFORMATION *FspFv;
  UINT32 Offset;
  UINT64 ImageSize;
  UINT64 Size;
  EFI_FFS_FILE_HEADER2 *CurrentFile;
  BOOLEAN FspFound;

  Status = LibLocateFvViaFvId(FdData, FvId, &FvInFd);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  ImageSize = 0;
  Size = 0;
  FspFound = FALSE;
  FspFv = NULL;
  CurrentFv = FdData->Fv;
  while (CurrentFv) {
    if (CurrentFv->FfsNumbers > 0 && IsFsp(&CurrentFv->FfsHeader[0])) {
      Offset = CurrentFv->ImageAddress + CurrentFv->FfsAttuibutes[0].Offset;
      CurrentFile = (EFI_FFS_FILE_HEADER2 *)(FdBuffer + Offset);
      //
      // Skip FFS header
      //
      Offset += GetFfsHeaderLength((EFI_FFS_FILE_HEADER *)CurrentFile);
      //
      // Stip section header
      //
      Offset += GetSectionHeaderLength((EFI_COMMON_SECTION_HEADER *)(FdBuffer + Offset));
      //
      // We have raw FSP here, and 24 is the image size
      //
      ImageSize = *((UINT32 *)(FdBuffer + Offset + 24));
      //
      // 28 is the base address
      //
      *BaseAddr = *((UINT32 *)(FdBuffer + Offset + 28));
      FspFound = TRUE;
      FspFv = CurrentFv;
    }
    if (CurrentFv == FvInFd){
      break;
    }
    CurrentFv = CurrentFv->FvNext;
  }
  if (!FspFound) {
    return EFI_NOT_FOUND;
  }
  //
  // Check if FSP binary contains this FV
  //
  while (FspFv != NULL) {
    Size += FspFv->FvHeader->FvLength;
    if (FspFv == FvInFd) {
      break;
    }
    FspFv = FspFv->FvNext;
  }
  if (Size <= ImageSize) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

static
VOID
AddPadFile(EFI_FIRMWARE_VOLUME_HEADER *Fv, EFI_FFS_FILE_HEADER2 *PadFile, UINT32 PadFileSize) {
  UINT32 hdrSize;

  if (Fv->Attributes & EFI_FVB2_ERASE_POLARITY) {
    memset(PadFile, -1, PadFileSize);
  }
  else {
    memset(PadFile, 0, PadFileSize);
  }
  PadFile->Type = EFI_FV_FILETYPE_FFS_PAD;
  PadFile->Attributes = 0;

  //
  // Write pad file size (calculated size minus next file header size)
  //
  if (PadFileSize >= MAX_FFS_SIZE) {
    memset(PadFile->Size, 0, sizeof(UINT8)* 3);
    ((EFI_FFS_FILE_HEADER2 *)PadFile)->ExtendedSize = PadFileSize;
    PadFile->Attributes |= FFS_ATTRIB_LARGE_FILE;
    hdrSize = sizeof(EFI_FFS_FILE_HEADER2);
  }
  else {
    PadFile->Size[0] = (UINT8)(PadFileSize & 0xFF);
    PadFile->Size[1] = (UINT8)((PadFileSize >> 8) & 0xFF);
    PadFile->Size[2] = (UINT8)((PadFileSize >> 16) & 0xFF);
    hdrSize = sizeof(EFI_FFS_FILE_HEADER);
  }

  //
  // Fill in checksums and state, they must be 0 for checksumming.
  //
  PadFile->IntegrityCheck.Checksum.Header = 0;
  PadFile->IntegrityCheck.Checksum.File = 0;
  PadFile->State = 0;
  PadFile->IntegrityCheck.Checksum.Header = CalculateChecksum8((UINT8 *)PadFile, hdrSize);
  PadFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;

  PadFile->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
  if (Fv->Attributes & EFI_FVB2_ERASE_POLARITY) {
    PadFile->State = (UINT8)~(PadFile->State);
  }
}

static
UINT8* ReadFileToBuffer(CONST CHAR8 *FdName, UINT32 *FdSize) {
  FILE* file;

  UINT8 *FdBuffer = NULL;

  file = fopen(FdName, "rb");
  if (file == NULL)
    return NULL;

  fseek(file, 0, SEEK_SET);
  fseek(file, 0, SEEK_END);
  *FdSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  FdBuffer = malloc(*FdSize);
  if (FdBuffer == NULL) {
    goto FAIL;
  }
  if (fread(FdBuffer, 1, *FdSize, file) != *FdSize) {
    goto FAIL;
  }
  fclose(file);
  return FdBuffer;
FAIL:
  free(FdBuffer);
  fclose(file);
  return NULL;
}

static UINT32 CalcuFfsSize(EFI_FIRMWARE_VOLUME_HEADER* Fv, CONST EFI_FFS_FILE_HEADER2 *Ffs) {
  EFI_FFS_FILE_HEADER2        *NextFile;
  UINTN FfsSize;
  UINTN FvSize;
  UINTN Offset;

  FfsSize = GetFfsFileLength((EFI_FFS_FILE_HEADER *)Ffs);
  FfsSize += (UINT8 *)ALIGN_POINTER(((UINT8 *)Ffs + FfsSize), 8) - ((UINT8 *)Ffs + FfsSize);
  FvBufGetSize(Fv, &FvSize);
  Offset = (UINT8 *)Ffs - (UINT8 *)Fv;
  if (Offset + FfsSize < FvSize) {
    NextFile = (EFI_FFS_FILE_HEADER2 *)((UINT8 *)Ffs + FfsSize);
    if (NextFile->Type == EFI_FV_FILETYPE_FFS_PAD) {
      FfsSize += GetFfsFileLength((EFI_FFS_FILE_HEADER *)NextFile);
    }
  }
  return FfsSize;
}

static
EFI_STATUS
ReadFfsAlignment(
IN EFI_FFS_FILE_HEADER    *FfsFile,
IN OUT UINT32             *Alignment
)
{
  //
  // Verify input parameters.
  //
  if (FfsFile == NULL || Alignment == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch ((FfsFile->Attributes >> 3) & 0x07) {

  case 0:
    //
    // 1 byte alignment
    //
    *Alignment = 0;
    break;

  case 1:
    //
    // 16 byte alignment
    //
    *Alignment = 4;
    break;

  case 2:
    //
    // 128 byte alignment
    //
    *Alignment = 7;
    break;

  case 3:
    //
    // 512 byte alignment
    //
    *Alignment = 9;
    break;

  case 4:
    //
    // 1K byte alignment
    //
    *Alignment = 10;
    break;

  case 5:
    //
    // 4K byte alignment
    //
    *Alignment = 12;
    break;

  case 6:
    //
    // 32K byte alignment
    //
    *Alignment = 15;
    break;

  case 7:
    //
    // 64K byte alignment
    //
    *Alignment = 16;
    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

static
BOOLEAN
ReplaceFfs(EFI_FIRMWARE_VOLUME_HEADER* Fv, EFI_FFS_FILE_HEADER2 *InputFfs, EFI_FFS_FILE_HEADER2 *OldFfs) {
  UINT32 FfsSize;
  UINT32 NewFileSize;
  UINT32 Offset;
  UINT32 Align;
  UINT32 HdrSize;
  UINT32 PadSize;
  EFI_FFS_FILE_HEADER2 *Pad;

  Align = 0;
  PadSize = 0;
  Pad = NULL;
  ReadFfsAlignment((EFI_FFS_FILE_HEADER *)InputFfs, &Align);
  Align = 1 << Align;
  HdrSize = GetFfsHeaderLength((EFI_FFS_FILE_HEADER *)InputFfs);

  FfsSize = CalcuFfsSize(Fv, OldFfs);
  //
  // Align data
  //
  if ((((UINT8 *)OldFfs - (UINT8 *)Fv) + HdrSize) % Align != 0) {
    PadSize = ((UINT8 *)OldFfs - (UINT8 *)Fv) + sizeof (EFI_FFS_FILE_HEADER)+HdrSize;
    while (PadSize % Align != 0) {
      PadSize++;
    }
    PadSize -= HdrSize;
    PadSize -= ((UINT8 *)OldFfs - (UINT8 *)Fv);
    if (FfsSize < PadSize) {
      return FALSE;
    }
    FfsSize -= PadSize;
    Pad = OldFfs;
    OldFfs = (EFI_FFS_FILE_HEADER2 *)((UINT8 *)OldFfs + PadSize);
  }

  NewFileSize = GetFfsFileLength((EFI_FFS_FILE_HEADER *)InputFfs);
  Offset = (UINT8 *)ALIGN_POINTER(((UINT8 *)OldFfs + NewFileSize), 8) - ((UINT8 *)OldFfs + NewFileSize);
  if (FfsSize >= NewFileSize && FfsSize - NewFileSize <= 7) {
    memcpy(OldFfs, (UINT8 *)InputFfs, NewFileSize);
    if (Fv->Attributes & EFI_FVB2_ERASE_POLARITY) {
      memset((UINT8 *)OldFfs + NewFileSize, -1, FfsSize - NewFileSize);
    }
    else {
      memset((UINT8 *)OldFfs + NewFileSize, 0, FfsSize - NewFileSize);
    }
  }
  else if (FfsSize >= NewFileSize + sizeof(EFI_FFS_FILE_HEADER) + Offset) {
    memcpy(OldFfs, (UINT8 *)InputFfs, NewFileSize);
    AddPadFile(
      Fv,
      (EFI_FFS_FILE_HEADER2 *)((UINT8 *)OldFfs + NewFileSize + Offset),
      FfsSize - NewFileSize - Offset
      );
  }
  else {
    return FALSE;
  }
  if (Fv->Attributes & EFI_FVB2_ERASE_POLARITY) {
    OldFfs->State = (UINT8)~(InputFfs->State);
  }
  if (PadSize != 0) {
    AddPadFile(Fv, Pad, PadSize);
  }
  return TRUE;
}

static

EFI_STATUS
AddFfs(UINT8 *FdBuffer, UINT32 ImageAddress, EFI_FIRMWARE_VOLUME_HEADER* Fv, EFI_FFS_FILE_HEADER2 *InputFfs, UINT32 *OffsetAdded) {
  UINTN FreeOffset;
  UINTN Offset;
  UINTN FfsSize;
  EFI_STATUS Status;
  EFI_FFS_FILE_HEADER2 *CurrentFile;
  EFI_FFS_FILE_HEADER FreeHeader;

  if (Fv->Attributes & EFI_FVB2_ERASE_POLARITY) {
    memset(&FreeHeader, -1, sizeof(EFI_FFS_FILE_HEADER));
  }
  else {
    memset(&FreeHeader, 0, sizeof(EFI_FFS_FILE_HEADER));
  }

  FfsSize = GetFfsFileLength((EFI_FFS_FILE_HEADER *)InputFfs);

  Offset = 0;
  CurrentFile = NULL;
  FreeOffset = 0;
  do {
    if (FreeOffset == 0 && memcmp(FdBuffer + ImageAddress + (UINTN)ALIGN_POINTER(Offset, 8), &FreeHeader, sizeof(EFI_FFS_FILE_HEADER)) == 0) {
      //
      // Offset of free FV space found
      //
      FreeOffset = (UINTN)ALIGN_POINTER(Offset, 8);
    }
    Status = FvBufFindNextFile(FdBuffer + ImageAddress, &Offset, (VOID **)&CurrentFile);
    if (Status == EFI_NOT_FOUND) {
      CurrentFile = NULL;
      break;
    }
    else if (EFI_ERROR(Status)) {
      return Status;
    }

    if (CurrentFile != NULL && CurrentFile->Type == EFI_FV_FILETYPE_FFS_PAD &&
      ReplaceFfs(Fv, InputFfs, CurrentFile)) {
      *OffsetAdded = (UINT8 *)CurrentFile - (FdBuffer + ImageAddress);
      return EFI_SUCCESS;
    }
  } while (CurrentFile != NULL);

  if (FreeOffset != 0) {
    if (Fv->FvLength - FreeOffset < FfsSize) {
      return EFI_ABORTED;
    }
    if (Fv->Attributes & EFI_FVB2_ERASE_POLARITY) {
      InputFfs->State = (UINT8)~(InputFfs->State);
    }
    memcpy(FdBuffer + ImageAddress + FreeOffset, InputFfs, FfsSize);
    *OffsetAdded = FreeOffset;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

static
EFI_STATUS
FindPreviousFile(VOID *Fv, VOID *CurrentFile, VOID **PreFile) {
  EFI_STATUS Status;
  VOID *File = NULL;
  UINTN Offset = 0;

  do {
    *PreFile = File;
    Status = FvBufFindNextFile(Fv, &Offset, &File);
    if (Status == EFI_NOT_FOUND) {
      CurrentFile = NULL;
      break;
    }
    else if (EFI_ERROR(Status)) {
      return Status;
    }
    if (File == CurrentFile) {
      return EFI_SUCCESS;
    }
  } while (CurrentFile != NULL);
  *PreFile = NULL;
  return Status;
}

static
BOOLEAN
NeedNewPath(FV_INFORMATION *FvInFd, CHAR8 *FvId, UINT32 FileIndex, BOOLEAN IsAdd) {
  UINT32 Index;

  Index = 0;

  if (strcmp(FvId, FvInFd->FvName) != 0) {
    return FALSE;
  }
  if (IsAdd) {
    return TRUE;
  }
  if (FvInFd->FfsAttuibutes[FileIndex].FvLevel != 1) {
    return FALSE;
  }

  for (Index = 0; Index <= FvInFd->FfsNumbers; Index++) {
    if (FvInFd->FfsAttuibutes[Index].FvLevel != 1) {
      continue;
    }
    switch (FvInFd->FfsHeader[Index].Type) {
    case EFI_FV_FILETYPE_PEI_CORE:
    case EFI_FV_FILETYPE_PEIM:
    case EFI_FV_FILETYPE_SECURITY_CORE:
      return TRUE;
    }
  }
  return FALSE;
}

static UINT32 FindFile(FV_INFORMATION *FvInFd, UINT8 FvLevel, CHAR8 *File, UINT32 *MatchIndex) {
  UINT32 Index = 0;
  CHAR16 *UIName;
  CHAR16 *FfsUIName;
  UINT32 FileNumber = 0;

  UIName = (CHAR16 *)malloc(_MAX_PATH);
  if (NULL == UIName) {
    return 0;
  }
  FfsUIName = (CHAR16 *)malloc(_MAX_PATH);
  if (NULL == FfsUIName) {
    free(UIName);
    return 0;
  }
  LibAscii2Unicode(File, UIName);
  for (Index = 0; Index <= FvInFd->FfsNumbers; Index++) {
    //
    // Compare the New File Name with UI Name of FFS
    // NOTE: The UI Name is Unicode, but the New File Name is Ascii.
    //
    memcpy(FfsUIName, (CHAR16 *)(FvInFd->FfsAttuibutes[Index].UiName), _MAX_PATH);

    if (FvInFd->FfsAttuibutes[Index].UiNameSize > 0 && memcmp(UIName, FfsUIName, FvInFd->FfsAttuibutes[Index].UiNameSize) == 0) {
      FileNumber += 1;
      *MatchIndex = Index;
      if (FileNumber > 1) {
        break;
      }
    }

  }
  free(UIName);
  free(FfsUIName);

  return FileNumber;
}

/**
  Search the config file from the path list.

  Split the path from env PATH, and then search the cofig
  file from these paths. The priority is from left to
  right of PATH string. When met the first Config file, it
  will break and return the pointer to the full file name.

  @param  PathList         the pointer to the path list.
  @param  FileName         the pointer to the file name.

  @retval The pointer to the file name.
  @return NULL       An error occurred.
**/
CHAR8 *
SearchConfigFromPathList (
  IN  CHAR8  *PathList,
  IN  CHAR8  *FileName
)
{
  CHAR8  *CurDir;
  CHAR8  *FileNamePath;

  CurDir       = NULL;
  FileNamePath = NULL;

#ifndef __GNUC__
  CurDir = strtok (PathList,";");
#else
  CurDir = strtok (PathList,":");
#endif
  while (CurDir != NULL) {
    FileNamePath  = (char *)calloc(
                     strlen (CurDir) + strlen (OS_SEP_STR) +strlen (FileName) + 1,
                     sizeof(char)
                     );
    if (FileNamePath == NULL) {
      return NULL;
    }
    sprintf(FileNamePath, "%s%c%s", CurDir, OS_SEP, FileName);
    if (access (FileNamePath, 0) != -1) {
      return FileNamePath;
    }
#ifndef __GNUC__
    CurDir = strtok(NULL, ";");
#else
    CurDir = strtok(NULL, ":");
#endif
    free (FileNamePath);
    FileNamePath = NULL;
  }
  return NULL;
}

UINT32 lenList(FILENode* head){
    FILENode *p = head;
    UINT32 sum=0;
    if(head==NULL) return 0;
    while(p!=NULL){
        sum+=1;
        p=p->Next;
    }
    return sum;
}

void sortList(FILENode* head){
    UINT32 len = lenList(head);
    FILENode *p = head;
    UINT32 i;
    UINT32 j;
    if(len==0) return;
    for(i=1; i<len; ++i){
        p = head;
        for(j=0; j<len-i; j++){
            if(p->SubLevel < p->Next->SubLevel){
                CHAR8 *FileName = p->FileName;
                UINT8 tmp = p->SubLevel;
                p->SubLevel = p->Next->SubLevel;
                p->Next->SubLevel = tmp;
                p->FileName = p->Next->FileName;
                p->Next->FileName = FileName;
            }
            p=p->Next;
        }
    }
}

BOOLEAN
ParseSection (
  IN  EFI_FFS_FILE_HEADER2  *InputFfs
)
{
  BOOLEAN             UISectionFlag;
  UINT32              SectionLength;
  UINT32              ParsedLength;
  UINT32              FfsFileSize;
  UINT8               *Ptr;
  EFI_SECTION_TYPE    Type;

  UISectionFlag       = FALSE;
  Ptr                 = NULL;
  SectionLength       = 0;
  ParsedLength        = GetFfsHeaderLength((EFI_FFS_FILE_HEADER *)InputFfs);
  FfsFileSize         = GetFfsFileLength((EFI_FFS_FILE_HEADER *)InputFfs);

  while (ParsedLength < FfsFileSize) {
    Ptr           = (UINT8 *)InputFfs + ParsedLength;
    SectionLength = GetLength (((EFI_COMMON_SECTION_HEADER *) Ptr)->Size);
    Type          = ((EFI_COMMON_SECTION_HEADER *) Ptr)->Type;

    //
    // This is sort of an odd check, but is necessary because FFS files are
    // padded to a QWORD boundary, meaning there is potentially a whole section
    // header worth of 0xFF bytes.
    //
    if ((SectionLength == 0xffffff) && (Type == 0xff)) {
      ParsedLength += 4;
      continue;
    }
    if (Type == EFI_SECTION_USER_INTERFACE) {
      UISectionFlag = TRUE;
      break;
    }
    ParsedLength += SectionLength;
    //
    // We make then next section begin on a 4-byte boundary
    //
    ParsedLength = GetOccupiedSize (ParsedLength, 4);
  }
  return UISectionFlag;

}


/**

  Show the FD image layout information. Only display the modules with UI name.

  @param[in]   FdInName    Input FD binary/image file name;
  @param[in]   FvName      The FV ID in the FD file;
  @param[in]   ViewFlag    Is this call for view or other operate(add/del/replace)
  @param[in]   FdData      The Fd data structure store the FD information.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
FmmtImageView (
  IN     CHAR8*           FdInName,
  IN     CHAR8*           FvName,
  IN     BOOLEAN          ViewFlag,
  IN     FIRMWARE_DEVICE  **FdData
)
{
  EFI_STATUS                  Status;
  EFI_STATUS                  ErrorStatus;
  FIRMWARE_DEVICE             *LocalFdData;
  FV_INFORMATION              *CurrentFv;
  FILE                        *InputFile;
  UINT32                      FvSize;
  UINTN                       BytesRead;
  EFI_FIRMWARE_VOLUME_HEADER  *FvImage;
  UINT32                      FfsCount;
  UINT8                       FvCount;
  CHAR8                       *TemDir;

  LocalFdData    = NULL;
  CurrentFv      = NULL;
  FvImage        = NULL;
  TemDir         = NULL;
  FvSize         = 0;
  BytesRead      = 0;
  FfsCount       = 0;
  FvCount        = 0;
  ErrorStatus    = EFI_SUCCESS;

  //
  // Check the FD file name/path.
  //
  if (FdInName == NULL) {
    Error("FMMT", 0, 1001, "Invalid parameter! Please specify <input-binary-file>", FdInName);
    Usage();
    return EFI_INVALID_PARAMETER;
  }

  //
  // Open the file containing the FV
  //
  InputFile = fopen (FdInName, "rb");
  if (InputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening the input file", FdInName);
    return EFI_INVALID_PARAMETER;
  }

  Status = LibFindFvInFd (InputFile, &LocalFdData);

  if (EFI_ERROR(Status)) {
    Error("FMMT", 0, 1001, "Error while search FV in FD", "");
    fclose (InputFile);
    return EFI_ABORTED;
  }

  CurrentFv = LocalFdData->Fv;


  do {

    memset (CurrentFv->FvName, '\0', _MAX_PATH);

    if (FvCount == 0) {
      sprintf (CurrentFv->FvName, "FV%d", FvCount);
    } else {
      sprintf (CurrentFv->FvName, "FV%d", FvCount);
    }

    //
    // Determine size of FV
    //
    if (fseek (InputFile, CurrentFv->ImageAddress, SEEK_SET) != 0) {
      Error (NULL, 0, 0003, "error parsing FV image", "%s FD file is invalid", InputFile);
      fclose (InputFile);
      ErrorStatus = EFI_ABORTED;
      goto Done;
    }

    Status = LibGetFvSize(InputFile, &FvSize);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "error parsing FV image", "%s Header is invalid", InputFile);
      fclose (InputFile);
      ErrorStatus = EFI_ABORTED;
      goto Done;
    }

    //
    // Seek to the start of the image, then read the entire FV to the buffer
    //
    fseek (InputFile, CurrentFv->ImageAddress, SEEK_SET);


    FvImage = (EFI_FIRMWARE_VOLUME_HEADER *) malloc (FvSize);

    if (FvImage == NULL) {
      Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
      fclose (InputFile);
      ErrorStatus = EFI_ABORTED;
      goto Done;
    }

    BytesRead = fread (FvImage, 1, FvSize, InputFile);
    if ((unsigned int) BytesRead != FvSize) {
      Error ("FMMT", 0, 0004, "error reading FvImage from", FdInName);
      free (FvImage);
      fclose (InputFile);
      ErrorStatus = EFI_ABORTED;
      goto Done;
    }

    //
    // Collect FV information each by each.
    //
    Status = LibGetFvInfo (FvImage, CurrentFv, FvName, 0, &CurrentFv->EncapData, &FfsCount, &FvCount, ViewFlag, FALSE);
    if (FvImage != NULL) {
      free (FvImage);
      FvImage = NULL;
    }
    if (EFI_ERROR (Status)) {
      Error ("FMMT", 0, 0004, "error while get information from FV %s", FvName);
      ErrorStatus = Status;
      //
      // If the FV to be parsed error is the same with the input FV in add, replace and delete
      // operation, abort the program directly.
      //
      if ((FvName != NULL) && ((CurrentFv->FvName) != NULL) && !strcmp(CurrentFv->FvName, FvName)) {
        fclose (InputFile);
        ErrorStatus = EFI_ABORTED;
        goto Done;
      }
    }


    FfsCount = 0;

    CurrentFv = CurrentFv->FvNext;

  } while (CurrentFv != NULL);

  fclose (InputFile);

  if (ViewFlag) {

    TemDir = getcwd (NULL, _MAX_PATH);
    TemDir = realloc (TemDir, _MAX_PATH);
    if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
      Error("FMMT", 0, 1001, "The directory is too long.", "");
      ErrorStatus = EFI_ABORTED;
      goto Done;
    }
    strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
    strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);

    mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);

    LibRmDir (TemDir);
  }
Done:
  if (!ViewFlag) {
    *FdData = LocalFdData;
  } else {
    LibFmmtFreeFd( LocalFdData);
  }
  return ErrorStatus;
}

/**
  Add FFS file into a specify FV.

  @param[in]   FdInName     Input FD binary/image file name;
  @param[in]   FileList     The FV ID and FFS file Data;
  @param[in]   count        The length of FileList;
  @param[in]   FdOutName    Name of output FD binary/image file.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
FmmtImageAdd(
  IN     CHAR8*    FdInName,
  IN     Data      *FileList,
  IN     int       count,
  IN     CHAR8*    FdOutName
)
{
  EFI_STATUS                  Status;
  FIRMWARE_DEVICE             *FdData;
  FV_INFORMATION              *FvInFd;
  FILE*                       NewFdFile;
  FILE*                       NewFvFile;
  UINT64                      NewFvLength;
  VOID*                       Buffer;
  CHAR8                       *TemDir;
  UINT8                       FvNumInFd;
  UINT8                       FvNumInFdCounter;
  UINT8                       NewAddedFfsLevel;
  FFS_INFORMATION             *OutputFileName;
  UINT32                      Index;
  UINT32                      EndId;
  UINT8                       *FdBuffer;
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  UINT32                      FdSize;
  EFI_FFS_FILE_HEADER2        *InputFfs;
  UINT32                      NewFileSize;
  UINT64                      BaseAddr;
  UINT32                      OffsetAdded;
  int                         i;
  int                         j;
  Data                        *tmp;
  CHAR8*                      FvId;
  CHAR8*                      NewFile;
  FILENode                    *NewFileNode;
  BOOLEAN                     HasUISection;
  HasUISection                = FALSE;
  Index                       = 0;
  EndId                       = 0;
  NewFvLength                 = 0;
  FvNumInFd                   = 0;
  FvNumInFdCounter            = 0;
  NewAddedFfsLevel            = 0;
  FdData                      = NULL;
  FvInFd                      = NULL;
  NewFdFile                   = NULL;
  NewFvFile                   = NULL;
  Buffer                      = NULL;
  TemDir                      = NULL;
  OutputFileName              = NULL;
  FvId                        = NULL;
  NewFile                     = NULL;

  FdBuffer                    = NULL;
  InputFfs                    = NULL;
  BaseAddr                    = 0;
  OffsetAdded                 = 0;
  FdSize                      = 0;

    for (i = 0; i < count; i ++){
        tmp = FileList + i;
        FvId = tmp->FvId;
        FdData = tmp->FdData;
        if (FdData == NULL) {
            Status = FmmtImageView (FdInName, FvId, FALSE, &FdData);
            if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
                Error ("FMMT", 0, 0004, "error while parsing FD Image", "Gathering FD information failed!");
                return Status;
            }
            if (FdData == NULL) {
                Error ("FMMT", 0, 0004, "error while parsing FD Image", "");
                return EFI_ABORTED;
            }

            Status = LibLocateFvViaFvId (FdData, FvId, &FvInFd);
            if (EFI_ERROR (Status)) {
                Error ("FMMT", 0, 0005, "error while locate FV in FD", "");
                return Status;
            }
            (FileList + i) -> FdData = FdData;
            (FileList + i) -> FvInFd = FvInFd;
            (FileList + i) -> FvLevel = FvInFd -> FvLevel;
        }
    }

  for (i = 0; i < count; i++) {
      for (j = i + 1; j < count; j++){
        if (((FileList + i)->FvId == NULL) || ((FileList + j)->FvId == NULL)) {
          continue;
        }
        if (strcmp((FileList + j)->FvId, (FileList + i)->FvInFd->FvName) == 0){
          NewFileNode = (FileList + j)->NewFile;
          while (NewFileNode ->Next != NULL) {
            NewFileNode = NewFileNode->Next;
          }
          NewFileNode->Next = (FileList + i)->NewFile;
          (FileList + i)->FvId = NULL;
            }
    }
    }

    for (i = 0; i < count; i ++){
        if ((FileList + i)->FvId == NULL) {
            continue;
        }
        sortList ((FileList + i)-> NewFile);
    }

    TemDir = getcwd(NULL, _MAX_PATH);
    TemDir = realloc (TemDir, _MAX_PATH);
    if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
      Error("FMMT", 0, 1001, "The directory is too long.", "");
      return EFI_ABORTED;
    }
    strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
    strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);

    if (FdBuffer == NULL) {
        FdBuffer = ReadFileToBuffer(FdInName, &FdSize);
        if (FdBuffer == NULL) {
            Error("FMMT", 0, 0004, "error while adding file", "cannot read input file.");
            return EFI_ABORTED;
        }
    }

    for (i = 0; i < count; i ++){
        tmp = FileList + i;
        FvId = tmp->FvId;
    if (FvId == NULL) {
            continue;
        }
        FdData = tmp->FdData;
        FvInFd = tmp->FvInFd;
        NewFileNode =tmp->NewFile;

        NewFile = NewFileNode->FileName;
        InputFfs = (EFI_FFS_FILE_HEADER2 *)ReadFileToBuffer(NewFile, &NewFileSize);
        if (InputFfs == NULL) {
            Error("FMMT", 0, 0004, "error while adding file", "cannot read input file.");
            Status = EFI_ABORTED;
            goto FAILED;
        }
        HasUISection = FALSE;
        HasUISection = ParseSection(InputFfs);
        if (!HasUISection) {
            printf ("WARNING: The newly add file must have a user interface (UI) section, otherwise it cannot be deleted or replaced. \n");
        }
        if (NeedNewPath(FvInFd, FvId, 0, TRUE)) {
            do {
                NewFile = NewFileNode->FileName;
                //
                // TODO: currently only root FV is handled
                //
                InputFfs = (EFI_FFS_FILE_HEADER2 *)ReadFileToBuffer(NewFile, &NewFileSize);
                if (InputFfs == NULL) {
                    Error("FMMT", 0, 0004, "error while adding file", "cannot read input file.");
                    Status =  EFI_ABORTED;
                    goto FAILED;
                }

                Fv = (EFI_FIRMWARE_VOLUME_HEADER *)(FdBuffer + FvInFd->ImageAddress);

                Status = AddFfs(FdBuffer, FvInFd->ImageAddress, Fv, InputFfs, &OffsetAdded);
                if (EFI_ERROR(Status)) {
                    Error("FMMT", 0, 0003, "error while adding file", "Not enough space to add FFS");
                    goto FAILED;
                }
                //
                // Calculate base address of Current FV
                //
                if (InputFfs->Type == EFI_FV_FILETYPE_PEIM || InputFfs->Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
                    Status = GetBaseAddrFromFsp(FdData, FdBuffer, FvId, &BaseAddr);
                    if (!EFI_ERROR(Status)) {
                        mFvHeader = FvInFd->FvHeader;
                        mFvLength = (UINT32)FvInFd->FvHeader->FvLength;
                        RebaseFfs(BaseAddr, NewFile, (EFI_FFS_FILE_HEADER *)(FdBuffer + FvInFd->ImageAddress + OffsetAdded), OffsetAdded);
                    }
                    else {
                        Status = GetBaseAddrFromVtf(FdData, FvId, &BaseAddr);
                        if (!EFI_ERROR(Status)) {
                            mFvHeader = FvInFd->FvHeader;
                            mFvLength = (UINT32)FvInFd->FvHeader->FvLength;
                            RebaseFfs(BaseAddr, NewFile, (EFI_FFS_FILE_HEADER *)(FdBuffer + FvInFd->ImageAddress + OffsetAdded), OffsetAdded);
                        }
                    }
                }
                NewFileNode = NewFileNode->Next;
                free (InputFfs);
                InputFfs = NULL;
            } while (NewFileNode != NULL);
        } else {
            do {
                NewFile = NewFileNode->FileName;
                if (strlen (NewFile) > _MAX_PATH - 1) {
                  Error ("FMMT", 0, 2000, "error while adding file", "New file name is too long!");
                  Status = EFI_ABORTED;
                  goto FAILED;
                }
                FvNumInFd = ((UINT8)atoi(FvId + 2) - (UINT8)atoi(FvInFd->FvName + 2));
                if (FvInFd->FfsNumbers == 0) {
                    NewAddedFfsLevel = FvInFd->FfsAttuibutes[0].Level;
                }
                for (Index = 0; Index <= FvInFd->FfsNumbers; Index++) {
                    if (FvInFd->FfsAttuibutes[Index].IsFvStart == 1) {
                        FvNumInFdCounter++;
                    }
                    if ( FvNumInFdCounter == FvNumInFd && FvInFd->FfsAttuibutes[Index].IsFvEnd == 1) {
                        NewAddedFfsLevel = FvInFd->FfsAttuibutes[Index].Level;
                        EndId = Index+1;
                        break;
                    }
                    if (FvInFd->FfsAttuibutes[Index].IsFvEnd == 1) {
                        FvNumInFdCounter--;
                        if (FvNumInFdCounter == 0) {
                            FvNumInFd--;
                        }
                    }
                }

                //
                // Add the new file into FV.
                //
                FvInFd->FfsNumbers += 1;
                for (Index = FvInFd->FfsNumbers; Index > EndId; Index--) {
                    FvInFd->FfsAttuibutes[Index] = FvInFd->FfsAttuibutes[Index - 1];
                }
                strncpy(FvInFd->FfsAttuibutes[EndId].FfsName, NewFile, _MAX_PATH - 1);
                FvInFd->FfsAttuibutes[EndId].FfsName[_MAX_PATH - 1] = 0;
                FvInFd->FfsAttuibutes[EndId].Level = NewAddedFfsLevel;
                memset (&FvInFd->FfsAttuibutes[EndId].GuidName, '\0', sizeof(EFI_GUID));
                if (EndId > 0) {
                    FvInFd->FfsAttuibutes[EndId].FvLevel = FvInFd->FfsAttuibutes[EndId - 1].FvLevel;
                    FvInFd->FfsAttuibutes[EndId - 1].IsFvEnd = 0;
                }
                FvInFd->FfsAttuibutes[EndId].IsFvEnd = 1;
                FvInFd->FfsAttuibutes[EndId].IsFvStart = 0;
                NewFileNode = NewFileNode->Next;
            } while (NewFileNode != NULL);

            mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);

            Status = LibEncapNewFvFile (FvInFd, TemDir, NULL, 0, &OutputFileName);
            if (EFI_ERROR (Status)) {
                Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Make new FV file failed!");
                goto FAILED;
            }

            NewFvFile = fopen (OutputFileName->FFSName, "rb+");
            if (NewFvFile == NULL) {
                Error ("FMMT", 0, 0003, "error Open FV file", "cannot Create a new FD file.");
                Status =  EFI_ABORTED;
                goto FAILED;
            }

            fseek(NewFvFile,0,SEEK_SET);
            fseek(NewFvFile,0,SEEK_END);

            NewFvLength = ftell(NewFvFile);
            fseek(NewFvFile,0,SEEK_SET);
            Buffer = malloc ((size_t)NewFvLength);
            if (Buffer == NULL)  {
                Status =  EFI_ABORTED;
                fclose(NewFvFile);
                goto FAILED;
            }

            if (fread (Buffer, 1, (size_t) NewFvLength, NewFvFile) != (size_t) NewFvLength) {
                Error ("FMMT", 0, 0003, "error reading FV file %s", OutputFileName->FFSName);
                free (Buffer);
                Status =  EFI_ABORTED;
                fclose(NewFvFile);
                goto FAILED;
            }

            if (NewFvLength <= FvInFd->FvHeader->FvLength) {
                memcpy(FdBuffer+FvInFd->ImageAddress,Buffer,(size_t) NewFvLength);
            } else {
                Error ("FMMT", 0, 0004, "error writing FD file", "The add ffs file is too large.");
            }
            fclose(NewFvFile);
            free(Buffer);
        }
    }

    LibRmDir(TemDir);

    NewFdFile = fopen(FdOutName, "wb");
    if (NewFdFile == NULL) {
        Error("FMMT", 0, 0004, "error while encapsulate FD Image", "Cannot open target FD file!");
        Status = EFI_ABORTED;
        goto FAILED;
    }

    fwrite(FdBuffer, 1, FdSize, NewFdFile);
    fclose(NewFdFile);
    free(FdBuffer);
    free(InputFfs);
    printf ("Create New FD file successfully. \n\nDone! \n");
    return EFI_SUCCESS;

  FAILED:
    if (FdBuffer != NULL) {
      free(FdBuffer);
    }
    if (InputFfs != NULL) {
      free(InputFfs);
    }
    return Status;
}

/**
Delete a root FV from FD.

@param[in]   FdInName     Input FD binary/image file name;
@param[in]   FvName       FV name;
@param[in]   FdOutName    Name of output fd file.

@retval      EFI_SUCCESS
@retval      EFI_INVALID_PARAMETER
@retval      EFI_ABORTED

**/
EFI_STATUS
FmmtImageDeleteFv(
  IN     CHAR8*  FdInName,
  IN     CHAR8*  FvName,
  IN     CHAR8*  FdOutName
)
{
  EFI_STATUS Status;
  FV_INFORMATION *FvInFd;
  FILE *NewFdFile;
  CHAR8 *TemDir;

  UINT8 *FdBuffer = NULL;
  UINT8 *FdBak = NULL;
  UINT32 FdSize = 0;

  FIRMWARE_DEVICE *FdData = NULL;

  TemDir = getcwd(NULL, _MAX_PATH);
  TemDir = realloc (TemDir, _MAX_PATH);
  if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    Error("FMMT", 0, 1001,  "The directory is too long.", "");
    return EFI_ABORTED;
  }
  strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
  strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);

  Status = FmmtImageView(FdInName, NULL, FALSE, &FdData);
  if (EFI_ERROR(Status) && Status != EFI_UNSUPPORTED) {
    Error("FMMT", 0, 0004, "error while parsing FD Image", "Gathering FD information failed!");
    goto END;
  }
  if (FdData == NULL) {
    Error("FMMT", 0, 0004, "error while parsing FD Image", "");
    Status = EFI_ABORTED;
    goto END;
  }

  FvInFd = FdData->Fv;
  while (FvInFd) {
    if (FvInFd->FvUiName && strcmp(FvInFd->FvUiName, FvName) == 0) {
      break;
    }
    FvInFd = FvInFd->FvNext;
  }
  if (!FvInFd) {
    Error("FMMT", 0, 0004, "error while deleting root FV", "Cannot find this FV!");
    Status = EFI_ABORTED;
    goto END;
  }
  FdBuffer = ReadFileToBuffer(FdInName, &FdSize);
  FdBak = FdBuffer;
  if (FdBuffer == NULL) {
    Error("FMMT", 0, 0004, "error while deleting root FV", "Cannot read FD file!");
    Status = EFI_ABORTED;
    goto END;
  }

  if (FvInFd->ImageAddress == 0) {
    FdBuffer = FdBuffer + FvInFd->FvHeader->FvLength;
    FdSize -= (UINT32)FvInFd->FvHeader->FvLength;
  } else {
    if (FvInFd->FvHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
      memset(FdBuffer + FvInFd->ImageAddress, -1, (size_t)FvInFd->FvHeader->FvLength);
    }
    else {
      memset(FdBuffer + FvInFd->ImageAddress, 0, (size_t)FvInFd->FvHeader->FvLength);
    }
  }

  NewFdFile = fopen(FdOutName, "wb");
  if (NewFdFile == NULL) {
    Error("FMMT", 0, 0004, "error while deleting root FV", "Cannot open target FD file!");
    Status = EFI_ABORTED;
    goto END;
  }
  fwrite(FdBuffer, 1, FdSize, NewFdFile);
  fclose(NewFdFile);

  LibRmDir(TemDir);

  printf("Create New FD file successfully. \n\nDone! \n");
END:
  LibFmmtFreeFd(FdData);
  free(FdBak);
  return Status;
}

/**
  Delete an FFS file from a specify FV.

  @param[in]   FdInName     Input FD binary/image file name;
  @param[in]   FileList     The FV ID and FFS file Data;
  @param[in]   count        The length of FileList;
  @param[in]   FdOutName    Name of output fd file.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
FmmtImageDelete (
  IN     CHAR8*    FdInName,
  IN     Data      *FileList,
  IN     int       count,
  IN     CHAR8*    FdOutName
)
{
  EFI_STATUS                  Status;
  FIRMWARE_DEVICE             *FdData;
  FV_INFORMATION              *FvInFd;
  UINT32                      Index;
  UINT32                      FfsFoundFlag;
  FFS_INFORMATION             *OutputFileName;
  FILE*                       NewFdFile;
  FILE*                       NewFvFile;
  UINT64                      NewFvLength;
  VOID*                       Buffer;
  CHAR8                       *TemDir;
  UINT8                       FvNumInFd;
  UINT32                      Offset;
  UINT8                       *FdBuffer;
  EFI_FFS_FILE_HEADER2        *CurrentFile;
  EFI_FFS_FILE_HEADER2        *PreFile;
  Data                        *tmp;
  CHAR8*                      FvId;
  CHAR8*                      DelFile;
  FILENode                    *OldFileNode;
  int                         i;
  UINT32                      FfsSize;
  UINT32                      FdSize;
  int                         j;

  FdSize                      = 0;
  Index                       = 0;
  NewFvLength                 = 0;
  FfsFoundFlag                = 0;
  FdData                      = NULL;
  FvInFd                      = NULL;
  OutputFileName              = NULL;
  NewFdFile                   = NULL;
  NewFvFile                   = NULL;
  Buffer                      = NULL;
  TemDir                      = NULL;
  FvNumInFd                   = 0;
  Offset                      = 0;
  FdBuffer                    = NULL;

    for (i = 0; i < count; i ++){
        tmp = FileList + i;
        FvId = tmp->FvId;
        FdData = tmp->FdData;
        if (FdData == NULL) {
            Status = FmmtImageView (FdInName, FvId, FALSE, &FdData);
            if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
                Error ("FMMT", 0, 0004, "error while parsing FD Image", "Gathering FD information failed!");
                return Status;
            }
            if (FdData == NULL) {
                Error ("FMMT", 0, 0004, "error while parsing FD Image", "");
                return EFI_ABORTED;
            }

            Status = LibLocateFvViaFvId (FdData, FvId, &FvInFd);
            if (EFI_ERROR (Status)) {
                Error ("FMMT", 0, 0005, "error while locate FV in FD", "");
                return Status;
            }
            (FileList + i) -> FdData = FdData;
            (FileList + i) -> FvInFd = FvInFd;
            (FileList + i) -> FvLevel = FvInFd -> FvLevel;
        }
        FvNumInFd = ((UINT8)atoi(FvId + 2) - (UINT8)atoi(FvInFd->FvName + 2)) + 1;
        OldFileNode = tmp-> OldFile;
        do {
            DelFile = OldFileNode->FileName;
            if (FvInFd == NULL) {
              break;
            }
            FfsFoundFlag = FindFile(FvInFd, FvNumInFd, DelFile, &Index);
            if (FfsFoundFlag) {
                if (FfsFoundFlag > 1) {
                    printf("Duplicated file found in this FV, file name: %s\n", DelFile);
                    return EFI_ABORTED;
                }
            } else {
                printf ("Could not found the FFS file from FD!, file name: %s\n", DelFile);
                return EFI_ABORTED;
            }
            OldFileNode -> SubLevel = FvInFd -> FfsAttuibutes[Index].Level;
            OldFileNode = OldFileNode->Next;
        } while (OldFileNode != NULL);
    }

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++)
        {
            if (((FileList + i)->FvId == NULL) || ((FileList + j)->FvId == NULL)) {
                continue;
            }
            if (strcmp((FileList + j)->FvId, (FileList + i)->FvInFd->FvName) == 0){
                OldFileNode = (FileList + j)->OldFile;
                while (OldFileNode ->Next != NULL) {
                    OldFileNode = OldFileNode->Next;
                }
                OldFileNode->Next = (FileList + i)->OldFile;
                (FileList + i)->FvId = NULL;
            }
            }
    }

    for (i = 0; i < count; i ++){
        if ((FileList + i)->FvId == NULL) {
            continue;
        }
        sortList ((FileList + i)-> OldFile);
    }

    TemDir = getcwd(NULL, _MAX_PATH);
    TemDir = realloc (TemDir, _MAX_PATH);
    if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
      Error("FMMT", 0, 1001, "The directory is too long.", "");
      return EFI_ABORTED;
    }
    strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
    strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);

    if (FdBuffer == NULL) {
        FdBuffer = ReadFileToBuffer(FdInName, &FdSize);
        if (FdBuffer == NULL) {
            Error("FMMT", 0, 0004, "error while deleting file", "cannot read input file.");
            return EFI_ABORTED;
        }
    }

    for (i = 0; i < count; i ++){
        tmp = FileList + i;
        FvId = tmp->FvId;
        if (FvId == NULL) {
            continue;
        }
        FdData = tmp->FdData;
        FvInFd = tmp->FvInFd;
        FvNumInFd = ((UINT8)atoi(FvId + 2) - (UINT8)atoi(FvInFd->FvName + 2)) + 1;
        OldFileNode = tmp->OldFile;
        DelFile = OldFileNode -> FileName;
        FfsFoundFlag = FindFile(FvInFd, FvNumInFd, DelFile, &Index);
        if (FfsFoundFlag && NeedNewPath(FvInFd, FvId, Index, FALSE)) {
            do {
                DelFile = OldFileNode -> FileName;
                FfsFoundFlag = FindFile(FvInFd, FvNumInFd, DelFile, &Index);
                //
                // TODO: currently only root FV is handled
                //
                Offset = FvInFd->ImageAddress + FvInFd->FfsAttuibutes[Index].Offset;
                if (FdBuffer != NULL) {
                  CurrentFile = (EFI_FFS_FILE_HEADER2 *)(FdBuffer + Offset);

                  FfsSize = CalcuFfsSize((EFI_FIRMWARE_VOLUME_HEADER *)(FdBuffer + FvInFd->ImageAddress), CurrentFile);

                  FindPreviousFile((EFI_FIRMWARE_VOLUME_HEADER *)(FdBuffer + FvInFd->ImageAddress), CurrentFile, (VOID **) &PreFile);
                  if (PreFile != NULL && PreFile->Type == EFI_FV_FILETYPE_FFS_PAD) {
                    FfsSize += (UINT8 *)CurrentFile - (UINT8 *)PreFile;
                    CurrentFile = PreFile;
                  }
                  AddPadFile((EFI_FIRMWARE_VOLUME_HEADER *)(FdBuffer + FvInFd->ImageAddress), CurrentFile, FfsSize);
                }
                OldFileNode = OldFileNode -> Next;
            } while (OldFileNode != NULL);
        } else {
            do {
                DelFile = OldFileNode -> FileName;
                FfsFoundFlag = FindFile(FvInFd, FvNumInFd, DelFile, &Index);

                if (FfsFoundFlag) {
                    //
                    // Delete this FFS file from Current FV structure.
                    //
                    Status = LibFmmtDeleteFile (FvInFd->FfsAttuibutes[Index].FfsName);
                    if (EFI_ERROR (Status)) {
                        Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Delete the specified file failed!");
                        Error ("FMMT", 0, 0004, "Cannot find the file need to delete", "Please check the name of the file you want to delete!");
                        goto FAILED;
                    }

                    memset(FvInFd->FfsAttuibutes[Index].FfsName, '\0', _MAX_PATH);
                    FvInFd->FfsAttuibutes[Index].Level   = 0xFF;

                    //
                    // Since we can avoid to add NULL ffs file, at this time we don't need to decrease the FFS number.
                    // If decrease operation executed, we should adjust the ffs list. It will bring in more complex.
                    //
                    //FvInFd->FfsNumbers                    -= 1;
                    memset(FvInFd->FfsAttuibutes[Index].UiName, '\0', _MAX_PATH * sizeof (CHAR16));
                   if (FvInFd->FfsAttuibutes[Index].FvLevel > 1) {
                       for (j = Index - 1; j >= 0; j--) {
                           if (FvInFd->FfsAttuibutes[j].FvLevel == FvInFd->FfsAttuibutes[Index].FvLevel - 1) {
                               break;
                           }
                       }
                      if (Index+1 <= FvInFd->FfsNumbers) {
                          if (FvInFd->FfsAttuibutes[Index+1].FvLevel == FvInFd->FfsAttuibutes[Index].FvLevel + 1) {
                             for (j = Index+1; j <= (signed)FvInFd->FfsNumbers; j++) {
                                if (FvInFd->FfsAttuibutes[j].FvLevel > FvInFd->FfsAttuibutes[Index].FvLevel) {
                                   Status = LibFmmtDeleteFile(FvInFd->FfsAttuibutes[j].FfsName);
                                   if (EFI_ERROR(Status)) {
                                       Error("FMMT", 0, 0004, "error while encapsulate FD Image", "Delete the specified file failed!");
                                       return Status;
                                   }
                                   memset(FvInFd->FfsAttuibutes[j].FfsName, '\0', _MAX_PATH);
                                   FvInFd->FfsAttuibutes[j].Level = 0xFF;
                                }
                             }
                          }
                      }
                   }
                } else {
                    printf ("Could not found the FFS file from FD!, file name: %s\n", DelFile);
                    Status =  EFI_ABORTED;
                    goto FAILED;
                }
                OldFileNode = OldFileNode -> Next;

            }while (OldFileNode != NULL);

            mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);

            Status = LibEncapNewFvFile (FvInFd, TemDir, NULL, 0, &OutputFileName);
            if (EFI_ERROR (Status)) {
                Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Make new FV file failed!");
                goto FAILED;
            }

            NewFvFile = fopen (OutputFileName->FFSName, "rb+");
            if (NewFvFile == NULL) {
                Error ("FMMT", 0, 0003, "error Open FV file", "cannot Create a new FD file.");
                Status =  EFI_ABORTED;
                goto FAILED;
            }

            fseek(NewFvFile,0,SEEK_SET);
            fseek(NewFvFile,0,SEEK_END);

            NewFvLength = ftell(NewFvFile);
            fseek(NewFvFile,0,SEEK_SET);
            Buffer = malloc ((size_t)NewFvLength);
            if (Buffer == NULL)  {
              fclose(NewFvFile);
              Status =  EFI_ABORTED;
              goto FAILED;
            }

            if (fread (Buffer, 1, (size_t) NewFvLength, NewFvFile) != (size_t) NewFvLength) {
                Error ("FMMT", 0, 0003, "error reading FV file %s", OutputFileName->FFSName);
                fclose(NewFvFile);
                free(Buffer);
                Status =  EFI_ABORTED;
                goto FAILED;
            }
            memcpy(FdBuffer+FvInFd->ImageAddress,Buffer,(size_t) NewFvLength);
            free(Buffer);
            fclose(NewFvFile);
        }
    }

    LibRmDir(TemDir);

    NewFdFile = fopen(FdOutName, "wb");
    if (NewFdFile == NULL) {
      Error("FMMT", 0, 0004, "error while encapsulate FD Image", "Cannot open target FD file!");
      Status = EFI_ABORTED;
      goto FAILED;
    }

    fwrite(FdBuffer, 1, FdSize, NewFdFile);
    fclose(NewFdFile);
    free(FdBuffer);
    printf("Create New FD file successfully. \n\nDone! \n");
    return EFI_SUCCESS;

  FAILED:
    free(FdBuffer);
    return Status;
}

/**
  Replace the exist FFS file with new one from a specify FV.

  @param[in]   FdInName     Input FD binary/image file name;
  @param[in]   FileList     The FV ID and FFS file Data;
  @param[in]   count        The length of FileList;
  @param[in]   FdOutName    Name of output fd file.

  @retval      EFI_SUCCESS
  @retval      EFI_INVALID_PARAMETER
  @retval      EFI_ABORTED

**/
EFI_STATUS
FmmtImageReplace (
  IN     CHAR8*    FdInName,
  IN     Data      *FileList,
  IN     int       count,
  IN     CHAR8*    FdOutName
)
{
  EFI_STATUS                  Status;
  FIRMWARE_DEVICE             *FdData;
  FV_INFORMATION              *FvInFd;
  UINT32                      Index;
  UINT32                      FfsFoundFlag;
  FFS_INFORMATION             *OutputFileName;
  FILE*                       NewFdFile;
  FILE*                       NewFvFile;
  UINT64                      NewFvLength;
  VOID*                       Buffer;
  CHAR8                       *TemDir;
  UINT32                      Offset;
  UINT8                       *FdBuffer;
  UINT8                       FvNumInFd;
  EFI_FFS_FILE_HEADER2        *CurrentFile;
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  UINT32                      FdSize;
  EFI_FFS_FILE_HEADER2        *InputFfs;
  UINT32                      NewFileSize;
  UINT32                      PadFileSize;
  UINT64                      BaseAddr;
  UINT32                      OffsetAdded;
  Data                        *tmp;
  CHAR8*                      FvId;
  CHAR8*                      OldFile;
  CHAR8*                      NewFile;
  int                         i;
  int                         j;
  FILENode                    *OldFileNode;
  FILENode                    *NewFileNode;
  BOOLEAN                     HasUISection;
  HasUISection                = FALSE;
  Index                       = 0;
  NewFvLength                 = 0;
  FfsFoundFlag                = 0;
  FdData                      = NULL;
  FvInFd                      = NULL;
  OutputFileName              = NULL;
  NewFdFile                   = NULL;
  NewFvFile                   = NULL;
  Buffer                      = NULL;
  TemDir                      = NULL;
  Offset                      = 0;
  FdBuffer                    = NULL;
  InputFfs                    = NULL;
  BaseAddr                    = 0;
  OffsetAdded                 = 0;
  FdSize                      = 0;

    for (i = 0; i < count; i ++){
        tmp = FileList + i;
        FvId = tmp->FvId;
        FdData = tmp->FdData;
        if (FdData == NULL){
            Status = FmmtImageView(FdInName, FvId, FALSE, &FdData);
            if (EFI_ERROR(Status) && Status != EFI_UNSUPPORTED) {
                Error("FMMT", 0, 0004, "error while parsing FD Image", "Gathering information failed!");
                return Status;
            }

            if (FdData == NULL) {
                Error("FMMT", 0, 0004, "error while parsing FD Image", "");
                return EFI_ABORTED;
            }

            Status = LibLocateFvViaFvId(FdData, FvId, &FvInFd);
            if (EFI_ERROR(Status)) {
                Error("FMMT", 0, 0005, "error while locate FV in FD", "");
                return Status;
            }
            (FileList + i) -> FdData = FdData;
            (FileList + i) -> FvInFd = FvInFd;
            (FileList + i) -> FvLevel = FvInFd -> FvLevel;
        }
        FvNumInFd = ((UINT8)atoi(FvId + 2) - (UINT8)atoi(FvInFd->FvName + 2)) + 1;
        OldFileNode = tmp-> OldFile;
        NewFileNode = tmp-> NewFile;
        do {
            OldFile = OldFileNode -> FileName;
            NewFile = NewFileNode -> FileName;
            if (FvInFd == NULL) {
              break;
            }
            FfsFoundFlag = FindFile(FvInFd, FvNumInFd, OldFile, &Index);

            if (FfsFoundFlag) {
                if (FfsFoundFlag > 1) {
                    printf("Duplicated file found in this FV, file name: %s\n", OldFile);
                    return EFI_ABORTED;
                }
                //
                // Replace this FFS file with the new one.
                // strcpy (FvInFd->FfsAttuibutes[Index].FfsName, NewFile);
            } else {
                printf ("Could not found the FFS file need to be replaced from FD! file name: %s\n", OldFile);
                return EFI_ABORTED;
            }
            OldFileNode -> SubLevel = FvInFd -> FfsAttuibutes[Index].Level;
            NewFileNode -> SubLevel = FvInFd -> FfsAttuibutes[Index].Level;
            OldFileNode = OldFileNode->Next;
            NewFileNode = NewFileNode->Next;
        } while (OldFileNode != NULL && NewFileNode != NULL);
    }

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++)
        {
            if (((FileList + i)->FvId == NULL) || ((FileList + j)->FvId == NULL)) {
                continue;
            }
            if (strcmp((FileList + j)->FvId, (FileList + i)->FvInFd->FvName) == 0){
                OldFileNode = (FileList + j)->OldFile;
                NewFileNode = (FileList + j)->NewFile;
                while (OldFileNode->Next != NULL && NewFileNode->Next != NULL) {
                    OldFileNode = OldFileNode->Next;
                    NewFileNode = NewFileNode->Next;
                }
                OldFileNode->Next = (FileList + i)->OldFile;
                NewFileNode->Next = (FileList + i)->NewFile;
                (FileList + i)->FvId = NULL;
            }
        }
    }

    for (i = 0; i < count; i ++){
        if ((FileList + i)->FvId == NULL) {
            continue;
        }
        sortList ((FileList + i)-> OldFile);
        sortList ((FileList + i)-> NewFile);
    }
    TemDir = getcwd (NULL, _MAX_PATH);
    TemDir = realloc (TemDir, _MAX_PATH);
    if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
      Error("FMMT", 0, 1001,  "The directory  is too long.", "");
      return EFI_ABORTED;
    }
    strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
    strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);
    mkdir(TemDir, S_IRWXU | S_IRWXG | S_IRWXO);
    if (FdBuffer == NULL) {
        FdBuffer = ReadFileToBuffer(FdInName, &FdSize);
        if (FdBuffer == NULL) {
          Error("FMMT", 0, 0004, "error while replacing file", "cannot read input file.");
          return EFI_ABORTED;
        }
    }

    for (i = 0; i < count; i ++){
        tmp = FileList + i;
        FvId = tmp->FvId;
        if (FvId == NULL) {
            continue;
        }
        FdData = tmp->FdData;
        FvInFd = tmp->FvInFd;
        FvNumInFd = ((UINT8)atoi(FvId + 2) - (UINT8)atoi(FvInFd->FvName + 2)) + 1;
        OldFileNode = tmp-> OldFile;
        OldFile = OldFileNode -> FileName;
        NewFileNode = tmp-> NewFile;
        FfsFoundFlag = FindFile(FvInFd, FvNumInFd, OldFile, &Index);

        NewFile = NewFileNode->FileName;
        InputFfs = (EFI_FFS_FILE_HEADER2 *)ReadFileToBuffer(NewFile, &NewFileSize);
        if (InputFfs == NULL) {
            Error("FMMT", 0, 0004, "error while replacing file", "cannot read input file.");
            free (FdBuffer);
            return EFI_ABORTED;
        }
        HasUISection = FALSE;
        HasUISection = ParseSection(InputFfs);
        if (!HasUISection) {
            printf ("WARNING: The newly replace file must have a user interface (UI) section, otherwise it cannot be deleted or replaced. \n");
        }
        if (FfsFoundFlag && NeedNewPath(FvInFd, FvId, Index, FALSE)) {
            do {
                OldFile = OldFileNode -> FileName;
                NewFile = NewFileNode -> FileName;
                FfsFoundFlag = FindFile(FvInFd, FvNumInFd, OldFile, &Index);
                //
                // TODO: currently only root FV is handled
                //
                InputFfs = (EFI_FFS_FILE_HEADER2 *)ReadFileToBuffer(NewFile, &NewFileSize);
                if (InputFfs == NULL) {
                    Error("FMMT", 0, 0004, "error while replacing file", "cannot read input file.");
                    free (FdBuffer);
                    return EFI_ABORTED;
                }

                Offset = FvInFd->ImageAddress + FvInFd->FfsAttuibutes[Index].Offset;
                Fv = (EFI_FIRMWARE_VOLUME_HEADER *)(FdBuffer + FvInFd->ImageAddress);
                OffsetAdded = FvInFd->FfsAttuibutes[Index].Offset;

                if (!ReplaceFfs(Fv, InputFfs, (EFI_FFS_FILE_HEADER2 *)(FdBuffer + Offset))) {
                  Status = AddFfs(FdBuffer, FvInFd->ImageAddress, Fv, InputFfs, &OffsetAdded);
                  if (EFI_ERROR(Status)) {
                    Error("FMMT", 0, 0003, "error while replacing file", "cannot add ffs");
                    goto END;
                  }
                  //
                  // Set original FFS to PAD file
                 //
                  CurrentFile = (EFI_FFS_FILE_HEADER2 *)(FdBuffer + FvInFd->ImageAddress + FvInFd->FfsAttuibutes[Index].Offset);
                  PadFileSize = CalcuFfsSize(Fv, CurrentFile);
                  AddPadFile(Fv, CurrentFile, PadFileSize);
                }

                //
                // Calculate base address of Current FV
                //
                if (InputFfs->Type == EFI_FV_FILETYPE_PEIM || InputFfs->Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
                  Status = GetBaseAddrFromFsp(FdData, FdBuffer, FvId, &BaseAddr);
                  if (!EFI_ERROR(Status)) {
                    mFvHeader = FvInFd->FvHeader;
                    mFvLength = (UINT32)FvInFd->FvHeader->FvLength;
                    RebaseFfs(BaseAddr, NewFile, (EFI_FFS_FILE_HEADER *)(FdBuffer + FvInFd->ImageAddress + OffsetAdded), OffsetAdded);
                  }
                  else {
                    Status = GetBaseAddrFromVtf(FdData, FvId, &BaseAddr);
                    if (!EFI_ERROR(Status)) {
                      mFvHeader = FvInFd->FvHeader;
                      mFvLength = (UINT32)FvInFd->FvHeader->FvLength;
                      RebaseFfs(BaseAddr, NewFile, (EFI_FFS_FILE_HEADER *)(FdBuffer + FvInFd->ImageAddress + OffsetAdded), OffsetAdded);
                    }
                  }
                }
                OldFileNode = OldFileNode -> Next;
                NewFileNode = NewFileNode -> Next;
                free (InputFfs);
                InputFfs = NULL;
            } while (OldFileNode != NULL && NewFileNode!= NULL);
        } else {
            do {
                OldFile = OldFileNode->FileName;
                NewFile = NewFileNode->FileName;
                FfsFoundFlag = FindFile(FvInFd, FvNumInFd, OldFile, &Index);
                //
                // Replace this FFS file with the new one.
                //
                if (strlen (NewFile) > _MAX_PATH - 1) {
                  Error ("FMMT", 0, 2000, "error while replacing file", "New file name is too long!");
                  free (FdBuffer);
                  return EFI_ABORTED;
                }
                strncpy(FvInFd->FfsAttuibutes[Index].FfsName, NewFile, _MAX_PATH - 1);
                FvInFd->FfsAttuibutes[Index].FfsName[_MAX_PATH - 1] = 0;
                OldFileNode = OldFileNode->Next;
                NewFileNode = NewFileNode->Next;
            } while (OldFileNode != NULL && NewFileNode != NULL);
            Status = LibEncapNewFvFile (FvInFd, TemDir, NULL, 0, &OutputFileName);
            if (EFI_ERROR (Status)) {
                Error ("FMMT", 0, 0004, "error while encapsulate FD Image", "Make new FV file failed!");
                free (FdBuffer);
                return Status;
            }

            NewFvFile = fopen (OutputFileName->FFSName, "rb+");
            if (NewFvFile == NULL) {
                Error ("FMMT", 0, 0003, "error Open FV file", "cannot Create a new FD file.");
                free (FdBuffer);
                return EFI_ABORTED;
            }

            fseek(NewFvFile,0,SEEK_SET);
            fseek(NewFvFile,0,SEEK_END);

            NewFvLength = ftell(NewFvFile);
            fseek(NewFvFile,0,SEEK_SET);
            Buffer = malloc ((size_t)NewFvLength);
            if (Buffer == NULL)  {
                fclose(NewFvFile);
                free (FdBuffer);
                return EFI_ABORTED;
            }

            if (fread (Buffer, 1, (size_t) NewFvLength, NewFvFile) != (size_t) NewFvLength) {
                Error ("FMMT", 0, 0003, "error reading FV file %s", OutputFileName->FFSName);
                free(Buffer);
                free (FdBuffer);
                fclose(NewFvFile);
                return EFI_ABORTED;
            }
            if (NewFvLength <= FvInFd->FvHeader->FvLength) {
                memcpy(FdBuffer+FvInFd->ImageAddress,Buffer,(size_t) NewFvLength);
            }else {
                Error ("FMMT", 0, 0004, "error writing FD file", "The replace ffs file is too large.");
            }
            free(Buffer);
            fclose(NewFvFile);
        }
    }

    LibRmDir(TemDir);

    NewFdFile = fopen(FdOutName, "wb");
    if (NewFdFile == NULL) {
      Error("FMMT", 0, 0004, "error while replacing file", "Cannot open target FD file!");
      Status = EFI_ABORTED;
      goto END;
    }
    fwrite(FdBuffer, 1, FdSize, NewFdFile);
    fclose(NewFdFile);
    free(FdBuffer);
    printf("Create New FD file successfully. \n\nDone! \n");
    return EFI_SUCCESS;

  END:
    if (FdBuffer != NULL) {
      free(FdBuffer);
    }
    if (InputFfs != NULL) {
      free(InputFfs);
    }
    return EFI_ABORTED;
}

/**

The main entry of FMMT.

@param  argc   The number of input parameters.
@param  *argv[]  The array pointer to the parameters.

@retval  0     The application exited normally.
@retval  1     An error occurred.

**/
int main(
  int      Argc,
  char     *Argv[]
)
{
  EFI_STATUS                    Status;
  CHAR8                         *TemDir;
  FILE                          *CheckFileExist;
  CHAR8                         *InFilePath;
  CHAR8                         FullGuidToolDefinition[_MAX_PATH];
  CHAR8                         *FileName;
  UINTN                         FileNameIndex;
  CHAR8                         *PathList;
  UINTN                         EnvLen;
  CHAR8                         *NewPathList;
  Data                          *FileData;
  int                           index;
  int                           count;
  int                           exist;
  int                           j;
  FILENode                      *p;
  FILENode                      *q;

  p                             = NULL;
  q                             = NULL;
  TemDir                        = NULL;
  CheckFileExist                = NULL;
  PathList                      = NULL;
  NewPathList                   = NULL;
  EnvLen                        = 0;
  count                         = 0;
  exist                         = -1;

  TemDir = getcwd (NULL, _MAX_PATH);
  TemDir = realloc (TemDir, _MAX_PATH);
  if (strlen (TemDir) + strlen(OS_SEP_STR) + strlen (TEMP_DIR_NAME) > _MAX_PATH - 1) {
    Error("FMMT", 0, 1001,  "The directory is too long.", "");
    return 1;
  }
  strncat (TemDir, OS_SEP_STR, _MAX_PATH - strlen(TemDir) - 1);
  strncat (TemDir, TEMP_DIR_NAME, _MAX_PATH - strlen(TemDir) - 1);

  //
  // Print utility header
  //
  printf ("Intel(R) %s. Version %d.%d, %s. %s.\n",
    UTILITY_NAME,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION,
    __DATE__,
    __BUILD_VERSION
    );

  //
  // Should have more than 1 arguments.
  //
  if (Argc <= 1) {
    Usage();
    return 1;
  }

  //
  // Workaroud: the first call to this function
  //            returns a file name ends with dot
  //
#ifndef __GNUC__
  tmpnam (NULL);
#else
  CHAR8 tmp[] = "/tmp/fileXXXXXX";
  UINTN Fdtmp;
  Fdtmp = mkstemp(tmp);
  close(Fdtmp);
#endif

  //
  // Save, skip filename arg
  //
  FileName = Argv[0];
  Argc--;
  Argv++;

  //
  // Get the same path with the application itself
  //
  if (strlen (FileName) > _MAX_PATH - 1) {
    Error ("FMMT", 0, 2000, "Parameter: The input filename is too long", NULL);
    return 1;
  }
  strncpy (FullGuidToolDefinition, FileName, _MAX_PATH - 1);
  FullGuidToolDefinition[_MAX_PATH - 1] = 0;
  FileNameIndex = strlen (FullGuidToolDefinition);
  while (FileNameIndex != 0) {
    FileNameIndex --;
    if (FullGuidToolDefinition[FileNameIndex] == OS_SEP) {
    FullGuidToolDefinition[FileNameIndex] = 0;
      break;
    }
  }
  //
  // Build the path list for Config file scan. The priority is below.
  // 1. Scan the current path
  // 2. Scan the same path with the application itself
  // 3. Scan the current %PATH% of OS environment
  // 4. Use the build-in default configuration
  //
  PathList = getenv("PATH");
  if (PathList == NULL) {
    Error (NULL, 0, 1001, "Option: Environment variable 'PATH' does not exist", NULL);
    return 1;
  }
  EnvLen = strlen(PathList);
  NewPathList  = (char *)calloc(
                     strlen (".")
                     + strlen (";")
                     + strlen (FullGuidToolDefinition)
                     + strlen (";")
                     + EnvLen
                     + 1,
                     sizeof(char)
                  );
  if (NewPathList == NULL) {
    Error (NULL, 0, 4001, "Resource: Memory can't be allocated", NULL);
    PathList = NULL;
    free (PathList);
    return 1;
  }
#ifndef __GNUC__
  sprintf (NewPathList, "%s;%s;%s", ".", FullGuidToolDefinition, PathList);
#else
  sprintf (NewPathList, "%s:%s:%s", ".", FullGuidToolDefinition, PathList);
#endif
  PathList = NULL;
  free (PathList);

  //
  // Load Guid Tools definition
  //
  InFilePath = SearchConfigFromPathList(NewPathList, mGuidToolDefinition);
  free (NewPathList);
  if (InFilePath != NULL) {
    printf ("\nThe Guid Tool Definition comes from the '%s'. \n", InFilePath);
    mParsedGuidedSectionTools = ParseGuidedSectionToolsFile (InFilePath);
    free (InFilePath);
  } else {
    //
    // Use the pre-defined standard guided tools.
    //
  printf ("\nThe Guid Tool Definition comes from the build-in default configuration. \n");
    mParsedGuidedSectionTools = LibPreDefinedGuidedTools ();
  }

  if ((strcmp(Argv[0], "-v") == 0) || (strcmp(Argv[0], "-V") == 0)) {
    //
    // View the FD binary image information.
    //
    if (Argc <= 1) {
      Error("FMMT", 0, 1001, "Invalid parameter, Please make sure the parameter is correct.", "");
      Usage ();
      return 1;
    }

    //
    // Open the file containing the FV to check whether it exist or not
    //
    CheckFileExist = fopen (Argv[1], "rb");
    if (CheckFileExist == NULL) {
      Error ("FMMT", 0, 0001, "Error opening the input binary file, Please make sure the <input-binary-file> exist!", Argv[1]);
      return 1;
    }
    fclose(CheckFileExist);

    Status = FmmtImageView(Argv[1], NULL, TRUE, NULL);

    if (EFI_ERROR (Status)) {
      Error("FMMT", 0, 1001,  "Error while view the FD image file.", "");
      LibRmDir (TemDir);
      return 1;
    }

  } else if ((strcmp(Argv[0], "-d") == 0) || (strcmp(Argv[0], "-D") == 0)) {
    //
    // Delete some named FFS file from FD binary image.
    //
    if (!((Argc == 4) || ((Argc - 3) % 2 == 0))) {
      Error("FMMT", 0, 1001,  "Invalid parameter, Please make sure the parameter is correct.", "");
      Usage ();
      return 1;
    }

    //
    // Open the file containing the FV to check whether it exist or not
    //
    CheckFileExist = fopen (Argv[1], "rb");
    if (CheckFileExist == NULL) {
      Error ("FMMT", 0, 0001, "Error opening the input binary file, Please make sure the <input-binary-file> exist!", Argv[1]);
      return 1;
    }
    fclose(CheckFileExist);

    if ((Argc - 3) % 2 == 0) {
        FileData = malloc(sizeof (Data) * (Argc - 3)/2);
        if (FileData == NULL) {
          Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
          return 1;
        }
        for(index = 0; index < (Argc - 3)/2; index ++) {
            p = malloc(sizeof (FILENode));
            if (p == NULL) {
              Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
              free (FileData);
              return 1;
            }
            p -> FileName = Argv[3 + index * 2];
            p -> SubLevel = 0;
            exist = -1;
            for (j = 0; j < count; j ++) {
                if ((strcmp(Argv[2 + index * 2], (FileData + j) -> FvId) == 0)) {
                    exist = j;
                    break;
                }
            }
            if (exist >= 0) {
                p -> Next = (FileData + j) -> OldFile;
                (FileData + j) -> OldFile = p;
            } else {
                (FileData + count) -> NewFile = NULL;
                (FileData + count) -> FdData = NULL;
                (FileData + count) -> FvLevel = 0;
                (FileData + count) -> FvInFd = NULL;
                (FileData + count) -> FvId = Argv[2 + index * 2];;
                (FileData + count) -> OldFile = p;
                p -> Next = NULL;
                count ++;
            }
        }

        if (count <= 0) {
            Error("FMMT", 0, 0004, "error while parsing FD Image", "Gathering information failed!");
        }
        for (index = 0; index < count; index ++) {
            for (j = index + 1; j < count; j ++) {
                if ((strcmp((FileData + index)->FvId, (FileData + j)->FvId) < 0)) {
                    CHAR8 *tmp = (FileData + index)->FvId;
                    FILENode *t = (FileData + index)->OldFile;
                    (FileData + index)->FvId = (FileData + j)->FvId;
                    (FileData + index)-> OldFile = (FileData + j)->OldFile;
                    (FileData + j)-> OldFile = t;
                    (FileData + j)-> FvId = tmp;
                }
            }
        }

        //
        // Delete some FFS file
        //
        Status = FmmtImageDelete(Argv[1], FileData, count, Argv[Argc-1]);
        for (index = 0; index < count; index ++) {
          if ((FileData + index) ->NewFile != NULL) {
            free ((FileData + index)->NewFile);
            (FileData + index)->NewFile = NULL;
          }
          if ((FileData + index)->OldFile != NULL) {
            free ((FileData + index)->OldFile);
            (FileData + index)->OldFile = NULL;
          }
        }
        for (index = 0; index < count; index ++) {
          if ((FileData + index)->FdData != NULL) {
            LibFmmtFreeFd ((FileData + index)->FdData);
          }
        }
        free (FileData);
        if (EFI_ERROR (Status)) {
            Error("FMMT", 0, 1001,  "Error while delete some named ffs file from the FD image file.", "");
            LibRmDir (TemDir);
            return 1;
        }
    } else {
        //
        // Delete FV
        //
        Status = FmmtImageDeleteFv(Argv[1], Argv[2], Argv[3]);
        if (EFI_ERROR (Status)) {
            Error("FMMT", 0, 1001,  "Error while delete the entire FV from the FD image file.", "");
            LibRmDir (TemDir);
            return 1;
        }
    }

  } else if ((strcmp(Argv[0], "-a") == 0) || (strcmp(Argv[0], "-A") == 0)) {
    //
    // Add some named FFS file into FD binary image.
    //
    if ((Argc - 3 ) % 2 != 0) {
      Error("FMMT", 0, 1001,  "Invalid parameter, Please make sure the parameter is correct.", "");
      Usage ();
      return 1;
    }

    //
    // Open the file containing the FV to check whether it exist or not
    //
    CheckFileExist = fopen (Argv[1], "rb");
    if (CheckFileExist == NULL) {
      Error ("FMMT", 0, 0001, "Error opening the input binary file, Please make sure the <input-binary-file> exist!", Argv[1]);
      return 1;
    }
    fclose(CheckFileExist);

    //
    // Check whether the new added file exist or not.
    //
    for (index = 1; index < (Argc - 1) / 2; index ++) {
        CheckFileExist = fopen(Argv[2 * index + 1], "rb");
        if (CheckFileExist == NULL) {
            Error("FMMT", 0, 0001, "Could not open the new FFS file, Please make sure the new FFS file exist.", Argv[2 * index + 1]);
            return 1;
        }
        fclose(CheckFileExist);
    }

    FileData = malloc(sizeof (Data) * (Argc - 3)/2);
    if (FileData == NULL) {
      Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
      return 1;
    }
    for(index = 0; index < (Argc - 3)/2; index ++) {
        p = malloc(sizeof (FILENode));
        if (p == NULL) {
          Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
          free (FileData);
          return 1;
        }
        p -> FileName = Argv[3 + index * 2];
        p -> SubLevel = 0;
        exist = -1;
        for (j = 0; j < count; j ++) {
            if ((strcmp(Argv[2 + index * 2], (FileData + j) -> FvId) == 0)) {
                exist = j;
                break;
            }
        }
        if (exist >= 0) {
            p -> Next = (FileData + j) -> NewFile;
            (FileData + j) -> NewFile = p;
        } else {
            (FileData + count) -> OldFile = NULL;
            (FileData + count) -> FdData = NULL;
            (FileData + count) -> FvLevel = 0;
            (FileData + count) -> FvInFd = NULL;
            (FileData + count) -> FvId = Argv[2 + index * 2];
            (FileData + count) -> NewFile = p;
            p -> Next = NULL;
            count ++;
        }
    }

    if (count <= 0) {
        Error("FMMT", 0, 0004, "error while parsing FD Image", "Gathering information failed!");
    }

  for (index = 0; index < count; index ++) {
    for (j = index + 1; j < count; j ++) {
      if ((strcmp((FileData + index)->FvId, (FileData + j)->FvId) < 0)) {
        CHAR8 *tmp = (FileData + index)->FvId;
        FILENode *temp = (FileData + index)->NewFile;
        (FileData + index)->FvId = (FileData + j)->FvId;
        (FileData + index)-> NewFile = (FileData + j)->NewFile;
        (FileData + j)-> NewFile = temp;
        (FileData + j)-> FvId = tmp;
      }
    }
  }

    Status = FmmtImageAdd(Argv[1], FileData, count, Argv[Argc-1]);
    for (index = 0; index < count; index ++) {
      if ((FileData + index)->NewFile != NULL) {
        free ((FileData + index)->NewFile);
        (FileData + index)->NewFile = NULL;
      }
      if ((FileData + index)->OldFile != NULL) {
        free ((FileData + index)->OldFile);
        (FileData + index)->OldFile = NULL;
      }
    }
    for (index = 0; index < count; index ++) {
      if ((FileData + index)->FdData != NULL) {
        LibFmmtFreeFd ((FileData + index)->FdData);
      }
    }
    free (FileData);

    if (EFI_ERROR (Status)) {
      Error("FMMT", 0, 1001,  "Error while add some named ffs file into the FD image file.", "");
      LibRmDir (TemDir);
      return 1;
    }

  } else if ((strcmp(Argv[0], "-r") == 0) || (strcmp(Argv[0], "-R") == 0)) {
    //
    // Replace some named FFS file in the FD binary.
    //
    if ((Argc - 3) % 3 != 0) {
      Error("FMMT", 0, 1001,  "Invalid parameter, Please make sure the parameter is correct.", "");
      Usage();
      return 1;
    }

    //
    // Open the file containing the FV to check whether it exist or not
    //
    CheckFileExist = fopen (Argv[1], "rb");
    if (CheckFileExist == NULL) {
      Error ("FMMT", 0, 0001, "Error opening the input binary file, Please make sure the <input-binary-file> exist!", Argv[1]);
      return 1;
    }
    fclose(CheckFileExist);

    //
    // Check whether the new FFS file exist or not.
    //
    for (index = 1; index < Argc/3; index ++) {
        CheckFileExist = fopen(Argv[3 * index + 1], "rb");
        if (CheckFileExist == NULL) {
            Error ("FMMT", 0, 0001, "Could not open the new FFS file, Please make sure the new FFS file exist.", Argv[3 * index + 1]);
            return 1;
       }
       fclose(CheckFileExist);
    }

    FileData = malloc(sizeof (Data) * (Argc - 3)/3);
    if (FileData == NULL) {
      Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
      return 1;
    }
    for(index = 0; index < (Argc - 3)/3; index ++) {
        p = malloc(sizeof (FILENode)); //p for old file
        if (p == NULL) {
          Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
          free (FileData);
          return 1;
        }
        q = malloc(sizeof (FILENode)); //q for new file
        if (q == NULL) {
          Error ("FMMT", 0, 4001, "Resource: Memory can't be allocated", NULL);
          free (FileData);
          free (p);
          return 1;
        }
        p -> FileName = Argv[3 + index * 3];
        q -> FileName = Argv[4 + index * 3];
        p -> SubLevel = 0;
        q -> SubLevel = 0;
        exist = -1;
        for (j = 0; j < count; j ++) {
            if ((strcmp(Argv[2 + index * 3], (FileData + j) -> FvId) == 0)) {
                exist = j;
                break;
            }
        }
        if (exist >= 0) {
            p -> Next = (FileData + j) -> OldFile;
            (FileData + j) -> OldFile = p;
            q -> Next = (FileData + j) -> NewFile;
            (FileData + j) -> NewFile = q;
        } else {
            (FileData + count) -> FdData = NULL;
            (FileData + count) -> FvLevel = 0;
            (FileData + count) -> FvInFd = NULL;
            (FileData + count) -> FvId = Argv[2 + index * 3];;
            (FileData + count) -> OldFile = p;
            (FileData + count) -> NewFile = q;
            p -> Next = NULL;
            q -> Next = NULL;
            count ++;
        }
    }

    if (count <= 0) {
        Error("FMMT", 0, 0004, "error while parsing FD Image", "Gathering information failed!");
    }
    for (index = 0; index < count; index ++) {
        for (j = index + 1; j < count; j ++) {
            if ((strcmp((FileData + index)->FvId, (FileData + j)->FvId) < 0)) {
                CHAR8 *tmp = (FileData + index)->FvId;
                FILENode *Old = (FileData + index)->OldFile;
                FILENode *New = (FileData + index)->NewFile;
                (FileData + index)->FvId = (FileData + j)->FvId;
                (FileData + index)->OldFile = (FileData + j)->OldFile;
                (FileData + index)->NewFile = (FileData + j)->NewFile;
                (FileData + j)->OldFile = Old;
                (FileData + j)->NewFile = New;
                (FileData + j)->FvId = tmp;
            }
        }
    }

    Status = FmmtImageReplace(Argv[1], FileData, count, Argv[Argc-1]);
    for (index = 0; index < count; index ++) {
      if ((FileData + index)->NewFile != NULL) {
        free ((FileData + index)->NewFile);
        (FileData + index)->NewFile = NULL;
      }
      if ((FileData + index)->OldFile != NULL) {
        free ((FileData + index)->OldFile);
        (FileData + index)->OldFile = NULL;
      }
    }
    for (index = 0; index < count; index ++) {
      if ((FileData + index)->FdData != NULL) {
        LibFmmtFreeFd ((FileData + index)->FdData);
      }
    }
    free (FileData);
    if (EFI_ERROR (Status)) {
      Error("FMMT", 0, 1001,  "Error while replace the named ffs file in the FD image file with the new ffs file.", "");
      LibRmDir (TemDir);
      return 1;
    }

  } else if ((strcmp(Argv[0], "-h") == 0) || (strcmp(Argv[0], "--help") == 0) ||
             (strcmp(Argv[0], "-?") == 0) || (strcmp(Argv[0], "/?") == 0)) {
    //
    // print help information to user.
    //
    Usage();

  } else {
    //
    // Invalid parameter.
    //
    printf("\n");
    Error("FMMT", 0, 1001,  "Invalid parameter", Argv[0]);
    Usage();
    return 1;
  }

  return 0;
}

