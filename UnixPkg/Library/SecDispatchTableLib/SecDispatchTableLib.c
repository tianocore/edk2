/** @file
  Allows an override of the SEC SEC PPI Dispatch Table. This allows 
  customized PPIs to be passed into the PEI Core.

Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

--*/

#include <PiPei.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SecDispatchTableLib.h>


/**
  Return the number of bytes that OverrideDispatchTable() will append to
  the dispatch table.

  @return  Size of table in bytes OverrideDispatchTable() will return

**/
UINTN
EFIAPI
OverrideDispatchTableExtraSize (
  )
{
  return 0;
}


/**
  Allow an override of the Sec PPI Dispatch Table. This table contains PPIs passed
  up from SEC to PEI. 

  @param  OriginalTable         SECs default PPI dispatch table
  @param  OriginalTableSize     Size of SECs default PPI dispatch table
  @param  NewTable              New dispatch table
  @param  NewTableSize          Size of of the NewTable in bytes

  @return EFI_SUCCESS table was copied

**/
EFI_STATUS
EFIAPI
OverrideDispatchTable (
  IN CONST EFI_PEI_PPI_DESCRIPTOR          *OriginalTable,
  IN       UINTN                           OriginalTableSize,
  IN OUT   EFI_PEI_PPI_DESCRIPTOR          *NewTable,
  IN       UINTN                           NewTableSize
  )
{
  CopyMem (NewTable, OriginalTable, OriginalTableSize);

  return EFI_SUCCESS;
}
