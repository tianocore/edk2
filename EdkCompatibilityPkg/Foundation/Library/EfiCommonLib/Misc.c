/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  misc.c

Abstract:
  
--*/

#include "Tiano.h"
#include "pei.h"
#include "cpuio.h"
#include EFI_PPI_CONSUMER (PciCfg)
#include EFI_PPI_CONSUMER (PciCfg2)
#include EFI_PROTOCOL_CONSUMER (PciRootBridgeIo)

//
// Modular variable used by common libiary in PEI phase
//
EFI_GUID              mPeiCpuIoPpiGuid  = PEI_CPU_IO_PPI_GUID;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
EFI_GUID              mPeiPciCfgPpiGuid = PEI_PCI_CFG_PPI_GUID;
PEI_PCI_CFG_PPI       *PciCfgPpi        = NULL;
#else
EFI_GUID              mPeiPciCfgPpiGuid = EFI_PEI_PCI_CFG2_PPI_GUID;
EFI_PEI_PCI_CFG2_PPI  *PciCfgPpi        = NULL;
#endif
EFI_PEI_SERVICES      **mPeiServices    = NULL;
PEI_CPU_IO_PPI        *CpuIoPpi         = NULL;

//
// Modular variable used by common libiary in DXE phase
//
EFI_SYSTEM_TABLE      *mST  = NULL;
EFI_BOOT_SERVICES     *mBS  = NULL;
EFI_RUNTIME_SERVICES  *mRT  = NULL;

EFI_STATUS
EfiInitializeCommonDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN VOID                 *SystemTable
  )
/*++

Routine Description:

  Initialize lib function calling phase: PEI or DXE
  
Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
{
  mPeiServices  = NULL;
  CpuIoPpi      = NULL;
  PciCfgPpi     = NULL;

  if (ImageHandle == NULL) {
    //
    // The function is called in PEI phase, use PEI interfaces
    //
    mPeiServices = (EFI_PEI_SERVICES **) SystemTable;
    ASSERT (mPeiServices == NULL);

    CpuIoPpi  = (**mPeiServices).CpuIo;
    PciCfgPpi = (**mPeiServices).PciCfg;

  } else {
    //
    // ImageHandle is not NULL. The function is called in DXE phase
    //
    mST = SystemTable;
    ASSERT (mST != NULL);

    mBS = mST->BootServices;
    mRT = mST->RuntimeServices;
    ASSERT (mBS != NULL);
    ASSERT (mRT != NULL);

    //
    // Should be at EFI_D_INFO, but lets us know things are running
    //
    DEBUG ((EFI_D_INFO, "EfiInitializeCommonDriverLib: Started in DXE\n"));
    return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EfiCommonIoWrite (
  IN  UINT8       Width,
  IN  UINTN       Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Io write operation.

Arguments:

  Width   - Width of write operation
  Address - Start IO address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns: 

  Status code

--*/
{
  EFI_STATUS                      Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *RootBridgeIo;

  if (mPeiServices != NULL) {
    //
    // The function is called in PEI phase, use PEI interfaces
    //
    Status = CpuIoPpi->Io.Write (
                            mPeiServices,
                            CpuIoPpi,
                            Width,
                            Address,
                            Count,
                            Buffer
                            );
  } else {
    //
    // The function is called in DXE phase
    //
    Status = mBS->LocateProtocol (
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    (VOID **) &RootBridgeIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = RootBridgeIo->Io.Write (RootBridgeIo, Width, Address, Count, Buffer);
  }

  return Status;
}


EFI_STATUS
EfiCommonIoRead (
  IN  UINT8       Width,
  IN  UINTN       Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Io read operation.

Arguments:

  Width   - Width of read operation
  Address - Start IO address to read
  Count   - Read count
  Buffer  - Buffer to store result

Returns: 

  Status code

--*/
{
  EFI_STATUS                      Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *RootBridgeIo;

  if (mPeiServices != NULL) {
    //
    // The function is called in PEI phase, use PEI interfaces
    //
    Status = CpuIoPpi->Io.Read (
                            mPeiServices,
                            CpuIoPpi,
                            Width,
                            Address,
                            Count,
                            Buffer
                            );
  } else {
    //
    // The function is called in DXE phase
    //
    Status = mBS->LocateProtocol (
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    (VOID **) &RootBridgeIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = RootBridgeIo->Io.Read (RootBridgeIo, Width, Address, Count, Buffer);
  }

  return Status;
}


EFI_STATUS
EfiCommonPciWrite (
  IN  UINT8       Width,
  IN  UINT64      Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Pci write operation

Arguments:

  Width   - Width of PCI write
  Address - PCI address to write
  Count   - Write count
  Buffer  - Buffer to write to the address

Returns: 

  Status code

--*/
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINT8                           *Buffer8;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *RootBridgeIo;

  if (mPeiServices != NULL) {
    //
    // The function is called in PEI phase, use PEI interfaces
    //
    Buffer8 = Buffer;
    for (Index = 0; Index < Count; Index++) {
      Status = PciCfgPpi->Write (
                            mPeiServices,
                            PciCfgPpi,
                            Width,
                            Address,
                            Buffer8
                            );

      if (EFI_ERROR (Status)) {
        return Status;
      }

      Buffer8 += Width;
    }

  } else {
    //
    // The function is called in DXE phase
    //
    Status = mBS->LocateProtocol (
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    (VOID **) &RootBridgeIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = RootBridgeIo->Pci.Write (
                                RootBridgeIo,
                                Width,
                                Address,
                                Count,
                                Buffer
                                );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EfiCommonPciRead (
  IN  UINT8       Width,
  IN  UINT64      Address,
  IN  UINTN       Count,
  IN  OUT VOID    *Buffer
  )
/*++

Routine Description:

  Pci read operation

Arguments:

  Width   - Width of PCI read
  Address - PCI address to read
  Count   - Read count
  Buffer  - Output buffer for the read

Returns: 

  Status code

--*/
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  UINT8                           *Buffer8;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *RootBridgeIo;

  if (mPeiServices != NULL) {
    //
    // The function is called in PEI phase, use PEI interfaces
    //
    Buffer8 = Buffer;
    for (Index = 0; Index < Count; Index++) {
      Status = PciCfgPpi->Read (
                            mPeiServices,
                            PciCfgPpi,
                            Width,
                            Address,
                            Buffer8
                            );

      if (EFI_ERROR (Status)) {
        return Status;
      }

      Buffer8 += Width;
    }

  } else {
    //
    // The function is called in DXE phase
    //
    Status = mBS->LocateProtocol (
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    (VOID **) &RootBridgeIo
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = RootBridgeIo->Pci.Read (
                                RootBridgeIo,
                                Width,
                                Address,
                                Count,
                                Buffer
                                );
  }

  return EFI_SUCCESS;
}
