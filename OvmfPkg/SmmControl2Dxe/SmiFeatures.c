/**@file
  Negotiate SMI features with QEMU, and configure UefiCpuPkg/PiSmmCpuDxeSmm
  accordingly.

  Copyright (C) 2016-2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>

#include "SmiFeatures.h"

//
// The following bit value stands for "broadcast SMI" in the
// "etc/smi/supported-features" and "etc/smi/requested-features" fw_cfg files.
//
#define ICH9_LPC_SMI_F_BROADCAST BIT0

//
// Provides a scratch buffer (allocated in EfiReservedMemoryType type memory)
// for the S3 boot script fragment to write to and read from. The buffer
// captures a combined fw_cfg item selection + write command using the DMA
// access method. Note that we don't trust the runtime OS to preserve the
// contents of the buffer, the boot script will first rewrite it.
//
#pragma pack (1)
typedef struct {
  FW_CFG_DMA_ACCESS Access;
  UINT64            Features;
} SCRATCH_BUFFER;
#pragma pack ()

//
// These carry the selector keys of the "etc/smi/requested-features" and
// "etc/smi/features-ok" fw_cfg files from NegotiateSmiFeatures() to
// SaveSmiFeatures().
//
STATIC FIRMWARE_CONFIG_ITEM mRequestedFeaturesItem;
STATIC FIRMWARE_CONFIG_ITEM mFeaturesOkItem;

//
// Carries the negotiated SMI features from NegotiateSmiFeatures() to
// SaveSmiFeatures().
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
  // We want broadcast SMI and nothing else.
  //
  mSmiFeatures &= ICH9_LPC_SMI_F_BROADCAST;
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
  Append a boot script fragment that will re-select the previously negotiated
  SMI features during S3 resume.

  @param[in] S3SaveState  The EFI_S3_SAVE_STATE_PROTOCOL instance to append to
                          the S3 boot script with.
**/
VOID
SaveSmiFeatures (
  IN EFI_S3_SAVE_STATE_PROTOCOL *S3SaveState
  )
{
  SCRATCH_BUFFER *ScratchBuffer;
  EFI_STATUS     Status;
  UINT64         AccessAddress;
  UINT32         ControlPollData;
  UINT32         ControlPollMask;
  UINT16         FeaturesOkItemAsUint16;
  UINT8          FeaturesOkData;
  UINT8          FeaturesOkMask;

  ScratchBuffer = AllocateReservedPool (sizeof *ScratchBuffer);
  if (ScratchBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: scratch buffer allocation failed\n",
      __FUNCTION__));
    goto FatalError;
  }

  //
  // Populate the scratch buffer with a select + write fw_cfg DMA command that
  // will write the negotiated feature bitmap into
  // "etc/smi/requested-features".
  //
  ScratchBuffer->Access.Control = SwapBytes32 (
                                    (UINT32)mRequestedFeaturesItem << 16 |
                                    FW_CFG_DMA_CTL_SELECT |
                                    FW_CFG_DMA_CTL_WRITE
                                    );
  ScratchBuffer->Access.Length = SwapBytes32 (
                                   (UINT32)sizeof ScratchBuffer->Features);
  ScratchBuffer->Access.Address = SwapBytes64 (
                                    (UINTN)&ScratchBuffer->Features);
  ScratchBuffer->Features       = mSmiFeatures;

  //
  // Copy the scratch buffer into the boot script. When replayed, this
  // EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE will restore the current contents of the
  // scratch buffer, in-place.
  //
  Status = S3SaveState->Write (
                          S3SaveState,                      // This
                          EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, // OpCode
                          EfiBootScriptWidthUint8,          // Width
                          (UINT64)(UINTN)ScratchBuffer,     // Address
                          sizeof *ScratchBuffer,            // Count
                          (VOID*)ScratchBuffer              // Buffer
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE: %r\n",
      __FUNCTION__, __LINE__, Status));
    goto FatalError;
  }

  //
  // Append an opcode that will write the address of the scratch buffer to the
  // fw_cfg DMA address register, which consists of two 32-bit IO ports. The
  // second (highest address, least significant) write will start the transfer.
  //
  AccessAddress = SwapBytes64 ((UINTN)&ScratchBuffer->Access);
  Status = S3SaveState->Write (
                          S3SaveState,                     // This
                          EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                          EfiBootScriptWidthUint32,        // Width
                          (UINT64)0x514,                   // Address
                          (UINTN)2,                        // Count
                          &AccessAddress                   // Buffer
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: EFI_BOOT_SCRIPT_IO_WRITE_OPCODE: %r\n",
      __FUNCTION__, __LINE__, Status));
    goto FatalError;
  }

  //
  // The EFI_BOOT_SCRIPT_MEM_POLL_OPCODE will wait until the Control word reads
  // as zero (transfer complete). As timeout we use MAX_UINT64 * 100ns, which
  // is approximately 58494 years.
  //
  ControlPollData = 0;
  ControlPollMask = MAX_UINT32;
  Status = S3SaveState->Write (
                     S3SaveState,                                   // This
                     EFI_BOOT_SCRIPT_MEM_POLL_OPCODE,               // OpCode
                     EfiBootScriptWidthUint32,                      // Width
                     (UINT64)(UINTN)&ScratchBuffer->Access.Control, // Address
                     &ControlPollData,                              // Data
                     &ControlPollMask,                              // DataMask
                     MAX_UINT64                                     // Delay
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: EFI_BOOT_SCRIPT_MEM_POLL_OPCODE: %r\n",
      __FUNCTION__, __LINE__, Status));
    goto FatalError;
  }

  //
  // Select the "etc/smi/features-ok" fw_cfg file, which invokes the feature
  // validation & lockdown. (The validation succeeded at first boot.)
  //
  FeaturesOkItemAsUint16 = (UINT16)mFeaturesOkItem;
  Status = S3SaveState->Write (
                          S3SaveState,                     // This
                          EFI_BOOT_SCRIPT_IO_WRITE_OPCODE, // OpCode
                          EfiBootScriptWidthUint16,        // Width
                          (UINT64)FW_CFG_IO_SELECTOR,      // Address
                          (UINTN)1,                        // Count
                          &FeaturesOkItemAsUint16          // Buffer
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: EFI_BOOT_SCRIPT_IO_WRITE_OPCODE: %r\n",
      __FUNCTION__, __LINE__, Status));
    goto FatalError;
  }

  //
  // Read the contents (one byte) of "etc/smi/features-ok". If the value is
  // one, we're good. Otherwise, continue reading the data port: QEMU returns 0
  // past the end of the fw_cfg item, so this will hang the resume process,
  // which matches our intent.
  //
  FeaturesOkData = 1;
  FeaturesOkMask = MAX_UINT8;
  Status = S3SaveState->Write (
                     S3SaveState,                    // This
                     EFI_BOOT_SCRIPT_IO_POLL_OPCODE, // OpCode
                     EfiBootScriptWidthUint8,        // Width
                     (UINT64)(UINTN)FW_CFG_IO_DATA,  // Address
                     &FeaturesOkData,                // Data
                     &FeaturesOkMask,                // DataMask
                     MAX_UINT64                      // Delay
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a:%d: EFI_BOOT_SCRIPT_IO_POLL_OPCODE: %r\n",
      __FUNCTION__, __LINE__, Status));
    goto FatalError;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: ScratchBuffer@%p\n", __FUNCTION__,
    (VOID *)ScratchBuffer));
  return;

FatalError:
  ASSERT (FALSE);
  CpuDeadLoop ();
}
