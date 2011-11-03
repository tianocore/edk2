/** @file
  This file contains two sets of callback routines for undi3.0 and undi3.1.
  the callback routines for Undi3.1 have an extra parameter UniqueId which
  stores the interface context for the NIC that snp is trying to talk.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Snp.h"

//
// Global variables
// these 2 global variables are used only for 3.0 undi. we could not place
// them in the snp structure because we will not know which snp structure
// in the callback context!
//
BOOLEAN              mInitializeLock = TRUE;
EFI_LOCK             mLock;

//
// End Global variables
//
extern EFI_PCI_IO_PROTOCOL  *mPciIo;

/**
  Convert a virtual or CPU address provided by SNP to a physical or device
  address.

  This is a callback routine supplied to UNDI at undi_start time. Since EFI uses
  the identical mapping, this routine returns the physical address same as the
  virtual address for most of the addresses. an address above 4GB cannot
  generally be used as a device address, it needs to be mapped to a lower
  physical address. This routine does not call the map routine itself, but it
  assumes that the mapping was done at the time of providing the address to
  UNDI. This routine just looks up the address in a map table (which is the v2p
  structure chain).

  @param  CpuAddr        Virtual address.
  @param  DeviceAddrPtr  Pointer to the physical address, or 0 in case of any
                         error.

**/
VOID
EFIAPI
SnpUndi32CallbackV2p30 (
  IN UINT64     CpuAddr,
  IN OUT UINT64 DeviceAddrPtr
  )
{
  V2P  *V2p;
  //
  // Do nothing if virtual address is zero or physical pointer is NULL.
  // No need to map if the virtual address is within 4GB limit since
  // EFI uses identical mapping
  //
  if ((CpuAddr == 0) || (DeviceAddrPtr == 0)) {
    DEBUG ((EFI_D_NET, "\nv2p: Null virtual address or physical pointer.\n"));
    return ;
  }

  if (CpuAddr < FOUR_GIGABYTES) {
    *(UINT64 *) (UINTN) DeviceAddrPtr = CpuAddr;
    return ;
  }
  //
  // SNP creates a vaddr tp paddr mapping at the time of calling undi with any
  // big address, this callback routine just looks up in the v2p list and
  // returns the physical address for any given virtual address.
  //
  if (FindV2p (&V2p, (VOID *) (UINTN) CpuAddr) != EFI_SUCCESS) {
    *(UINT64 *) (UINTN) DeviceAddrPtr = CpuAddr;
  } else {
    *(UINT64 *) (UINTN) DeviceAddrPtr = V2p->PhysicalAddress;
  }
}

/**
  Acquire or release a lock of an exclusive access to a critical section of the
  code/data.

  This is a callback routine supplied to UNDI at undi_start time.

  @param Enable   Non-zero indicates acquire; Zero indicates release.

**/
VOID
EFIAPI
SnpUndi32CallbackBlock30 (
  IN UINT32 Enable
  )
{
  //
  // tcpip was calling snp at tpl_notify and if we acquire a lock that was
  // created at a lower level (TPL_CALLBACK) it gives an assert!
  //
  if (mInitializeLock) {
    EfiInitializeLock (&mLock, TPL_NOTIFY);
    mInitializeLock = FALSE;
  }

  if (Enable != 0) {
    EfiAcquireLock (&mLock);
  } else {
    EfiReleaseLock (&mLock);
  }
}

/**
  Delay MicroSeconds of micro seconds.

  This is a callback routine supplied to UNDI at undi_start time.

  @param MicroSeconds  Number of micro seconds to pause, ususlly multiple of 10.

**/
VOID
EFIAPI
SnpUndi32CallbackDelay30 (
  IN UINT64 MicroSeconds
  )
{
  if (MicroSeconds != 0) {
    gBS->Stall ((UINTN) MicroSeconds);
  }
}

/**
  IO routine for UNDI.

  This is a callback routine supplied to UNDI at undi_start time. This is not
  currently being used by UNDI3.0 because Undi3.0 uses io/mem offsets relative
  to the beginning of the device io/mem address and so it needs to use the
  PCI_IO_FUNCTION that abstracts the start of the device's io/mem addresses.
  Since SNP cannot retrive the context of the undi3.0 interface it cannot use
  the PCI_IO_FUNCTION that specific for that NIC and uses one global IO
  functions structure, this does not work. This however works fine for EFI1.0
  Undis because they use absolute addresses for io/mem access.

  @param ReadOrWrite  Indicates read or write, IO or Memory.
  @param NumBytes     Number of bytes to read or write.
  @param Address      IO or memory address to read from or write to.
  @param BufferAddr   Memory location to read into or that contains the bytes to
                      write.

**/
VOID
EFIAPI
SnpUndi32CallbackMemio30 (
  IN UINT8      ReadOrWrite,
  IN UINT8      NumBytes,
  IN UINT64     Address,
  IN OUT UINT64 BufferAddr
  )
{
  EFI_PCI_IO_PROTOCOL_WIDTH Width;

  switch (NumBytes) {
  case 2:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 1;
    break;

  case 4:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 2;
    break;

  case 8:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 3;
    break;

  default:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 0;
  }

  switch (ReadOrWrite) {
  case PXE_IO_READ:
    mPciIo->Io.Read (
                 mPciIo,
                 Width,
                 1,    // BAR 1, IO base address
                 Address,
                 1,    // count
                 (VOID *) (UINTN) BufferAddr
                 );
    break;

  case PXE_IO_WRITE:
    mPciIo->Io.Write (
                 mPciIo,
                 Width,
                 1,    // BAR 1, IO base address
                 Address,
                 1,    // count
                 (VOID *) (UINTN) BufferAddr
                 );
    break;

  case PXE_MEM_READ:
    mPciIo->Mem.Read (
                  mPciIo,
                  Width,
                  0,  // BAR 0, Memory base address
                  Address,
                  1,  // count
                  (VOID *) (UINTN) BufferAddr
                  );
    break;

  case PXE_MEM_WRITE:
    mPciIo->Mem.Write (
                  mPciIo,
                  Width,
                  0,  // BAR 0, Memory base address
                  Address,
                  1,  // count
                  (VOID *) (UINTN) BufferAddr
                  );
    break;
  }

  return ;
}

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
  IN UINT64 UniqueId,
  IN UINT32 Enable
  )
{
  SNP_DRIVER  *Snp;

  Snp = (SNP_DRIVER *) (UINTN) UniqueId;
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
  @param MicroSeconds  Number of micro seconds to pause, ususlly multiple of 10.

**/
VOID
EFIAPI
SnpUndi32CallbackDelay (
  IN UINT64 UniqueId,
  IN UINT64 MicroSeconds
  )
{
  if (MicroSeconds != 0) {
    gBS->Stall ((UINTN) MicroSeconds);
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
  IN UINT64     UniqueId,
  IN UINT8      ReadOrWrite,
  IN UINT8      NumBytes,
  IN UINT64     MemOrPortAddr,
  IN OUT UINT64 BufferPtr
  )
{
  SNP_DRIVER                *Snp;
  EFI_PCI_IO_PROTOCOL_WIDTH Width;

  Snp   = (SNP_DRIVER *) (UINTN) UniqueId;

  Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 0;
  switch (NumBytes) {
  case 2:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 1;
    break;

  case 4:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 2;
    break;

  case 8:
    Width = (EFI_PCI_IO_PROTOCOL_WIDTH) 3;
    break;
  }

  switch (ReadOrWrite) {
  case PXE_IO_READ:
    Snp->PciIo->Io.Read (
                     Snp->PciIo,
                     Width,
                     Snp->IoBarIndex,      // BAR 1 (for 32bit regs), IO base address
                     MemOrPortAddr,
                     1,                    // count
                     (VOID *) (UINTN) BufferPtr
                     );
    break;

  case PXE_IO_WRITE:
    Snp->PciIo->Io.Write (
                     Snp->PciIo,
                     Width,
                     Snp->IoBarIndex,      // BAR 1 (for 32bit regs), IO base address
                     MemOrPortAddr,
                     1,                    // count
                     (VOID *) (UINTN) BufferPtr
                     );
    break;

  case PXE_MEM_READ:
    Snp->PciIo->Mem.Read (
                      Snp->PciIo,
                      Width,
                      Snp->MemoryBarIndex,  // BAR 0, Memory base address
                      MemOrPortAddr,
                      1,                    // count
                      (VOID *) (UINTN) BufferPtr
                      );
    break;

  case PXE_MEM_WRITE:
    Snp->PciIo->Mem.Write (
                      Snp->PciIo,
                      Width,
                      Snp->MemoryBarIndex,  // BAR 0, Memory base address
                      MemOrPortAddr,
                      1,                    // count
                      (VOID *) (UINTN) BufferPtr
                      );
    break;
  }

  return ;
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
  IN UINT64     UniqueId,
  IN UINT64     CpuAddr,
  IN UINT32     NumBytes,
  IN UINT32     Direction,
  IN OUT UINT64 DeviceAddrPtr
  )
{
  EFI_PHYSICAL_ADDRESS          *DevAddrPtr;
  EFI_PCI_IO_PROTOCOL_OPERATION DirectionFlag;
  UINTN                         BuffSize;
  SNP_DRIVER                    *Snp;
  UINTN                         Index;
  EFI_STATUS                    Status;

  BuffSize    = (UINTN) NumBytes;
  Snp         = (SNP_DRIVER *) (UINTN) UniqueId;
  DevAddrPtr  = (EFI_PHYSICAL_ADDRESS *) (UINTN) DeviceAddrPtr;

  if (CpuAddr == 0) {
    *DevAddrPtr = 0;
    return ;
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
    return ;
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
    DEBUG ((EFI_D_INFO, "SNP maplist is FULL\n"));
    *DevAddrPtr = 0;
    return ;
  }

  Snp->MapList[Index].VirtualAddress = (EFI_PHYSICAL_ADDRESS) CpuAddr;

  Status = Snp->PciIo->Map (
                         Snp->PciIo,
                         DirectionFlag,
                         (VOID *) (UINTN) CpuAddr,
                         &BuffSize,
                         DevAddrPtr,
                         &(Snp->MapList[Index].MapCookie)
                         );
  if (Status != EFI_SUCCESS) {
    *DevAddrPtr                        = 0;
    Snp->MapList[Index].VirtualAddress = 0;
  }

  return ;
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
  IN UINT64 UniqueId,
  IN UINT64 CpuAddr,
  IN UINT32 NumBytes,
  IN UINT32 Direction,
  IN UINT64 DeviceAddr
  )
{
  SNP_DRIVER  *Snp;
  UINT16      Index;

  Snp = (SNP_DRIVER *) (UINTN) UniqueId;

  for (Index = 0; Index < MAX_MAP_LENGTH; Index++) {
    if (Snp->MapList[Index].VirtualAddress == CpuAddr) {
      break;
    }
  }

  if (Index >= MAX_MAP_LENGTH) {
    DEBUG ((EFI_D_ERROR, "SNP could not find a mapping, failed to unmap.\n"));
    return ;
  }

  Snp->PciIo->Unmap (Snp->PciIo, Snp->MapList[Index].MapCookie);
  Snp->MapList[Index].VirtualAddress = 0;
  Snp->MapList[Index].MapCookie      = NULL;
  return ;
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
  IN UINT64             UniqueId,
  IN UINT64             CpuAddr,
  IN UINT32             NumBytes,
  IN UINT32             Direction,
  IN UINT64             DeviceAddr
  )
{
  if ((CpuAddr == 0) || (DeviceAddr == 0) || (NumBytes == 0)) {
    return ;

  }

  switch (Direction) {
  case FROM_DEVICE:
    CopyMem ((UINT8 *) (UINTN) CpuAddr, (UINT8 *) (UINTN) DeviceAddr, NumBytes);
    break;

  case TO_DEVICE:
    CopyMem ((UINT8 *) (UINTN) DeviceAddr, (UINT8 *) (UINTN) CpuAddr, NumBytes);
    break;
  }

  return ;
}
