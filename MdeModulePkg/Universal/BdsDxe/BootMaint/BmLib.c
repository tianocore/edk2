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

/**
  Wrap original AllocatePool gBS call
  and ZeroMem gBS call into a single
  function in order to decrease code length


  @param Size            The size to allocate

  @return  Valid pointer to the allocated buffer
  @retval  Null for failure

**/
VOID *
EfiAllocateZeroPool (
  IN UINTN            Size
  )
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

/**

  Find the first instance of this Protocol
  in the system and return it's interface


  @param ProtocolGuid    - Provides the protocol to search for
  @param Interface       - On return, a pointer to the first interface
                         that matches ProtocolGuid

  @retval  EFI_SUCCESS      A protocol instance matching ProtocolGuid was found
  @retval  EFI_NOT_FOUND    No protocol instances were found that match ProtocolGuid

**/
EFI_STATUS
EfiLibLocateProtocol (
  IN  EFI_GUID    *ProtocolGuid,
  OUT VOID        **Interface
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  ProtocolGuid,
                  NULL,
                  (VOID **) Interface
                  );
  return Status;
}

/**

  Function opens and returns a file handle to the root directory of a volume.


  @param DeviceHandle    - A handle for a device

  @return A valid file handle or NULL is returned

**/
EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE                   DeviceHandle
  )
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

/**

  Helper function called as part of the code needed
  to allocate the proper sized buffer for various
  EFI interfaces.


  @param Status          - Current status
  @param Buffer          - Current allocated buffer, or NULL
  @param BufferSize      - Current buffer size needed

  @retval  TRUE  if the buffer was reallocated and the caller
                 should try the API again.

**/
BOOLEAN
EfiGrowBuffer (
  IN OUT EFI_STATUS   *Status,
  IN OUT VOID         **Buffer,
  IN UINTN            BufferSize
  )
{
  BOOLEAN TryAgain;

  //
  // If this is an initial request, buffer will be null with a new buffer size
  //
  if ((*Buffer == NULL) && (BufferSize != 0)) {
    *Status = EFI_BUFFER_TOO_SMALL;
  }
  //
  // If the status code is "buffer too small", resize the buffer
  //
  TryAgain = FALSE;
  if (*Status == EFI_BUFFER_TOO_SMALL) {

    SafeFreePool (*Buffer);

    *Buffer = EfiAllocateZeroPool (BufferSize);

    if (*Buffer != NULL) {
      TryAgain = TRUE;
    } else {
      *Status = EFI_OUT_OF_RESOURCES;
    }
  }
  //
  // If there's an error, free the buffer
  //
  if (!TryAgain && EFI_ERROR (*Status) && (*Buffer != NULL)) {
    SafeFreePool (*Buffer);
    *Buffer = NULL;
  }

  return TryAgain;
}

/**
  Function returns the value of the specified variable.


  @param Name            - A Null-terminated Unicode string that is
                         the name of the vendor's variable.
  @param VendorGuid      - A unique identifier for the vendor.

  @return               The payload of the variable.

**/
VOID *
EfiLibGetVariable (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid
  )
{
  UINTN VarSize;

  return BdsLibGetVariableAndSize (Name, VendorGuid, &VarSize);
}

/**
  Function deletes the variable specified by VarName and VarGuid.


  @param VarName         - A Null-terminated Unicode string that is
                         the name of the vendor's variable.
                         
  @param VendorGuid           - A unique identifier for the vendor.

  @retval  EFI_SUCCESS           The variable was found and removed
  @retval  EFI_UNSUPPORTED       The variable store was inaccessible
  @retval  EFI_OUT_OF_RESOURCES  The temporary buffer was not available
  @retval  EFI_NOT_FOUND         The variable was not found

**/
EFI_STATUS
EfiLibDeleteVariable (
  IN CHAR16   *VarName,
  IN EFI_GUID *VarGuid
  )
{
  VOID        *VarBuf;
  EFI_STATUS  Status;

  VarBuf  = EfiLibGetVariable (VarName, VarGuid);
  Status  = EFI_NOT_FOUND;

  if (VarBuf != NULL) {
    //
    // Delete variable from Storage
    //
    Status = gRT->SetVariable (VarName, VarGuid, VAR_FLAG, 0, NULL);
    ASSERT (!EFI_ERROR (Status));
    SafeFreePool (VarBuf);
  }

  return Status;
}

/**

  Function gets the file system information from an open file descriptor,
  and stores it in a buffer allocated from pool.


  @param FHand           The file handle.

  @return                A pointer to a buffer with file information or NULL is returned

**/
EFI_FILE_SYSTEM_VOLUME_LABEL_INFO *
EfiLibFileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE      FHand
  )
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

/**
  Duplicate a string.

  @param Src             The source.

  @return A new string which is duplicated copy of the source.
  @retval NULL If there is not enought memory.

**/
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
  if (Dest != NULL) {
    CopyMem (Dest, Src, Size);
  }

  return Dest;
}

/**

  Function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

  @param FHand           File Handle.

  @return                A pointer to a buffer with file information or NULL is returned

**/
EFI_FILE_INFO *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
  )
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

/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      - A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  UINTN Count;
  UINTN Size;

  Count = 0;
  while (GetNextDevicePathInstance (&DevicePath, &Size)) {
    Count += 1;
  }

  return Count;
}

/**
  Adjusts the size of a previously allocated buffer.


  @param OldPool         - A pointer to the buffer whose size is being adjusted.
  @param OldSize         - The size of the current buffer.
  @param NewSize         - The size of the new buffer.

  @retval  EFI_SUCEESS            The requested number of bytes were allocated.
  @retval  EFI_OUT_OF_RESOURCES   The pool requested could not be allocated.
  @retval  EFI_INVALID_PARAMETER  The buffer was invalid.

**/
VOID *
EfiReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  )
{
  VOID  *NewPool;

  NewPool = NULL;
  if (NewSize != 0) {
    NewPool = EfiAllocateZeroPool (NewSize);
  }

  if (OldPool != NULL) {
    if (NewPool != NULL) {
      CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
    }

    SafeFreePool (OldPool);
  }

  return NewPool;
}

/**
  Compare two EFI_TIME data.


  @param FirstTime       - A pointer to the first EFI_TIME data.
  @param SecondTime      - A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
TimeCompare (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  )
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

/**
  Get a string from the Data Hub record based on 
  a device path.

  @param DevPath         The device Path.

  @return A string located from the Data Hub records based on
          the device path.

**/
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
