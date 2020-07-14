/**@file
  Negotiate SMI features with QEMU, and configure UefiCpuPkg/PiSmmCpuDxeSmm
  accordingly.

  Copyright (C) 2016-2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>

#include "SmiFeatures.h"

//
// The following bit value stands for "broadcast SMI" in the
// "etc/smi/supported-features" and "etc/smi/requested-features" fw_cfg files.
//
#define ICH9_LPC_SMI_F_BROADCAST BIT0
//
// The following bit value stands for "enable CPU hotplug, and inject an SMI
// with control value ICH9_APM_CNT_CPU_HOTPLUG upon hotplug", in the
// "etc/smi/supported-features" and "etc/smi/requested-features" fw_cfg files.
//
#define ICH9_LPC_SMI_F_CPU_HOTPLUG BIT1

//
// Provides a scratch buffer (allocated in EfiReservedMemoryType type memory)
// for the S3 boot script fragment to write to and read from.
//
#pragma pack (1)
typedef union {
  UINT64 Features;
  UINT8  FeaturesOk;
} SCRATCH_BUFFER;
#pragma pack ()

//
// These carry the selector keys of the "etc/smi/requested-features" and
// "etc/smi/features-ok" fw_cfg files from NegotiateSmiFeatures() to
// AppendFwCfgBootScript().
//
STATIC FIRMWARE_CONFIG_ITEM mRequestedFeaturesItem;
STATIC FIRMWARE_CONFIG_ITEM mFeaturesOkItem;

//
// Carries the negotiated SMI features from NegotiateSmiFeatures() to
// AppendFwCfgBootScript().
//
STATIC UINT64 mSmiFeatures;

/**
  Negotiate SMI features with QEMU.

  @retval FALSE  If SMI feature negotiation is not supported by QEMU. This is
                 not an error, it just means that SaveSmiFeatures() should not
                 be called.

  @retval TRUE   SMI feature negotiation is supported, and it has completed
                 successfully as well. (Failure to negotiate is a fatal error
                 and the function never returns in that case.)
**/
BOOLEAN
NegotiateSmiFeatures (
  VOID
  )
{
  FIRMWARE_CONFIG_ITEM SupportedFeaturesItem;
  UINTN                SupportedFeaturesSize;
  UINTN                RequestedFeaturesSize;
  UINTN                FeaturesOkSize;
  UINT64               RequestedFeaturesMask;

  //
  // Look up the fw_cfg files used for feature negotiation. The selector keys
  // of "etc/smi/requested-features" and "etc/smi/features-ok" are saved
  // statically. If the files are missing, then QEMU doesn't support SMI
  // feature negotiation.
  //
  if (RETURN_ERROR (QemuFwCfgFindFile ("etc/smi/supported-features",
                      &SupportedFeaturesItem, &SupportedFeaturesSize)) ||
      RETURN_ERROR (QemuFwCfgFindFile ("etc/smi/requested-features",
                      &mRequestedFeaturesItem, &RequestedFeaturesSize)) ||
      RETURN_ERROR (QemuFwCfgFindFile ("etc/smi/features-ok",
                      &mFeaturesOkItem, &FeaturesOkSize))) {
    DEBUG ((DEBUG_INFO, "%a: SMI feature negotiation unavailable\n",
      __FUNCTION__));
    return FALSE;
  }

  //
  // If the files are present but their sizes disagree with us, that's a fatal
  // error (we can't trust the behavior of SMIs either way).
  //
  if (SupportedFeaturesSize != sizeof mSmiFeatures ||
      RequestedFeaturesSize != sizeof mSmiFeatures ||
      FeaturesOkSize != sizeof (UINT8)) {
    DEBUG ((DEBUG_ERROR, "%a: size mismatch in feature negotiation\n",
      __FUNCTION__));
    goto FatalError;
  }

  //
  // Get the features supported by the host.
  //
  QemuFwCfgSelectItem (SupportedFeaturesItem);
  QemuFwCfgReadBytes (sizeof mSmiFeatures, &mSmiFeatures);

  //
  // We want broadcast SMI, SMI on CPU hotplug, and nothing else.
  //
  RequestedFeaturesMask = ICH9_LPC_SMI_F_BROADCAST;
  if (!MemEncryptSevIsEnabled ()) {
    //
    // For now, we only support hotplug with SEV disabled.
    //
    RequestedFeaturesMask |= ICH9_LPC_SMI_F_CPU_HOTPLUG;
  }
  mSmiFeatures &= RequestedFeaturesMask;
  QemuFwCfgSelectItem (mRequestedFeaturesItem);
  QemuFwCfgWriteBytes (sizeof mSmiFeatures, &mSmiFeatures);

  //
  // Invoke feature validation in QEMU. If the selection is accepted, the
  // features will be locked down. If the selection is rejected, feature
  // negotiation remains open; however we don't know what to do in that case,
  // so that's a fatal error.
  //
  QemuFwCfgSelectItem (mFeaturesOkItem);
  if (QemuFwCfgRead8 () != 1) {
    DEBUG ((DEBUG_ERROR, "%a: negotiation failed for feature bitmap 0x%Lx\n",
      __FUNCTION__, mSmiFeatures));
    goto FatalError;
  }

  if ((mSmiFeatures & ICH9_LPC_SMI_F_BROADCAST) == 0) {
    //
    // If we can't get broadcast SMIs from QEMU, that's acceptable too,
    // although not optimal.
    //
    DEBUG ((DEBUG_INFO, "%a: SMI broadcast unavailable\n", __FUNCTION__));
  } else {
    //
    // Configure the traditional AP sync / SMI delivery mode for
    // PiSmmCpuDxeSmm. Effectively, restore the UefiCpuPkg defaults, from which
    // the original QEMU behavior (i.e., unicast SMI) used to differ.
    //
    if (RETURN_ERROR (PcdSet64S (PcdCpuSmmApSyncTimeout, 1000000)) ||
        RETURN_ERROR (PcdSet8S (PcdCpuSmmSyncMode, 0x00))) {
      DEBUG ((DEBUG_ERROR, "%a: PiSmmCpuDxeSmm PCD configuration failed\n",
        __FUNCTION__));
      goto FatalError;
    }
    DEBUG ((DEBUG_INFO, "%a: using SMI broadcast\n", __FUNCTION__));
  }

  if ((mSmiFeatures & ICH9_LPC_SMI_F_CPU_HOTPLUG) == 0) {
    DEBUG ((DEBUG_INFO, "%a: CPU hotplug not negotiated\n", __FUNCTION__));
  } else {
    DEBUG ((DEBUG_INFO, "%a: CPU hotplug with SMI negotiated\n",
      __FUNCTION__));
  }

  //
  // Negotiation successful (although we may not have gotten the optimal
  // feature set).
  //
  return TRUE;

FatalError:
  ASSERT (FALSE);
  CpuDeadLoop ();
  //
  // Keep the compiler happy.
  //
  return FALSE;
}

/**
  FW_CFG_BOOT_SCRIPT_CALLBACK_FUNCTION provided to QemuFwCfgS3Lib.
**/
STATIC
VOID
EFIAPI
AppendFwCfgBootScript (
  IN OUT VOID *Context,              OPTIONAL
  IN OUT VOID *ExternalScratchBuffer
  )
{
  SCRATCH_BUFFER *ScratchBuffer;
  RETURN_STATUS  Status;

  ScratchBuffer = ExternalScratchBuffer;

  //
  // Write the negotiated feature bitmap into "etc/smi/requested-features".
  //
  ScratchBuffer->Features = mSmiFeatures;
  Status = QemuFwCfgS3ScriptWriteBytes (mRequestedFeaturesItem,
             sizeof ScratchBuffer->Features);
  if (RETURN_ERROR (Status)) {
    goto FatalError;
  }

  //
  // Read back "etc/smi/features-ok". This invokes the feature validation &
  // lockdown. (The validation succeeded at first boot.)
  //
  Status = QemuFwCfgS3ScriptReadBytes (mFeaturesOkItem,
             sizeof ScratchBuffer->FeaturesOk);
  if (RETURN_ERROR (Status)) {
    goto FatalError;
  }

  //
  // If "etc/smi/features-ok" read as 1, we're good. Otherwise, hang the S3
  // resume process.
  //
  Status = QemuFwCfgS3ScriptCheckValue (&ScratchBuffer->FeaturesOk,
             sizeof ScratchBuffer->FeaturesOk, MAX_UINT8, 1);
  if (RETURN_ERROR (Status)) {
    goto FatalError;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: SMI feature negotiation boot script saved\n",
    __FUNCTION__));
  return;

FatalError:
  ASSERT (FALSE);
  CpuDeadLoop ();
}


/**
  Append a boot script fragment that will re-select the previously negotiated
  SMI features during S3 resume.
**/
VOID
SaveSmiFeatures (
  VOID
  )
{
  RETURN_STATUS Status;

  //
  // We are already running at TPL_CALLBACK, on the stack of
  // OnS3SaveStateInstalled(). But that's okay, we can easily queue more
  // notification functions while executing a notification function.
  //
  Status = QemuFwCfgS3CallWhenBootScriptReady (AppendFwCfgBootScript, NULL,
             sizeof (SCRATCH_BUFFER));
  if (RETURN_ERROR (Status)) {
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}
