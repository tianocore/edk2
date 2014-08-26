/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_MP_CORE_INFO_GUID_H_
#define __ARM_MP_CORE_INFO_GUID_H_

#define MAX_CPUS_PER_MPCORE_SYSTEM    0x04
#define SCU_CONFIG_REG_OFFSET         0x04
#define MPIDR_U_BIT_MASK              0x40000000

typedef struct {
  UINT32                ClusterId;
  UINT32                CoreId;

  // MP Core Mailbox
  EFI_PHYSICAL_ADDRESS  MailboxSetAddress;
  EFI_PHYSICAL_ADDRESS  MailboxGetAddress;
  EFI_PHYSICAL_ADDRESS  MailboxClearAddress;
  UINT64                MailboxClearValue;
} ARM_CORE_INFO;

typedef struct{
        UINT64   Signature;
        UINT32   Length;
        UINT32   Revision;
        UINT64   OemId;
        UINT64   OemTableId;
        UINTN    OemRevision;
        UINTN    CreatorId;
        UINTN    CreatorRevision;
        EFI_GUID Identifier;
        UINTN    DataLen;
} ARM_PROCESSOR_TABLE_HEADER;

typedef struct {
        ARM_PROCESSOR_TABLE_HEADER   Header;
        UINTN                        NumberOfEntries;
        ARM_CORE_INFO                *ArmCpus;
} ARM_PROCESSOR_TABLE;


#define ARM_MP_CORE_INFO_GUID \
  { 0xa4ee0728, 0xe5d7, 0x4ac5,  {0xb2, 0x1e, 0x65, 0x8e, 0xd8, 0x57, 0xe8, 0x34} }

#define EFI_ARM_PROCESSOR_TABLE_SIGNATURE        SIGNATURE_64 ('C', 'P', 'U', 'T', 'A', 'B', 'L', 'E')
#define EFI_ARM_PROCESSOR_TABLE_REVISION         0x00010000 //1.0
#define EFI_ARM_PROCESSOR_TABLE_OEM_ID           SIGNATURE_64('A','R','M',' ', 'L', 't', 'd', ' ')
#define EFI_ARM_PROCESSOR_TABLE_OEM_TABLE_ID     SIGNATURE_64('V', 'E', 'R', 'S', 'A', 'T', 'I', 'L')
#define EFI_ARM_PROCESSOR_TABLE_OEM_REVISION     0x00000001
#define EFI_ARM_PROCESSOR_TABLE_CREATOR_ID       0xA5A5A5A5
#define EFI_ARM_PROCESSOR_TABLE_CREATOR_REVISION 0x01000001

extern EFI_GUID gArmMpCoreInfoGuid;

#endif /* MPCOREINFO_H_ */
