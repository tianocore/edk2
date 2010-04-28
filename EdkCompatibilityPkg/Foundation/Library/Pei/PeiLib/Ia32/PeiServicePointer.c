/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeiServicePointer.c

Abstract:

--*/

#include "Tiano.h"
#include "PeiApi.h"
#include "PeiLib.h"

#if (PI_SPECIFICATION_VERSION >= 0x00010000)

#ifdef EFI_NT_EMULATOR
EFI_PEI_SERVICES  **gPeiServices;
#endif


VOID
SetPeiServicesTablePointer (
  IN EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Save PeiService pointer so that it can be retrieved anywhere.

Arguments:

  PeiServices     - The direct pointer to PeiServiceTable.
  PhyscialAddress - The physcial address of variable PeiServices.
  
Returns:
  NONE
  
--*/          
{

#ifdef EFI_NT_EMULATOR

  //
  // For NT32, set EFI_PEI_SERVICES** to global variable.
  //
  gPeiServices = PeiServices;
#else

  //
  // For X86 processor,the EFI_PEI_SERVICES** is stored in the 
  // 4 bytes immediately preceding the Interrupt Descriptor Table.
  //
  UINTN IdtBaseAddress;
  IdtBaseAddress = (UINTN)ReadIdtBase();
  *(UINTN*)(IdtBaseAddress - 4) = (UINTN)PeiServices;

#endif  
}


EFI_PEI_SERVICES **
GetPeiServicesTablePointer ( 
  VOID
  )
/*++

Routine Description:

  Get PeiService pointer.

Arguments:

  NONE.
  
Returns:
  The direct pointer to PeiServiceTable.
  
--*/          
{
  EFI_PEI_SERVICES  **PeiServices;

#ifdef EFI_NT_EMULATOR

  //
  // For NT32, set EFI_PEI_SERVICES** to global variable.
  //
  PeiServices = gPeiServices;
#else

  //
  // For X86 processor,the EFI_PEI_SERVICES** is stored in the 
  // 4 bytes immediately preceding the Interrupt Descriptor Table.
  //
  UINTN IdtBaseAddress;
  IdtBaseAddress = (UINTN)ReadIdtBase();
  PeiServices = (EFI_PEI_SERVICES **)(UINTN)(*(UINTN*)(IdtBaseAddress - 4));
#endif
  return PeiServices;
}


VOID
MigrateIdtTable (
  IN EFI_PEI_SERVICES  **PeiServices
  )
/*++

Routine Description:

  Migrate IDT from temporary memory to real memory where preceded with 4 bytes for
  storing PeiService pointer.

Arguments:

  PeiServices   - The direct pointer to PeiServiceTable.
  
Returns:

  NONE.
  
--*/            
{
#ifndef EFI_NT_EMULATOR
  UINT16          IdtEntrySize;
  UINTN           OldIdtBase;
  UINTN           Size;
  VOID            *NewIdtBase;
  EFI_STATUS      Status;
  
  IdtEntrySize    = ReadIdtLimit();
  OldIdtBase      = ReadIdtBase();
  Size = sizeof(PEI_IDT_TABLE) + (IdtEntrySize + 1);  
  Status = (*PeiServices)->AllocatePool (PeiServices, Size, &NewIdtBase);
  ASSERT_PEI_ERROR (PeiServices, Status);
  (*PeiServices)->CopyMem ((VOID*)((UINTN)NewIdtBase + sizeof(PEI_IDT_TABLE)), (VOID*)OldIdtBase, (IdtEntrySize + 1));
  SetIdtBase(((UINTN)NewIdtBase + sizeof(PEI_IDT_TABLE)), IdtEntrySize);
  SetPeiServicesTablePointer(PeiServices);
#endif
}

#endif
