/** @file

  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/SynchronizationLib.h>
#include <Uefi/UefiBaseType.h>
#include <Library/TdxLib.h>
#include <IndustryStandard/IntelTdx.h>
#include <IndustryStandard/Tdx.h>
#include <Library/TdxMailboxLib.h>

volatile VOID  *mMailBox  = NULL;
UINT32         mNumOfCpus = 0;

/**
  This function will be called by BSP to get the CPU number.

  @retval   CPU number
**/
UINT32
EFIAPI
GetCpusNum (
  VOID
  )
{
  if (mNumOfCpus == 0) {
    mNumOfCpus = TdVCpuNum ();
  }

  return mNumOfCpus;
}

/**
  Get the address of Td mailbox.
**/
volatile VOID *
EFIAPI
GetTdxMailBox (
  VOID
  )
{
  if (mMailBox == NULL) {
    mMailBox = (VOID *)(UINTN)PcdGet32 (PcdOvmfSecGhcbBackupBase);
  }

  return mMailBox;
}

/**
  This function will be called by BSP to wakeup APs the are spinning on mailbox
  in protected mode

  @param[in] Command          Command to send APs
  @param[in] WakeupVector     If used, address for APs to start executing
  @param[in] WakeArgsX        Args to pass to APs for excuting commands
**/
VOID
EFIAPI
MpSendWakeupCommand (
  IN UINT16  Command,
  IN UINT64  WakeupVector,
  IN UINT64  WakeupArgs1,
  IN UINT64  WakeupArgs2,
  IN UINT64  WakeupArgs3,
  IN UINT64  WakeupArgs4
  )
{
  volatile MP_WAKEUP_MAILBOX  *MailBox;

  MailBox               = (volatile MP_WAKEUP_MAILBOX *)GetTdxMailBox ();
  MailBox->ApicId       = MP_CPU_PROTECTED_MODE_MAILBOX_APICID_INVALID;
  MailBox->WakeUpVector = 0;
  MailBox->Command      = MpProtectedModeWakeupCommandNoop;
  MailBox->ApicId       = MP_CPU_PROTECTED_MODE_MAILBOX_APICID_BROADCAST;
  MailBox->WakeUpVector = WakeupVector;
  MailBox->WakeUpArgs1  = WakeupArgs1;
  MailBox->WakeUpArgs2  = WakeupArgs2;
  MailBox->WakeUpArgs3  = WakeupArgs3;
  MailBox->WakeUpArgs4  = WakeupArgs4;
  AsmCpuid (0x01, NULL, NULL, NULL, NULL);
  MailBox->Command = Command;
  AsmCpuid (0x01, NULL, NULL, NULL, NULL);
  return;
}

/**
  BSP wait until all the APs arriving. It means the task triggered by BSP is started.
**/
VOID
EFIAPI
MpSerializeStart (
  VOID
  )
{
  volatile MP_WAKEUP_MAILBOX  *MailBox;
  UINT32                      NumOfCpus;

  NumOfCpus = GetCpusNum ();
  MailBox   = (volatile MP_WAKEUP_MAILBOX *)GetTdxMailBox ();

  DEBUG ((DEBUG_VERBOSE, "Waiting for APs to arriving. NumOfCpus=%d, MailBox=%p\n", NumOfCpus, MailBox));
  while (MailBox->NumCpusArriving != (NumOfCpus -1)) {
    CpuPause ();
  }

  DEBUG ((DEBUG_VERBOSE, "Releasing APs\n"));
  MailBox->NumCpusExiting = NumOfCpus;
  InterlockedIncrement ((UINT32 *)&MailBox->NumCpusArriving);
}

/**
  BSP wait until all the APs arriving. It means the task triggered by BSP is ended.
**/
VOID
EFIAPI
MpSerializeEnd (
  VOID
  )
{
  volatile MP_WAKEUP_MAILBOX  *MailBox;

  MailBox = (volatile MP_WAKEUP_MAILBOX *)GetTdxMailBox ();
  DEBUG ((DEBUG_VERBOSE, "Waiting for APs to finish\n"));
  while (MailBox->NumCpusExiting != 1 ) {
    CpuPause ();
  }

  DEBUG ((DEBUG_VERBOSE, "Restarting APs\n"));
  MailBox->Command         = MpProtectedModeWakeupCommandNoop;
  MailBox->NumCpusArriving = 0;
  InterlockedDecrement ((UINT32 *)&MailBox->NumCpusExiting);
}
