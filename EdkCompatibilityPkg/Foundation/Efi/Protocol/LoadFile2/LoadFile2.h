/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LoadFile2.h

Abstract:

  Load File2 protocol as defined in the UEFI specification.

  Load File2 protocol exists to support to obtain files from arbitrary devices
  but are not used as boot options.

--*/

#ifndef _LOAD_FILE2_H_
#define _LOAD_FILE2_H_

#define EFI_LOAD_FILE2_PROTOCOL_GUID \
  { \
  0x4006c0c1, 0xfcb3, 0x403e, {0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d} \
  }

EFI_FORWARD_DECLARATION (EFI_LOAD_FILE2_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_LOAD_FILE2_LOAD_FILE) (
  IN EFI_LOAD_FILE2_PROTOCOL          * This,
  IN EFI_DEVICE_PATH_PROTOCOL         * FilePath,
  IN BOOLEAN                          BootPolicy,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer OPTIONAL
  )
/*++

  Routine Description:
    Causes the driver to load a specified file.

  Arguments:
    This       - Protocol instance pointer.
    FilePath   - The device specific path of the file to load.
    BootPolicy - Should always be FALSE.
    BufferSize - On input the size of Buffer in bytes. On output with a return
                  code of EFI_SUCCESS, the amount of data transferred to 
                  Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                  the size of Buffer required to retrieve the requested file.
    Buffer     - The memory buffer to transfer the file to. IF Buffer is NULL,
                  then no the size of the requested file is returned in 
                  BufferSize.

  Returns:
    EFI_SUCCESS           - The file was loaded.
    EFI_UNSUPPORTED       - BootPolicy is TRUE.
    EFI_INVALID_PARAMETER - FilePath is not a valid device path, or 
                             BufferSize is NULL.
    EFI_NO_MEDIA          - No medium was present to load the file.
    EFI_DEVICE_ERROR      - The file was not loaded due to a device error.
    EFI_NO_RESPONSE       - The remote system did not respond.
    EFI_NOT_FOUND         - The file was not found
    EFI_ABORTED           - The file load process was manually cancelled.
    EFI_BUFFER_TOO_SMALL  - The BufferSize is too small to read the current 
                             directory entry. BufferSize has been updated with
                             the size needed to complete the request.

--*/
;

struct _EFI_LOAD_FILE2_PROTOCOL {
  EFI_LOAD_FILE2_LOAD_FILE LoadFile;
};

extern EFI_GUID gEfiLoadFile2ProtocolGuid;

#endif
