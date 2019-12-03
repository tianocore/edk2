/** @file

  Stateful and implicitly initialized fw_cfg library implementation.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Protocol/IoMmu.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>

#include "QemuFwCfgLibInternal.h"

STATIC BOOLEAN mQemuFwCfgSupported = FALSE;
STATIC BOOLEAN mQemuFwCfgDmaSupported;

STATIC EDKII_IOMMU_PROTOCOL        *mIoMmuProtocol;

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
  DEBUG ((EFI_D_INFO, "FW CFG Signature: 0x%x\n", Signature));
  QemuFwCfgSelectItem (QemuFwCfgItemInterfaceVersion);
  Revision = QemuFwCfgRead32 ();
  DEBUG ((EFI_D_INFO, "FW CFG Revision: 0x%x\n", Revision));
  if ((Signature != SIGNATURE_32 ('Q', 'E', 'M', 'U')) ||
      (Revision < 1)
     ) {
    DEBUG ((EFI_D_INFO, "QemuFwCfg interface not supported.\n"));
    mQemuFwCfgSupported = FALSE;
    return RETURN_SUCCESS;
  }

  if ((Revision & FW_CFG_F_DMA) == 0) {
    DEBUG ((DEBUG_INFO, "QemuFwCfg interface (IO Port) is supported.\n"));
  } else {
    mQemuFwCfgDmaSupported = TRUE;
    DEBUG ((DEBUG_INFO, "QemuFwCfg interface (DMA) is supported.\n"));
  }

  if (mQemuFwCfgDmaSupported && MemEncryptSevIsEnabled ()) {
    EFI_STATUS   Status;

    //
    // IoMmuDxe driver must have installed the IOMMU protocol. If we are not
    // able to locate the protocol then something must have gone wrong.
    //
    Status = gBS->LocateProtocol (&gEdkiiIoMmuProtocolGuid, NULL,
                    (VOID **)&mIoMmuProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
        "QemuFwCfgSevDma %a:%a Failed to locate IOMMU protocol.\n",
        gEfiCallerBaseName, __FUNCTION__));
      ASSERT (FALSE);
      CpuDeadLoop ();
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
  Function is used for allocating a bi-directional FW_CFG_DMA_ACCESS used
  between Host and device to exchange the information. The buffer must be free'd
  using FreeFwCfgDmaAccessBuffer ().

**/
STATIC
VOID
AllocFwCfgDmaAccessBuffer (
  OUT   VOID     **Access,
  OUT   VOID     **MapInfo
  )
{
  UINTN                 Size;
  UINTN                 NumPages;
  EFI_STATUS            Status;
  VOID                  *HostAddress;
  EFI_PHYSICAL_ADDRESS  DmaAddress;
  VOID                  *Mapping;

  Size = sizeof (FW_CFG_DMA_ACCESS);
  NumPages = EFI_SIZE_TO_PAGES (Size);

  //
  // As per UEFI spec, in order to map a host address with
  // BusMasterCommomBuffer64, the buffer must be allocated using the IOMMU
  // AllocateBuffer()
  //
  Status = mIoMmuProtocol->AllocateBuffer (
                             mIoMmuProtocol,
                             AllocateAnyPages,
                             EfiBootServicesData,
                             NumPages,
                             &HostAddress,
                             EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to allocate FW_CFG_DMA_ACCESS\n", gEfiCallerBaseName,
      __FUNCTION__));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  //
  // Avoid exposing stale data even temporarily: zero the area before mapping
  // it.
  //
  ZeroMem (HostAddress, Size);

  //
  // Map the host buffer with BusMasterCommonBuffer64
  //
  Status = mIoMmuProtocol->Map (
                             mIoMmuProtocol,
                             EdkiiIoMmuOperationBusMasterCommonBuffer64,
                             HostAddress,
                             &Size,
                             &DmaAddress,
                             &Mapping
                             );
  if (EFI_ERROR (Status)) {
    mIoMmuProtocol->FreeBuffer (mIoMmuProtocol, NumPages, HostAddress);
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to Map() FW_CFG_DMA_ACCESS\n", gEfiCallerBaseName,
      __FUNCTION__));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  if (Size < sizeof (FW_CFG_DMA_ACCESS)) {
    mIoMmuProtocol->Unmap (mIoMmuProtocol, Mapping);
    mIoMmuProtocol->FreeBuffer (mIoMmuProtocol, NumPages, HostAddress);
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to Map() - requested 0x%Lx got 0x%Lx\n", gEfiCallerBaseName,
      __FUNCTION__, (UINT64)sizeof (FW_CFG_DMA_ACCESS), (UINT64)Size));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  *Access = HostAddress;
  *MapInfo = Mapping;
}

/**
  Function is to used for freeing the Access buffer allocated using
  AllocFwCfgDmaAccessBuffer()

**/
STATIC
VOID
FreeFwCfgDmaAccessBuffer (
  IN  VOID    *Access,
  IN  VOID    *Mapping
  )
{
  UINTN       NumPages;
  EFI_STATUS  Status;

  NumPages = EFI_SIZE_TO_PAGES (sizeof (FW_CFG_DMA_ACCESS));

  Status = mIoMmuProtocol->Unmap (mIoMmuProtocol, Mapping);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to UnMap() Mapping 0x%Lx\n", gEfiCallerBaseName,
      __FUNCTION__, (UINT64)(UINTN)Mapping));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  Status = mIoMmuProtocol->FreeBuffer (mIoMmuProtocol, NumPages, Access);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to Free() 0x%Lx\n", gEfiCallerBaseName, __FUNCTION__,
      (UINT64)(UINTN)Access));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
}

/**
  Function is used for mapping host address to device address. The buffer must
  be unmapped with UnmapDmaDataBuffer ().

**/
STATIC
VOID
MapFwCfgDmaDataBuffer (
  IN  BOOLEAN               IsWrite,
  IN  VOID                  *HostAddress,
  IN  UINT32                Size,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **MapInfo
  )
{
  EFI_STATUS              Status;
  UINTN                   NumberOfBytes;
  VOID                    *Mapping;
  EFI_PHYSICAL_ADDRESS    PhysicalAddress;

  NumberOfBytes = Size;
  Status = mIoMmuProtocol->Map (
                             mIoMmuProtocol,
                             (IsWrite ?
                              EdkiiIoMmuOperationBusMasterRead64 :
                              EdkiiIoMmuOperationBusMasterWrite64),
                             HostAddress,
                             &NumberOfBytes,
                             &PhysicalAddress,
                             &Mapping
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to Map() Address 0x%Lx Size 0x%Lx\n", gEfiCallerBaseName,
      __FUNCTION__, (UINT64)(UINTN)HostAddress, (UINT64)Size));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  if (NumberOfBytes < Size) {
    mIoMmuProtocol->Unmap (mIoMmuProtocol, Mapping);
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to Map() - requested 0x%x got 0x%Lx\n", gEfiCallerBaseName,
      __FUNCTION__, Size, (UINT64)NumberOfBytes));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  *DeviceAddress = PhysicalAddress;
  *MapInfo = Mapping;
}

STATIC
VOID
UnmapFwCfgDmaDataBuffer (
  IN  VOID  *Mapping
  )
{
  EFI_STATUS  Status;

  Status = mIoMmuProtocol->Unmap (mIoMmuProtocol, Mapping);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "%a:%a failed to UnMap() Mapping 0x%Lx\n", gEfiCallerBaseName,
      __FUNCTION__, (UINT64)(UINTN)Mapping));
    ASSERT (FALSE);
    CpuDeadLoop ();
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
VOID
InternalQemuFwCfgDmaBytes (
  IN     UINT32   Size,
  IN OUT VOID     *Buffer OPTIONAL,
  IN     UINT32   Control
  )
{
  volatile FW_CFG_DMA_ACCESS LocalAccess;
  volatile FW_CFG_DMA_ACCESS *Access;
  UINT32                     AccessHigh, AccessLow;
  UINT32                     Status;
  VOID                       *AccessMapping, *DataMapping;
  VOID                       *DataBuffer;

  ASSERT (Control == FW_CFG_DMA_CTL_WRITE || Control == FW_CFG_DMA_CTL_READ ||
    Control == FW_CFG_DMA_CTL_SKIP);

  if (Size == 0) {
    return;
  }

  Access = &LocalAccess;
  AccessMapping = NULL;
  DataMapping = NULL;
  DataBuffer = Buffer;

  //
  // When SEV is enabled, map Buffer to DMA address before issuing the DMA
  // request
  //
  if (MemEncryptSevIsEnabled ()) {
    VOID                  *AccessBuffer;
    EFI_PHYSICAL_ADDRESS  DataBufferAddress;

    //
    // Allocate DMA Access buffer
    //
    AllocFwCfgDmaAccessBuffer (&AccessBuffer, &AccessMapping);

    Access = AccessBuffer;

    //
    // Map actual data buffer
    //
    if (Control != FW_CFG_DMA_CTL_SKIP) {
      MapFwCfgDmaDataBuffer (
        Control == FW_CFG_DMA_CTL_WRITE,
        Buffer,
        Size,
        &DataBufferAddress,
        &DataMapping
        );

      DataBuffer = (VOID *) (UINTN) DataBufferAddress;
    }
  }

  Access->Control = SwapBytes32 (Control);
  Access->Length  = SwapBytes32 (Size);
  Access->Address = SwapBytes64 ((UINTN)DataBuffer);

  //
  // Delimit the transfer from (a) modifications to Access, (b) in case of a
  // write, from writes to Buffer by the caller.
  //
  MemoryFence ();

  //
  // Start the transfer.
  //
  AccessHigh = (UINT32)RShiftU64 ((UINTN)Access, 32);
  AccessLow  = (UINT32)(UINTN)Access;
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
    Status = SwapBytes32 (Access->Control);
    ASSERT ((Status & FW_CFG_DMA_CTL_ERROR) == 0);
  } while (Status != 0);

  //
  // After a read, the caller will want to use Buffer.
  //
  MemoryFence ();

  //
  // If Access buffer was dynamically allocated then free it.
  //
  if (AccessMapping != NULL) {
    FreeFwCfgDmaAccessBuffer ((VOID *)Access, AccessMapping);
  }

  //
  // If DataBuffer was mapped then unmap it.
  //
  if (DataMapping != NULL) {
    UnmapFwCfgDmaDataBuffer (DataMapping);
  }
}
