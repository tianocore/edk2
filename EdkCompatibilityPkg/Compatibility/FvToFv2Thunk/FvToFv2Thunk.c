/** @file
UEFI PI specification supersedes Inte's Framework Specification.
EFI_FIRMWARE_VOLUME_PROTOCOL defined in Intel Framework Pkg is replaced by
EFI_FIRMWARE_VOLUME2_PROTOCOL in MdePkg.
This module produces FV on top of FV2. This module is used on platform when both of
these two conditions are true:
1) Framework module consuming FV is present
2) And the platform only produces FV2

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
Module Name:

**/

#include <PiDxe.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolume.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

  @param  This                  Calling context
  @param  Attributes            output buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  OUT FRAMEWORK_EFI_FV_ATTRIBUTES   *Attributes
  );

/**
  Sets volume attributes

  @param  This                  Calling context
  @param  Attributes            Buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL     *This,
  IN OUT FRAMEWORK_EFI_FV_ATTRIBUTES  *Attributes
  );

/**
  Read the requested file (NameGuid) and returns data in Buffer.

  @param  This                  Calling context
  @param  NameGuid              Filename identifying which file to read
  @param  Buffer                Pointer to pointer to buffer in which contents of file are returned.
                                <br>
                                If Buffer is NULL, only type, attributes, and size are returned as
                                there is no output buffer.
                                <br>
                                If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
                                from BS pool by ReadFile
                                <br>
                                If Buffer != NULL and *Buffer != NULL, the output buffer has been
                                allocated by the caller and is being passed in.
  @param  BufferSize            Indicates the buffer size passed in, and on output the size
                                required to complete the read
  @param  FoundType             Indicates the type of the file who's data is returned
  @param  FileAttributes        Indicates the attributes of the file who's data is resturned
  @param  AuthenticationStatus  Indicates the authentication status of the data

  @retval EFI_SUCCESS
  @retval EFI_WARN_BUFFER_TOO_SMALL
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
EFI_STATUS
EFIAPI
FvReadFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT EFI_FV_FILETYPE               *FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT UINT32                        *AuthenticationStatus
  );

/**
  Read the requested section from the specified file and returns data in Buffer.

  @param  This                  Calling context
  @param  NameGuid              Filename identifying the file from which to read
  @param  SectionType           Indicates what section type to retrieve
  @param  SectionInstance       Indicates which instance of SectionType to retrieve
  @param  Buffer                Pointer to pointer to buffer in which contents of file are returned.
                                <br>
                                If Buffer is NULL, only type, attributes, and size are returned as
                                there is no output buffer.
                                <br>
                                If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
                                from BS pool by ReadFile
                                <br>
                                If Buffer != NULL and *Buffer != NULL, the output buffer has been
                                allocated by the caller and is being passed in.
  @param  BufferSize            Indicates the buffer size passed in, and on output the size
                                required to complete the read
  @param  AuthenticationStatus  Indicates the authentication status of the data

  @retval EFI_SUCCESS
  @retval EFI_WARN_BUFFER_TOO_SMALL
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
EFI_STATUS
EFIAPI 
FvReadSection (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN EFI_SECTION_TYPE               SectionType,
  IN UINTN                          SectionInstance,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT UINT32                        *AuthenticationStatus
  );

/**
  Write the supplied file (NameGuid) to the FV.

  @param  This                  Calling context
  @param  NumberOfFiles         Indicates the number of file records pointed to by FileData
  @param  WritePolicy           Indicates the level of reliability of the write with respect to
                                things like power failure events.
  @param  FileData              A pointer to an array of EFI_FV_WRITE_FILE_DATA structures. Each
                                element in the array indicates a file to write, and there are
                                NumberOfFiles elements in the input array.

  @retval EFI_SUCCESS
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_DEVICE_ERROR
  @retval EFI_WRITE_PROTECTED
  @retval EFI_NOT_FOUND
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI 
FvWriteFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL      *This,
  IN UINT32                            NumberOfFiles,
  IN FRAMEWORK_EFI_FV_WRITE_POLICY     WritePolicy,
  IN FRAMEWORK_EFI_FV_WRITE_FILE_DATA  *FileData
  );

/**
  Given the input key, search for the next matching file in the volume.

  @param  This                  Calling context
  @param  Key                   Pointer to a caller allocated buffer that contains an implementation
                                specific key that is used to track where to begin searching on
                                successive calls.
  @param  FileType              Indicates the file type to filter for
  @param  NameGuid              Guid filename of the file found
  @param  Attributes            Attributes of the file found
  @param  Size                  Size in bytes of the file found

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
EFI_STATUS
EFIAPI 
FvGetNextFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT VOID                       *Key,
  IN OUT EFI_FV_FILETYPE            *FileType,
  OUT EFI_GUID                      *NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT UINTN                         *Size
  );

#define FIRMWARE_VOLUME_PRIVATE_DATA_SIGNATURE  EFI_SIGNATURE_32 ('f', 'v', 't', 'h')

typedef struct {
  UINTN                          Signature;
  EFI_FIRMWARE_VOLUME_PROTOCOL   FirmwareVolume;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;
} FIRMWARE_VOLUME_PRIVATE_DATA;

#define FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS(a) CR (a, FIRMWARE_VOLUME_PRIVATE_DATA, FirmwareVolume, FIRMWARE_VOLUME_PRIVATE_DATA_SIGNATURE)

//
// Firmware Volume Protocol template
//
EFI_EVENT  mFvRegistration;

FIRMWARE_VOLUME_PRIVATE_DATA gFirmwareVolumePrivateDataTemplate = {
  FIRMWARE_VOLUME_PRIVATE_DATA_SIGNATURE,
  {
    FvGetVolumeAttributes,
    FvSetVolumeAttributes,
    FvReadFile,
    FvReadSection,
    FvWriteFile,
    FvGetNextFile,
    0,
    NULL
  },
  NULL
};

//
// Module globals
//

VOID
EFIAPI
FvNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                    Status;
  UINTN                         BufferSize;
  EFI_HANDLE                    Handle;
  FIRMWARE_VOLUME_PRIVATE_DATA  *Private;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *FirmwareVolume;

  while (TRUE) {
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    mFvRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // Exit Path of While Loop....
      //
      break;
    }

    //
    // Skip this handle if the Firmware Volume Protocol is already installed
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiFirmwareVolumeProtocolGuid,
                    (VOID **)&FirmwareVolume
                    );
    if (!EFI_ERROR (Status)) {
      continue;
    }

    //
    // Allocate private data structure
    //
    Private = AllocateCopyPool (sizeof (FIRMWARE_VOLUME_PRIVATE_DATA), &gFirmwareVolumePrivateDataTemplate);
    if (Private == NULL) {
      continue;
    }

    //
    // Retrieve the Firmware Volume2 Protocol
    //
    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&Private->FirmwareVolume2
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Fill in rest of private data structure
    //
    Private->FirmwareVolume.KeySize      = Private->FirmwareVolume2->KeySize;
    Private->FirmwareVolume.ParentHandle = Private->FirmwareVolume2->ParentHandle;

    //
    // Install Firmware Volume Protocol onto same handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiFirmwareVolumeProtocolGuid,
                    &Private->FirmwareVolume,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
}


/**
  The user Entry Point for DXE driver. The user code starts with this function
  as the real entry point for the image goes into a library that calls this 
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeFirmwareVolume2 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EfiCreateProtocolNotifyEvent (
    &gEfiFirmwareVolume2ProtocolGuid,
    TPL_CALLBACK,
    FvNotificationEvent,
    NULL,
    &mFvRegistration
    );
  return EFI_SUCCESS;
}

/**
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

  @param  This                  Calling context
  @param  Attributes            output buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  OUT FRAMEWORK_EFI_FV_ATTRIBUTES   *Attributes
  )
{
  EFI_STATUS                     Status;
  FIRMWARE_VOLUME_PRIVATE_DATA   *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;

  Private = FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume2 = Private->FirmwareVolume2;

  Status = FirmwareVolume2->GetVolumeAttributes (
                              FirmwareVolume2,
                              Attributes
                              );
  if (!EFI_ERROR (Status)) {
    *Attributes = (*Attributes & 0x1ff) | ((UINTN)EFI_FV_ALIGNMENT_2 << RShiftU64((*Attributes & EFI_FV2_ALIGNMENT), 16));
  }
  return Status;
}

/**
  Sets volume attributes

  @param  This                  Calling context
  @param  Attributes            Buffer which contains attributes

  @retval EFI_INVALID_PARAMETER
  @retval EFI_DEVICE_ERROR
  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL     *This,
  IN OUT FRAMEWORK_EFI_FV_ATTRIBUTES  *Attributes
  )
{
  FIRMWARE_VOLUME_PRIVATE_DATA   *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;
  INTN                           Alignment;
  EFI_FV_ATTRIBUTES              Fv2Attributes; 

  Private = FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume2 = Private->FirmwareVolume2;

  Fv2Attributes = (*Attributes & 0x1ff);
  Alignment = LowBitSet64 (RShiftU64 (*Attributes, 16) & 0xffff);
  if (Alignment != -1) {
    Fv2Attributes |= LShiftU64 (Alignment, 16);
  }
  return FirmwareVolume2->SetVolumeAttributes (
                            FirmwareVolume2,
                            &Fv2Attributes
                            );
}

/**
  Read the requested file (NameGuid) and returns data in Buffer.

  @param  This                  Calling context
  @param  NameGuid              Filename identifying which file to read
  @param  Buffer                Pointer to pointer to buffer in which contents of file are returned.
                                <br>
                                If Buffer is NULL, only type, attributes, and size are returned as
                                there is no output buffer.
                                <br>
                                If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
                                from BS pool by ReadFile
                                <br>
                                If Buffer != NULL and *Buffer != NULL, the output buffer has been
                                allocated by the caller and is being passed in.
  @param  BufferSize            Indicates the buffer size passed in, and on output the size
                                required to complete the read
  @param  FoundType             Indicates the type of the file who's data is returned
  @param  FileAttributes        Indicates the attributes of the file who's data is resturned
  @param  AuthenticationStatus  Indicates the authentication status of the data

  @retval EFI_SUCCESS
  @retval EFI_WARN_BUFFER_TOO_SMALL
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
EFI_STATUS
EFIAPI
FvReadFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT EFI_FV_FILETYPE               *FoundType,
  OUT EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
  OUT UINT32                        *AuthenticationStatus
  )
{
  FIRMWARE_VOLUME_PRIVATE_DATA   *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;

  Private = FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume2 = Private->FirmwareVolume2;

  return FirmwareVolume2->ReadFile (
                            FirmwareVolume2,
                            NameGuid,
                            Buffer,
                            BufferSize,
                            FoundType,
                            FileAttributes,
                            AuthenticationStatus
                            );
}

/**
  Read the requested section from the specified file and returns data in Buffer.

  @param  This                  Calling context
  @param  NameGuid              Filename identifying the file from which to read
  @param  SectionType           Indicates what section type to retrieve
  @param  SectionInstance       Indicates which instance of SectionType to retrieve
  @param  Buffer                Pointer to pointer to buffer in which contents of file are returned.
                                <br>
                                If Buffer is NULL, only type, attributes, and size are returned as
                                there is no output buffer.
                                <br>
                                If Buffer != NULL and *Buffer == NULL, the output buffer is allocated
                                from BS pool by ReadFile
                                <br>
                                If Buffer != NULL and *Buffer != NULL, the output buffer has been
                                allocated by the caller and is being passed in.
  @param  BufferSize            Indicates the buffer size passed in, and on output the size
                                required to complete the read
  @param  AuthenticationStatus  Indicates the authentication status of the data

  @retval EFI_SUCCESS
  @retval EFI_WARN_BUFFER_TOO_SMALL
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
EFI_STATUS
EFIAPI 
FvReadSection (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN EFI_GUID                       *NameGuid,
  IN EFI_SECTION_TYPE               SectionType,
  IN UINTN                          SectionInstance,
  IN OUT VOID                       **Buffer,
  IN OUT UINTN                      *BufferSize,
  OUT UINT32                        *AuthenticationStatus
  )
{
  FIRMWARE_VOLUME_PRIVATE_DATA   *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;

  Private = FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume2 = Private->FirmwareVolume2;

  return FirmwareVolume2->ReadSection (
                            FirmwareVolume2,
                            NameGuid,
                            SectionType,
                            SectionInstance,
                            Buffer,
                            BufferSize,
                            AuthenticationStatus
                            );
}

/**
  Write the supplied file (NameGuid) to the FV.

  @param  This                  Calling context
  @param  NumberOfFiles         Indicates the number of file records pointed to by FileData
  @param  WritePolicy           Indicates the level of reliability of the write with respect to
                                things like power failure events.
  @param  FileData              A pointer to an array of EFI_FV_WRITE_FILE_DATA structures. Each
                                element in the array indicates a file to write, and there are
                                NumberOfFiles elements in the input array.

  @retval EFI_SUCCESS
  @retval EFI_OUT_OF_RESOURCES
  @retval EFI_DEVICE_ERROR
  @retval EFI_WRITE_PROTECTED
  @retval EFI_NOT_FOUND
  @retval EFI_INVALID_PARAMETER

**/
EFI_STATUS
EFIAPI 
FvWriteFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL      *This,
  IN UINT32                            NumberOfFiles,
  IN FRAMEWORK_EFI_FV_WRITE_POLICY     WritePolicy,
  IN FRAMEWORK_EFI_FV_WRITE_FILE_DATA  *FileData
  )
{
  FIRMWARE_VOLUME_PRIVATE_DATA   *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;

  Private = FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume2 = Private->FirmwareVolume2;

  return FirmwareVolume2->WriteFile (
                            FirmwareVolume2,
                            NumberOfFiles,
                            WritePolicy,
                            (EFI_FV_WRITE_FILE_DATA *)FileData
                            );
}

/**
  Given the input key, search for the next matching file in the volume.

  @param  This                  Calling context
  @param  Key                   Pointer to a caller allocated buffer that contains an implementation
                                specific key that is used to track where to begin searching on
                                successive calls.
  @param  FileType              Indicates the file type to filter for
  @param  NameGuid              Guid filename of the file found
  @param  Attributes            Attributes of the file found
  @param  Size                  Size in bytes of the file found

  @retval EFI_SUCCESS
  @retval EFI_NOT_FOUND
  @retval EFI_DEVICE_ERROR
  @retval EFI_ACCESS_DENIED

**/
EFI_STATUS
EFIAPI 
FvGetNextFile (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT VOID                       *Key,
  IN OUT EFI_FV_FILETYPE            *FileType,
  OUT EFI_GUID                      *NameGuid,
  OUT EFI_FV_FILE_ATTRIBUTES        *Attributes,
  OUT UINTN                         *Size
  )
{
  FIRMWARE_VOLUME_PRIVATE_DATA   *Private;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FirmwareVolume2;

  Private = FIRMWARE_VOLUME_PRIVATE_DATA_FROM_THIS (This);
  FirmwareVolume2 = Private->FirmwareVolume2;

  return FirmwareVolume2->GetNextFile (
                            FirmwareVolume2,
                            Key,
                            FileType,
                            NameGuid,
                            Attributes,
                            Size
                            );
}
