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

#include <Protocol/FdtClient.h>

#include "VirtMmCommunication.h"

STATIC UINT64  mUefiVarsAddr;

STATIC
EFI_STATUS
EFIAPI
VirtMmHwFind (
  VOID
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  EFI_STATUS           Status;
  CONST UINT64         *Reg;
  UINT32               RegSize;
  UINTN                AddressCells, SizeCells;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (
                        FdtClient,
                        UEFI_VARS_FDT_COMPAT,
                        (CONST VOID **)&Reg,
                        &AddressCells,
                        &SizeCells,
                        &RegSize
                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: node compatible=%a not found (%r)\n",
      __func__,
      UEFI_VARS_FDT_NODE,
      Status
      ));
    return EFI_NOT_FOUND;
  }

  ASSERT (AddressCells == 2);
  ASSERT (SizeCells == 2);
  ASSERT (RegSize == 2 * sizeof (UINT64));

  mUefiVarsAddr = SwapBytes64 (Reg[0]);
  DEBUG ((DEBUG_VERBOSE, "%a: address: 0x%lx\n", __func__, mUefiVarsAddr));

  return RETURN_SUCCESS;
}

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
  UINT32      Magic, AddrLo, AddrHi;
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

  AddrLo = (UINT32)mCommunicateBufferPhys;
  AddrHi = (UINT32)RShiftU64 (mCommunicateBufferPhys, 32);
  MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_BUFFER_ADDR_LO, AddrLo);
  MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_BUFFER_ADDR_HI, AddrHi);
  MmioWrite32 (mUefiVarsAddr + UEFI_VARS_REG_BUFFER_SIZE, MAX_BUFFER_SIZE);

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

  Status = VirtMmHwCommand (UEFI_VARS_CMD_MM);
  DEBUG ((DEBUG_VERBOSE, "%a: Status: %r\n", __func__, Status));

  return Status;
}
