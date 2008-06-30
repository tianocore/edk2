/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixFwh.h

Abstract:

  Unix FWH PPI as defined in Tiano

--*/

#ifndef __UNIX_PEI_FWH_H__
#define __UNIX_PEI_FWH_H__

#include <UnixDxe.h>

#define UNIX_FWH_PPI_GUID \
  { \
    0xf2f0dc30, 0x8985, 0x11db, {0xa1, 0x5b, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef
EFI_STATUS
(EFIAPI *UNIX_FWH_INFORMATION) (
  IN     UINTN                  Index,
  IN OUT EFI_PHYSICAL_ADDRESS   *FdBase,
  IN OUT UINT64                 *FdSize
  );

/*++

Routine Description:
  Return the FD Size and base address. Since the FD is loaded from a 
  file into host memory only the SEC will know it's address.

Arguments:
  Index  - Which FD, starts at zero.
  FdSize - Size of the FD in bytes
  FdBase - Start address of the FD. Assume it points to an FV Header

Returns:
  EFI_SUCCESS     - Return the Base address and size of the FV
  EFI_UNSUPPORTED - Index does nto map to an FD in the system

--*/
typedef struct {
  UNIX_FWH_INFORMATION  UnixFwh;
} UNIX_FWH_PPI;

extern EFI_GUID gUnixFwhPpiGuid;

#endif
