/** @file
  Debug Port Library implementation based on usb3 debug port.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <PiDxe.h>
#include <Library/UefiBootServicesTableLib.h>
#include "DebugCommunicationLibUsb3Internal.h"

/**
  Allocate aligned memory for XHC's usage.

  @param  BufferSize      The size, in bytes, of the Buffer.
  
  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID*
AllocateAlignBuffer (
  IN UINTN                    BufferSize
  )
{
  VOID      *Buf;
  
  Buf = NULL;
  
  if (gBS != NULL) {
    Buf = (VOID *)(UINTN)0xFFFFFFFF;
    gBS->AllocatePages (
           AllocateMaxAddress,
           EfiACPIMemoryNVS,
           EFI_SIZE_TO_PAGES (BufferSize),
           (EFI_PHYSICAL_ADDRESS *) &Buf
           );
  }

  return Buf;
}
