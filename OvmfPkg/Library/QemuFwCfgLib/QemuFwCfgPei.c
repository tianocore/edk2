/** @file

  Stateful and implicitly initialized fw_cfg library implementation.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/MemEncryptSevLib.h>

#include "QemuFwCfgLibInternal.h"

STATIC BOOLEAN mQemuFwCfgSupported = FALSE;
STATIC BOOLEAN mQemuFwCfgDmaSupported;


/**
  Returns a boolean indicating if the firmware configuration interface
  is available or not.

  This function may change fw_cfg state.

  @retval    TRUE   The interface is available
  @retval    FALSE  The interface is not available

**/
BOOLEAN
EFIAPI
QemuFwCfgIsAvailable (
  VOID
  )
{
  return InternalQemuFwCfgIsAvailable ();
}


RETURN_STATUS
EFIAPI
QemuFwCfgInitialize (
  VOID
  )
{
  UINT32 Signature;
  UINT32 Revision;

  //
  // Enable the access routines while probing to see if it is supported.
  // For probing we always use the IO Port (IoReadFifo8()) access method.
  //
  mQemuFwCfgSupported = TRUE;
  mQemuFwCfgDmaSupported = FALSE;

  QemuFwCfgSelectItem (QemuFwCfgItemSignature);
  Signature = QemuFwCfgRead32 ();
  DEBUG ((DEBUG_INFO, "FW CFG Signature: 0x%x\n", Signature));
  QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
  Revision = QemuFwCfgRead32 ();
  DEBUG ((DEBUG_INFO, "FW CFG Revision: 0x%x\n", Revision));
  if ((Signature != SIGNATURE_32 ('Q', 'E', 'M', 'U')) ||
      (Revision < 1)
     ) {
    DEBUG ((DEBUG_INFO, "QemuFwCfg interface not supported.\n"));
    mQemuFwCfgSupported = FALSE;
    return RETURN_SUCCESS;
  }

  if ((Revision & FW_CFG_F_DMA) == 0) {
    DEBUG ((DEBUG_INFO, "QemuFwCfg interface (IO Port) is supported.\n"));
  } else {
    //
    // If SEV is enabled then we do not support DMA operations in PEI phase.
    // This is mainly because DMA in SEV guest requires using bounce buffer
    // (which need to allocate dynamic memory and allocating a PAGE size'd
    // buffer can be challenge in PEI phase)
    //
    if (MemEncryptSevIsEnabled ()) {
      DEBUG ((DEBUG_INFO, "SEV: QemuFwCfg fallback to IO Port interface.\n"));
    } else {
      mQemuFwCfgDmaSupported = TRUE;
      DEBUG ((DEBUG_INFO, "QemuFwCfg interface (DMA) is supported.\n"));
    }
  }
  return RETURN_SUCCESS;
}


/**
  Returns a boolean indicating if the firmware configuration interface is
  available for library-internal purposes.

  This function never changes fw_cfg state.

  @retval    TRUE   The interface is available internally.
  @retval    FALSE  The interface is not available internally.
**/
BOOLEAN
InternalQemuFwCfgIsAvailable (
  VOID
  )
{
  return mQemuFwCfgSupported;
}

/**
  Returns a boolean indicating whether QEMU provides the DMA-like access method
  for fw_cfg.

  @retval    TRUE   The DMA-like access method is available.
  @retval    FALSE  The DMA-like access method is unavailable.
**/
BOOLEAN
InternalQemuFwCfgDmaIsAvailable (
  VOID
  )
{
  return mQemuFwCfgDmaSupported;
}

/**
  Transfer an array of bytes, or skip a number of bytes, using the DMA
  interface.

  @param[in]     Size     Size in bytes to transfer or skip.

  @param[in,out] Buffer   Buffer to read data into or write data from. Ignored,
                          and may be NULL, if Size is zero, or Control is
                          FW_CFG_DMA_CTL_SKIP.

  @param[in]     Control  One of the following:
                          FW_CFG_DMA_CTL_WRITE - write to fw_cfg from Buffer.
                          FW_CFG_DMA_CTL_READ  - read from fw_cfg into Buffer.
                          FW_CFG_DMA_CTL_SKIP  - skip bytes in fw_cfg.
**/
VOID
InternalQemuFwCfgDmaBytes (
  IN     UINT32   Size,
  IN OUT VOID     *Buffer OPTIONAL,
  IN     UINT32   Control
  )
{
  volatile FW_CFG_DMA_ACCESS Access;
  UINT32                     AccessHigh, AccessLow;
  UINT32                     Status;

  ASSERT (Control == FW_CFG_DMA_CTL_WRITE || Control == FW_CFG_DMA_CTL_READ ||
    Control == FW_CFG_DMA_CTL_SKIP);

  if (Size == 0) {
    return;
  }

  //
  // SEV does not support DMA operations in PEI stage, we should
  // not have reached here.
  //
  ASSERT (!MemEncryptSevIsEnabled ());

  Access.Control = SwapBytes32 (Control);
  Access.Length  = SwapBytes32 (Size);
  Access.Address = SwapBytes64 ((UINTN)Buffer);

  //
  // Delimit the transfer from (a) modifications to Access, (b) in case of a
  // write, from writes to Buffer by the caller.
  //
  MemoryFence ();

  //
  // Start the transfer.
  //
  AccessHigh = (UINT32)RShiftU64 ((UINTN)&Access, 32);
  AccessLow  = (UINT32)(UINTN)&Access;
  IoWrite32 (FW_CFG_IO_DMA_ADDRESS,     SwapBytes32 (AccessHigh));
  IoWrite32 (FW_CFG_IO_DMA_ADDRESS + 4, SwapBytes32 (AccessLow));

  //
  // Don't look at Access.Control before starting the transfer.
  //
  MemoryFence ();

  //
  // Wait for the transfer to complete.
  //
  do {
    Status = SwapBytes32 (Access.Control);
    ASSERT ((Status & FW_CFG_DMA_CTL_ERROR) == 0);
  } while (Status != 0);

  //
  // After a read, the caller will want to use Buffer.
  //
  MemoryFence ();
}

