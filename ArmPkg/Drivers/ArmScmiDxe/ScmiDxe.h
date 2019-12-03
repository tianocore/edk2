/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/
#ifndef SCMI_DXE_H_
#define SCMI_DXE_H_

#include "ScmiPrivate.h"

#define MAX_VENDOR_LEN       SCMI_MAX_STR_LEN

/** Pointer to protocol initialization function.

  @param[in]  Handle  A pointer to the EFI_HANDLE on which the protocol
                      interface is to be installed.

  @retval EFI_SUCCESS  Protocol interface installed successfully.
**/
typedef
EFI_STATUS
(EFIAPI *SCMI_PROTOCOL_INIT_FXN)(
  IN  EFI_HANDLE  *Handle
  );

typedef struct {
  SCMI_PROTOCOL_ID Id;            // Protocol Id.
  SCMI_PROTOCOL_INIT_FXN InitFn;  // Protocol init function.
} SCMI_PROTOCOL_ENTRY;

#endif /* SCMI_DXE_H_ */
