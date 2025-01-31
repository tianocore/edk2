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

/*
  Qemu hooks up the uefi vars communication device like this:

        platform-bus@c000000 {
                interrupt-parent = <0x8002>;
                ranges = <0x00 0x00 0xc000000 0x2000000>;
                #address-cells = <0x01>;
                #size-cells = <0x01>;
                compatible = "qemu,platform", "simple-bus";

                qemu-uefi-vars@0 {
                        reg = <0x00 0x10>;
                        compatible = "qemu,uefi-vars";
                };
        };

  So we have to lookup both platform bus and our device to figure where it
  actually is mapped in mmio space.
*/

STATIC UINT64  mUefiVarsAddr;

STATIC
UINT64
VirtMmGetValue (
  CONST VOID  *Data,
  UINT32      Size,
  UINT32      *Pos,
  UINT32      Cells
  )
{
  UINT32  *Ptr32;
  UINT64  *Ptr64;
  UINT64  Value;

  switch (Cells) {
    case 1:
      ASSERT (*Pos + 4 <= Size);
      Ptr32 = (UINT32 *)(Data + *Pos);
      *Pos += 4;
      Value = SwapBytes32 (*Ptr32);
      break;
    case 2:
      ASSERT (*Pos + 8 <= Size);
      Ptr64 = (UINT64 *)(Data + *Pos);
      *Pos += 8;
      Value = SwapBytes64 (ReadUnaligned64 (Ptr64));
      break;
    default:
      ASSERT (!"unsupported cell size");
      Value = 0;
  }

  return Value;
}

STATIC
EFI_STATUS
EFIAPI
VirtMmGetProp (
  IN  FDT_CLIENT_PROTOCOL  *FdtClient,
  IN  CHAR8                *Compatible,
  IN  CHAR8                *Property,
  OUT CONST VOID           **Values,
  OUT UINT32               *ValSize
  )
{
  EFI_STATUS  Status;

  Status = FdtClient->FindCompatibleNodeProperty (
                        FdtClient,
                        Compatible,
                        Property,
                        Values,
                        ValSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: node compatible=\"%a/%a\" not found (%r)\n",
      __func__,
      Compatible,
      Property,
      Status
      ));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: %a/%a: ok\n", __func__, Compatible, Property));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VirtMmHwFind (
  VOID
  )
{
  FDT_CLIENT_PROTOCOL  *FdtClient;
  EFI_STATUS           Status;
  UINT32               Pos;
  CONST VOID           *Ranges;
  UINT32               RangesSize;
  CONST VOID           *Reg;
  UINT32               RegSize;

  UINT64  DevAddr;
  UINT64  DevSize;
  UINT64  BusChildAddr;
  UINT64  BusParentAddr;
  UINT64  BusSize;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  ASSERT_EFI_ERROR (Status);

  VirtMmGetProp (
    FdtClient,
    "qemu,platform",
    "ranges",
    &Ranges,
    &RangesSize
    );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  VirtMmGetProp (
    FdtClient,
    UEFI_VARS_FDT_COMPAT,
    "reg",
    &Reg,
    &RegSize
    );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  if (RegSize == 2 * sizeof (UINT32)) {
    ASSERT (RangesSize == RegSize + sizeof (UINT64));
    Pos           = 0;
    BusChildAddr  = VirtMmGetValue (Ranges, RangesSize, &Pos, 1);
    BusParentAddr = VirtMmGetValue (Ranges, RangesSize, &Pos, 2);
    BusSize       = VirtMmGetValue (Ranges, RangesSize, &Pos, 1);
    Pos           = 0;
    DevAddr       = VirtMmGetValue (Reg, RegSize, &Pos, 1);
    DevSize       = VirtMmGetValue (Reg, RegSize, &Pos, 1);
  } else {
    DEBUG ((DEBUG_ERROR, "%a: unexpected regsize\n", __func__));
    return RETURN_UNSUPPORTED;
  }

  DEBUG ((DEBUG_VERBOSE, "%a: %a\n", __func__, "qemu,platform"));
  DEBUG ((DEBUG_VERBOSE, "%a:   bus child  %8lx\n", __func__, BusChildAddr));
  DEBUG ((DEBUG_VERBOSE, "%a:   bus parent %8lx\n", __func__, BusParentAddr));
  DEBUG ((DEBUG_VERBOSE, "%a:   bus size   %8lx\n", __func__, BusSize));
  DEBUG ((DEBUG_VERBOSE, "%a: %a\n", __func__, UEFI_VARS_FDT_COMPAT));
  DEBUG ((DEBUG_VERBOSE, "%a:   dev addr   %8lx\n", __func__, DevAddr));
  DEBUG ((DEBUG_VERBOSE, "%a:   dev size   %8lx\n", __func__, DevSize));

  mUefiVarsAddr = DevAddr - BusChildAddr + BusParentAddr;
  DEBUG ((DEBUG_VERBOSE, "%a:   -> mmio at %8lx\n", __func__, mUefiVarsAddr));
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
