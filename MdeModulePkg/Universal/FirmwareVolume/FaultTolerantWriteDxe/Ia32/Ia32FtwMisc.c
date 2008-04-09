/** @file
  
  Ia32 platform related code to support FtwLite.
  
Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

**/


#include <FtwLite.h>

//
// MACROs for boot block update
//
#define BOOT_BLOCK_BASE 0xFFFF0000

//
// (LPC -- D31:F0)
//
#define LPC_BUS_NUMBER    0x00
#define LPC_DEVICE_NUMBER 0x1F
#define LPC_IF            0xF0
//
// Top swap
//
#define GEN_STATUS    0xD4
#define TOP_SWAP_BIT  (1 << 13)

STATIC
UINT32
ReadPciRegister (
  IN UINT32                 Offset
  )
/*++

Routine Description:

  Read PCI register value.

Arguments:

  Offset  - Offset of the register

Returns:

  The value.

--*/
{
  EFI_STATUS                      Status;
  UINT32                          Value;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;

  Value   = 0;
  Status  = gBS->LocateProtocol (&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **) &PciRootBridgeIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Locate PCI root bridge io protocol - %r", Status));
    return 0;
  }

  Status = PciRootBridgeIo->Pci.Read (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint32,
                                  EFI_PCI_ADDRESS (
                                    LPC_BUS_NUMBER,
                                    LPC_DEVICE_NUMBER,
                                    LPC_IF,
                                    Offset
                                    ),
                                  1,
                                  &Value
                                  );
  ASSERT_EFI_ERROR (Status);

  return Value;
}

STATIC
EFI_STATUS
GetSwapState (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice,
  OUT BOOLEAN               *SwapState
  )
/*++

Routine Description:

  Get swap state

Arguments:

  FtwLiteDevice - Calling context
  SwapState     - Swap state

Returns:

  EFI_SUCCESS - State successfully got

--*/
{
  //
  // Top swap status is 13 bit
  //
  *SwapState = (BOOLEAN) ((ReadPciRegister (GEN_STATUS) & TOP_SWAP_BIT) != 0);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetSwapState (
  IN EFI_FTW_LITE_DEVICE    *FtwLiteDevice,
  IN  BOOLEAN               TopSwap
  )
/*++

Routine Description:
    Set swap state.

Arguments:
    FtwLiteDevice  - Indicates a pointer to the calling context.  
    TopSwap        - New swap state

Returns:
    EFI_SUCCESS   - The function completed successfully

Note:
    the Top-Swap bit (bit 13, D31: F0, Offset D4h). Note that
    software will not be able to clear the Top-Swap bit until the system is
    rebooted without GNT[A]# being pulled down.

--*/
{
  UINT32                          GenStatus;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeIo;
  EFI_STATUS                      Status;

  //
  // Top-Swap bit (bit 13, D31: F0, Offset D4h)
  //
  GenStatus = ReadPciRegister (GEN_STATUS);

  //
  // Set 13 bit, according to input NewSwapState
  //
  if (TopSwap) {
    GenStatus |= TOP_SWAP_BIT;
  } else {
    GenStatus &= ~TOP_SWAP_BIT;
  }

  Status = gBS->LocateProtocol (&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID **) &PciRootBridgeIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Locate PCI root bridge io protocol - %r", Status));
    return Status;
  }
  //
  // Write back the GenStatus register
  //
  Status = PciRootBridgeIo->Pci.Write (
                                  PciRootBridgeIo,
                                  EfiPciWidthUint32,
                                  EFI_PCI_ADDRESS (
                                    LPC_BUS_NUMBER,
                                    LPC_DEVICE_NUMBER,
                                    LPC_IF,
                                    GEN_STATUS
                                    ),
                                  1,
                                  &GenStatus
                                  );

  DEBUG_CODE_BEGIN ();
    if (TopSwap) {
      DEBUG ((EFI_D_ERROR, "SAR: Set top swap\n"));
    } else {
      DEBUG ((EFI_D_ERROR, "SAR: Clear top swap\n"));
    }
  DEBUG_CODE_END ();

  return EFI_SUCCESS;
}

BOOLEAN
IsBootBlock (
  EFI_FTW_LITE_DEVICE                 *FtwLiteDevice,
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvBlock,
  EFI_LBA                             Lba
  )
/*++

Routine Description:

  Check whether the block is a boot block.

Arguments:

  FtwLiteDevice - Calling context
  FvBlock       - Fvb protocol instance
  Lba           - Lba value

Returns:

  Is a boot block or not

--*/
{
  EFI_STATUS                          Status;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *BootFvb;

  Status = GetFvbByAddress (BOOT_BLOCK_BASE, &BootFvb);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  //
  // Compare the Fvb
  //
  return (BOOLEAN) (FvBlock == BootFvb);
}

EFI_STATUS
FlushSpareBlockToBootBlock (
  EFI_FTW_LITE_DEVICE      *FtwLiteDevice
  )
/*++

Routine Description:
    Copy the content of spare block to a boot block. Size is FTW_BLOCK_SIZE.
    Spare block is accessed by FTW backup FVB protocol interface. LBA is 
    FtwLiteDevice->FtwSpareLba.
    Boot block is accessed by BootFvb protocol interface. LBA is 0.

Arguments:
    FtwLiteDevice  - The private data of FTW_LITE driver

Returns:
    EFI_SUCCESS              - Spare block content is copied to boot block
    EFI_INVALID_PARAMETER    - Input parameter error
    EFI_OUT_OF_RESOURCES     - Allocate memory error
    EFI_ABORTED              - The function could not complete successfully

Notes:
    FTW will do extra work on boot block update.
    FTW should depend on a protocol of EFI_ADDRESS_RANGE_SWAP_PROTOCOL, 
    which is produced by a chipset driver.

    FTW updating boot block steps:
    1. Erase top swap block (0xFFFE-0xFFFEFFFF) and write data to it ready
    2. Read data from top swap block to memory buffer
    3. SetSwapState(EFI_SWAPPED)
    4. Erasing boot block (0xFFFF-0xFFFFFFFF)
    5. Programming boot block until the boot block is ok.
    6. SetSwapState(UNSWAPPED)

    Notes:
     1. Since the SwapState bit is saved in CMOS, FTW can restore and continue 
     even in the scenario of power failure.
     2. FTW shall not allow to update boot block when battery state is error.

--*/
{
  EFI_STATUS                          Status;
  UINTN                               Length;
  UINT8                               *Buffer;
  UINTN                               Count;
  UINT8                               *Ptr;
  UINTN                               Index;
  BOOLEAN                             TopSwap;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *BootFvb;
  EFI_LBA                             BootLba;

  //
  // Allocate a memory buffer
  //
  Length  = FtwLiteDevice->SpareAreaLength;
  Buffer  = AllocatePool (Length);
  if (Buffer == NULL) {
  }
  //
  // Get TopSwap bit state
  //
  Status = GetSwapState (FtwLiteDevice, &TopSwap);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "FtwLite: Get Top Swapped status - %r\n", Status));
    FreePool (Buffer);
    return EFI_ABORTED;
  }

  if (TopSwap) {
    //
    // Get FVB of current boot block
    //
    Status = GetFvbByAddress (FtwLiteDevice->SpareAreaAddress + FTW_BLOCK_SIZE, &BootFvb);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      return Status;
    }
    //
    // Read data from current boot block
    //
    BootLba = 0;
    Ptr     = Buffer;
    for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
      Count = FtwLiteDevice->SizeOfSpareBlock;
      Status = BootFvb->Read (
                          BootFvb,
                          BootLba + Index,
                          0,
                          &Count,
                          Ptr
                          );
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        return Status;
      }

      Ptr += Count;
    }

  } else {
    //
    // Read data from spare block
    //
    Ptr = Buffer;
    for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
      Count = FtwLiteDevice->SizeOfSpareBlock;
      Status = FtwLiteDevice->FtwBackupFvb->Read (
                                              FtwLiteDevice->FtwBackupFvb,
                                              FtwLiteDevice->FtwSpareLba + Index,
                                              0,
                                              &Count,
                                              Ptr
                                              );
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        return Status;
      }

      Ptr += Count;
    }
    //
    // Set TopSwap bit
    //
    Status = SetSwapState (FtwLiteDevice, TRUE);
    DEBUG ((EFI_D_ERROR, "FtwLite: Set Swap State - %r\n", Status));
    ASSERT_EFI_ERROR (Status);
  }
  //
  // Erase boot block. After setting TopSwap bit, it's spare block now!
  //
  Status = FtwEraseSpareBlock (FtwLiteDevice);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return EFI_ABORTED;
  }
  //
  // Write memory buffer to currenet spare block
  //
  Ptr = Buffer;
  for (Index = 0; Index < FtwLiteDevice->NumberOfSpareBlock; Index += 1) {
    Count = FtwLiteDevice->SizeOfSpareBlock;
    Status = FtwLiteDevice->FtwBackupFvb->Write (
                                            FtwLiteDevice->FtwBackupFvb,
                                            FtwLiteDevice->FtwSpareLba + Index,
                                            0,
                                            &Count,
                                            Ptr
                                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_FTW_LITE, "FtwLite: FVB Write boot block - %r\n", Status));
      FreePool (Buffer);
      return Status;
    }

    Ptr += Count;
  }

  FreePool (Buffer);

  //
  // Clear TopSwap bit
  //
  Status = SetSwapState (FtwLiteDevice, FALSE);
  DEBUG ((EFI_D_ERROR, "FtwLite: Clear Swap State - %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
