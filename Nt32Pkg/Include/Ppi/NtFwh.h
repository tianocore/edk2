/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  NtFwh.h

Abstract:

  WinNt FWH PPI as defined in Tiano

**/

#ifndef __NT_PEI_FWH_H__
#define __NT_PEI_FWH_H__

#include <WinNtDxe.h>

#define NT_FWH_PPI_GUID \
  { \
    0x4e76928f, 0x50ad, 0x4334, {0xb0, 0x6b, 0xa8, 0x42, 0x13, 0x10, 0x8a, 0x57 } \
  }

typedef
EFI_STATUS
(EFIAPI *NT_FWH_INFORMATION) (
  IN     UINTN                  Index,
  IN OUT EFI_PHYSICAL_ADDRESS   * FdBase,
  IN OUT UINT64                 *FdSize
  );

/*++

Routine Description:
  Return the FD Size and base address. Since the FD is loaded from a 
  file into Windows memory only the SEC will know it's address.

Arguments:
  Index  - Which FD, starts at zero.
  FdSize - Size of the FD in bytes
  FdBase - Start address of the FD. Assume it points to an FV Header

Returns:
  EFI_SUCCESS     - Return the Base address and size of the FV
  EFI_UNSUPPORTED - Index does nto map to an FD in the system

--*/
typedef struct {
  NT_FWH_INFORMATION  NtFwh;
} NT_FWH_PPI;

extern EFI_GUID gNtFwhPpiGuid;

#endif
