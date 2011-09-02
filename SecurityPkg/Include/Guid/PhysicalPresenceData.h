/** @file
  Define the variable data structures used for TCG physical presence.
  The TPM request from firmware or OS is saved to variable. And it is
  cleared after it is processed in the next boot cycle. The TPM response 
  is saved to variable.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  UINT8   Flags;
} EFI_PHYSICAL_PRESENCE;

//
// The definition bit of the flags
//
#define FLAG_NO_PPI_PROVISION                    BIT0
#define FLAG_NO_PPI_CLEAR                        BIT1
#define FLAG_NO_PPI_MAINTENANCE                  BIT2
#define FLAG_RESET_TRACK                         BIT3

#define H2NS(x)        ((((x) << 8) | ((x) >> 8)) & 0xffff)
#define H2NL(x)        (H2NS ((x) >> 16) | (H2NS ((x) & 0xffff) << 16))

//
// The definition of physical presence operation actions
//
#define NO_ACTION                               0
#define ENABLE                                  1
#define DISABLE                                 2
#define ACTIVATE                                3
#define DEACTIVATE                              4 
#define CLEAR                                   5
#define ENABLE_ACTIVATE                         6
#define DEACTIVATE_DISABLE                      7
#define SET_OWNER_INSTALL_TRUE                  8
#define SET_OWNER_INSTALL_FALSE                 9
#define ENABLE_ACTIVATE_OWNER_TRUE              10
#define DEACTIVATE_DISABLE_OWNER_FALSE          11
#define DEFERRED_PP_UNOWNERED_FIELD_UPGRADE     12
#define SET_OPERATOR_AUTH                       13
#define CLEAR_ENABLE_ACTIVATE                   14
#define SET_NO_PPI_PROVISION_FALSE              15
#define SET_NO_PPI_PROVISION_TRUE               16
#define SET_NO_PPI_CLEAR_FALSE                  17
#define SET_NO_PPI_CLEAR_TRUE                   18
#define SET_NO_PPI_MAINTENANCE_FALSE            19
#define SET_NO_PPI_MAINTENANCE_TRUE             20
#define ENABLE_ACTIVATE_CLEAR                   21
#define ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE   22

extern EFI_GUID  gEfiPhysicalPresenceGuid;

#endif

