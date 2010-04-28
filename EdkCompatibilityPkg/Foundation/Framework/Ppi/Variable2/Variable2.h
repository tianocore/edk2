/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Variable2.h
    
Abstract:

  Read-only Variable2 Service PPI as defined in PI1.0

--*/

#ifndef _PEI_READ_ONLY_VARIABLE2_PPI_H
#define _PEI_READ_ONLY_VARIABLE2_PPI_H

#include "EfiVariable.h"

#define EFI_PEI_READ_ONLY_VARIABLE2_PPI_GUID \
  { \
    0x2ab86ef5, 0xecb5, 0x4134, {0xb5, 0x56, 0x38, 0x54, 0xca, 0x1f, 0xe1, 0xb4} \
  }

EFI_FORWARD_DECLARATION (EFI_PEI_READ_ONLY_VARIABLE2_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_VARIABLE2) (
  IN CONST EFI_PEI_READ_ONLY_VARIABLE2_PPI    *This,
  IN CONST CHAR16                             *VariableName,
  IN CONST EFI_GUID                           *VariableGuid,
  OUT UINT32                                  *Attributes,
  IN OUT UINTN                                *DataSize,
  OUT VOID                                    *Data
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_GET_NEXT_VARIABLE_NAME2) (
  IN CONST EFI_PEI_READ_ONLY_VARIABLE2_PPI    *This,
  IN OUT UINTN                                *VariableNameSize,
  IN OUT CHAR16                               *VariableName,
  IN OUT EFI_GUID                             *VariableGuid
  );

struct _EFI_PEI_READ_ONLY_VARIABLE2_PPI {
  EFI_PEI_GET_VARIABLE2            GetVariable;
  EFI_PEI_GET_NEXT_VARIABLE_NAME2  GetNextVariableName;
};

extern EFI_GUID gPeiReadOnlyVariable2PpiGuid;

#endif
