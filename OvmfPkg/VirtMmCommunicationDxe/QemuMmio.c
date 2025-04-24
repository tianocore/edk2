/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/QemuUefiVars.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "VirtMmCommunication.h"

UINT64  mUefiVarsAddr;

STATIC
EFI_STATUS
VirtMmHwMemAttr (
  )
{
  EFI_STATUS  Status;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  mUefiVarsAddr,
                  EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AddMemorySpace failed: %r\n", __func__, Status));
    return RETURN_UNSUPPORTED;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  mUefiVarsAddr,
                  EFI_PAGE_SIZE,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: SetMemorySpaceAttributes failed: %r\n", __func__, Status));
    return RETURN_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtMmHwCommand (
  UINT32  Cmd
  )
{
  UINT32  Count;
  UINT32  Sts;

  MmioWrite16 (mUefiVarsAddr + UEFI_VARS_REG_CMD_STS, Cmd);
  for (Count = 0; Count < 100; Count++) {
    Sts = MmioRead16 (mUefiVarsAddr + UEFI_VARS_REG_CMD_STS);
    DEBUG ((DEBUG_VERBOSE, "%a: Sts: 0x%x\n", __func__, Sts));
    switch (Sts) {
      case UEFI_VARS_STS_SUCCESS:
        return RETURN_SUCCESS;
      case UEFI_VARS_STS_BUSY:
        CpuPause ();
        break;
      case UEFI_VARS_STS_ERR_NOT_SUPPORTED:
        return RETURN_UNSUPPORTED;
      case UEFI_VARS_STS_ERR_BAD_BUFFER_SIZE:
        return RETURN_BAD_BUFFER_SIZE;
      default:
        return RETURN_DEVICE_ERROR;
    }
  }

  return RETURN_TIMEOUT;
}

EFI_STATUS
EFIAPI
VirtMmHwInit (
  VOID
  )
{
  UINT32      Magic, AddrLo, AddrHi, Flags;
  EFI_STATUS  Status;

  Status = VirtMmHwFind ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: VirtMmHwFind() failed: %d\n", __func__, Status));
    return Status;
  }

  VirtMmHwMemAttr ();

  Magic = MmioRead16 (mUefiVarsAddr + UEFI_VARS_REG_MAGIC);
  if (Magic != UEFI_VARS_MAGIC_VALUE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Magic value mismatch (0x%x != 0x%x)\n",
      __func__,
      Magic,
      UEFI_VARS_MAGIC_VALUE
      ));
    return RETURN_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "%a: Magic 0x%x, good\n", __func__, Magic));

  Status = VirtMmHwCommand (UEFI_VARS_CMD_RESET);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Reset failed: %d\n", __func__, Status));
    return Status;
  }

  MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_BUFFER_SIZE, MAX_BUFFER_SIZE);

  Flags = MmioRead32 (mUefiVarsAddr + UEFI_VARS_REG_FLAGS);
  if (Flags & UEFI_VARS_FLAG_USE_PIO) {
    mUsePioTransfer = TRUE;
    DEBUG ((DEBUG_INFO, "%a: using pio transfer mode\n", __func__));
  } else {
    AddrLo = (UINT32)mCommunicateBufferPhys;
    AddrHi = (UINT32)RShiftU64 (mCommunicateBufferPhys, 32);
    MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_DMA_BUFFER_ADDR_LO, AddrLo);
    MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_DMA_BUFFER_ADDR_HI, AddrHi);
    DEBUG ((DEBUG_INFO, "%a: using dma transfer mode\n", __func__));
  }

  return RETURN_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtMmHwPioTransfer (
  VOID     *Buffer,
  UINT32   BufferSize,
  BOOLEAN  ToDevice
  )
{
  UINT32      *Ptr  = Buffer;
  UINT32      Bytes = 0;
  UINT32      Crc1;
  UINT32      Crc2;
  EFI_STATUS  Status;

  Status = VirtMmHwCommand (UEFI_VARS_CMD_PIO_ZERO_OFFSET);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: zero offset failed: %d\n", __func__, Status));
    return Status;
  }

  while (Bytes < BufferSize) {
    if (ToDevice) {
      MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_PIO_BUFFER_TRANSFER, *Ptr);
    } else {
      *Ptr = MmioRead32 (mUefiVarsAddr + UEFI_VARS_REG_PIO_BUFFER_TRANSFER);
    }

    Bytes += sizeof (*Ptr);
    Ptr++;
  }

  Crc1 = CalculateCrc32c (Buffer, Bytes, 0);
  Crc2 = MmioRead32 (mUefiVarsAddr + UEFI_VARS_REG_PIO_BUFFER_CRC32C);
  if (Crc1 != Crc2) {
    DEBUG ((DEBUG_ERROR, "%a: crc32c mismatch (0x%08x,0x%08x)\n", __func__, Crc1, Crc2));
    return RETURN_DEVICE_ERROR;
  }

  return RETURN_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtMmHwVirtMap (
  VOID
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_VERBOSE, "%a: << %lx\n", __func__, mUefiVarsAddr));
  Status = gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mUefiVarsAddr);
  DEBUG ((DEBUG_VERBOSE, "%a: >> %lx\n", __func__, mUefiVarsAddr));

  return Status;
}

EFI_STATUS
EFIAPI
VirtMmHwComm (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Cmd;

  Cmd    = mUsePioTransfer ? UEFI_VARS_CMD_PIO_MM : UEFI_VARS_CMD_DMA_MM;
  Status = VirtMmHwCommand (Cmd);
  DEBUG ((DEBUG_VERBOSE, "%a: Status: %r\n", __func__, Status));

  return Status;
}
