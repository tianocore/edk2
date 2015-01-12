/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



  @file
  PchInit.h

  @brief
  This file defines the PCH Init PPI

**/
#ifndef _PCH_INIT_H_
#define _PCH_INIT_H_

//
// Define the PCH Init PPI GUID
//


#include <Protocol/PchPlatformPolicy.h>
#define PCH_INIT_PPI_GUID \
  { \
    0x9ea894a, 0xbe0d, 0x4230, 0xa0, 0x3, 0xed, 0xc6, 0x93, 0xb4, 0x8e, 0x95 \
  }
extern EFI_GUID               gPchInitPpiGuid;

///
/// Forward reference for ANSI C compatibility
///
typedef struct _PCH_INIT_PPI  PCH_INIT_PPI;

///
/// Data structure definitions
///
typedef enum _CPU_STRAP_OPERATION {
  GetCpuStrapSetData,
  SetCpuStrapSetData,
  LockCpuStrapSetData
} CPU_STRAP_OPERATION;

typedef
EFI_STATUS
(EFIAPI *PCH_USB_INIT) (
  IN  EFI_PEI_SERVICES            **PeiServices
  )
/**

  @brief
  The function performing USB init in PEI phase. This could be used by USB recovery
  or debug features that need USB initialization during PEI phase.
  Note: Before executing this function, please be sure that PCH_INIT_PPI.Initialize
  has been done and PchUsbPolicyPpi has been installed.

  @param[in] PeiServices    General purpose services available to every PEIM

  @retval EFI_SUCCESS       The function completed successfully
  @retval Others            All other error conditions encountered result in an ASSERT.

**/
;

///
/// PCH_INIT_PPI Structure Definition
///
struct _PCH_INIT_PPI {
  PCH_USB_INIT          UsbInit;
};

#endif
