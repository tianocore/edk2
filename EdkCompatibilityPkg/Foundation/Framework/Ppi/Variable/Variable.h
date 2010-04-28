/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Variable.h
    
Abstract:

  Read-only Variable Service PPI as defined in Tiano

--*/

#ifndef _PEI_READ_ONLY_VARIABLE_PPI_H
#define _PEI_READ_ONLY_VARIABLE_PPI_H

#include "EfiVariable.h"

#define PEI_READ_ONLY_VARIABLE_ACCESS_PPI_GUID \
  { \
    0x3cdc90c6, 0x13fb, 0x4a75, {0x9e, 0x79, 0x59, 0xe9, 0xdd, 0x78, 0xb9, 0xfa} \
  }


typedef
EFI_STATUS
(EFIAPI *PEI_GET_VARIABLE) (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  OUT UINT32                      *Attributes OPTIONAL,
  IN OUT UINTN                    *DataSize,
  OUT VOID                        *Data
  );

typedef
EFI_STATUS
(EFIAPI *PEI_GET_NEXT_VARIABLE_NAME) (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 * VendorGuid
  );

typedef struct PEI_READ_ONLY_VARIABLE_PPI {
  PEI_GET_VARIABLE            PeiGetVariable;
  PEI_GET_NEXT_VARIABLE_NAME  PeiGetNextVariableName;
} PEI_READ_ONLY_VARIABLE_PPI;

extern EFI_GUID gPeiReadOnlyVariablePpiGuid;

#endif
