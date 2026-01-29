/** @file
  This file contains the callback routines for undi3.1.
  the callback routines for Undi3.1 have an extra parameter UniqueId which
  stores the interface context for the NIC that snp is trying to talk.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Snp.h"

/**
  Acquire or release a lock of the exclusive access to a critical section of the
  code/data.

  This is a callback routine supplied to UNDI3.1 at undi_start time.
  New callbacks for 3.1: there won't be a virtual2physical callback for UNDI 3.1
  because undi3.1 uses the MemMap call to map the required address by itself!

  @param UniqueId  This was supplied to UNDI at Undi_Start, SNP uses this to
                      store Undi interface context (Undi does not read or write
                      this variable).
  @param Enable    Non-zero indicates acquire; Zero indicates release.

**/
VOID
EFIAPI
SnpUndi32CallbackBlock (
  IN UINT64  UniqueId,
  IN UINT32  Enable
  )
{
  SNP_DRIVER  *Snp;

  Snp = (SNP_DRIVER *)(UINTN)UniqueId;
  //
  // tcpip was calling snp at tpl_notify and when we acquire a lock that was
  // created at a lower level (TPL_CALLBACK) it gives an assert!
  //
  if (Enable != 0) {
    EfiAcquireLock (&Snp->Lock);
  } else {
    EfiReleaseLock (&Snp->Lock);
  }
}

/**
  Delay MicroSeconds of micro seconds.

  This is a callback routine supplied to UNDI at undi_start time.

  @param UniqueId      This was supplied to UNDI at Undi_Start, SNP uses this to
                       store Undi interface context (Undi does not read or write
                       this variable).
  @param MicroSeconds  Number of micro seconds to pause, usually multiple of 10.

**/
VOID
EFIAPI
SnpUndi32CallbackDelay (
  IN UINT64  UniqueId,
  IN UINT64  MicroSeconds
  )
{
  if (MicroSeconds != 0) {
    gBS->Stall ((UINTN)MicroSeconds);
  }
}

/**
  IO routine for UNDI3.1.

  This is a callback routine supplied to UNDI at undi_start time.

  @param UniqueId       This was supplied to UNDI at Undi_Start, SNP uses this
                        to store Undi interface context (Undi does not read or
                        write this variable).
  @param ReadOrWrite    Indicates read or write, IO or Memory.
  @param NumBytes       Number of bytes to read or write.
  @param MemOrPortAddr  IO or memory address to read from or write to.
  @param BufferPtr      Memory location to read into or that contains the bytes
                        to write.

**/
VOID
EFIAPI
SnpUndi32CallbackMemio (
  IN UINT64      UniqueId,
  IN UINT8       ReadOrWrite,
  IN UINT8       NumBytes,
  IN UINT64      MemOrPortAddr,
  IN OUT UINT64  BufferPtr
  )
{
  SNP_DRIVER                 *Snp;
  EFI_PCI_IO_PROTOCOL_WIDTH  Width;

  Snp = (SNP_DRIVER *)(UINTN)UniqueId;

  Width = (EFI_PCI_IO_PROTOCOL_WIDTH)0;
  switch (NumBytes) {
    case 2:
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH)1;
      break;

    case 4:
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH)2;
      break;

    case 8:
      Width = (EFI_PCI_IO_PROTOCOL_WIDTH)3;
      break;
  }

  switch (ReadOrWrite) {
    case PXE_IO_READ:
      ASSERT (Snp->IoBarIndex < PCI_MAX_BAR);
      if (Snp->IoBarIndex < PCI_MAX_BAR) {
        Snp->PciIo->Io.Read (
                         Snp->PciIo,
                         Width,
                         Snp->IoBarIndex,    // BAR 1 (for 32bit regs), IO base address
                         MemOrPortAddr,
                         1,                  // count
                         (VOID *)(UINTN)BufferPtr
                         );
      }

      break;

    case PXE_IO_WRITE:
      ASSERT (Snp->IoBarIndex < PCI_MAX_BAR);
      if (Snp->IoBarIndex < PCI_MAX_BAR) {
        Snp->PciIo->Io.Write (
                         Snp->PciIo,
                         Width,
                         Snp->IoBarIndex,    // BAR 1 (for 32bit regs), IO base address
                         MemOrPortAddr,
                         1,                  // count
                         (VOID *)(UINTN)BufferPtr
                         );
      }

      break;

    case PXE_MEM_READ:
      ASSERT (Snp->MemoryBarIndex < PCI_MAX_BAR);
      if (Snp->MemoryBarIndex < PCI_MAX_BAR) {
        Snp->PciIo->Mem.Read (
                          Snp->PciIo,
                          Width,
                          Snp->MemoryBarIndex, // BAR 0, Memory base address
                          MemOrPortAddr,
                          1,                  // count
                          (VOID *)(UINTN)BufferPtr
                          );
      }

      break;

    case PXE_MEM_WRITE:
      ASSERT (Snp->MemoryBarIndex < PCI_MAX_BAR);
      if (Snp->MemoryBarIndex < PCI_MAX_BAR) {
        Snp->PciIo->Mem.Write (
                          Snp->PciIo,
                          Width,
                          Snp->MemoryBarIndex, // BAR 0, Memory base address
                          MemOrPortAddr,
                          1,                  // count
                          (VOID *)(UINTN)BufferPtr
                          );
      }

      break;
  }

  return;
}

/**
  Map a CPU address to a device address.

  This is a callback routine supplied to UNDI at undi_start time.

  @param UniqueId      This was supplied to UNDI at Undi_Start, SNP uses this to
                       store Undi interface context (Undi does not read or write
                       this variable).
  @param CpuAddr       Virtual address to be mapped.
  @param NumBytes      Size of memory to be mapped.
  @param Direction     Direction of data flow for this memory's usage:
                       cpu->device, device->cpu or both ways.
  @param DeviceAddrPtr Pointer to return the mapped device address.

**/
VOID
EFIAPI
SnpUndi32CallbackMap (
  IN UINT64      UniqueId,
  IN UINT64      CpuAddr,
  IN UINT32      NumBytes,
  IN UINT32      Direction,
  IN OUT UINT64  DeviceAddrPtr
  )
{
  EFI_PHYSICAL_ADDRESS           *DevAddrPtr;
  EFI_PCI_IO_PROTOCOL_OPERATION  DirectionFlag;
  UINTN                          BuffSize;
  SNP_DRIVER                     *Snp;
  UINTN                          Index;
  EFI_STATUS                     Status;

  BuffSize   = (UINTN)NumBytes;
  Snp        = (SNP_DRIVER *)(UINTN)UniqueId;
  DevAddrPtr = (EFI_PHYSICAL_ADDRESS *)(UINTN)DeviceAddrPtr;

  if (CpuAddr == 0) {
    *DevAddrPtr = 0;
    return;
  }

  switch (Direction) {
    case TO_AND_FROM_DEVICE:
      DirectionFlag = EfiPciIoOperationBusMasterCommonBuffer;
      break;

    case FROM_DEVICE:
      DirectionFlag = EfiPciIoOperationBusMasterWrite;
      break;

    case TO_DEVICE:
      DirectionFlag = EfiPciIoOperationBusMasterRead;
      break;

    default:
      *DevAddrPtr = 0;
      //
      // any non zero indicates error!
      //
      return;
  }

  //
  // find an unused map_list entry
  //
  for (Index = 0; Index < MAX_MAP_LENGTH; Index++) {
    if (Snp->MapList[Index].VirtualAddress == 0) {
      break;
    }
  }

  if (Index >= MAX_MAP_LENGTH) {
    DEBUG ((DEBUG_INFO, "SNP maplist is FULL\n"));
    *DevAddrPtr = 0;
    return;
  }

  Snp->MapList[Index].VirtualAddress = (EFI_PHYSICAL_ADDRESS)CpuAddr;

  Status = Snp->PciIo->Map (
                         Snp->PciIo,
                         DirectionFlag,
                         (VOID *)(UINTN)CpuAddr,
                         &BuffSize,
                         DevAddrPtr,
                         &(Snp->MapList[Index].MapCookie)
                         );
  if (Status != EFI_SUCCESS) {
    *DevAddrPtr                        = 0;
    Snp->MapList[Index].VirtualAddress = 0;
  }

  return;
}

/**
  Unmap an address that was previously mapped using map callback.

  This is a callback routine supplied to UNDI at undi_start time.

  @param UniqueId    This was supplied to UNDI at Undi_Start, SNP uses this to
                     store. Undi interface context (Undi does not read or write
                     this variable).
  @param CpuAddr     Virtual address that was mapped.
  @param NumBytes    Size of memory mapped.
  @param Direction   Direction of data flow for this memory's usage:
                     cpu->device, device->cpu or both ways.
  @param DeviceAddr  The mapped device address.

**/
VOID
EFIAPI
SnpUndi32CallbackUnmap (
  IN UINT64  UniqueId,
  IN UINT64  CpuAddr,
  IN UINT32  NumBytes,
  IN UINT32  Direction,
  IN UINT64  DeviceAddr
  )
{
  SNP_DRIVER  *Snp;
  UINT16      Index;

  Snp = (SNP_DRIVER *)(UINTN)UniqueId;

  for (Index = 0; Index < MAX_MAP_LENGTH; Index++) {
    if (Snp->MapList[Index].VirtualAddress == CpuAddr) {
      break;
    }
  }

  if (Index >= MAX_MAP_LENGTH) {
    DEBUG ((DEBUG_ERROR, "SNP could not find a mapping, failed to unmap.\n"));
    return;
  }

  Snp->PciIo->Unmap (Snp->PciIo, Snp->MapList[Index].MapCookie);
  Snp->MapList[Index].VirtualAddress = 0;
  Snp->MapList[Index].MapCookie      = NULL;
  return;
}

/**
  Synchronize the virtual buffer contents with the mapped buffer contents.

  This is a callback routine supplied to UNDI at undi_start time. The virtual
  and mapped buffers need not correspond to the same physical memory (especially
  if the virtual address is > 4GB). Depending on the direction for which the
  buffer is mapped, undi will need to synchronize their contents whenever it
  writes to/reads from the buffer using either the cpu address or the device
  address.
  EFI does not provide a sync call since virt=physical, we should just do the
  synchronization ourselves here.

  @param UniqueId    This was supplied to UNDI at Undi_Start, SNP uses this to
                     store Undi interface context (Undi does not read or write
                     this variable).
  @param CpuAddr     Virtual address that was mapped.
  @param NumBytes    Size of memory mapped.
  @param Direction   Direction of data flow for this memory's usage:
                     cpu->device, device->cpu or both ways.
  @param DeviceAddr  The mapped device address.

**/
VOID
EFIAPI
SnpUndi32CallbackSync (
  IN UINT64  UniqueId,
  IN UINT64  CpuAddr,
  IN UINT32  NumBytes,
  IN UINT32  Direction,
  IN UINT64  DeviceAddr
  )
{
  if ((CpuAddr == 0) || (DeviceAddr == 0) || (NumBytes == 0)) {
    return;
  }

  switch (Direction) {
    case FROM_DEVICE:
      CopyMem ((UINT8 *)(UINTN)CpuAddr, (UINT8 *)(UINTN)DeviceAddr, NumBytes);
      break;

    case TO_DEVICE:
      CopyMem ((UINT8 *)(UINTN)DeviceAddr, (UINT8 *)(UINTN)CpuAddr, NumBytes);
      break;
  }

  return;
}
