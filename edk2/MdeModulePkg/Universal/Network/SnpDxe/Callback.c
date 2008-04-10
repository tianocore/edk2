/** @file
  This file contains two sets of callback routines for undi3.0 and undi3.1.
  the callback routines for Undi3.1 have an extra parameter UniqueId which
  stores the interface context for the NIC that snp is trying to talk.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
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
STATIC BOOLEAN              mInitializeLock = TRUE;
STATIC EFI_LOCK             mLock;

//
// End Global variables
//
extern EFI_PCI_IO_PROTOCOL  *mPciIoFncs;

VOID
snp_undi32_callback_v2p_30 (
  IN UINT64     CpuAddr,
  IN OUT UINT64 DeviceAddrPtr
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 UNDI call this routine with a virtual or CPU address that SNP provided
 to convert it to a physical or device address. Since EFI uses the identical
 mapping, this routine returns the physical address same as the virtual address
 for most of the addresses. an address above 4GB cannot generally be used as a
 device address, it needs to be mapped to a lower physical address. This routine
 does not call the map routine itself, but it assumes that the mapping was done
 at the time of providing the address to UNDI. This routine just looks up the
 address in a map table (which is the v2p structure chain)

Arguments:
 CpuAddr - virtual address of a buffer
 DeviceAddrPtr - pointer to the physical address

Returns:
 void - The DeviceAddrPtr will contain 0 in case of any error

--*/
{
  struct s_v2p  *v2p;
  //
  // Do nothing if virtual address is zero or physical pointer is NULL.
  // No need to map if the virtual address is within 4GB limit since
  // EFI uses identical mapping
  //
  if ((CpuAddr == 0) || (DeviceAddrPtr == 0)) {
    DEBUG ((EFI_D_ERROR, "\nv2p: Null virtual address or physical pointer.\n"));
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
  if (find_v2p (&v2p, (VOID *) (UINTN) CpuAddr) != EFI_SUCCESS) {
    *(UINT64 *) (UINTN) DeviceAddrPtr = CpuAddr;
  } else {
    *(UINT64 *) (UINTN) DeviceAddrPtr = v2p->paddr;
  }
}

VOID
snp_undi32_callback_block_30 (
  IN UINT32 Enable
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 UNDI call this routine when it wants to have exclusive access to a critical
 section of the code/data

Arguments:
 Enable - non-zero indicates acquire
          zero indicates release

Returns:
 void
--*/
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

VOID
snp_undi32_callback_delay_30 (
  IN UINT64 MicroSeconds
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 UNDI call this routine with the number of micro seconds when it wants to
 pause.

Arguments:
 MicroSeconds - number of micro seconds to pause, ususlly multiple of 10

Returns:
 void
--*/
{
  if (MicroSeconds != 0) {
    gBS->Stall ((UINTN) MicroSeconds);
  }
}

VOID
snp_undi32_callback_memio_30 (
  IN UINT8      ReadOrWrite,
  IN UINT8      NumBytes,
  IN UINT64     Address,
  IN OUT UINT64 BufferAddr
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 This is the IO routine for UNDI. This is not currently being used by UNDI3.0
 because Undi3.0 uses io/mem offsets relative to the beginning of the device
 io/mem address and so it needs to use the PCI_IO_FUNCTION that abstracts the
 start of the device's io/mem addresses. Since SNP cannot retrive the context
 of the undi3.0 interface it cannot use the PCI_IO_FUNCTION that specific for
 that NIC and uses one global IO functions structure, this does not work.
 This however works fine for EFI1.0 Undis because they use absolute addresses
 for io/mem access.

Arguments:
  ReadOrWrite - indicates read or write, IO or Memory
  NumBytes    - number of bytes to read or write
  Address     - IO or memory address to read from or write to
  BufferAddr  - memory location to read into or that contains the bytes
                to write

Returns:

--*/
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
    mPciIoFncs->Io.Read (
                    mPciIoFncs,
                    Width,
                    1,    // BAR 1, IO base address
                    Address,
                    1,    // count
                    (VOID *) (UINTN) BufferAddr
                    );
    break;

  case PXE_IO_WRITE:
    mPciIoFncs->Io.Write (
                    mPciIoFncs,
                    Width,
                    1,    // BAR 1, IO base address
                    Address,
                    1,    // count
                    (VOID *) (UINTN) BufferAddr
                    );
    break;

  case PXE_MEM_READ:
    mPciIoFncs->Mem.Read (
                      mPciIoFncs,
                      Width,
                      0,  // BAR 0, Memory base address
                      Address,
                      1,  // count
                      (VOID *) (UINTN) BufferAddr
                      );
    break;

  case PXE_MEM_WRITE:
    mPciIoFncs->Mem.Write (
                      mPciIoFncs,
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
//
// New callbacks for 3.1:
// there won't be a virtual2physical callback for UNDI 3.1 because undi3.1 uses
// the MemMap call to map the required address by itself!
//
VOID
snp_undi32_callback_block (
  IN UINT64 UniqueId,
  IN UINT32 Enable
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI3.1 at undi_start time.
 UNDI call this routine when it wants to have exclusive access to a critical
 section of the code/data

Arguments:
 UniqueId - This was supplied to UNDI at Undi_Start, SNP uses this to store
            Undi interface context (Undi does not read or write this variable)
 Enable   - non-zero indicates acquire
            zero indicates release

Returns:
 void

--*/
{
  SNP_DRIVER  *snp;

  snp = (SNP_DRIVER *) (UINTN) UniqueId;
  //
  // tcpip was calling snp at tpl_notify and when we acquire a lock that was
  // created at a lower level (TPL_CALLBACK) it gives an assert!
  //
  if (Enable != 0) {
    EfiAcquireLock (&snp->lock);
  } else {
    EfiReleaseLock (&snp->lock);
  }
}

VOID
snp_undi32_callback_delay (
  IN UINT64 UniqueId,
  IN UINT64 MicroSeconds
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 UNDI call this routine with the number of micro seconds when it wants to
 pause.

Arguments:
 MicroSeconds - number of micro seconds to pause, ususlly multiple of 10

Returns:
 void
--*/
{
  if (MicroSeconds != 0) {
    gBS->Stall ((UINTN) MicroSeconds);
  }
}

/*
 *  IO routine for UNDI start CPB.
 */
VOID
snp_undi32_callback_memio (
  UINT64 UniqueId,
  UINT8  ReadOrWrite,
  UINT8  NumBytes,
  UINT64 Address,
  UINT64 BufferAddr
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 This is the IO routine for UNDI3.1.

Arguments:
  ReadOrWrite - indicates read or write, IO or Memory
  NumBytes    - number of bytes to read or write
  Address     - IO or memory address to read from or write to
  BufferAddr  - memory location to read into or that contains the bytes
                to write

Returns:

--*/
{
  SNP_DRIVER                *snp;
  EFI_PCI_IO_PROTOCOL_WIDTH Width;

  snp   = (SNP_DRIVER *) (UINTN) UniqueId;

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
    snp->IoFncs->Io.Read (
                      snp->IoFncs,
                      Width,
                      snp->IoBarIndex,      // BAR 1 (for 32bit regs), IO base address
                      Address,
                      1,                    // count
                      (VOID *) (UINTN) BufferAddr
                      );
    break;

  case PXE_IO_WRITE:
    snp->IoFncs->Io.Write (
                      snp->IoFncs,
                      Width,
                      snp->IoBarIndex,      // BAR 1 (for 32bit regs), IO base address
                      Address,
                      1,                    // count
                      (VOID *) (UINTN) BufferAddr
                      );
    break;

  case PXE_MEM_READ:
    snp->IoFncs->Mem.Read (
                      snp->IoFncs,
                      Width,
                      snp->MemoryBarIndex,  // BAR 0, Memory base address
                      Address,
                      1,                    // count
                      (VOID *) (UINTN) BufferAddr
                      );
    break;

  case PXE_MEM_WRITE:
    snp->IoFncs->Mem.Write (
                      snp->IoFncs,
                      Width,
                      snp->MemoryBarIndex,  // BAR 0, Memory base address
                      Address,
                      1,                    // count
                      (VOID *) (UINTN) BufferAddr
                      );
    break;
  }

  return ;
}

VOID
snp_undi32_callback_map (
  IN UINT64     UniqueId,
  IN UINT64     CpuAddr,
  IN UINT32     NumBytes,
  IN UINT32     Direction,
  IN OUT UINT64 DeviceAddrPtr
  )
/*++

Routine Description:
  This is a callback routine supplied to UNDI at undi_start time.
  UNDI call this routine when it has to map a CPU address to a device
  address.

Arguments:
  UniqueId      - This was supplied to UNDI at Undi_Start, SNP uses this to store
                  Undi interface context (Undi does not read or write this variable)
  CpuAddr       - Virtual address to be mapped!
  NumBytes      - size of memory to be mapped
  Direction     - direction of data flow for this memory's usage:
                  cpu->device, device->cpu or both ways
  DeviceAddrPtr - pointer to return the mapped device address

Returns:
  None

--*/
{
  EFI_PHYSICAL_ADDRESS          *DevAddrPtr;
  EFI_PCI_IO_PROTOCOL_OPERATION DirectionFlag;
  UINTN                         BuffSize;
  SNP_DRIVER                    *snp;
  UINTN                         Index;
  EFI_STATUS                    Status;

  BuffSize    = (UINTN) NumBytes;
  snp         = (SNP_DRIVER *) (UINTN) UniqueId;
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
    if (snp->map_list[Index].virt == 0) {
      break;
    }
  }

  if (Index >= MAX_MAP_LENGTH) {
    DEBUG ((EFI_D_INFO, "SNP maplist is FULL\n"));
    *DevAddrPtr = 0;
    return ;
  }

  snp->map_list[Index].virt = (EFI_PHYSICAL_ADDRESS) CpuAddr;

  Status = snp->IoFncs->Map (
                          snp->IoFncs,
                          DirectionFlag,
                          (VOID *) (UINTN) CpuAddr,
                          &BuffSize,
                          DevAddrPtr,
                          &(snp->map_list[Index].map_cookie)
                          );
  if (Status != EFI_SUCCESS) {
    *DevAddrPtr               = 0;
    snp->map_list[Index].virt = 0;
  }

  return ;
}

VOID
snp_undi32_callback_unmap (
  IN UINT64 UniqueId,
  IN UINT64 CpuAddr,
  IN UINT32 NumBytes,
  IN UINT32 Direction,
  IN UINT64 DeviceAddr
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 UNDI call this routine when it wants to unmap an address that was previously
 mapped using map callback

Arguments:
 UniqueId - This was supplied to UNDI at Undi_Start, SNP uses this to store
            Undi interface context (Undi does not read or write this variable)
 CpuAddr  - Virtual address that was mapped!
 NumBytes - size of memory mapped
 Direction- direction of data flow for this memory's usage:
            cpu->device, device->cpu or both ways
 DeviceAddr - the mapped device address

Returns:

--*/
{
  SNP_DRIVER  *snp;
  UINT16      Index;

  snp = (SNP_DRIVER *) (UINTN) UniqueId;

  for (Index = 0; Index < MAX_MAP_LENGTH; Index++) {
    if (snp->map_list[Index].virt == CpuAddr) {
      break;
    }
  }

  if (Index >= MAX_MAP_LENGTH)
  {
    DEBUG ((EFI_D_ERROR, "SNP could not find a mapping, failed to unmap.\n"));
    return ;
  }

  snp->IoFncs->Unmap (snp->IoFncs, snp->map_list[Index].map_cookie);
  snp->map_list[Index].virt       = 0;
  snp->map_list[Index].map_cookie = NULL;
  return ;
}

VOID
snp_undi32_callback_sync (
  UINT64 UniqueId,
  UINT64 CpuAddr,
  UINT32 NumBytes,
  UINT32 Direction,
  UINT64 DeviceAddr
  )
/*++

Routine Description:
 This is a callback routine supplied to UNDI at undi_start time.
 UNDI call this routine when it wants synchronize the virtual buffer contents
 with the mapped buffer contents. The virtual and mapped buffers need not
 correspond to the same physical memory (especially if the virtual address is
 > 4GB). Depending on the direction for which the buffer is mapped, undi will
 need to synchronize their contents whenever it writes to/reads from the buffer
 using either the cpu address or the device address.

 EFI does not provide a sync call, since virt=physical, we sould just do
 the synchronization ourself here!

Arguments:
 UniqueId - This was supplied to UNDI at Undi_Start, SNP uses this to store
            Undi interface context (Undi does not read or write this variable)
 CpuAddr  - Virtual address that was mapped!
 NumBytes - size of memory mapped
 Direction- direction of data flow for this memory's usage:
            cpu->device, device->cpu or both ways
 DeviceAddr - the mapped device address

Returns:

--*/
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
