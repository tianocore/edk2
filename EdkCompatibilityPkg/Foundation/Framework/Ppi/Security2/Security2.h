/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Security2.h

Abstract:

  PI 1.0 spec definition.

--*/


#ifndef __SECURITY2_PPI_H__
#define __SECURITY2_PPI_H__

#define EFI_PEI_SECURITY2_PPI_GUID \
  { 0xdcd0be23, 0x9586, 0x40f4, {0xb6, 0x43, 0x6, 0x52, 0x2c, 0xed, 0x4e, 0xde}}


EFI_FORWARD_DECLARATION (EFI_PEI_SECURITY2_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_SECURITY_AUTHENTICATION_STATE) (
  IN CONST EFI_PEI_SERVICES       **PeiServices,
  IN CONST EFI_PEI_SECURITY2_PPI  *This,
  IN UINT32                       AuthenticationStatus,
  IN EFI_PEI_FV_HANDLE            FvHandle,
  IN EFI_PEI_FILE_HANDLE          FileHandle,
  IN OUT BOOLEAN                  *DeferExection
  );

struct _EFI_PEI_SECURITY2_PPI {
  EFI_PEI_SECURITY_AUTHENTICATION_STATE  AuthenticationState;
};


extern EFI_GUID gEfiPeiSecurity2PpiGuid;

#endif
