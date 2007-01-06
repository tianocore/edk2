/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixThunk.h

Abstract:

  Unix Thunk interface PPI

--*/

#ifndef __UNIX_PEI_UNIX_THUNK_H__
#define __UNIX_PEI_UNIX_THUNK_H__

#include <UnixDxe.h>

#define PEI_UNIX_THUNK_PPI_GUID \
  { \
    0xf2f830f2, 0x8985, 0x11db, {0x80, 0x6b, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef
VOID *
(EFIAPI *PEI_UNIX_THUNK_INTERFACE) (
  VOID
  );

/*++

Routine Description:
  Export of EFI_UNIX_THUNK_PROTOCOL from the Unix SEC.

Arguments:
  InterfaceBase - Address of the EFI_UNIX_THUNK_PROTOCOL

Returns:
  EFI_SUCCESS - Data returned

--*/
typedef struct {
  PEI_UNIX_THUNK_INTERFACE  UnixThunk;
} PEI_UNIX_THUNK_PPI;

extern EFI_GUID gPeiUnixThunkPpiGuid;

#endif
