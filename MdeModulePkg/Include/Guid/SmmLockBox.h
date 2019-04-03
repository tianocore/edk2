/** @file
  SmmLockBox guid header file.

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_LOCK_BOX_GUID_H_
#define _SMM_LOCK_BOX_GUID_H_

#define EFI_SMM_LOCK_BOX_COMMUNICATION_GUID \
  {0x2a3cfebd, 0x27e8, 0x4d0a, {0x8b, 0x79, 0xd6, 0x88, 0xc2, 0xa3, 0xe1, 0xc0}}

//
// Below data structure is used for communication between PEI/DXE to SMM.
//

#define EFI_SMM_LOCK_BOX_COMMAND_SAVE                 0x1
#define EFI_SMM_LOCK_BOX_COMMAND_UPDATE               0x2
#define EFI_SMM_LOCK_BOX_COMMAND_RESTORE              0x3
#define EFI_SMM_LOCK_BOX_COMMAND_SET_ATTRIBUTES       0x4
#define EFI_SMM_LOCK_BOX_COMMAND_RESTORE_ALL_IN_PLACE 0x5

typedef struct {
  UINT32                         Command;
  UINT32                         DataLength;
  UINT64                         ReturnStatus;
} EFI_SMM_LOCK_BOX_PARAMETER_HEADER;

typedef struct {
  EFI_SMM_LOCK_BOX_PARAMETER_HEADER  Header;
  GUID                               Guid;
  PHYSICAL_ADDRESS                   Buffer;
  UINT64                             Length;
} EFI_SMM_LOCK_BOX_PARAMETER_SAVE;

typedef struct {
  EFI_SMM_LOCK_BOX_PARAMETER_HEADER  Header;
  GUID                               Guid;
  UINT64                             Offset;
  PHYSICAL_ADDRESS                   Buffer;
  UINT64                             Length;
} EFI_SMM_LOCK_BOX_PARAMETER_UPDATE;

typedef struct {
  EFI_SMM_LOCK_BOX_PARAMETER_HEADER  Header;
  GUID                               Guid;
  PHYSICAL_ADDRESS                   Buffer;
  UINT64                             Length;
} EFI_SMM_LOCK_BOX_PARAMETER_RESTORE;

typedef struct {
  EFI_SMM_LOCK_BOX_PARAMETER_HEADER  Header;
  GUID                               Guid;
  UINT64                             Attributes;
} EFI_SMM_LOCK_BOX_PARAMETER_SET_ATTRIBUTES;

typedef struct {
  EFI_SMM_LOCK_BOX_PARAMETER_HEADER  Header;
} EFI_SMM_LOCK_BOX_PARAMETER_RESTORE_ALL_IN_PLACE;

extern EFI_GUID gEfiSmmLockBoxCommunicationGuid;

#endif
