/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Decompress.h

Abstract:

  PI 1.0 spec definition.

--*/

#ifndef __DECOMPRESS_PPI_H__
#define __DECOMPRESS_PPI_H__


#define EFI_PEI_DECOMPRESS_PPI_GUID \
  { 0x1a36e4e7, 0xfab6, 0x476a, {0x8e, 0x75, 0x69, 0x5a, 0x5, 0x76, 0xfd, 0xd7}}

EFI_FORWARD_DECLARATION (EFI_PEI_DECOMPRESS_PPI);


typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_DECOMPRESS_DECOMPRESS)(
  IN  CONST EFI_PEI_DECOMPRESS_PPI           *This,
  IN  CONST EFI_COMPRESSION_SECTION          *InputSection,
  OUT VOID                                   **OutputBuffer,
  OUT UINTN                                  *OutputSize
  );

struct _EFI_PEI_DECOMPRESS_PPI {
  EFI_PEI_DECOMPRESS_DECOMPRESS   Decompress;
};



extern EFI_GUID gEfiPeiDecompressPpiGuid;

#endif
