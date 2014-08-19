/** @file
File IO routines inspired by Streams with an EFI flavor

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Basic support for opening files on different device types. The device string
is in the form of DevType:Path. Current DevType is required as there is no
current mounted device concept of current working directory concept implement
by this library.

Device names are case insensitive and only check the leading characters for
unique matches. Thus the following are all the same:
LoadFile0:
l0:
L0:
Lo0:

Supported Device Names:
A0x1234:0x12 - A memory buffer starting at address 0x1234 for 0x12 bytes
l1:          - EFI LoadFile device one.
B0:          - EFI BlockIo zero.
fs3:         - EFI Simple File System device 3
Fv2:         - EFI Firmware VOlume device 2
10.0.1.102:  - TFTP service IP followed by the file name
**/

#include <PiDxe.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadFile.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/EfiFileLib.h>
#include <Library/PcdLib.h>
#include <Library/EblNetworkLib.h>


CHAR8 *gCwd = NULL;

CONST EFI_GUID gZeroGuid  = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

#define EFI_OPEN_FILE_GUARD_HEADER  0x4B4D4641
#define EFI_OPEN_FILE_GUARD_FOOTER  0x444D5A56

// Need to defend against this overflowing
#define MAX_CMD_LINE  0x200

typedef struct {
  UINT32            Header;
  EFI_OPEN_FILE     File;
  UINT32            Footer;
} EFI_OPEN_FILE_GUARD;


// globals to store current open device info
EFI_HANDLE            *mBlkIo = NULL;
UINTN                 mBlkIoCount = 0;

EFI_HANDLE            *mFs = NULL;
UINTN                 mFsCount = 0;
// mFsInfo[] array entries must match mFs[] handles
EFI_FILE_SYSTEM_INFO  **mFsInfo = NULL;

EFI_HANDLE            *mFv = NULL;
UINTN                 mFvCount = 0;
EFI_HANDLE            *mLoadFile = NULL;
UINTN                 mLoadFileCount = 0;



/**
Internal worker function to validate a File handle.

@param  File    Open File Handle

@return TRUE    File is valid
@return FALSE   File is not valid


**/
BOOLEAN
FileHandleValid (
  IN EFI_OPEN_FILE  *File
  )
{
  EFI_OPEN_FILE_GUARD  *GuardFile;

  // Look right before and after file structure for the correct signatures
  GuardFile = BASE_CR (File, EFI_OPEN_FILE_GUARD, File);
  if ((GuardFile->Header != EFI_OPEN_FILE_GUARD_HEADER) ||
    (GuardFile->Footer != EFI_OPEN_FILE_GUARD_FOOTER) ) {
      return FALSE;
    }

    return TRUE;
}

/**
Internal worker function. If Buffer is not NULL free it.

@param  Buffer    Buffer to FreePool()

**/
VOID
EblFreePool (
  IN  VOID  *Buffer
  )
{
  if (Buffer != NULL) {
    FreePool (Buffer);
  }
}

/**
Update Device List Global Variables

**/
VOID
EblUpdateDeviceLists (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINTN                             Size;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Fs;
  EFI_FILE_HANDLE                   Root;
  UINTN                             Index;

  if (mBlkIo != NULL) {
    FreePool (mBlkIo);
  }
  gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &mBlkIoCount, &mBlkIo);



  if (mFv != NULL) {
    FreePool (mFv);
  }
  gBS->LocateHandleBuffer (ByProtocol, &gEfiFirmwareVolume2ProtocolGuid, NULL, &mFvCount, &mFv);

  if (mLoadFile != NULL) {
    FreePool (mLoadFile);
  }
  gBS->LocateHandleBuffer (ByProtocol, &gEfiLoadFileProtocolGuid, NULL, &mLoadFileCount, &mLoadFile);

  if (mFs != NULL) {
    FreePool (mFs);
  }

  if (&mFsInfo[0] != NULL) {
    // Need to Free the mFsInfo prior to recalculating mFsCount so don't move this code
    for (Index = 0; Index < mFsCount; Index++) {
      if (mFsInfo[Index] != NULL) {
        FreePool (mFsInfo[Index]);
      }
    }
    FreePool (mFsInfo);
  }

  gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &mFsCount, &mFs);


  mFsInfo = AllocateZeroPool (mFsCount * sizeof (EFI_FILE_SYSTEM_INFO *));
  if (mFsInfo == NULL) {
    // If we can't do this then we can't support file system entries
    mFsCount = 0;
  } else {
    // Loop through all the file system structures and cache the file system info data
    for (Index =0; Index < mFsCount; Index++) {
      Status = gBS->HandleProtocol (mFs[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
      if (!EFI_ERROR (Status)) {
        Status = Fs->OpenVolume (Fs, &Root);
        if (!EFI_ERROR (Status)) {
          // Get information about the volume
          Size = 0;
          Status = Root->GetInfo (Root, &gEfiFileSystemInfoGuid, &Size, mFsInfo[Index]);
          if (Status == EFI_BUFFER_TOO_SMALL) {
            mFsInfo[Index] = AllocatePool (Size);
            Status = Root->GetInfo (Root, &gEfiFileSystemInfoGuid, &Size, mFsInfo[Index]);
          }

          Root->Close (Root);
        }
      }
    }
  }
}


/**
PathName is in the form <device name>:<path> for example fs1:\ or ROOT:\.
Return TRUE if the <devce name> prefix of PathName matches a file system
Volume Name. MatchIndex is the array  index in mFsInfo[] of the match,
and it can be used with mFs[] to find the handle that needs to be opened

@param  PathName      PathName to check
@param  FileStart     Index of the first character of the <path>
@param  MatchIndex    Index in mFsInfo[] that matches

@return TRUE      PathName matches a Volume Label and MatchIndex is valid
@return FALSE     PathName does not match a Volume Label MatchIndex undefined

**/
BOOLEAN
EblMatchVolumeName (
  IN  CHAR8   *PathName,
  IN  UINTN   FileStart,
  OUT UINTN   *MatchIndex
  )
{
  UINTN   Index;
  UINTN   Compare;
  UINTN   VolStrLen;
  BOOLEAN Match;

  for (Index =0; Index < mFsCount; Index++) {
    if (mFsInfo[Index] == NULL) {
      // FsInfo is not valid so skip it
      continue;
    }
    VolStrLen = StrLen (mFsInfo[Index]->VolumeLabel);
    for (Compare = 0, Match = TRUE; Compare < (FileStart - 1); Compare++) {
      if (Compare > VolStrLen) {
        Match = FALSE;
        break;
      }
      if (PathName[Compare] != (CHAR8)mFsInfo[Index]->VolumeLabel[Compare]) {
        // If the VolumeLabel has a space allow a _ to match with it in addition to ' '
        if (!((PathName[Compare] == '_') && (mFsInfo[Index]->VolumeLabel[Compare] == L' '))) {
          Match = FALSE;
          break;
        }
      }
    }
    if (Match) {
      *MatchIndex = Index;
      return TRUE;
    }
  }

  return FALSE;
}


/**
Return the number of devices of the current type active in the system

@param  Type      Device type to check

@return 0         Invalid type

**/
UINTN
EfiGetDeviceCounts (
  IN  EFI_OPEN_FILE_TYPE     DeviceType
  )
{
  switch (DeviceType) {
  case EfiOpenLoadFile:
    return mLoadFileCount;
  case EfiOpenFirmwareVolume:
    return mFvCount;
  case EfiOpenFileSystem:
    return mFsCount;
  case EfiOpenBlockIo:
    return mBlkIoCount;
  default:
    return 0;
  }
}

EFI_STATUS
ConvertIpStringToEfiIp (
  IN  CHAR8           *PathName,
  OUT EFI_IP_ADDRESS  *ServerIp
  )
{
  CHAR8     *Str;

  Str = PathName;
  ServerIp->v4.Addr[0] = (UINT8)AsciiStrDecimalToUintn (Str);

  Str = AsciiStrStr (Str, ".");
  if (Str == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ServerIp->v4.Addr[1] = (UINT8)AsciiStrDecimalToUintn (++Str);

  Str = AsciiStrStr (Str, ".");
  if (Str == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ServerIp->v4.Addr[2] = (UINT8)AsciiStrDecimalToUintn (++Str);

  Str = AsciiStrStr (Str, ".");
  if (Str == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ServerIp->v4.Addr[3] = (UINT8)AsciiStrDecimalToUintn (++Str);

  return EFI_SUCCESS;
}


/**
Internal work function to extract a device number from a string skipping
text. Easy way to extract numbers from strings like blk7:.

@param  Str   String to extract device number form

@return -1    Device string is not valid
@return       Device #

**/
UINTN
EblConvertDevStringToNumber (
  IN  CHAR8   *Str
  )
{
  UINTN   Max;
  UINTN   Index;


  // Find the first digit
  Max = AsciiStrLen (Str);
  for  (Index = 0; !((*Str >= '0') && (*Str <= '9')) && (Index < Max); Index++) {
    Str++;
  }
  if (Index == Max) {
    return (UINTN)-1;
  }

  return AsciiStrDecimalToUintn (Str);
}


/**
Internal work function to fill in EFI_OPEN_FILE information for the Fs and BlkIo

@param  File        Open file handle
@param  FileName    Name of file after device stripped off


**/
EFI_STATUS
EblFileDevicePath (
  IN OUT EFI_OPEN_FILE  *File,
  IN  CHAR8             *FileName,
  IN  CONST UINT64      OpenMode
  )
{
  EFI_STATUS                        Status;
  UINTN                             Size;
  FILEPATH_DEVICE_PATH              *FilePath;
  EFI_DEVICE_PATH_PROTOCOL          *FileDevicePath;
  CHAR16                            UnicodeFileName[MAX_PATHNAME];
  EFI_BLOCK_IO_PROTOCOL             *BlkIo;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Fs;
  EFI_FILE_HANDLE                   Root;


  if ( *FileName != 0 ) {
    AsciiStrToUnicodeStr (FileName, UnicodeFileName);
  } else {
    AsciiStrToUnicodeStr ("\\", UnicodeFileName);
  }

  Size = StrSize (UnicodeFileName);
  FileDevicePath = AllocatePool (Size + SIZE_OF_FILEPATH_DEVICE_PATH + sizeof (EFI_DEVICE_PATH_PROTOCOL));
  if (FileDevicePath != NULL) {
    FilePath = (FILEPATH_DEVICE_PATH *) FileDevicePath;
    FilePath->Header.Type    = MEDIA_DEVICE_PATH;
    FilePath->Header.SubType = MEDIA_FILEPATH_DP;
    CopyMem (&FilePath->PathName, UnicodeFileName, Size);
    SetDevicePathNodeLength (&FilePath->Header, Size + SIZE_OF_FILEPATH_DEVICE_PATH);
    SetDevicePathEndNode (NextDevicePathNode (&FilePath->Header));

    if (File->EfiHandle != NULL) {
      File->DevicePath = DevicePathFromHandle (File->EfiHandle);
    }

    File->DevicePath = AppendDevicePath (File->DevicePath, FileDevicePath);
    FreePool (FileDevicePath);
  }

  Status = gBS->HandleProtocol (File->EfiHandle, &gEfiBlockIoProtocolGuid, (VOID **)&BlkIo);
  if (!EFI_ERROR (Status)) {
    File->FsBlockIoMedia = BlkIo->Media;
    File->FsBlockIo = BlkIo;

    // If we are not opening the device this will get over written with file info
    File->MaxPosition = MultU64x32 (BlkIo->Media->LastBlock + 1, BlkIo->Media->BlockSize);
  }

  if (File->Type == EfiOpenFileSystem) {
    Status = gBS->HandleProtocol (File->EfiHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
    if (!EFI_ERROR (Status)) {
      Status = Fs->OpenVolume (Fs, &Root);
      if (!EFI_ERROR (Status)) {
        // Get information about the volume
        Size = 0;
        Status = Root->GetInfo (Root, &gEfiFileSystemInfoGuid, &Size, File->FsInfo);
        if (Status == EFI_BUFFER_TOO_SMALL) {
          File->FsInfo = AllocatePool (Size);
          Status = Root->GetInfo (Root, &gEfiFileSystemInfoGuid, &Size, File->FsInfo);
        }

        // Get information about the file
        Status = Root->Open (Root, &File->FsFileHandle, UnicodeFileName, OpenMode, 0);
        if (!EFI_ERROR (Status)) {
          Size = 0;
          Status = File->FsFileHandle->GetInfo (File->FsFileHandle, &gEfiFileInfoGuid, &Size, NULL);
          if (Status == EFI_BUFFER_TOO_SMALL) {
            File->FsFileInfo = AllocatePool (Size);
            Status = File->FsFileHandle->GetInfo (File->FsFileHandle, &gEfiFileInfoGuid, &Size, File->FsFileInfo);
            if (!EFI_ERROR (Status)) {
              File->Size = (UINTN)File->FsFileInfo->FileSize;
              File->MaxPosition = (UINT64)File->Size;
            }
          }
        }

        Root->Close (Root);
      }
    }
  } else if (File->Type == EfiOpenBlockIo) {
    File->Size = (UINTN)File->MaxPosition;
  }

  return Status;
}

#define ToUpper(a)  ((((a) >= 'a') && ((a) <= 'z')) ? ((a) - 'a' + 'A') : (a))

EFI_STATUS
CompareGuidToString (
  IN  EFI_GUID    *Guid,
  IN  CHAR8       *String
  )
{
  CHAR8       AsciiGuid[64];
  CHAR8       *StringPtr;
  CHAR8       *GuidPtr;

  AsciiSPrint (AsciiGuid, sizeof(AsciiGuid), "%g", Guid);

  StringPtr = String;
  GuidPtr   = AsciiGuid;

  while ((*StringPtr != '\0') && (*GuidPtr != '\0')) {
    // Skip dashes
    if (*StringPtr == '-') {
      StringPtr++;
      continue;
    }

    if (*GuidPtr == '-') {
      GuidPtr++;
      continue;
    }

    if (ToUpper(*StringPtr) != ToUpper(*GuidPtr)) {
      return EFI_NOT_FOUND;
    }

    StringPtr++;
    GuidPtr++;
  }

  return EFI_SUCCESS;
}


/**
Internal work function to fill in EFI_OPEN_FILE information for the FV

@param  File        Open file handle
@param  FileName    Name of file after device stripped off


**/
EFI_STATUS
EblFvFileDevicePath (
  IN OUT EFI_OPEN_FILE  *File,
  IN  CHAR8             *FileName,
  IN  CONST UINT64      OpenMode
  )
{
  EFI_STATUS                          Status;
  EFI_STATUS                          GetNextFileStatus;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  UINTN                               Key;
  UINT32                              AuthenticationStatus;
  CHAR8                               AsciiSection[MAX_PATHNAME];
  VOID                                *Section;
  UINTN                               SectionSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_LBA                             Lba;
  UINTN                               BlockSize;
  UINTN                               NumberOfBlocks;
  EFI_FIRMWARE_VOLUME_HEADER          *FvHeader = NULL;
  UINTN                               Index;


  Status = gBS->HandleProtocol (File->EfiHandle, &gEfiFirmwareVolume2ProtocolGuid, (VOID **)&File->Fv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get FVB Info about the handle
  Status = gBS->HandleProtocol (File->EfiHandle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **)&Fvb);
  if (!EFI_ERROR (Status)) {
    Status = Fvb->GetPhysicalAddress (Fvb, &File->FvStart);
    if (!EFI_ERROR (Status)) {
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)File->FvStart;
      File->FvHeaderSize = sizeof (EFI_FIRMWARE_VOLUME_HEADER);
      for (Index = 0; FvHeader->BlockMap[Index].Length !=0; Index++) {
        File->FvHeaderSize += sizeof (EFI_FV_BLOCK_MAP_ENTRY);
      }

      for (Lba = 0, File->FvSize = 0, NumberOfBlocks = 0; ; File->FvSize += (BlockSize * NumberOfBlocks), Lba += NumberOfBlocks) {
        Status = Fvb->GetBlockSize (Fvb, Lba, &BlockSize, &NumberOfBlocks);
        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }
  }


  DevicePath = DevicePathFromHandle (File->EfiHandle);

  if (*FileName == '\0') {
    File->DevicePath = DuplicateDevicePath (DevicePath);
    File->Size = File->FvSize;
    File->MaxPosition = File->Size;
  } else {
    Key = 0;
    do {
      File->FvType = EFI_FV_FILETYPE_ALL;
      GetNextFileStatus = File->Fv->GetNextFile (
        File->Fv,
        &Key,
        &File->FvType,
        &File->FvNameGuid,
        &File->FvAttributes,
        &File->Size
        );
      if (!EFI_ERROR (GetNextFileStatus)) {
        // Compare GUID first
        Status = CompareGuidToString (&File->FvNameGuid, FileName);
        if (!EFI_ERROR(Status)) {
          break;
        }

        Section = NULL;
        Status = File->Fv->ReadSection (
          File->Fv,
          &File->FvNameGuid,
          EFI_SECTION_USER_INTERFACE,
          0,
          &Section,
          &SectionSize,
          &AuthenticationStatus
          );
        if (!EFI_ERROR (Status)) {
          UnicodeStrToAsciiStr (Section, AsciiSection);
          if (AsciiStriCmp (FileName, AsciiSection) == 0) {
            FreePool (Section);
            break;
          }
          FreePool (Section);
        }
      }
    } while (!EFI_ERROR (GetNextFileStatus));

    if (EFI_ERROR (GetNextFileStatus)) {
      return GetNextFileStatus;
    }

    if (OpenMode != EFI_SECTION_ALL) {
      // Calculate the size of the section we are targeting
      Section = NULL;
      File->Size = 0;
      Status = File->Fv->ReadSection (
        File->Fv,
        &File->FvNameGuid,
        (EFI_SECTION_TYPE)OpenMode,
        0,
        &Section,
        &File->Size,
        &AuthenticationStatus
        );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    File->MaxPosition = File->Size;
    EfiInitializeFwVolDevicepathNode (&DevicePathNode, &File->FvNameGuid);
    File->DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&DevicePathNode);
  }


  // FVB not required if FV was soft loaded...
  return EFI_SUCCESS;
}




/**
Open a device named by PathName. The PathName includes a device name and
path separated by a :. See file header for more details on the PathName
syntax. There is no checking to prevent a file from being opened more than
one type.

SectionType is only used to open an FV. Each file in an FV contains multiple
sections and only the SectionType section is opened.

For any file that is opened with EfiOpen() must be closed with EfiClose().

@param  PathName    Path to parse to open
@param  OpenMode    Same as EFI_FILE.Open()
@param  SectionType Section in FV to open.

@return NULL  Open failed
@return Valid EFI_OPEN_FILE handle

**/
EFI_OPEN_FILE *
EfiOpen (
  IN        CHAR8               *PathName,
  IN  CONST UINT64              OpenMode,
  IN  CONST EFI_SECTION_TYPE    SectionType
  )
{
  EFI_STATUS                Status;
  EFI_OPEN_FILE             *File;
  EFI_OPEN_FILE             FileData;
  UINTN                     StrLen;
  UINTN                     FileStart;
  UINTN                     DevNumber = 0;
  EFI_OPEN_FILE_GUARD       *GuardFile;
  BOOLEAN                   VolumeNameMatch;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     Size;
  EFI_IP_ADDRESS            Ip;
  CHAR8                     *CwdPlusPathName;
  UINTN                     Index;
  EFI_SECTION_TYPE          ModifiedSectionType;

  EblUpdateDeviceLists ();

  File = &FileData;
  ZeroMem (File, sizeof (EFI_OPEN_FILE));

  StrLen = AsciiStrSize (PathName);
  if (StrLen <= 1) {
    // Smallest valid path is 1 char and a null
    return NULL;
  }

  for (FileStart = 0; FileStart < StrLen; FileStart++) {
    if (PathName[FileStart] == ':') {
      FileStart++;
      break;
    }
  }

  //
  // Matching volume name has precedence over handle based names
  //
  VolumeNameMatch = EblMatchVolumeName (PathName, FileStart, &DevNumber);
  if (!VolumeNameMatch) {
    if (FileStart == StrLen) {
      // No Volume name or device name, so try Current Working Directory
      if (gCwd == NULL) {
        // No CWD
        return NULL;
      }

      // We could add a current working directory concept
      CwdPlusPathName = AllocatePool (AsciiStrSize (gCwd) + AsciiStrSize (PathName));
      if (CwdPlusPathName == NULL) {
        return NULL;
      }

      if ((PathName[0] == '/') || (PathName[0] == '\\')) {
        // PathName starts in / so this means we go to the root of the device in the CWD.
        CwdPlusPathName[0] = '\0';
        for (FileStart = 0; gCwd[FileStart] != '\0'; FileStart++) {
          CwdPlusPathName[FileStart] = gCwd[FileStart];
          if (gCwd[FileStart] == ':') {
            FileStart++;
            CwdPlusPathName[FileStart] = '\0';
            break;
          }
        }
      } else {
        AsciiStrCpy (CwdPlusPathName, gCwd);
        StrLen = AsciiStrLen (gCwd);
        if ((*PathName != '/') && (*PathName != '\\') && (gCwd[StrLen-1] != '/') && (gCwd[StrLen-1] != '\\')) {
          AsciiStrCat (CwdPlusPathName, "\\");
        }
      }

      AsciiStrCat (CwdPlusPathName, PathName);
      if (AsciiStrStr (CwdPlusPathName, ":") == NULL) {
        // Extra error check to make sure we don't recurse and blow stack
        return NULL;
      }

      File = EfiOpen (CwdPlusPathName, OpenMode, SectionType);
      FreePool (CwdPlusPathName);
      return File;
    }

    DevNumber = EblConvertDevStringToNumber ((CHAR8 *)PathName);
  }

  File->DeviceName = AllocatePool (StrLen);
  AsciiStrCpy (File->DeviceName, PathName);
  File->DeviceName[FileStart - 1] = '\0';
  File->FileName = &File->DeviceName[FileStart];
  if (File->FileName[0] == '\0') {
    // if it is just a file name use / as root
    File->FileName = "\\";
  }

  //
  // Use best match algorithm on the dev names so we only need to look at the
  // first few charters to match the full device name. Short name forms are
  // legal from the caller.
  //
  Status = EFI_SUCCESS;
  if (*PathName == 'f' || *PathName == 'F' || VolumeNameMatch) {
    if (PathName[1] == 's' || PathName[1] == 'S' || VolumeNameMatch) {
      if (DevNumber >= mFsCount) {
        goto ErrorExit;
      }
      File->Type = EfiOpenFileSystem;
      File->EfiHandle = mFs[DevNumber];
      Status = EblFileDevicePath (File, &PathName[FileStart], OpenMode);

    } else if (PathName[1] == 'v' || PathName[1] == 'V') {
      if (DevNumber >= mFvCount) {
        goto ErrorExit;
      }
      File->Type = EfiOpenFirmwareVolume;
      File->EfiHandle = mFv[DevNumber];

      if ((PathName[FileStart] == '/') || (PathName[FileStart] == '\\')) {
        // Skip leading / as its not really needed for the FV since no directories are supported
        FileStart++;
      }

      // Check for 2nd :
      ModifiedSectionType = SectionType;
      for (Index = FileStart; PathName[Index] != '\0'; Index++) {
        if (PathName[Index] == ':') {
          // Support fv0:\DxeCore:0x10
          // This means open the PE32 Section of the file
          ModifiedSectionType = (EFI_SECTION_TYPE)AsciiStrHexToUintn (&PathName[Index + 1]);
          PathName[Index] = '\0';
        }
      }
      File->FvSectionType = ModifiedSectionType;
      Status = EblFvFileDevicePath (File, &PathName[FileStart], ModifiedSectionType);
    }
  } else if ((*PathName == 'A') || (*PathName == 'a')) {
    // Handle a:0x10000000:0x1234 address form a:ADDRESS:SIZE
    File->Type = EfiOpenMemoryBuffer;
    // 1st colon is at PathName[FileStart - 1]
    File->Buffer = (VOID *)AsciiStrHexToUintn (&PathName[FileStart]);

    // Find 2nd colon
    while ((PathName[FileStart] != ':') && (PathName[FileStart] != '\0')) {
      FileStart++;
    }

    // If we ran out of string, there's no extra data
    if (PathName[FileStart] == '\0') {
      File->Size = 0;
    } else {
      File->Size = AsciiStrHexToUintn (&PathName[FileStart + 1]);
    }

    // if there's no number after the second colon, default
    // the end of memory
    if (File->Size == 0) {
      File->Size =  (UINTN)(0 - (UINTN)File->Buffer);
    }

    File->MaxPosition = File->Size;
    File->BaseOffset = (UINTN)File->Buffer;

  } else if (*PathName== 'l' || *PathName == 'L') {
    if (DevNumber >= mLoadFileCount) {
      goto ErrorExit;
    }
    File->Type = EfiOpenLoadFile;
    File->EfiHandle = mLoadFile[DevNumber];

    Status = gBS->HandleProtocol (File->EfiHandle, &gEfiLoadFileProtocolGuid, (VOID **)&File->LoadFile);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = gBS->HandleProtocol (File->EfiHandle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }
    File->DevicePath = DuplicateDevicePath (DevicePath);

  } else if (*PathName == 'b' || *PathName == 'B') {
    // Handle b#:0x10000000:0x1234 address form b#:ADDRESS:SIZE
    if (DevNumber >= mBlkIoCount) {
      goto ErrorExit;
    }
    File->Type = EfiOpenBlockIo;
    File->EfiHandle = mBlkIo[DevNumber];
    EblFileDevicePath (File, "", OpenMode);

    // 1st colon is at PathName[FileStart - 1]
    File->DiskOffset = AsciiStrHexToUintn (&PathName[FileStart]);

    // Find 2nd colon
    while ((PathName[FileStart] != ':') && (PathName[FileStart] != '\0')) {
      FileStart++;
    }

    // If we ran out of string, there's no extra data
    if (PathName[FileStart] == '\0') {
      Size = 0;
    } else {
      Size = AsciiStrHexToUintn (&PathName[FileStart + 1]);
    }

    // if a zero size is passed in (or the size is left out entirely),
    // go to the end of the device.
    if (Size == 0) {
      File->Size = File->Size - File->DiskOffset;
    } else {
      File->Size = Size;
    }

    File->MaxPosition = File->Size;
    File->BaseOffset = File->DiskOffset;
  } else if ((*PathName) >= '0' && (*PathName <= '9')) {

    // Get current IP address
    Status = EblGetCurrentIpAddress (&Ip);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Device IP Address is not configured.\n");
      goto ErrorExit;
    }


    // Parse X.X.X.X:Filename, only support IPv4 TFTP for now...
    File->Type = EfiOpenTftp;
    File->IsDirty = FALSE;
    File->IsBufferValid = FALSE;

    Status = ConvertIpStringToEfiIp (PathName, &File->ServerIp);
  }

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  GuardFile = (EFI_OPEN_FILE_GUARD *)AllocateZeroPool (sizeof (EFI_OPEN_FILE_GUARD));
  if (GuardFile == NULL) {
    goto ErrorExit;
  }

  GuardFile->Header = EFI_OPEN_FILE_GUARD_HEADER;
  CopyMem (&(GuardFile->File), &FileData, sizeof (EFI_OPEN_FILE));
  GuardFile->Footer = EFI_OPEN_FILE_GUARD_FOOTER;

  return &(GuardFile->File);

ErrorExit:
  FreePool (File->DeviceName);
  return NULL;
}

#define FILE_COPY_CHUNK 0x01000000

EFI_STATUS
EfiCopyFile (
  IN        CHAR8               *DestinationFile,
  IN        CHAR8               *SourceFile
  )
{
  EFI_OPEN_FILE *Source      = NULL;
  EFI_OPEN_FILE *Destination = NULL;
  EFI_STATUS    Status       = EFI_SUCCESS;
  VOID          *Buffer      = NULL;
  UINTN         Size;
  UINTN         Offset;
  UINTN         Chunk = FILE_COPY_CHUNK;

  Source = EfiOpen (SourceFile, EFI_FILE_MODE_READ, 0);
  if (Source == NULL) {
    AsciiPrint("Source file open error.\n");
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  Destination = EfiOpen (DestinationFile, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
  if (Destination == NULL) {
    AsciiPrint("Destination file open error.\n");
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  Buffer = AllocatePool(FILE_COPY_CHUNK);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Size = EfiTell(Source, NULL);

  for (Offset = 0; Offset + FILE_COPY_CHUNK <= Size; Offset += Chunk) {
    Chunk = FILE_COPY_CHUNK;

    Status = EfiRead(Source, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Read file error %r\n", Status);
      goto Exit;
    }

    Status = EfiWrite(Destination, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Write file error %r\n", Status);
      goto Exit;
    }
  }

  // Any left over?
  if (Offset < Size) {
    Chunk = Size - Offset;

    Status = EfiRead(Source, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Read file error\n");
      goto Exit;
    }

    Status = EfiWrite(Destination, Buffer, &Chunk);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Write file error\n");
      goto Exit;
    }
  }

Exit:
  if (Source != NULL) {
    Status = EfiClose(Source);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Source close error");
    }
  }

  if (Destination != NULL) {
    Status = EfiClose(Destination);
    if (EFI_ERROR(Status)) {
      AsciiPrint("Destination close error");
    }
  }

  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  return Status;
}

/**
Use DeviceType and Index to form a valid PathName and try and open it.

@param  DeviceType  Device type to open
@param  Index       Device Index to use. Zero relative.

@return NULL  Open failed
@return Valid EFI_OPEN_FILE handle

**/
EFI_OPEN_FILE  *
EfiDeviceOpenByType (
  IN  EFI_OPEN_FILE_TYPE    DeviceType,
  IN  UINTN                 Index
  )
{
  CHAR8   *DevStr;
  CHAR8   Path[MAX_CMD_LINE];

  switch (DeviceType) {
  case EfiOpenLoadFile:
    DevStr = "loadfile%d:";
    break;
  case EfiOpenFirmwareVolume:
    DevStr = "fv%d:";
    break;
  case EfiOpenFileSystem:
    DevStr = "fs%d:";
    break;
  case EfiOpenBlockIo:
    DevStr = "blk%d:";
    break;
  case EfiOpenMemoryBuffer:
    DevStr = "a%d:";
    break;
  default:
    return NULL;
  }

  AsciiSPrint (Path, MAX_PATHNAME, DevStr, Index);

  return EfiOpen (Path, EFI_FILE_MODE_READ, 0);
}


/**
Close a file handle opened by EfiOpen() and free all resources allocated by
EfiOpen().

@param  Stream    Open File Handle

@return EFI_INVALID_PARAMETER  Stream is not an Open File
@return EFI_SUCCESS            Steam closed

**/
EFI_STATUS
EfiClose (
  IN  EFI_OPEN_FILE     *File
  )
{
  EFI_STATUS          Status;
  UINT64              TftpBufferSize;

  if (!FileHandleValid (File)) {
    return EFI_INVALID_PARAMETER;
  }

  //Write the buffer contents to TFTP file.
  if ((File->Type == EfiOpenTftp) && (File->IsDirty)) {

    TftpBufferSize = File->Size;
    Status = EblMtftp (
      EFI_PXE_BASE_CODE_TFTP_WRITE_FILE,
      File->Buffer,
      TRUE,
      &TftpBufferSize,
      NULL,
      &File->ServerIp,
      (UINT8 *)File->FileName,
      NULL,
      FALSE
      );
    if (EFI_ERROR(Status)) {
      AsciiPrint("TFTP error during APPLE_NSP_TFTP_WRITE_FILE: %r\n", Status);
      return Status;
    }
  }

  if ((File->Type == EfiOpenLoadFile) ||
    ((File->Type == EfiOpenTftp) && (File->IsBufferValid == TRUE)) ||
    ((File->Type == EfiOpenFirmwareVolume) && (File->IsBufferValid == TRUE))) {
    EblFreePool(File->Buffer);
  }

  EblFreePool (File->DevicePath);
  EblFreePool (File->DeviceName);
  EblFreePool (File->FsFileInfo);
  EblFreePool (File->FsInfo);

  if (File->FsFileHandle != NULL) {
    File->FsFileHandle->Close (File->FsFileHandle);
  }

  // Need to free File and it's Guard structures
  EblFreePool (BASE_CR (File, EFI_OPEN_FILE_GUARD, File));
  return EFI_SUCCESS;
}


/**
Return the size of the file represented by Stream. Also return the current
Seek position. Opening a file will enable a valid file size to be returned.
LoadFile is an exception as a load file size is set to zero.

@param  Stream    Open File Handle

@return 0         Stream is not an Open File or a valid LoadFile handle

**/
UINTN
EfiTell (
  IN  EFI_OPEN_FILE     *File,
  OUT EFI_LBA           *CurrentPosition    OPTIONAL
  )
{
  EFI_STATUS Status;
  UINT64     BufferSize = 0;

  if (!FileHandleValid (File)) {
    return 0;
  }

  if (CurrentPosition != NULL) {
    *CurrentPosition = File->CurrentPosition;
  }

  if (File->Type == EfiOpenLoadFile) {
    // Figure out the File->Size
    File->Buffer = NULL;
    File->Size   = 0;
    Status = File->LoadFile->LoadFile (File->LoadFile, File->DevicePath, FALSE, &File->Size, File->Buffer);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      return 0;
    }

    File->MaxPosition = (UINT64)File->Size;
  } else if (File->Type == EfiOpenTftp) {

    Status = EblMtftp (
      EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
      NULL,
      FALSE,
      &BufferSize,
      NULL,
      &File->ServerIp,
      (UINT8 *)File->FileName,
      NULL,
      TRUE
      );
    if (EFI_ERROR(Status)) {
      AsciiPrint("TFTP error during APPLE_NSP_TFTP_GET_FILE_SIZE: %r\n", Status);
      return 0;
    }

    File->Size        = (UINTN)BufferSize;
    File->MaxPosition = File->Size;
  }

  return File->Size;
}


/**
Seek to the Offset location in the file. LoadFile and FV device types do
not support EfiSeek(). It is not possible to grow the file size using
EfiSeek().

SeekType defines how use Offset to calculate the new file position:
EfiSeekStart  : Position = Offset
EfiSeekCurrent: Position is Offset bytes from the current position
EfiSeekEnd    : Only supported if Offset is zero to seek to end of file.

@param  Stream    Open File Handle
@param  Offset    Offset to seek too.
@param  SeekType  Type of seek to perform


@return EFI_INVALID_PARAMETER  Stream is not an Open File
@return EFI_UNSUPPORTED        LoadFile and FV do not support Seek
@return EFI_NOT_FOUND          Seek past the end of the file.
@return EFI_SUCCESS            Steam closed

**/
EFI_STATUS
EfiSeek (
  IN  EFI_OPEN_FILE     *File,
  IN  EFI_LBA           Offset,
  IN  EFI_SEEK_TYPE     SeekType
  )
{
  EFI_STATUS    Status;
  UINT64        CurrentPosition;

  if (!FileHandleValid (File)) {
    return EFI_INVALID_PARAMETER;
  }

  if (File->Type == EfiOpenLoadFile) {
    // LoadFile does not support Seek
    return EFI_UNSUPPORTED;
  }

  CurrentPosition = File->CurrentPosition;
  switch (SeekType) {
  case EfiSeekStart:
    if (Offset > File->MaxPosition) {
      return EFI_NOT_FOUND;
    }
    CurrentPosition = Offset;
    break;

  case EfiSeekCurrent:
    if ((File->CurrentPosition + Offset) > File->MaxPosition) {
      return EFI_NOT_FOUND;
    }
    CurrentPosition += Offset;
    break;

  case EfiSeekEnd:
    if (Offset != 0) {
      // We don't support growing file size via seeking past end of file
      return EFI_UNSUPPORTED;
    }
    CurrentPosition = File->MaxPosition;
    break;

  default:
    return EFI_NOT_FOUND;
  }

  Status = EFI_SUCCESS;
  if (File->FsFileHandle != NULL) {
    Status = File->FsFileHandle->SetPosition (File->FsFileHandle, CurrentPosition);
  }

  if (!EFI_ERROR (Status)) {
    File->CurrentPosition = CurrentPosition;
  }

  return Status;
}

EFI_STATUS
CacheTftpFile (
  IN OUT  EFI_OPEN_FILE *File
  )
{
  EFI_STATUS          Status;
  UINT64              TftpBufferSize;

  if (File->IsBufferValid) {
    return EFI_SUCCESS;
  }

  // Make sure the file size is set.
  EfiTell (File, NULL);

  //Allocate a buffer to hold the whole file.
  File->Buffer = AllocatePool(File->Size);
  if (File->Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TftpBufferSize = File->Size;

  Status = EblMtftp (
    EFI_PXE_BASE_CODE_TFTP_READ_FILE,
    File->Buffer,
    FALSE,
    &TftpBufferSize,
    NULL,
    &File->ServerIp,
    (UINT8 *)File->FileName,
    NULL,
    FALSE);
  if (EFI_ERROR(Status)) {
    AsciiPrint("TFTP error during APPLE_NSP_TFTP_READ_FILE: %r\n", Status);
    FreePool(File->Buffer);
    return Status;
  }

  // Set the buffer valid flag.
  File->IsBufferValid = TRUE;

  return Status;
}

/**
Read BufferSize bytes from the current location in the file. For load file,
FV, and TFTP case you must read the entire file.

@param  Stream      Open File Handle
@param  Buffer      Caller allocated buffer.
@param  BufferSize  Size of buffer in bytes.


@return EFI_SUCCESS           Stream is not an Open File
@return EFI_END_OF_FILE Tried to read past the end of the file
@return EFI_INVALID_PARAMETER Stream is not an open file handle
@return EFI_BUFFER_TOO_SMALL  Buffer is not big enough to do the read
@return "other"               Error returned from device read

**/
EFI_STATUS
EfiRead (
  IN  EFI_OPEN_FILE       *File,
  OUT VOID                *Buffer,
  OUT UINTN               *BufferSize
  )
{
  EFI_STATUS            Status;
  UINT32                AuthenticationStatus;
  EFI_DISK_IO_PROTOCOL  *DiskIo;

  if (!FileHandleValid (File)) {
    return EFI_INVALID_PARAMETER;
  }

  // Don't read past the end of the file.
  if ((File->CurrentPosition + *BufferSize) > File->MaxPosition) {
    return EFI_END_OF_FILE;
  }

  switch (File->Type) {
  case EfiOpenLoadFile:
    // Figure out the File->Size
    EfiTell (File, NULL);

    Status = File->LoadFile->LoadFile (File->LoadFile, File->DevicePath, FALSE, BufferSize, Buffer);
    break;

  case EfiOpenFirmwareVolume:
    if (CompareGuid (&File->FvNameGuid, &gZeroGuid)) {
      // This is the entire FV device, so treat like a memory buffer
      CopyMem (Buffer, (VOID *)(UINTN)(File->FvStart + File->CurrentPosition), *BufferSize);
      File->CurrentPosition += *BufferSize;
      Status = EFI_SUCCESS;
    } else {
      if (File->Buffer == NULL) {
        if (File->FvSectionType == EFI_SECTION_ALL) {
          Status = File->Fv->ReadFile (
            File->Fv,
            &File->FvNameGuid,
            (VOID **)&File->Buffer,
            &File->Size,
            &File->FvType,
            &File->FvAttributes,
            &AuthenticationStatus
            );
        } else {
          Status = File->Fv->ReadSection (
            File->Fv,
            &File->FvNameGuid,
            File->FvSectionType,
            0,
            (VOID **)&File->Buffer,
            &File->Size,
            &AuthenticationStatus
            );
        }
        if (EFI_ERROR (Status)) {
          return Status;
        }
        File->IsBufferValid = TRUE;
      }
      // Operate on the cached buffer so Seek will work
      CopyMem (Buffer, File->Buffer + File->CurrentPosition, *BufferSize);
      File->CurrentPosition += *BufferSize;
      Status = EFI_SUCCESS;
    }
    break;

  case EfiOpenMemoryBuffer:
    CopyMem (Buffer, File->Buffer + File->CurrentPosition, *BufferSize);
    File->CurrentPosition += *BufferSize;
    Status = EFI_SUCCESS;
    break;

  case EfiOpenFileSystem:
    Status = File->FsFileHandle->Read (File->FsFileHandle, BufferSize, Buffer);
    File->CurrentPosition += *BufferSize;
    break;

  case EfiOpenBlockIo:
    Status = gBS->HandleProtocol(File->EfiHandle, &gEfiDiskIoProtocolGuid, (VOID **)&DiskIo);
    if (!EFI_ERROR(Status)) {
      Status = DiskIo->ReadDisk(DiskIo, File->FsBlockIoMedia->MediaId, File->DiskOffset + File->CurrentPosition, *BufferSize, Buffer);
    }
    File->CurrentPosition += *BufferSize;
    break;

  case EfiOpenTftp:
    // Cache the file if it hasn't been cached yet.
    if (File->IsBufferValid == FALSE) {
      Status = CacheTftpFile (File);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    // Copy out the requested data
    CopyMem (Buffer, File->Buffer + File->CurrentPosition, *BufferSize);
    File->CurrentPosition += *BufferSize;

    Status = EFI_SUCCESS;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  };

  return Status;
}


/**
Read the entire file into a buffer. This routine allocates the buffer and
returns it to the user full of the read data.

This is very useful for load file where it's hard to know how big the buffer
must be.

@param  Stream      Open File Handle
@param  Buffer      Pointer to buffer to return.
@param  BufferSize  Pointer to Size of buffer return..


@return EFI_SUCCESS           Stream is not an Open File
@return EFI_END_OF_FILE       Tried to read past the end of the file
@return EFI_INVALID_PARAMETER Stream is not an open file handle
@return EFI_BUFFER_TOO_SMALL  Buffer is not big enough to do the read
@return "other"               Error returned from device read

**/
EFI_STATUS
EfiReadAllocatePool (
  IN  EFI_OPEN_FILE     *File,
  OUT VOID              **Buffer,
  OUT UINTN             *BufferSize
  )
{
  if (!FileHandleValid (File)) {
    return EFI_INVALID_PARAMETER;
  }

  // Loadfile defers file size determination on Open so use tell to find it
  EfiTell (File, NULL);

  *BufferSize = File->Size;
  *Buffer = AllocatePool (*BufferSize);
  if (*Buffer == NULL) {
    return EFI_NOT_FOUND;
  }

  return EfiRead (File, *Buffer, BufferSize);
}


/**
Write data back to the file. For TFTP case you must write the entire file.

@param  Stream      Open File Handle
@param  Buffer      Pointer to buffer to return.
@param  BufferSize  Pointer to Size of buffer return..


@return EFI_SUCCESS           Stream is not an Open File
@return EFI_END_OF_FILE       Tried to read past the end of the file
@return EFI_INVALID_PARAMETER Stream is not an open file handle
@return EFI_BUFFER_TOO_SMALL  Buffer is not big enough to do the read
@return "other"               Error returned from device write

**/
EFI_STATUS
EfiWrite (
  IN  EFI_OPEN_FILE   *File,
  OUT VOID            *Buffer,
  OUT UINTN           *BufferSize
  )
{
  EFI_STATUS              Status;
  EFI_FV_WRITE_FILE_DATA  FileData;
  EFI_DISK_IO_PROTOCOL    *DiskIo;

  if (!FileHandleValid (File)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (File->Type) {
  case EfiOpenMemoryBuffer:
    if ((File->CurrentPosition + *BufferSize) > File->MaxPosition) {
      return EFI_END_OF_FILE;
    }

    CopyMem (File->Buffer + File->CurrentPosition, Buffer, *BufferSize);
    File->CurrentPosition += *BufferSize;
    Status = EFI_SUCCESS;

  case EfiOpenLoadFile:
    // LoadFile device is read only be definition
    Status = EFI_UNSUPPORTED;

  case EfiOpenFirmwareVolume:
    if (File->FvSectionType != EFI_SECTION_ALL) {
      // Writes not support to a specific section. You have to update entire file
      return EFI_UNSUPPORTED;
    }

    FileData.NameGuid       = &(File->FvNameGuid);
    FileData.Type           = File->FvType;
    FileData.FileAttributes = File->FvAttributes;
    FileData.Buffer         = Buffer;
    FileData.BufferSize     = (UINT32)*BufferSize;
    Status = File->Fv->WriteFile (File->Fv, 1, EFI_FV_UNRELIABLE_WRITE, &FileData);
    break;

  case EfiOpenFileSystem:
    Status = File->FsFileHandle->Write (File->FsFileHandle, BufferSize, Buffer);
    File->CurrentPosition += *BufferSize;
    break;

  case EfiOpenBlockIo:
    if ((File->CurrentPosition + *BufferSize) > File->MaxPosition) {
      return EFI_END_OF_FILE;
    }

    Status = gBS->HandleProtocol (File->EfiHandle, &gEfiDiskIoProtocolGuid, (VOID **)&DiskIo);
    if (!EFI_ERROR(Status)) {
      Status = DiskIo->WriteDisk (DiskIo, File->FsBlockIoMedia->MediaId, File->DiskOffset + File->CurrentPosition, *BufferSize, Buffer);
    }
    File->CurrentPosition += *BufferSize;
    break;

  case EfiOpenTftp:
    // Cache the file if it hasn't been cached yet.
    if (File->IsBufferValid == FALSE) {
      Status = CacheTftpFile(File);
      if (EFI_ERROR(Status)) {
        return Status;
      }
    }

    // Don't overwrite the buffer
    if ((File->CurrentPosition + *BufferSize) > File->MaxPosition) {
      UINT8 *TempBuffer;

      TempBuffer = File->Buffer;

      File->Buffer = AllocatePool ((UINTN)(File->CurrentPosition + *BufferSize));
      if (File->Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (File->Buffer, TempBuffer, File->Size);

      FreePool (TempBuffer);

      File->Size = (UINTN)(File->CurrentPosition + *BufferSize);
      File->MaxPosition = (UINT64)File->Size;
    }

    // Copy in the requested data
    CopyMem (File->Buffer + File->CurrentPosition, Buffer, *BufferSize);
    File->CurrentPosition += *BufferSize;

    // Mark the file dirty
    File->IsDirty = TRUE;

    Status = EFI_SUCCESS;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
  };

  return Status;
}


/**
Given Cwd expand Path to remove .. and replace them with real
directory names.

@param  Cwd     Current Working Directory
@param  Path    Path to expand

@return NULL     Cwd or Path are not valid
@return 'other'  Path with .. expanded

**/
CHAR8 *
ExpandPath (
  IN CHAR8    *Cwd,
  IN CHAR8    *Path
  )
{
  CHAR8   *NewPath;
  CHAR8   *Work, *Start, *End;
  UINTN   StrLen;
  INTN    i;

  if (Cwd == NULL || Path == NULL) {
    return NULL;
  }

  StrLen = AsciiStrSize (Cwd);
  if (StrLen <= 2) {
    // Smallest valid path is 1 char and a null
    return NULL;
  }

  StrLen = AsciiStrSize (Path);
  NewPath = AllocatePool (AsciiStrSize (Cwd) + StrLen + 1);
  if (NewPath == NULL) {
    return NULL;
  }
  AsciiStrCpy (NewPath, Cwd);

  End = Path + StrLen;
  for (Start = Path ;;) {
    Work = AsciiStrStr (Start, "..") ;
    if (Work == NULL) {
      // Remaining part of Path contains no more ..
      break;
    }

    // append path prior to ..
    AsciiStrnCat (NewPath, Start, Work - Start);
    StrLen = AsciiStrLen (NewPath);
    for (i = StrLen; i >= 0; i--) {
      if (NewPath[i] == ':') {
        // too many ..
        return NULL;
      }
      if (NewPath[i] == '/' || NewPath[i] == '\\') {
        if ((i > 0) && (NewPath[i-1] == ':')) {
          // leave the / before a :
          NewPath[i+1] = '\0';
        } else {
          // replace / will Null to remove trailing file/dir reference
          NewPath[i] = '\0';
        }
        break;
      }
    }

    Start = Work + 3;
  }

  // Handle the path that remains after the ..
  AsciiStrnCat (NewPath, Start, End - Start);

  return NewPath;
}


/**
Set the Current Working Directory (CWD). If a call is made to EfiOpen () and
the path does not contain a device name, The CWD is prepended to the path.

@param  Cwd     Current Working Directory to set


@return EFI_SUCCESS           CWD is set
@return EFI_INVALID_PARAMETER Cwd is not a valid device:path

**/
EFI_STATUS
EfiSetCwd (
  IN  CHAR8   *Cwd
  )
{
  EFI_OPEN_FILE *File;
  UINTN         Len;
  CHAR8         *Path;

  if (Cwd == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrCmp (Cwd, ".") == 0) {
    // cd . is a no-op
    return EFI_SUCCESS;
  }

  Path = Cwd;
  if (AsciiStrStr (Cwd, "..") != NULL) {
    if (gCwd == NULL) {
      // no parent
      return EFI_SUCCESS;
    }

    Len = AsciiStrLen (gCwd);
    if ((gCwd[Len-2] == ':') && ((gCwd[Len-1] == '/') || (gCwd[Len-1] == '\\'))) {
      // parent is device so nothing to do
      return EFI_SUCCESS;
    }

    // Expand .. in Cwd, given we know current working directory
    Path = ExpandPath (gCwd, Cwd);
    if (Path == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  File = EfiOpen (Path, EFI_FILE_MODE_READ, 0);
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (gCwd != NULL) {
    FreePool (gCwd);
  }

  // Use the info returned from EfiOpen as it can add in CWD if needed. So Cwd could be
  // relative to the current gCwd or not.
  gCwd = AllocatePool (AsciiStrSize (File->DeviceName) + AsciiStrSize (File->FileName) + 10);
  if (gCwd == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsciiStrCpy (gCwd, File->DeviceName);
  if (File->FileName == NULL) {
    AsciiStrCat (gCwd, ":\\");
  } else {
    AsciiStrCat (gCwd, ":");
    AsciiStrCat (gCwd, File->FileName);
  }


  EfiClose (File);
  if (Path != Cwd) {
    FreePool (Path);
  }
  return EFI_SUCCESS;
}


/**
Set the Current Working Directory (CWD). If a call is made to EfiOpen () and
the path does not contain a device name, The CWD is prepended to the path.
The CWD buffer is only valid until a new call is made to EfiSetCwd(). After
a call to EfiSetCwd() it is not legal to use the pointer returned by
this function.

@param  Cwd     Current Working Directory


@return ""      No CWD set
@return 'other' Returns buffer that contains CWD.

**/
CHAR8 *
EfiGetCwd (
  VOID
  )
{
  if (gCwd == NULL) {
    return "";
  }
  return gCwd;
}


