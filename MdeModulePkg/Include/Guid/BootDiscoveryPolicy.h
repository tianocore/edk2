/** @file
  Definition for structure & defines exported by Boot Discovery Policy UI

  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BOOT_DISCOVERY_POLICY_UI_LIB_H_
#define BOOT_DISCOVERY_POLICY_UI_LIB_H_

#define BDP_CONNECT_MINIMAL 0  /* Do not connect any additional devices */
#define BDP_CONNECT_NET     1
#define BDP_CONNECT_ALL     2

#define BOOT_DISCOVERY_POLICY_MGR_FORMSET_GUID  { 0x5b6f7107, 0xbb3c, 0x4660, { 0x92, 0xcd, 0x54, 0x26, 0x90, 0x28, 0x0b, 0xbd } }

#define BOOT_DISCOVERY_POLICY_VAR L"BootDiscoveryPolicy"

#endif
