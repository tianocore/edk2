/** @file
 Module produces PciCfgPpi2 on top of PciCfgPpi. It also updates the 
 PciCfg2Ppi pointer in the EFI_PEI_SERVICES upon a installation of
 EcpPeiPciCfgPpi. 

 EcpPeiPciCfgPpi is installed by a framework module which
 produce PciCfgPpi originally. Such framework module is updated based on the 
 following rule to install EcpPeiPciCfgPpi instead of updating the PciCfg pointer
 in the Framework PeiServicesTable: 
 
 Search pattern:
 	   PeiServices->PciCfg = <*>;
 Replace pattern:
         {
           static EFI_PEI_PPI_DESCRIPTOR gEcpPeiPciCfgPpiList = {
             (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
             &gEcpPeiPciCfgPpiGuid,
             <*>
           };
           (**PeiServices).InstallPpi (PeiServices, gEcpPeiPciCfgPpiList);
         }
 
 In addition, the PeiServicesTable definition in PeiApi.h is updated to
 
 struct _EFI_PEI_SERVICES {
   EFI_TABLE_HEADER              Hdr;
   ...
 
   //
   // Pointer to PPI interface
   //
 if (PI_SPECIFICATION_VERSION < 0x00010000)
 
   PEI_CPU_IO_PPI                 *CpuIo;
   ECP_PEI_PCI_CFG_PPI        *PciCfg;  //Changed.
 else
 ...
 endif
 
 };
 
 This change enable the detection of code segment which invokes PeiServices->PciCfg->Modify.
 Such code causes a build break as ECP_PEI_PCI_CFG_PPI does not has "Modify" field. 
 This should be updated to a call to PeiLibPciCfgModify as shown below:
 
 Search pattern:
 		*->Modify(<*>); 
 Replace pattern:
 		PeiLibPciCfgModify(<*>);



PIWG's PI specification replaces Inte's EFI Specification 1.10.
EFI_PEI_PCI_CFG_PPI defined in Inte's EFI Specification 1.10 is replaced by
EFI_PEI_PCI_CFG2_PPI in PI 1.0.
This module produces PciCfgPpi on top of PciCfgPpi2. This module is used on platform when both of
these two conditions are true:
1) Framework module present that produces PCI CFG PPI  AND
2) PI module that produces PCI CFG2 is not present

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Ppi/PciCfg.h>
#include <Ppi/PciCfg2.h>
#include <Ppi/EcpPciCfg.h>
#include <Library/DebugLib.h>

//
// Function Prototypes 
//

/**
  Notification service to be called when gEcpPeiPciCfgPpiGuid is installed.

  @param  PeiServices                 Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor          Address of the notification descriptor data structure. Type
          EFI_PEI_NOTIFY_DESCRIPTOR is defined above.
  @param  Ppi                             Address of the PPI that was installed.

  @retval   EFI_STATUS                This function will install a PPI to PPI database. The status
                                                  code will be the code for (*PeiServices)->InstallPpi.

**/
EFI_STATUS
EFIAPI
EcpPciCfgPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  );

//
// Function Prototypes
//
/**
  Reads from a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                           The width of the access. Enumerated in bytes.
                                                   See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                       The physical address of the access. The format of
                                                  the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                           A pointer to the buffer of data..


  @retval EFI_SUCCESS                 The function completed successfully.

  @retval EFI_DEVICE_ERROR        There was a problem with the transaction.

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

  @param  PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                  Pointer to local data for the interface.

  @param  Width                 The width of the access. Enumerated in bytes.
                                See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address               The physical address of the access. The format of
                                the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                A pointer to the buffer of data..


  @retval EFI_SUCCESS           The function completed successfully.

  @retval EFI_DEVICE_ERROR      There was a problem with the transaction.

  @retval EFI_DEVICE_NOT_READY  The device is not capable of supporting the operation at this time.

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

  @param  PeiServices                     An indirect pointer to the PEI Services Table
                                                      published by the PEI Foundation.

  @param  This                                Pointer to local data for the interface.

  @param  Width                             The width of the access. Enumerated in bytes. Type
                                                      EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  @param  Address                           The physical address of the access.

  @param  SetBits                            Points to value to bitwise-OR with the read configuration value.
                                                      The size of the value is determined by Width.

  @param  ClearBits                         Points to the value to negate and bitwise-AND with the read configuration value.
                                                      The size of the value is determined by Width.


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

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

//
// Module globals
//
EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnEcpPciCfgList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEcpPeiPciCfgPpiGuid,
  EcpPciCfgPpiNotifyCallback 
};

EFI_PEI_PCI_CFG2_PPI mPciCfg2Ppi = {
  PciCfg2Read,
  PciCfg2Write,
  PciCfg2Modify,
  0
};

EFI_PEI_PPI_DESCRIPTOR mPpiListPciCfg2 = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciCfg2PpiGuid,
  &mPciCfg2Ppi
};


/**

  Standard PEIM entry point.

  @param FileHandle   Handle of the file being invoked.
  @param PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS The interface could be successfully installed.

**/
EFI_STATUS
EFIAPI
PeimInitializePciCfg2 (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;
  VOID        *Ppi;

  //
  // Make sure no other module has install the first instance of gEfiPciCfg2PpiGuid.
  //
  Status = (*PeiServices)->LocatePpi (PeiServices, &gEfiPciCfg2PpiGuid, 0, NULL, &Ppi);
  ASSERT (Status == EFI_NOT_FOUND);

  //
  // Register a notification for ECP PCI CFG PPI
  //
  Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyOnEcpPciCfgList);
  ASSERT_EFI_ERROR (Status);
  return Status;
}


/**
  Notification service to be called when gEcpPeiPciCfgPpiGuid is installed.

  @param  PeiServices                 Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor          Address of the notification descriptor data structure. Type
          EFI_PEI_NOTIFY_DESCRIPTOR is defined above.
  @param  Ppi                             Address of the PPI that was installed.

  @retval   EFI_STATUS                This function will install a PPI to PPI database. The status
                                                  code will be the code for (*PeiServices)->InstallPpi.

**/
EFI_STATUS
EFIAPI
EcpPciCfgPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  //
  // When ECP PCI CFG PPI is installed, publish the PCI CFG2 PPI in the 
  // PEI Services Table and the PPI database
  //
  (*PeiServices)->PciCfg = &mPciCfg2Ppi;
  return (*PeiServices)->InstallPpi ((CONST EFI_PEI_SERVICES **)PeiServices, &mPpiListPciCfg2);
}

/**
  Reads from a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                           The width of the access. Enumerated in bytes.
                                                   See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                       The physical address of the access. The format of
                                                  the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                           A pointer to the buffer of data..


  @retval EFI_SUCCESS                 The function completed successfully.

  @retval EFI_DEVICE_ERROR        There was a problem with the transaction.

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
  EFI_STATUS           Status;
  EFI_PEI_PCI_CFG_PPI  *PciCfg;

  Status = (*PeiServices)->LocatePpi (
             PeiServices,
             &gEcpPeiPciCfgPpiGuid,
             0,
             NULL,
             (VOID **)&PciCfg
             );
  ASSERT_EFI_ERROR (Status);

  return PciCfg->Read ((EFI_PEI_SERVICES **)PeiServices, PciCfg, Width, Address, Buffer);
}

/**
  Write to a given location in the PCI configuration space.

  @param  PeiServices                   An indirect pointer to the PEI Services Table published by the PEI Foundation.

  @param  This                              Pointer to local data for the interface.

  @param  Width                            The width of the access. Enumerated in bytes.
                                                    See EFI_PEI_PCI_CFG_PPI_WIDTH above.

  @param  Address                         The physical address of the access. The format of
                                                    the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.

  @param  Buffer                            A pointer to the buffer of data..


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

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
  EFI_STATUS           Status;
  EFI_PEI_PCI_CFG_PPI  *PciCfg;

  Status = (*PeiServices)->LocatePpi (
             PeiServices,
             &gEcpPeiPciCfgPpiGuid,
             0,
             NULL,
             (VOID **)&PciCfg
             );
  ASSERT_EFI_ERROR (Status);

  return PciCfg->Write ((EFI_PEI_SERVICES **)PeiServices, PciCfg, Width, Address, Buffer);
}

/**
  PCI read-modify-write operation.

  @param  PeiServices                     An indirect pointer to the PEI Services Table
                                                      published by the PEI Foundation.

  @param  This                                Pointer to local data for the interface.

  @param  Width                             The width of the access. Enumerated in bytes. Type
                                                      EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().

  @param  Address                           The physical address of the access.

  @param  SetBits                            Points to value to bitwise-OR with the read configuration value.
                                                      The size of the value is determined by Width.

  @param  ClearBits                         Points to the value to negate and bitwise-AND with the read configuration value.
                                                      The size of the value is determined by Width.


  @retval EFI_SUCCESS                   The function completed successfully.

  @retval EFI_DEVICE_ERROR          There was a problem with the transaction.

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
  EFI_STATUS           Status;
  EFI_PEI_PCI_CFG_PPI  *PciCfg;

  Status = (*PeiServices)->LocatePpi (
             PeiServices,
             &gEfiPciCfgPpiInServiceTableGuid,
             0,
             NULL,
             (VOID **)&PciCfg
             );
  ASSERT_EFI_ERROR (Status);

  return PciCfg->Modify ((EFI_PEI_SERVICES **)PeiServices, PciCfg, Width, Address, *(UINTN *)SetBits, *(UINTN *)ClearBits);
}
