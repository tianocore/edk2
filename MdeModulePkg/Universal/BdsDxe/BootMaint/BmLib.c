/** @file
  Utility routines used by boot maintenance modules.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaint.h"

VOID *
EfiAllocateZeroPool (
  IN UINTN            Size
  )
/*++

Routine Description:
  Wrap original AllocatePool gBS call
  and ZeroMem gBS call into a single
  function in order to decrease code length

Arguments:

Returns:
  Valid pointer to the allocated buffer
  Null for failure

--*/
{
  EFI_STATUS  Status;
  VOID        *Ptr;
  Status = gBS->AllocatePool (EfiBootServicesData, Size, &Ptr);
  if (EFI_ERROR (Status)) {
    Ptr = NULL;
    return Ptr;
  }

  ZeroMem (Ptr, Size);
  return Ptr;
}

EFI_STATUS
EfiLibLocateProtocol (
  IN  EFI_GUID    *ProtocolGuid,
  OUT VOID        **Interface
  )
/*++

Routine Description:

  Find the first instance of this Protocol
  in the system and return it's interface

Arguments:

  ProtocolGuid    - Provides the protocol to search for
  Interface       - On return, a pointer to the first interface
                    that matches ProtocolGuid

Returns:

  EFI_SUCCESS     - A protocol instance matching ProtocolGuid was found

  EFI_NOT_FOUND   - No protocol instances were found that match ProtocolGuid

--*/
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  ProtocolGuid,
                  NULL,
                  (VOID **) Interface
                  );
  return Status;
}

EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE                   DeviceHandle
  )
/*++

Routine Description:

  Function opens and returns a file handle to the root directory of a volume.

Arguments:

  DeviceHandle         - A handle for a device

Returns:

  A valid file handle or NULL is returned

--*/
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_FILE_HANDLE                 File;

  File = NULL;

  //
  // File the file system interface to the device
  //
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID *) &Volume
                  );

  //
  // Open the root directory of the volume
  //
  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (
                      Volume,
                      &File
                      );
  }
  //
  // Done
  //
  return EFI_ERROR (Status) ? NULL : File;
}

BOOLEAN
EfiGrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
/*++

Routine Description:

    Helper function called as part of the code needed
    to allocate the proper sized buffer for various
    EFI interfaces.

Arguments:

    Status      - Current status

    Buffer      - Current allocated buffer, or NULL

    BufferSize  - Current buffer size needed

Returns:

    TRUE - if the buffer was reallocated and the caller
    should try the API again.

--*/
{
  BOOLEAN TryAgain;

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if (!*Buffer && BufferSize) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    SafeFreePool (*Buffer);

    *Buffer = EfiAllocateZeroPool (BufferSize);

    if (*Buffer) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && *Buffer) {
    SafeFreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

VOID *
EfiLibGetVariable (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid
  )
/*++

Routine Description:
  Function returns the value of the specified variable.

Arguments:
  Name                - A Null-terminated Unicode string that is
                        the name of the vendor's variable.

  VendorGuid          - A unique identifier for the vendor.

Returns:

  None

--*/
{
  UINTN VarSize;

  return BdsLibGetVariableAndSize (Name, VendorGuid, &VarSize);
}

EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  )
/*++

Routine Description:
  Function deletes the variable specified by VarName and VarGuid.

Arguments:
  VarName              - A Null-terminated Unicode string that is
                         the name of the vendor's variable.

  VendorGuid           - A unique identifier for the vendor.

Returns:

  EFI_SUCCESS          - The variable was found and removed

  EFI_UNSUPPORTED      - The variable store was inaccessible

  EFI_OUT_OF_RESOURCES - The temporary buffer was not available

  EFI_NOT_FOUND        - The variable was not found

--*/
{
  VOID        *VarBuf;
  EFI_STATUS  Status;

  VarBuf  = EfiLibGetVariable (VarName, VarGuid);
  Status  = EFI_NOT_FOUND;

  if (VarBuf) {
    //
    // Delete variable from Storage
    //
    Status = gRT->SetVariable (VarName, VarGuid, VAR_FLAG, 0, NULL);
    ASSERT (!EFI_ERROR (Status));
    SafeFreePool (VarBuf);
  }

  return Status;
}

EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *
EfiLibFileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE      FHand
  )
/*++

Routine Description:

  Function gets the file system information from an open file descriptor,
  and stores it in a buffer allocated from pool.

Arguments:

  Fhand         - A file handle

Returns:

  A pointer to a buffer with file information or NULL is returned

--*/
{
  EFI_STATUS                        Status;
  EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *Buffer;
  UINTN                             BufferSize;
  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = SIZE_OF_EFI_FILE_SYSTEM_VOLUME_LABEL_INFO + 200;

  //
  // Call the real function
  //
  while (EfiGrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                      FHand,
                      &gEfiFileSystemVolumeLabelInfoIdGuid,
                      &BufferSize,
                      Buffer
                      );
  }

  return Buffer;
}

CHAR16 *
EfiStrDuplicate (
  IN CHAR16   *Src
  )
{
  CHAR16  *Dest;
  UINTN   Size;

  Size  = StrSize (Src);
  Dest  = EfiAllocateZeroPool (Size);
  ASSERT (Dest != NULL);
  if (Dest) {
    CopyMem (Dest, Src, Size);
  }

  return Dest;
}

EFI_FILE_INFO *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
  )
/*++

Routine Description:

  Function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

Arguments:

  Fhand         - A file handle

Returns:

  A pointer to a buffer with file information or NULL is returned

--*/
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *Buffer;
  UINTN         BufferSize;

  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = SIZE_OF_EFI_FILE_INFO + 200;

  //
  // Call the real function
  //
  while (EfiGrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                      FHand,
                      &gEfiFileInfoGuid,
                      &BufferSize,
                      Buffer
                      );
  }

  return Buffer;
}

UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
/*++

Routine Description:
  Function is used to determine the number of device path instances
  that exist in a device path.

Arguments:
  DevicePath           - A pointer to a device path data structure.

Returns:

  This function counts and returns the number of device path instances
  in DevicePath.

--*/
{
  UINTN Count;
  UINTN Size;

  Count = 0;
  while (GetNextDevicePathInstance (&DevicePath, &Size)) {
    Count += 1;
  }

  return Count;
}

VOID *
EfiReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  )
/*++

Routine Description:
  Adjusts the size of a previously allocated buffer.

Arguments:
  OldPool               - A pointer to the buffer whose size is being adjusted.
  OldSize               - The size of the current buffer.
  NewSize               - The size of the new buffer.

Returns:

  EFI_SUCEESS           - The requested number of bytes were allocated.

  EFI_OUT_OF_RESOURCES  - The pool requested could not be allocated.

  EFI_INVALID_PARAMETER - The buffer was invalid.

--*/
{
  VOID  *NewPool;

  NewPool = NULL;
  if (NewSize) {
    NewPool = EfiAllocateZeroPool (NewSize);
  }

  if (OldPool) {
    if (NewPool) {
      CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
    }

    SafeFreePool (OldPool);
  }

  return NewPool;
}

BOOLEAN
TimeCompare (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  )
/*++

Routine Description:
  Compare two EFI_TIME data.

Arguments:

  FirstTime         - A pointer to the first EFI_TIME data.
  SecondTime        - A pointer to the second EFI_TIME data.

Returns:
  TRUE              The FirstTime is not later than the SecondTime.
  FALSE             The FirstTime is later than the SecondTime.

--*/
{
  if (FirstTime->Year != SecondTime->Year) {
    return (BOOLEAN) (FirstTime->Year < SecondTime->Year);
  } else if (FirstTime->Month != SecondTime->Month) {
    return (BOOLEAN) (FirstTime->Month < SecondTime->Month);
  } else if (FirstTime->Day != SecondTime->Day) {
    return (BOOLEAN) (FirstTime->Day < SecondTime->Day);
  } else if (FirstTime->Hour != SecondTime->Hour) {
    return (BOOLEAN) (FirstTime->Hour < SecondTime->Hour);
  } else if (FirstTime->Minute != SecondTime->Minute) {
    return (BOOLEAN) (FirstTime->Minute < FirstTime->Minute);
  } else if (FirstTime->Second != SecondTime->Second) {
    return (BOOLEAN) (FirstTime->Second < SecondTime->Second);
  }

  return (BOOLEAN) (FirstTime->Nanosecond <= SecondTime->Nanosecond);
}

UINT16 *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL                 *DevPath
  )
{
  EFI_STATUS                                  Status;
  UINT16                                      *Desc;
  EFI_DATA_HUB_PROTOCOL                       *Datahub;
  UINT64                                      Count;
  EFI_DATA_RECORD_HEADER                      *Record;
  EFI_SUBCLASS_TYPE1_HEADER                   *DataHdr;
  EFI_GUID                                    MiscGuid = EFI_MISC_SUBCLASS_GUID;
  EFI_MISC_ONBOARD_DEVICE_DATA                *ob;
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *Port;
  EFI_TIME                                    CurTime;

  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  (VOID **) &Datahub
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gRT->GetTime (&CurTime, NULL);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Count = 0;
  do {
    Status = Datahub->GetNextRecord (Datahub, &Count, NULL, &Record);

    if (EFI_ERROR (Status)) {
      break;
    }

    if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA && CompareGuid (&Record->DataRecordGuid, &MiscGuid)) {
      //
      // This record is what we need
      //
      DataHdr = (EFI_SUBCLASS_TYPE1_HEADER *) (Record + 1);
      if (EFI_MISC_ONBOARD_DEVICE_RECORD_NUMBER == DataHdr->RecordType) {
        ob = (EFI_MISC_ONBOARD_DEVICE_DATA *) (DataHdr + 1);
        if (BdsLibMatchDevicePaths ((EFI_DEVICE_PATH_PROTOCOL *) &ob->OnBoardDevicePath, DevPath)) {
          GetProducerString (&Record->ProducerName, ob->OnBoardDeviceDescription, &Desc);
          return Desc;
        }
      }

      if (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_RECORD_NUMBER == DataHdr->RecordType) {
        Port = (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) (DataHdr + 1);
        if (BdsLibMatchDevicePaths ((EFI_DEVICE_PATH_PROTOCOL *) &Port->PortPath, DevPath)) {
          GetProducerString (&Record->ProducerName, Port->PortExternalConnectorDesignator, &Desc);
          return Desc;
        }
      }
    }

  } while (TimeCompare (&Record->LogTime, &CurTime) && Count != 0);

  return NULL;
}
