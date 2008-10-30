/** @file
  Installs Single Segment Pci Configuration PPI.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Ppi/PciCfg2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>

#include <IndustryStandard/Pci.h>

/**
   Convert EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS to PCI_LIB_ADDRESS.

   @param Address   PCI address with
                    EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS format.

   @return The PCI address with PCI_LIB_ADDRESS format.

**/
UINTN
PciCfgAddressConvert (
  EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *Address
  )
{
  if (Address->ExtendedRegister == 0) {
    return PCI_LIB_ADDRESS (Address->Bus, Address->Device, Address->Function, Address->Register);
  }

  return PCI_LIB_ADDRESS (Address->Bus, Address->Device, Address->Function, Address->ExtendedRegister);
}

/**
  Reads from a given location in the PCI configuration space.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer          A pointer to the buffer of data..


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                time.

**/
EFI_STATUS
EFIAPI
PciCfg2Read (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
);

/**
  Write to a given location in the PCI configuration space.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer          A pointer to the buffer of data..


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                time.

**/
EFI_STATUS
EFIAPI
PciCfg2Write (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
);


/**
  PCI read-modify-write operation.

  @param  PeiServices     An indirect pointer to the PEI Services Table
                          published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes. Type
                          EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  @param  Address         The physical address of the access.

  @param  SetBits         Points to value to bitwise-OR with the read configuration value.

                          The size of the value is determined by Width.

  @param  ClearBits       Points to the value to negate and bitwise-AND with the read configuration value.
                          The size of the value is determined by Width.


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting
                                the operation at this time.

**/
EFI_STATUS
EFIAPI
PciCfg2Modify (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN        VOID                      *SetBits,
  IN        VOID                      *ClearBits
);



/**
  @par Ppi Description:
  The EFI_PEI_PCI_CFG2_PPI interfaces are used to abstract
  accesses to PCI controllers behind a PCI root bridge
  controller.

  @param Read     PCI read services.  See the Read() function description.

  @param Write    PCI write services.  See the Write() function description.

  @param Modify   PCI read-modify-write services.  See the Modify() function description.

  @param Segment  The PCI bus segment which the specified functions will access.

**/
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_PEI_PCI_CFG2_PPI gPciCfg2Ppi = {
  PciCfg2Read,
  PciCfg2Write,
  PciCfg2Modify
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_PEI_PPI_DESCRIPTOR gPciCfg2PpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciCfg2PpiGuid,
  &gPciCfg2Ppi
};

/**
  Reads from a given location in the PCI configuration space.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer          A pointer to the buffer of data..


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                time.

**/
EFI_STATUS
EFIAPI
PciCfg2Read (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
)
{
  UINTN  PciLibAddress;

  PciLibAddress = PciCfgAddressConvert ((EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *) &Address);

  if (Width == EfiPeiPciCfgWidthUint8) {
    *((UINT8 *) Buffer) = PciRead8 (PciLibAddress);
  } else if (Width == EfiPeiPciCfgWidthUint16) {
    *((UINT16 *) Buffer) = PciRead16 (PciLibAddress);
  } else if (Width == EfiPeiPciCfgWidthUint32) {
    *((UINT32 *) Buffer) = PciRead32 (PciLibAddress);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Write to a given location in the PCI configuration space.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer          A pointer to the buffer of data..


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this
                                time.

**/
EFI_STATUS
EFIAPI
PciCfg2Write (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
)
{
  UINTN  PciLibAddress;

  PciLibAddress = PciCfgAddressConvert ((EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *) &Address);

  if (Width == EfiPeiPciCfgWidthUint8) {
    PciWrite8 (PciLibAddress, *((UINT8 *) Buffer));
  } else if (Width == EfiPeiPciCfgWidthUint16) {
    PciWrite16 (PciLibAddress, *((UINT16 *) Buffer));
  } else if (Width == EfiPeiPciCfgWidthUint32) {
    PciWrite32 (PciLibAddress, *((UINT32 *) Buffer));
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
  PCI read-modify-write operation.

  @param  PeiServices     An indirect pointer to the PEI Services Table
                          published by the PEI Foundation.

  @param  This            Pointer to local data for the interface.

  @param  Width           The width of the access. Enumerated in bytes. Type
                          EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  @param  Address         The physical address of the access.

  @param  SetBits         Points to value to bitwise-OR with the read configuration value.

                          The size of the value is determined by Width.

  @param  ClearBits       Points to the value to negate and bitwise-AND with the read configuration value.
                          The size of the value is determined by Width.


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting
                                the operation at this time.

**/
EFI_STATUS
EFIAPI
PciCfg2Modify (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN        VOID                      *SetBits,
  IN        VOID                      *ClearBits
)
{
  UINTN   PciLibAddress;
  UINT16  ClearValue16;
  UINT16  SetValue16;
  UINT32  ClearValue32;
  UINT32  SetValue32;

  PciLibAddress = PciCfgAddressConvert ((EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS *) &Address);

  if (Width == EfiPeiPciCfgWidthUint8) {
    PciAndThenOr8 (PciLibAddress, (UINT8) (~(*(UINT8 *) ClearBits)), *((UINT8 *) SetBits));
  } else if (Width == EfiPeiPciCfgWidthUint16) {
    ClearValue16  = (UINT16) (~ReadUnaligned16 ((UINT16 *) ClearBits));
    SetValue16    = ReadUnaligned16 ((UINT16 *) SetBits);
    PciAndThenOr16 (PciLibAddress, ClearValue16, SetValue16);
  } else if (Width == EfiPeiPciCfgWidthUint32) {
    ClearValue32  = (UINT32) (~ReadUnaligned32 ((UINT32 *) ClearBits));
    SetValue32    = ReadUnaligned32 ((UINT32 *) SetBits);
    PciAndThenOr32 (PciLibAddress, ClearValue32, SetValue32);
  } else {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PeimInitializePciCfg (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS            Status;

  ASSERT ((**PeiServices).Hdr.Revision >= PEI_SERVICES_REVISION);

  (**PeiServices).PciCfg = &gPciCfg2Ppi;
  Status = (**PeiServices).InstallPpi ((CONST EFI_PEI_SERVICES **)PeiServices, &gPciCfg2PpiList);

  return Status;
}
