/** @file
  Arm SCMI Info Library.

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ARM_SCMI_INFO_LIB_H_
#define ARM_SCMI_INFO_LIB_H_

#include <ConfigurationManagerObject.h>

/** Populate a AML_CPC_INFO object based on SCMI information.

  @param[in]  DomainId    Identifier for the performance domain.
  @param[out] CpcInfo     If success, this structure was populated from
                          information queried to the SCP.

  @retval EFI_SUCCESS             Performance level got successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Out of resources.
  @retval !(EFI_SUCCESS)          Other errors.
**/
EFI_STATUS
EFIAPI
ArmScmiInfoGetFastChannel (
  IN  UINT32        DomainId,
  OUT AML_CPC_INFO  *CpcInfo
  );

#endif // ARM_SCMI_INFO_LIB_H_
