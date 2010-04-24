/** @file
  If the Variable services have PcdVariableCollectStatistics set to TRUE then 
  the EFI system table will contain statistical information about variable usage
  an this utility will print out the information. You can use console redirection
  to capture the data.
  
  Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Guid/VariableFormat.h>


/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this 
  function.


  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  VARIABLE_INFO_ENTRY   *VariableInfo;
  VARIABLE_INFO_ENTRY   *Entry;

  Status = EfiGetSystemConfigurationTable (&gEfiVariableGuid, (VOID **)&Entry);
  if (!EFI_ERROR (Status) && (Entry != NULL)) {
    Print (L"Non-Volatile EFI Variables:\n");
    VariableInfo = Entry;
    do {
      if (!VariableInfo->Volatile) {
        Print (
          L"%g R%03d(%03d) W%03d D%03d:%s\n", 
          &VariableInfo->VendorGuid,  
          VariableInfo->ReadCount,
          VariableInfo->CacheCount,
          VariableInfo->WriteCount,
          VariableInfo->DeleteCount,
          VariableInfo->Name
          );
      }

      VariableInfo = VariableInfo->Next;
    } while (VariableInfo != NULL);

    Print (L"Volatile EFI Variables:\n");
    VariableInfo = Entry;
    do {
      if (VariableInfo->Volatile) {
        Print (
          L"%g R%03d(%03d) W%03d D%03d:%s\n", 
          &VariableInfo->VendorGuid,  
          VariableInfo->ReadCount,
          VariableInfo->CacheCount,
          VariableInfo->WriteCount,
          VariableInfo->DeleteCount,
          VariableInfo->Name
          );
      }
      VariableInfo = VariableInfo->Next;
    } while (VariableInfo != NULL);

  } else {
    Print (L"Warning: Variable Dxe driver doesn't enable the feature of statistical information!\n");
    Print (L"If you want to see this info, please:\n");
    Print (L"  1. Set PcdVariableCollectStatistics as TRUE\n");
    Print (L"  2. Rebuild Variable Dxe driver\n");
    Print (L"  3. Run \"VariableInfo\" cmd again\n");

  }

  return Status;
}
