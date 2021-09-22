/** @file
  This file defines the SMM S3 communication hob structure.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAYLOAD_S3_COMMUNICATION_GUID_H_
#define PAYLOAD_S3_COMMUNICATION_GUID_H_

extern EFI_GUID gS3CommunicationGuid;

#pragma pack(1)

typedef struct {
  EFI_SMRAM_DESCRIPTOR  CommBuffer;
  BOOLEAN               PldAcpiS3Enable;
} PLD_S3_COMMUNICATION;

///
/// The information below is used for communication between bootloader and payload.
/// It is used to save/store some registers in S3 path
///
/// This region exists only when gEfiAcpiVariableGuid HOB exist.
/// when PLD_S3_INFO.PldAcpiS3Enable is false, the communication buffer is defined as below.
///

typedef struct {
  UINT32             ApicId;
  UINT32             SmmBase;
} CPU_SMMBASE;

typedef struct {
  UINT8              SwSmiData;
  UINT8              SwSmiTriggerValue;
  UINT16             Reserved;
  UINT32             CpuCount;
  CPU_SMMBASE        SmmBase[0];
} SMM_S3_INFO;

//
// Payload would save this structure to S3 communication area in normal boot.
// In S3 path, bootloader need restore SMM base and writie IO port 0xB2 with SwSmiTriggerValue
// to trigger SMI to let payload to restore S3.
//
typedef struct {
  EFI_HOB_GUID_TYPE  Header;
  SMM_S3_INFO        S3Info;
} PLD_TO_BL_SMM_INFO;

#pragma pack()

#endif
