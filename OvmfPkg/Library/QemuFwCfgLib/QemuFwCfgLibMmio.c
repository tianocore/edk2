/** @file

  Stateful and implicitly initialized fw_cfg library implementation.

  Copyright (C) 2013 - 2014, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/FdtClient.h>

STATIC UINTN mFwCfgSelectorAddress;
STATIC UINTN mFwCfgDataAddress;
STATIC UINTN mFwCfgDmaAddress;

/**
  Reads firmware configuration bytes into a buffer

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into  (OPTIONAL if Size is 0)

**/
typedef
VOID (EFIAPI READ_BYTES_FUNCTION) (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  );

/**
  Writes bytes from a buffer to firmware configuration

  @param[in] Size    Size in bytes to write
  @param[in] Buffer  Buffer to transfer data from (OPTIONAL if Size is 0)

**/
typedef
VOID (EFIAPI WRITE_BYTES_FUNCTION) (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  );

/**
  Skips bytes in firmware configuration

  @param[in] Size  Size in bytes to skip

**/
typedef
VOID (EFIAPI SKIP_BYTES_FUNCTION) (
  IN UINTN Size
  );

//
// Forward declaration of the two implementations we have.
//
STATIC READ_BYTES_FUNCTION MmioReadBytes;
STATIC WRITE_BYTES_FUNCTION MmioWriteBytes;
STATIC SKIP_BYTES_FUNCTION MmioSkipBytes;
STATIC READ_BYTES_FUNCTION DmaReadBytes;
STATIC WRITE_BYTES_FUNCTION DmaWriteBytes;
STATIC SKIP_BYTES_FUNCTION DmaSkipBytes;

//
// These correspond to the implementation we detect at runtime.
//
STATIC READ_BYTES_FUNCTION *InternalQemuFwCfgReadBytes = MmioReadBytes;
STATIC WRITE_BYTES_FUNCTION *InternalQemuFwCfgWriteBytes = MmioWriteBytes;
STATIC SKIP_BYTES_FUNCTION *InternalQemuFwCfgSkipBytes = MmioSkipBytes;


/**
  Returns a boolean indicating if the firmware configuration interface
  is available or not.

  This function may change fw_cfg state.

  @retval TRUE   The interface is available
  @retval FALSE  The interface is not available

**/
BOOLEAN
EFIAPI
QemuFwCfgIsAvailable (
  VOID
  )
{
  return (BOOLEAN)(mFwCfgSelectorAddress != 0 && mFwCfgDataAddress != 0);
}


RETURN_STATUS
EFIAPI
QemuFwCfgInitialize (
  VOID
  )
{
  EFI_STATUS                    Status;
  FDT_CLIENT_PROTOCOL           *FdtClient;
  CONST UINT64                  *Reg;
  UINT32                        RegSize;
  UINTN                         AddressCells, SizeCells;
  UINT64                        FwCfgSelectorAddress;
  UINT64                        FwCfgSelectorSize;
  UINT64                        FwCfgDataAddress;
  UINT64                        FwCfgDataSize;
  UINT64                        FwCfgDmaAddress;
  UINT64                        FwCfgDmaSize;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeReg (FdtClient, "qemu,fw-cfg-mmio",
                         (CONST VOID **)&Reg, &AddressCells, &SizeCells,
                         &RegSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN,
      "%a: No 'qemu,fw-cfg-mmio' compatible DT node found (Status == %r)\n",
      __FUNCTION__, Status));
    return EFI_SUCCESS;
  }

  ASSERT (AddressCells == 2);
  ASSERT (SizeCells == 2);
  ASSERT (RegSize == 2 * sizeof (UINT64));

  FwCfgDataAddress     = SwapBytes64 (Reg[0]);
  FwCfgDataSize        = 8;
  FwCfgSelectorAddress = FwCfgDataAddress + FwCfgDataSize;
  FwCfgSelectorSize    = 2;

  //
  // The following ASSERT()s express
  //
  //   Address + Size - 1 <= MAX_UINTN
  //
  // for both registers, that is, that the last byte in each MMIO range is
  // expressible as a MAX_UINTN. The form below is mathematically
  // equivalent, and it also prevents any unsigned overflow before the
  // comparison.
  //
  ASSERT (FwCfgSelectorAddress <= MAX_UINTN - FwCfgSelectorSize + 1);
  ASSERT (FwCfgDataAddress     <= MAX_UINTN - FwCfgDataSize     + 1);

  mFwCfgSelectorAddress = FwCfgSelectorAddress;
  mFwCfgDataAddress     = FwCfgDataAddress;

  DEBUG ((EFI_D_INFO, "Found FwCfg @ 0x%Lx/0x%Lx\n", FwCfgSelectorAddress,
    FwCfgDataAddress));

  if (SwapBytes64 (Reg[1]) >= 0x18) {
    FwCfgDmaAddress = FwCfgDataAddress + 0x10;
    FwCfgDmaSize    = 0x08;

    //
    // See explanation above.
    //
    ASSERT (FwCfgDmaAddress <= MAX_UINTN - FwCfgDmaSize + 1);

    DEBUG ((EFI_D_INFO, "Found FwCfg DMA @ 0x%Lx\n", FwCfgDmaAddress));
  } else {
    FwCfgDmaAddress = 0;
  }

  if (QemuFwCfgIsAvailable ()) {
    UINT32 Signature;

    QemuFwCfgSelectItem (QemuFwCfgItemSignature);
    Signature = QemuFwCfgRead32 ();
    if (Signature == SIGNATURE_32 ('Q', 'E', 'M', 'U')) {
      //
      // For DMA support, we require the DTB to advertise the register, and the
      // feature bitmap (which we read without DMA) to confirm the feature.
      //
      if (FwCfgDmaAddress != 0) {
        UINT32 Features;

        QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
        Features = QemuFwCfgRead32 ();
        if ((Features & FW_CFG_F_DMA) != 0) {
          mFwCfgDmaAddress = FwCfgDmaAddress;
          InternalQemuFwCfgReadBytes = DmaReadBytes;
          InternalQemuFwCfgWriteBytes = DmaWriteBytes;
          InternalQemuFwCfgSkipBytes = DmaSkipBytes;
        }
      }
    } else {
      mFwCfgSelectorAddress = 0;
      mFwCfgDataAddress     = 0;
    }
  }
  return RETURN_SUCCESS;
}


/**
  Selects a firmware configuration item for reading.

  Following this call, any data read from this item will start from the
  beginning of the configuration item's data.

  @param[in] QemuFwCfgItem  Firmware Configuration item to read

**/
VOID
EFIAPI
QemuFwCfgSelectItem (
  IN FIRMWARE_CONFIG_ITEM QemuFwCfgItem
  )
{
  if (QemuFwCfgIsAvailable ()) {
    MmioWrite16 (mFwCfgSelectorAddress, SwapBytes16 ((UINT16)QemuFwCfgItem));
  }
}


/**
  Slow READ_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
MmioReadBytes (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  )
{
  UINTN Left;
  UINT8 *Ptr;
  UINT8 *End;

#if defined(MDE_CPU_AARCH64) || defined(MDE_CPU_RISCV64)
  Left = Size & 7;
#else
  Left = Size & 3;
#endif

  Size -= Left;
  Ptr = Buffer;
  End = Ptr + Size;

#if defined(MDE_CPU_AARCH64) || defined(MDE_CPU_RISCV64)
  while (Ptr < End) {
    *(UINT64 *)Ptr = MmioRead64 (mFwCfgDataAddress);
    Ptr += 8;
  }
  if (Left & 4) {
    *(UINT32 *)Ptr = MmioRead32 (mFwCfgDataAddress);
    Ptr += 4;
  }
#else
  while (Ptr < End) {
    *(UINT32 *)Ptr = MmioRead32 (mFwCfgDataAddress);
    Ptr += 4;
  }
#endif

  if (Left & 2) {
    *(UINT16 *)Ptr = MmioRead16 (mFwCfgDataAddress);
    Ptr += 2;
  }
  if (Left & 1) {
    *Ptr = MmioRead8 (mFwCfgDataAddress);
  }
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
STATIC
VOID
DmaTransferBytes (
  IN     UINTN  Size,
  IN OUT VOID   *Buffer OPTIONAL,
  IN     UINT32 Control
  )
{
  volatile FW_CFG_DMA_ACCESS Access;
  UINT32                     Status;

  ASSERT (Control == FW_CFG_DMA_CTL_WRITE || Control == FW_CFG_DMA_CTL_READ ||
    Control == FW_CFG_DMA_CTL_SKIP);

  if (Size == 0) {
    return;
  }

  ASSERT (Size <= MAX_UINT32);

  Access.Control = SwapBytes32 (Control);
  Access.Length  = SwapBytes32 ((UINT32)Size);
  Access.Address = SwapBytes64 ((UINT64)(UINTN)Buffer);

  //
  // We shouldn't start the transfer before setting up Access.
  //
  MemoryFence ();

  //
  // This will fire off the transfer.
  //
#if defined(MDE_CPU_AARCH64) || defined(MDE_CPU_RISCV64)
  MmioWrite64 (mFwCfgDmaAddress, SwapBytes64 ((UINT64)&Access));
#else
  MmioWrite32 ((UINT32)(mFwCfgDmaAddress + 4), SwapBytes32 ((UINT32)&Access));
#endif

  //
  // We shouldn't look at Access.Control before starting the transfer.
  //
  MemoryFence ();

  do {
    Status = SwapBytes32 (Access.Control);
    ASSERT ((Status & FW_CFG_DMA_CTL_ERROR) == 0);
  } while (Status != 0);

  //
  // The caller will want to access the transferred data.
  //
  MemoryFence ();
}


/**
  Fast READ_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
DmaReadBytes (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  )
{
  DmaTransferBytes (Size, Buffer, FW_CFG_DMA_CTL_READ);
}


/**
  Reads firmware configuration bytes into a buffer

  If called multiple times, then the data read will continue at the offset of
  the firmware configuration item where the previous read ended.

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into

**/
VOID
EFIAPI
QemuFwCfgReadBytes (
  IN UINTN Size,
  IN VOID  *Buffer
  )
{
  if (QemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgReadBytes (Size, Buffer);
  } else {
    ZeroMem (Buffer, Size);
  }
}


/**
  Slow WRITE_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
MmioWriteBytes (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  )
{
  UINTN Idx;

  for (Idx = 0; Idx < Size; ++Idx) {
    MmioWrite8 (mFwCfgDataAddress, ((UINT8 *)Buffer)[Idx]);
  }
}


/**
  Fast WRITE_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
DmaWriteBytes (
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  )
{
  DmaTransferBytes (Size, Buffer, FW_CFG_DMA_CTL_WRITE);
}


/**
  Write firmware configuration bytes from a buffer

  If called multiple times, then the data written will continue at the offset
  of the firmware configuration item where the previous write ended.

  @param[in] Size    Size in bytes to write
  @param[in] Buffer  Buffer to read data from

**/
VOID
EFIAPI
QemuFwCfgWriteBytes (
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  if (QemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgWriteBytes (Size, Buffer);
  }
}


/**
  Slow SKIP_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
MmioSkipBytes (
  IN UINTN Size
  )
{
  UINTN ChunkSize;
  UINT8 SkipBuffer[256];

  //
  // Emulate the skip by reading data in chunks, and throwing it away. The
  // implementation below doesn't affect the static data footprint for client
  // modules. Large skips are not expected, therefore this fallback is not
  // performance critical. The size of SkipBuffer is thought not to exert a
  // large pressure on the stack.
  //
  while (Size > 0) {
    ChunkSize = MIN (Size, sizeof SkipBuffer);
    MmioReadBytes (ChunkSize, SkipBuffer);
    Size -= ChunkSize;
  }
}


/**
  Fast SKIP_BYTES_FUNCTION.
**/
STATIC
VOID
EFIAPI
DmaSkipBytes (
  IN UINTN Size
  )
{
  DmaTransferBytes (Size, NULL, FW_CFG_DMA_CTL_SKIP);
}


/**
  Skip bytes in the firmware configuration item.

  Increase the offset of the firmware configuration item without transferring
  bytes between the item and a caller-provided buffer. Subsequent read, write
  or skip operations will commence at the increased offset.

  @param[in] Size  Number of bytes to skip.
**/
VOID
EFIAPI
QemuFwCfgSkipBytes (
  IN UINTN Size
  )
{
  if (QemuFwCfgIsAvailable ()) {
    InternalQemuFwCfgSkipBytes (Size);
  }
}


/**
  Reads a UINT8 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT8
EFIAPI
QemuFwCfgRead8 (
  VOID
  )
{
  UINT8 Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}


/**
  Reads a UINT16 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT16
EFIAPI
QemuFwCfgRead16 (
  VOID
  )
{
  UINT16 Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}


/**
  Reads a UINT32 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT32
EFIAPI
QemuFwCfgRead32 (
  VOID
  )
{
  UINT32 Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}


/**
  Reads a UINT64 firmware configuration value

  @return  Value of Firmware Configuration item read

**/
UINT64
EFIAPI
QemuFwCfgRead64 (
  VOID
  )
{
  UINT64 Result;

  QemuFwCfgReadBytes (sizeof Result, &Result);
  return Result;
}


/**
  Find the configuration item corresponding to the firmware configuration file.

  @param[in]  Name  Name of file to look up.
  @param[out] Item  Configuration item corresponding to the file, to be passed
                    to QemuFwCfgSelectItem ().
  @param[out] Size  Number of bytes in the file.

  @retval RETURN_SUCCESS      If file is found.
  @retval RETURN_NOT_FOUND    If file is not found.
  @retval RETURN_UNSUPPORTED  If firmware configuration is unavailable.

**/
RETURN_STATUS
EFIAPI
QemuFwCfgFindFile (
  IN   CONST CHAR8           *Name,
  OUT  FIRMWARE_CONFIG_ITEM  *Item,
  OUT  UINTN                 *Size
  )
{
  UINT32 Count;
  UINT32 Idx;

  if (!QemuFwCfgIsAvailable ()) {
    return RETURN_UNSUPPORTED;
  }

  QemuFwCfgSelectItem (QemuFwCfgItemFileDir);
  Count = SwapBytes32 (QemuFwCfgRead32 ());

  for (Idx = 0; Idx < Count; ++Idx) {
    UINT32 FileSize;
    UINT16 FileSelect;
    CHAR8  FName[QEMU_FW_CFG_FNAME_SIZE];

    FileSize   = QemuFwCfgRead32 ();
    FileSelect = QemuFwCfgRead16 ();
    QemuFwCfgRead16 (); // skip the field called "reserved"
    InternalQemuFwCfgReadBytes (sizeof (FName), FName);

    if (AsciiStrCmp (Name, FName) == 0) {
      *Item = (FIRMWARE_CONFIG_ITEM) SwapBytes16 (FileSelect);
      *Size = SwapBytes32 (FileSize);
      return RETURN_SUCCESS;
    }
  }

  return RETURN_NOT_FOUND;
}
