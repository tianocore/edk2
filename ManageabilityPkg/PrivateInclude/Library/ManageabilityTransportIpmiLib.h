/** @file

  This file defines the manageability IPMI protocol specific transport data.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <IndustryStandard/IpmiKcs.h>
#include <Library/ManageabilityTransportLib.h>
#include <Library/PcdLib.h>

///
/// The IPMI command header which is apart from
/// the payload.
///
typedef struct {
  UINT8    Lun   : 2;
  UINT8    NetFn : 6;
  UINT8    Command;
} MANAGEABILITY_IPMI_TRANSPORT_HEADER;

///
/// Unless otherwise specified, commands that are listed as mandatory must be accessed
/// via LUN 00b.
///
#define MANAGEABILITY_IPMI_BMC_LUN  0

#define MANAGEABILITY_IPMI_NET_FUNC_MAX  0x3F

///
/// IPMI KCS register address configuration.
///
#define MANAGEABILITY_IPMI_KCS_ADDRESS_NOT_SPECIFIED  MAX_UINT16

#define MANAGEABILITY_IPMI_KCS_USE_LEGACY_ADDRESSES  \
  ((FixedPcdGet16 (PcdIpmiKcsIoDataAddress) == MANAGEABILITY_IPMI_KCS_ADDRESS_NOT_SPECIFIED) && \
   (FixedPcdGet16 (PcdIpmiKcsIoCommandAddress) == MANAGEABILITY_IPMI_KCS_ADDRESS_NOT_SPECIFIED))

#define MANAGEABILITY_IPMI_KCS_PCD_CONFIGURATION_VALID  \
  (((FixedPcdGet16 (PcdIpmiKcsIoDataAddress) == MANAGEABILITY_IPMI_KCS_ADDRESS_NOT_SPECIFIED) == \
    (FixedPcdGet16 (PcdIpmiKcsIoCommandAddress) == MANAGEABILITY_IPMI_KCS_ADDRESS_NOT_SPECIFIED)) && \
   (MANAGEABILITY_IPMI_KCS_USE_LEGACY_ADDRESSES || \
    (FixedPcdGet16 (PcdIpmiKcsIoDataAddress) != FixedPcdGet16 (PcdIpmiKcsIoCommandAddress))))

#define MANAGEABILITY_IPMI_KCS_DATA_ADDRESS  \
  (MANAGEABILITY_IPMI_KCS_USE_LEGACY_ADDRESSES ? \
   (FixedPcdGet16 (PcdIpmiKcsIoBaseAddress) + IPMI_KCS_DATA_IN_REGISTER_OFFSET) : \
   FixedPcdGet16 (PcdIpmiKcsIoDataAddress))

#define MANAGEABILITY_IPMI_KCS_COMMAND_ADDRESS  \
  (MANAGEABILITY_IPMI_KCS_USE_LEGACY_ADDRESSES ? \
   (FixedPcdGet16 (PcdIpmiKcsIoBaseAddress) + IPMI_KCS_COMMAND_REGISTER_OFFSET) : \
   FixedPcdGet16 (PcdIpmiKcsIoCommandAddress))

///
/// The IPMI Completion Code mapping.
///
typedef struct {
  UINT8                                        CompletionCode;
  CHAR16                                       *CompletionCodeString;
  MANAGEABILITY_TRANSPORT_ADDITIONAL_STATUS    AdditionalStatus;
} MANAGEABILITY_IPMI_COMPLETTION_CODE_MAPPING;
