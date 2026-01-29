/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_MP_CORE_INFO_GUID_H_
#define ARM_MP_CORE_INFO_GUID_H_

#define MAX_CPUS_PER_MPCORE_SYSTEM  0x04
#define SCU_CONFIG_REG_OFFSET       0x04
#define MPIDR_U_BIT_MASK            0x40000000

typedef struct {
  UINT64                  Mpidr;

  // MP Core Mailbox
  EFI_PHYSICAL_ADDRESS    MailboxSetAddress;
  EFI_PHYSICAL_ADDRESS    MailboxGetAddress;
  EFI_PHYSICAL_ADDRESS    MailboxClearAddress;
  UINT64                  MailboxClearValue;
} ARM_CORE_INFO;

#define ARM_MP_CORE_INFO_GUID \
  { 0xa4ee0728, 0xe5d7, 0x4ac5,  {0xb2, 0x1e, 0x65, 0x8e, 0xd8, 0x57, 0xe8, 0x34} }

extern EFI_GUID  gArmMpCoreInfoGuid;

#endif /* ARM_MP_CORE_INFO_GUID_H_ */
