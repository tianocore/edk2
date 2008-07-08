/** @file
  Fault tolerant write lite protocol defines only one interface to write 
  the buffer to the fault tolerant storage.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FW_FAULT_TOLERANT_WRITE_LITE_PROTOCOL_H__
#define __FW_FAULT_TOLERANT_WRITE_LITE_PROTOCOL_H__

#define EFI_FTW_LITE_PROTOCOL_GUID \
{ 0x3f557189, 0x8dae, 0x45ae, {0xa0, 0xb3, 0x2b, 0x99, 0xca, 0x7a, 0xa7, 0xa0 } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_FTW_LITE_PROTOCOL EFI_FTW_LITE_PROTOCOL;

//
// Protocol API definitions
//
/**
  Starts a target block update. This records information about the write
  in fault tolerant storage and will complete the write in a recoverable
  manner, ensuring at all times that either the original contents or
  the modified contents are available.

  @param  This                 The pointer to this protocol instance. 
  @param  FvBlockHandle        The handle of FVB protocol that provides services 
                               for reading, writing, and erasing the target 
                               block. 
  @param  Lba                  The logical block address of the target block. 
  @param  Offset               The offset within the target block to place the 
                               data. 
  @param  Length               The number of bytes to write to the target block. 
  @param  Buffer               The data to write. 

  @retval EFI_SUCCESS          The function completed successfully 
  @retval EFI_ABORTED          The function could not complete successfully. 
  @retval EFI_BAD_BUFFER_SIZE  The input data can't fit within the target block. 
  @retval EFI_ACCESS_DENIED    No writes have been allocated. 
  @retval EFI_OUT_OF_RESOURCES Cannot allocate enough memory resource.
  @retval EFI_NOT_FOUND        Cannot find FVB protocol by handle.
  
**/
typedef
EFI_STATUS
(EFIAPI * EFI_FTW_LITE_WRITE) (
  IN EFI_FTW_LITE_PROTOCOL             *This,
  IN EFI_HANDLE                        FvbHandle,
  IN EFI_LBA                           Lba,
  IN UINTN                             Offset,
  IN UINTN                             *NumBytes,
  IN VOID                              *Buffer
  )
;

//
// Protocol declaration
//
struct _EFI_FTW_LITE_PROTOCOL {
  EFI_FTW_LITE_WRITE               Write;
};

extern EFI_GUID gEfiFaultTolerantWriteLiteProtocolGuid;

#endif
