/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LoadFile2.h

Abstract:

  PI 1.0 spec definition.

--*/

#ifndef __LOAD_FILE_PPI_H__
#define __LOAD_FILE_PPI_H__

EFI_FORWARD_DECLARATION (EFI_PEI_LOAD_FILE_PPI);

#define EFI_PEI_LOAD_FILE_GUID \
  { 0xb9e0abfe, 0x5979, 0x4914, {0x97, 0x7f, 0x6d, 0xee, 0x78, 0xc2, 0x78, 0xa6}}


typedef
EFI_STATUS
(EFIAPI *EFI_PEI_LOAD_FILE) (
  IN    CONST EFI_PEI_LOAD_FILE_PPI   *This,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddress,
  OUT    UINT64                       *ImageSize,
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  );


struct _EFI_PEI_LOAD_FILE_PPI {
  EFI_PEI_LOAD_FILE LoadFile;
};


extern EFI_GUID gEfiLoadFile2PpiGuid;
#endif
