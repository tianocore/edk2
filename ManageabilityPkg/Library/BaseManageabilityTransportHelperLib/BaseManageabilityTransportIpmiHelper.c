/** @file
  Manageability IPMI Helper Library

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/ManageabilityTransportIpmiLib.h>

#include <IndustryStandard/Ipmi.h>

//
// BaseManageabilityTransportHelper is used by PEI, DXE and SMM.
// Make sure the global variables added here should be unchangeable.
//
MANAGEABILITY_IPMI_COMPLETTION_CODE_MAPPING  IpmiCompletionCodeMapping[] = {
  { IPMI_COMP_CODE_NORMAL,          L"IPMI Completion Code - Normal",          MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_NO_ERRORS       },
  { IPMI_COMP_CODE_NODE_BUSY,       L"IPMI Completion Code - Busy",            MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_BUSY_IN_READ    },
  { IPMI_COMP_CODE_INVALID_COMMAND, L"IPMI Completion Code - Invalid command", MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS_INVALID_COMMAND }
};

UINT8  IpmiCompletionCodeMappingEntries = sizeof (IpmiCompletionCodeMapping) / sizeof (MANAGEABILITY_IPMI_COMPLETTION_CODE_MAPPING);

///
/// IPMI Helper Functions.
///

/**
  This function returns a human readable string of IPMI KCS Completion Code
  and returns the corresponding additional status of transport interface.

  @param [in]  CompletionCode     The Completion Code returned from KCS.
  @param [out] CompletionCodeStr  Human readable string of IPMI Completion Code.
  @param [out] AdditionalStatus   Return the addtional status.

  @retval  EFI_SUCCESS            The information of Completion Code is returned.
  @retval  EFI_NOT_FOUND          No information of Completion Code is returned.
  @retval  EFI_INVALID_PARAMETER  The given parameter is incorrect.

**/
EFI_STATUS
IpmiHelperCheckCompletionCode (
  IN   UINT8                                      CompletionCode,
  OUT  CHAR16                                     **CompletionCodeStr,
  OUT  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS  *AdditionalStatus
  )
{
  UINT8                                        Index;
  MANAGEABILITY_IPMI_COMPLETTION_CODE_MAPPING  *ThisCcMapping;

  if ((CompletionCodeStr == NULL) || (AdditionalStatus == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *AdditionalStatus = 0;
  ThisCcMapping     = IpmiCompletionCodeMapping;
  for (Index = 0; Index < IpmiCompletionCodeMappingEntries; Index++) {
    if (ThisCcMapping->CompletionCode == CompletionCode) {
      *CompletionCodeStr = ThisCcMapping->CompletionCodeString;
      *AdditionalStatus  = ThisCcMapping->AdditionalStatus;
      return EFI_SUCCESS;
    }

    ThisCcMapping++;
  }

  return EFI_NOT_FOUND;
}
