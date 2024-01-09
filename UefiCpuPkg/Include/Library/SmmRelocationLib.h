/** @file
  Header file for SMM Relocation Library.

  @copyright
  INTEL CONFIDENTIAL
  Copyright (C) 2023 Intel Corporation.

  This software and the related documents are Intel copyrighted materials,
  and your use of them is governed by the express license under which they
  were provided to you ("License"). Unless the License provides otherwise,
  you may not use, modify, copy, publish, distribute, disclose or transmit
  this software or the related documents without Intel's prior written
  permission.

  This software and the related documents are provided as is, with no
  express or implied warranties, other than those that are expressly stated
  in the License.

@par Specification
**/

#ifndef _SMM_RELOCATION_LIB_H_
#define _SMM_RELOCATION_LIB_H_

#include <Ppi/MpServices2.h>

/**
  CPU SmmBase Relocation Init.

  This function is to relocate CPU SmmBase.

  @param[in] MpServices2        Pointer to this instance of the MpServices.

  @retval EFI_UNSUPPORTED       CPU SmmBase Relocation unsupported.
  @retval EFI_OUT_OF_RESOURCES  CPU SmmBase Relocation failed.
  @retval EFI_SUCCESS           CPU SmmBase Relocated successfully.

**/
EFI_STATUS
EFIAPI
SmmRelocationInit (
  IN EDKII_PEI_MP_SERVICES2_PPI  *MpServices2
  );

#endif
