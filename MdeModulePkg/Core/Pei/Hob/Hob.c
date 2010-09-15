/** @file
  This module provide Hand-Off Block manupulation.
  
Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PeiMain.h"

/**

  Gets the pointer to the HOB List.

  @param PeiServices                   An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param HobList                       Pointer to the HOB List.

  @retval EFI_SUCCESS                  Get the pointer of HOB List
  @retval EFI_NOT_AVAILABLE_YET        the HOB List is not yet published
  @retval EFI_INVALID_PARAMETER        HobList is NULL (in debug mode)

**/
EFI_STATUS
EFIAPI
PeiGetHobList (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID          **HobList
  )
{
  PEI_CORE_INSTANCE *PrivateData;
  
  //
  // Only check this parameter in debug mode
  //
  
  DEBUG_CODE_BEGIN ();  
    if (HobList == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  DEBUG_CODE_END ();
  
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  *HobList    = PrivateData->HobList.Raw;

  return EFI_SUCCESS;   
}


/**
  Add a new HOB to the HOB List.

  @param PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Type             Type of the new HOB.
  @param Length           Length of the new HOB to allocate.
  @param Hob              Pointer to the new HOB.

  @return  EFI_SUCCESS           Success to create hob.
  @retval  EFI_INVALID_PARAMETER if Hob is NULL
  @retval  EFI_NOT_AVAILABLE_YET if HobList is still not available.
  @retval  EFI_OUT_OF_RESOURCES  if there is no more memory to grow the Hoblist.

**/
EFI_STATUS
EFIAPI
PeiCreateHob (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN UINT16            Type,
  IN UINT16            Length,
  IN OUT VOID          **Hob
  )
{
  EFI_STATUS                           Status;
  EFI_HOB_HANDOFF_INFO_TABLE           *HandOffHob;
  EFI_HOB_GENERIC_HEADER               *HobEnd;
  EFI_PHYSICAL_ADDRESS                 FreeMemory;


  Status = PeiGetHobList (PeiServices, Hob);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  HandOffHob = *Hob;

  Length     = (UINT16)((Length + 0x7) & (~0x7));

  FreeMemory = HandOffHob->EfiFreeMemoryTop -
               HandOffHob->EfiFreeMemoryBottom;

  if (FreeMemory < Length) {
    DEBUG ((EFI_D_ERROR, "PeiCreateHob fail: Length - 0x%08x\n", (UINTN)Length));
    DEBUG ((EFI_D_ERROR, "  FreeMemoryTop    - 0x%08x\n", (UINTN)HandOffHob->EfiFreeMemoryTop));
    DEBUG ((EFI_D_ERROR, "  FreeMemoryBottom - 0x%08x\n", (UINTN)HandOffHob->EfiFreeMemoryBottom));
    return EFI_OUT_OF_RESOURCES;
  }
  
  *Hob = (VOID*) (UINTN) HandOffHob->EfiEndOfHobList;
  ((EFI_HOB_GENERIC_HEADER*) *Hob)->HobType   = Type;
  ((EFI_HOB_GENERIC_HEADER*) *Hob)->HobLength = Length;
  ((EFI_HOB_GENERIC_HEADER*) *Hob)->Reserved  = 0;

  HobEnd = (EFI_HOB_GENERIC_HEADER*) ((UINTN) *Hob + Length);
  HandOffHob->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS) (UINTN) HobEnd;
 
  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = (UINT16) sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;
  HobEnd++;
  HandOffHob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) HobEnd;

  return EFI_SUCCESS;   
}

/**

  Builds a Handoff Information Table HOB

  @param BootMode        - Current Bootmode
  @param MemoryBegin     - Start Memory Address.
  @param MemoryLength    - Length of Memory.

  @return EFI_SUCCESS Always success to initialize HOB.

**/
EFI_STATUS
PeiCoreBuildHobHandoffInfoTable (
  IN EFI_BOOT_MODE         BootMode,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE   *Hob;
  EFI_HOB_GENERIC_HEADER       *HobEnd;

  Hob                      = (VOID *)(UINTN)MemoryBegin;
  HobEnd                   = (EFI_HOB_GENERIC_HEADER*) (Hob+1);
  Hob->Header.HobType      = EFI_HOB_TYPE_HANDOFF;
  Hob->Header.HobLength    = (UINT16) sizeof (EFI_HOB_HANDOFF_INFO_TABLE);
  Hob->Header.Reserved     = 0;
  
  HobEnd->HobType          = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength        = (UINT16) sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved         = 0;

  Hob->Version             = EFI_HOB_HANDOFF_TABLE_VERSION;
  Hob->BootMode            = BootMode;
  
  Hob->EfiMemoryTop        = MemoryBegin + MemoryLength;
  Hob->EfiMemoryBottom     = MemoryBegin;
  Hob->EfiFreeMemoryTop    = MemoryBegin + MemoryLength;
  Hob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS) (UINTN) (HobEnd + 1);
  Hob->EfiEndOfHobList     = (EFI_PHYSICAL_ADDRESS) (UINTN) HobEnd;

  return EFI_SUCCESS;
}
