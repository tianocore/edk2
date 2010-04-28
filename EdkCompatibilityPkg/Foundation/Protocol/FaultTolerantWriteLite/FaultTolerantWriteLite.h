/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FaultTolerantWriteLite.h

Abstract:

  This is a simple fault tolerant write driver, based on PlatformFd library.
  And it only supports write BufferSize <= SpareAreaLength.

--*/

#ifndef _FW_FAULT_TOLERANT_WRITE_LITE_PROTOCOL_H_
#define _FW_FAULT_TOLERANT_WRITE_LITE_PROTOCOL_H_

#define EFI_FTW_LITE_PROTOCOL_GUID \
{ 0x3f557189, 0x8dae, 0x45ae, {0xa0, 0xb3, 0x2b, 0x99, 0xca, 0x7a, 0xa7, 0xa0} }

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_FTW_LITE_PROTOCOL);

//
// Protocol API definitions
//

typedef
EFI_STATUS
(EFIAPI * EFI_FTW_LITE_WRITE) (
  IN EFI_FTW_LITE_PROTOCOL             *This,
  IN EFI_HANDLE                        FvbHandle,
  IN EFI_LBA                           Lba,
  IN UINTN                             Offset,
  IN UINTN                             *NumBytes,
  IN VOID                              *Buffer
  );
/*++

Routine Description:

  Starts a target block update. This records information about the write 
  in fault tolerant storage and will complete the write in a recoverable 
  manner, ensuring at all times that either the original contents or 
  the modified contents are available.

Arguments:

  This             - Calling context
  FvBlockHandle    - The handle of FVB protocol that provides services for 
                     reading, writing, and erasing the target block.
  Lba              - The logical block address of the target block.  
  Offset           - The offset within the target block to place the data.
  Length           - The number of bytes to write to the target block.
  Buffer           - The data to write.

Returns:

  EFI_SUCCESS          - The function completed successfully
  EFI_ABORTED          - The function could not complete successfully.
  EFI_BAD_BUFFER_SIZE  - The write would span a block boundary, 
                         which is not a valid action.
  EFI_ACCESS_DENIED    - No writes have been allocated.
  EFI_NOT_READY        - The last write has not been completed.  
                         Restart () must be called to complete it.

--*/

//
// Protocol declaration
//
struct _EFI_FTW_LITE_PROTOCOL {
  EFI_FTW_LITE_WRITE               Write;
};

extern EFI_GUID gEfiFaultTolerantWriteLiteProtocolGuid;

#endif
