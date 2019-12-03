/** @file
  Define the variable data structures used for TCG physical presence.
  The TPM request from firmware or OS is saved to variable. And it is
  cleared after it is processed in the next boot cycle. The TPM response
  is saved to variable.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PHYSICAL_PRESENCE_DATA_GUID_H__
#define __PHYSICAL_PRESENCE_DATA_GUID_H__

#define EFI_PHYSICAL_PRESENCE_DATA_GUID \
  { \
    0xf6499b1, 0xe9ad, 0x493d, { 0xb9, 0xc2, 0x2f, 0x90, 0x81, 0x5c, 0x6c, 0xbc }\
  }

#define PHYSICAL_PRESENCE_VARIABLE  L"PhysicalPresence"

typedef struct {
  UINT8   PPRequest;      ///< Physical Presence request command.
  UINT8   LastPPRequest;
  UINT32  PPResponse;
} EFI_PHYSICAL_PRESENCE;

//
// The definition of physical presence operation actions
//
#define PHYSICAL_PRESENCE_NO_ACTION                               0
#define PHYSICAL_PRESENCE_ENABLE                                  1
#define PHYSICAL_PRESENCE_DISABLE                                 2
#define PHYSICAL_PRESENCE_ACTIVATE                                3
#define PHYSICAL_PRESENCE_DEACTIVATE                              4
#define PHYSICAL_PRESENCE_CLEAR                                   5
#define PHYSICAL_PRESENCE_ENABLE_ACTIVATE                         6
#define PHYSICAL_PRESENCE_DEACTIVATE_DISABLE                      7
#define PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE                  8
#define PHYSICAL_PRESENCE_SET_OWNER_INSTALL_FALSE                 9
#define PHYSICAL_PRESENCE_ENABLE_ACTIVATE_OWNER_TRUE              10
#define PHYSICAL_PRESENCE_DEACTIVATE_DISABLE_OWNER_FALSE          11
#define PHYSICAL_PRESENCE_DEFERRED_PP_UNOWNERED_FIELD_UPGRADE     12
#define PHYSICAL_PRESENCE_SET_OPERATOR_AUTH                       13
#define PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE                   14
#define PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_FALSE              15
#define PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_TRUE               16
#define PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_FALSE                  17
#define PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE                   18
#define PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_FALSE            19
#define PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_TRUE             20
#define PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR                   21
#define PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE   22

//
// This variable is used to save TPM Management Flags and corresponding operations.
// It should be protected from malicious software (e.g. Set it as read-only variable).
//
#define PHYSICAL_PRESENCE_FLAGS_VARIABLE  L"PhysicalPresenceFlags"
typedef struct {
  UINT8   PPFlags;
} EFI_PHYSICAL_PRESENCE_FLAGS;

//
// The definition bit of the TPM Management Flags
//
#define FLAG_NO_PPI_PROVISION                    BIT0
#define FLAG_NO_PPI_CLEAR                        BIT1
#define FLAG_NO_PPI_MAINTENANCE                  BIT2
#define FLAG_RESET_TRACK                         BIT3

extern EFI_GUID  gEfiPhysicalPresenceGuid;

#endif

