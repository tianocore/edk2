/** @file
PEIM to produce gPeiUsbHostControllerPpiGuid based on gPeiUsbControllerPpiGuid
which is used to enable recovery function from USB Drivers.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved. <BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UhcPeim.h"

/**
  Initializes Usb Host Controller.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            PPI successfully installed.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resource.

**/
EFI_STATUS
EFIAPI
UhcPeimEntry (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  PEI_USB_CONTROLLER_PPI      *ChipSetUsbControllerPpi;
  EFI_STATUS                  Status;
  UINT8                       Index;
  UINTN                       ControllerType;
  UINTN                       BaseAddress;
  UINTN                       MemPages;
  USB_UHC_DEV                 *UhcDev;
  EFI_PHYSICAL_ADDRESS        TempPtr;

  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  Status = PeiServicesLocatePpi (
             &gPeiUsbControllerPpiGuid,
             0,
             NULL,
             (VOID **) &ChipSetUsbControllerPpi
             );
  //
  // If failed to locate, it is a bug in dispather as depex has gPeiUsbControllerPpiGuid.
  //
  ASSERT_EFI_ERROR (Status);

  Index = 0;
  while (TRUE) {
    Status = ChipSetUsbControllerPpi->GetUsbController (
                                        (EFI_PEI_SERVICES **) PeiServices,
                                        ChipSetUsbControllerPpi,
                                        Index,
                                        &ControllerType,
                                        &BaseAddress
                                        );
    //
    // When status is error, meant no controller is found
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // This PEIM is for UHC type controller.
    //
    if (ControllerType != PEI_UHCI_CONTROLLER) {
      Index++;
      continue;
    }

    MemPages = sizeof (USB_UHC_DEV) / EFI_PAGE_SIZE + 1;

    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               MemPages,
               &TempPtr
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    UhcDev = (USB_UHC_DEV *) ((UINTN) TempPtr);
    UhcDev->Signature   = USB_UHC_DEV_SIGNATURE;
    UhcDev->UsbHostControllerBaseAddress = (UINT32) BaseAddress;

    //
    // Init local memory management service
    //
    Status = InitializeMemoryManagement (UhcDev);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Initialize Uhc's hardware
    //
    Status = InitializeUsbHC (UhcDev);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    UhcDev->UsbHostControllerPpi.ControlTransfer          = UhcControlTransfer;
    UhcDev->UsbHostControllerPpi.BulkTransfer             = UhcBulkTransfer;
    UhcDev->UsbHostControllerPpi.GetRootHubPortNumber     = UhcGetRootHubPortNumber;
    UhcDev->UsbHostControllerPpi.GetRootHubPortStatus     = UhcGetRootHubPortStatus;
    UhcDev->UsbHostControllerPpi.SetRootHubPortFeature    = UhcSetRootHubPortFeature;
    UhcDev->UsbHostControllerPpi.ClearRootHubPortFeature  = UhcClearRootHubPortFeature;

    UhcDev->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    UhcDev->PpiDescriptor.Guid  = &gPeiUsbHostControllerPpiGuid;
    UhcDev->PpiDescriptor.Ppi   = &UhcDev->UsbHostControllerPpi;

    Status = PeiServicesInstallPpi (&UhcDev->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      Index++;
      continue;
    }

    Index++;
  }

  return EFI_SUCCESS;
}

/**
  Submits control transfer to a target USB device.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress          The target device address.
  @param  DeviceSpeed            Target device speed.
  @param  MaximumPacketLength    Maximum packet size the default control transfer 
                                 endpoint is capable of sending or receiving.
  @param  Request                USB device request to send.
  @param  TransferDirection      Specifies the data direction for the data stage.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.
  @param  TimeOut                Indicates the maximum timeout, in millisecond.
  @param  TransferResult         Return the result of this control transfer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
UhcControlTransfer (
  IN     EFI_PEI_SERVICES               **PeiServices,
  IN     PEI_USB_HOST_CONTROLLER_PPI    *This,
  IN     UINT8                          DeviceAddress,
  IN     UINT8                          DeviceSpeed,
  IN     UINT8                          MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST         *Request,
  IN     EFI_USB_DATA_DIRECTION         TransferDirection,
  IN OUT VOID                           *Data                 OPTIONAL,
  IN OUT UINTN                          *DataLength           OPTIONAL,
  IN     UINTN                          TimeOut,
  OUT    UINT32                         *TransferResult
  )
{
  USB_UHC_DEV *UhcDev;
  UINT32      StatusReg;
  UINT8       PktID;
  QH_STRUCT   *PtrQH;
  TD_STRUCT   *PtrTD;
  TD_STRUCT   *PtrPreTD;
  TD_STRUCT   *PtrSetupTD;
  TD_STRUCT   *PtrStatusTD;
  EFI_STATUS  Status;
  UINT32      DataLen;
  UINT8       *PtrDataSource;
  UINT8       *Ptr;
  UINT8       DataToggle;

  UhcDev      = PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS (This);

  StatusReg   = UhcDev->UsbHostControllerBaseAddress + USBSTS;

  PktID       = INPUT_PACKET_ID;

  if (Request == NULL || TransferResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // if errors exist that cause host controller halt,
  // then return EFI_DEVICE_ERROR.
  //

  if (!IsStatusOK (UhcDev, StatusReg)) {
    ClearStatusReg (UhcDev, StatusReg);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (UhcDev, StatusReg);

  //
  // generate Setup Stage TD
  //

  PtrQH = UhcDev->ConfigQH;

  GenSetupStageTD (
    UhcDev,
    DeviceAddress,
    0,
    DeviceSpeed,
    (UINT8 *) Request,
    (UINT8) sizeof (EFI_USB_DEVICE_REQUEST),
    &PtrSetupTD
    );

  //
  // link setup TD structures to QH structure
  //
  LinkTDToQH (PtrQH, PtrSetupTD);

  PtrPreTD = PtrSetupTD;

  //
  //  Data Stage of Control Transfer
  //
  switch (TransferDirection) {

  case EfiUsbDataIn:
    PktID         = INPUT_PACKET_ID;
    PtrDataSource = Data;
    DataLen       = (UINT32) *DataLength;
    Ptr           = PtrDataSource;
    break;

  case EfiUsbDataOut:
    PktID         = OUTPUT_PACKET_ID;
    PtrDataSource = Data;
    DataLen       = (UINT32) *DataLength;
    Ptr           = PtrDataSource;
    break;

  //
  // no data stage
  //
  case EfiUsbNoData:
    if (*DataLength != 0) {
      return EFI_INVALID_PARAMETER;
    }

    PktID         = OUTPUT_PACKET_ID;
    PtrDataSource = NULL;
    DataLen       = 0;
    Ptr           = NULL;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  DataToggle  = 1;

  PtrTD       = PtrSetupTD;
  while (DataLen > 0) {
    //
    // create TD structures and link together
    //
    UINT8 PacketSize;

    //
    // PacketSize is the data load size of each TD carries.
    //
    PacketSize = (UINT8) DataLen;
    if (DataLen > MaximumPacketLength) {
      PacketSize = MaximumPacketLength;
    }

    GenDataTD (
      UhcDev,
      DeviceAddress,
      0,
      Ptr,
      PacketSize,
      PktID,
      DataToggle,
      DeviceSpeed,
      &PtrTD
      );

    //
    // Link two TDs in vertical depth
    //
    LinkTDToTD (PtrPreTD, PtrTD);
    PtrPreTD = PtrTD;

    DataToggle ^= 1;
    Ptr += PacketSize;
    DataLen -= PacketSize;
  }

  //
  // PtrPreTD points to the last TD before the Setup-Stage TD.
  //
  PtrPreTD = PtrTD;

  //
  // Status Stage of Control Transfer
  //
  if (PktID == OUTPUT_PACKET_ID) {
    PktID = INPUT_PACKET_ID;
  } else {
    PktID = OUTPUT_PACKET_ID;
  }
  //
  // create Status Stage TD structure
  //
  CreateStatusTD (
    UhcDev,
    DeviceAddress,
    0,
    PktID,
    DeviceSpeed,
    &PtrStatusTD
    );

  LinkTDToTD (PtrPreTD, PtrStatusTD);

  //
  // Poll QH-TDs execution and get result.
  // detail status is returned
  //
  Status = ExecuteControlTransfer (
            UhcDev,
            PtrSetupTD,
            DataLength,
            TimeOut,
            TransferResult
            );

  //
  // TRUE means must search other framelistindex
  //
  SetQHVerticalValidorInvalid(PtrQH, FALSE);
  DeleteQueuedTDs (UhcDev, PtrSetupTD);

  //
  // if has errors that cause host controller halt, then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (UhcDev, StatusReg)) {

    ClearStatusReg (UhcDev, StatusReg);
    *TransferResult |= EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (UhcDev, StatusReg);

  return Status;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  MaximumPacketLength   Maximum packet size the endpoint is capable of 
                                sending or receiving.
  @param  Data                  Array of pointers to the buffers of data to transmit 
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to use of 
                                the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
UhcBulkTransfer (
  IN     EFI_PEI_SERVICES               **PeiServices,
  IN     PEI_USB_HOST_CONTROLLER_PPI    *This,
  IN     UINT8                          DeviceAddress,
  IN     UINT8                          EndPointAddress,
  IN     UINT8                          MaximumPacketLength,
  IN OUT VOID                           *Data,
  IN OUT UINTN                          *DataLength,
  IN OUT UINT8                          *DataToggle,
  IN     UINTN                          TimeOut,
  OUT    UINT32                         *TransferResult
  )
{
  USB_UHC_DEV             *UhcDev;
  UINT32                  StatusReg;

  UINT32                  DataLen;

  QH_STRUCT               *PtrQH;
  TD_STRUCT               *PtrFirstTD;
  TD_STRUCT               *PtrTD;
  TD_STRUCT               *PtrPreTD;

  UINT8                   PktID;
  UINT8                   *PtrDataSource;
  UINT8                   *Ptr;

  BOOLEAN                 IsFirstTD;

  EFI_STATUS              Status;

  EFI_USB_DATA_DIRECTION  TransferDirection;

  BOOLEAN                 ShortPacketEnable;

  UINT16                  CommandContent;

  UhcDev = PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS (This);

  //
  // Enable the maximum packet size (64bytes)
  // that can be used for full speed bandwidth reclamation
  // at the end of a frame.
  //
  CommandContent = USBReadPortW (UhcDev, UhcDev->UsbHostControllerBaseAddress + USBCMD);
  if ((CommandContent & USBCMD_MAXP) != USBCMD_MAXP) {
    CommandContent |= USBCMD_MAXP;
    USBWritePortW (UhcDev, UhcDev->UsbHostControllerBaseAddress + USBCMD, CommandContent);
  }

  StatusReg   = UhcDev->UsbHostControllerBaseAddress + USBSTS;

  //
  // these code lines are added here per complier's strict demand
  //
  PktID             = INPUT_PACKET_ID;
  PtrTD             = NULL;
  PtrFirstTD        = NULL;
  PtrPreTD          = NULL;
  DataLen           = 0;
  Ptr               = NULL;

  ShortPacketEnable = FALSE;

  if ((DataLength == 0) || (Data == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MaximumPacketLength != 8 && MaximumPacketLength != 16
      && MaximumPacketLength != 32 && MaximumPacketLength != 64) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // if has errors that cause host controller halt, then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (UhcDev, StatusReg)) {

    ClearStatusReg (UhcDev, StatusReg);
    *TransferResult = EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (UhcDev, StatusReg);

  if ((EndPointAddress & 0x80) != 0) {
    TransferDirection = EfiUsbDataIn;
  } else {
    TransferDirection = EfiUsbDataOut;
  }

  switch (TransferDirection) {

  case EfiUsbDataIn:
    ShortPacketEnable = TRUE;
    PktID             = INPUT_PACKET_ID;
    PtrDataSource     = Data;
    DataLen           = (UINT32) *DataLength;
    Ptr               = PtrDataSource;
    break;

  case EfiUsbDataOut:
    PktID         = OUTPUT_PACKET_ID;
    PtrDataSource = Data;
    DataLen       = (UINT32) *DataLength;
    Ptr           = PtrDataSource;
    break;

  default:
    break;
  }

  PtrQH = UhcDev->BulkQH;

  IsFirstTD = TRUE;
  while (DataLen > 0) {
    //
    // create TD structures and link together
    //
    UINT8 PacketSize;

    PacketSize = (UINT8) DataLen;
    if (DataLen > MaximumPacketLength) {
      PacketSize = MaximumPacketLength;
    }

    GenDataTD (
      UhcDev,
      DeviceAddress,
      EndPointAddress,
      Ptr,
      PacketSize,
      PktID,
      *DataToggle,
      USB_FULL_SPEED_DEVICE,
      &PtrTD
      );

    //
    // Enable short packet detection.
    // (default action is disabling short packet detection)
    //
    if (ShortPacketEnable) {
      EnableorDisableTDShortPacket (PtrTD, TRUE);
    }

    if (IsFirstTD) {
      PtrFirstTD            = PtrTD;
      PtrFirstTD->PtrNextTD = NULL;
      IsFirstTD             = FALSE;
    } else {
      //
      // Link two TDs in vertical depth
      //
      LinkTDToTD (PtrPreTD, PtrTD);
    }

    PtrPreTD = PtrTD;

    *DataToggle ^= 1;
    Ptr += PacketSize;
    DataLen -= PacketSize;
  }
  //
  // link TD structures to QH structure
  //
  LinkTDToQH (PtrQH, PtrFirstTD);

  //
  // Execute QH-TD and get result
  //
  //
  // detail status is put into the Result field in the pIRP
  // the Data Toggle value is also re-updated to the value
  // of the last successful TD
  //
  Status = ExecBulkTransfer (
            UhcDev,
            PtrFirstTD,
            DataLength,
            DataToggle,
            TimeOut,
            TransferResult
            );

  //
  // Delete Bulk transfer TD structure
  //
  DeleteQueuedTDs (UhcDev, PtrFirstTD);

  //
  // if has errors that cause host controller halt, then return EFI_DEVICE_ERROR directly.
  //
  if (!IsStatusOK (UhcDev, StatusReg)) {

    ClearStatusReg (UhcDev, StatusReg);
    *TransferResult |= EFI_USB_ERR_SYSTEM;
    return EFI_DEVICE_ERROR;
  }

  ClearStatusReg (UhcDev, StatusReg);

  return Status;
}

/**
  Retrieves the number of root hub ports.

  @param[in]  PeiServices   The pointer to the PEI Services Table.
  @param[in]  This          The pointer to this instance of the 
                            PEI_USB_HOST_CONTROLLER_PPI.
  @param[out] PortNumber    The pointer to the number of the root hub ports.                                
                                
  @retval EFI_SUCCESS           The port number was retrieved successfully.
  @retval EFI_INVALID_PARAMETER PortNumber is NULL.

**/
EFI_STATUS
EFIAPI
UhcGetRootHubPortNumber (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *This,
  OUT UINT8                         *PortNumber
  )
{
  USB_UHC_DEV *UhcDev;
  UINT32      PSAddr;
  UINT16      RHPortControl;
  UINT32      Index;

  UhcDev = PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS (This);

  if (PortNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *PortNumber = 0;

  for (Index = 0; Index < 2; Index++) {
    PSAddr = UhcDev->UsbHostControllerBaseAddress + USBPORTSC1 + Index * 2;
    RHPortControl = USBReadPortW (UhcDev, PSAddr);
    //
    // Port Register content is valid
    //
    if (RHPortControl != 0xff) {
      (*PortNumber)++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Retrieves the current status of a USB root hub port.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  PortNumber             The root hub port to retrieve the state from.  
  @param  PortStatus             Variable to receive the port state.

  @retval EFI_SUCCESS            The status of the USB root hub port specified.
                                 by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid.

**/
EFI_STATUS
EFIAPI
UhcGetRootHubPortStatus (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *This,
  IN  UINT8                         PortNumber,
  OUT EFI_USB_PORT_STATUS           *PortStatus
  )
{
  USB_UHC_DEV *UhcDev;
  UINT32      PSAddr;
  UINT16      RHPortStatus;
  UINT8       TotalPortNumber;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UhcGetRootHubPortNumber (PeiServices, This, &TotalPortNumber);
  if (PortNumber > TotalPortNumber) {
    return EFI_INVALID_PARAMETER;
  }

  UhcDev                        = PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS (This);
  PSAddr                        = UhcDev->UsbHostControllerBaseAddress + USBPORTSC1 + PortNumber * 2;

  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  RHPortStatus = USBReadPortW (UhcDev, PSAddr);

  //
  // Current Connect Status
  //
  if ((RHPortStatus & USBPORTSC_CCS) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
  }
  //
  // Port Enabled/Disabled
  //
  if ((RHPortStatus & USBPORTSC_PED) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
  }
  //
  // Port Suspend
  //
  if ((RHPortStatus & USBPORTSC_SUSP) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }
  //
  // Port Reset
  //
  if ((RHPortStatus & USBPORTSC_PR) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_RESET;
  }
  //
  // Low Speed Device Attached
  //
  if ((RHPortStatus & USBPORTSC_LSDA) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
  }
  //
  //   Fill Port Status Change bits
  //
  //
  // Connect Status Change
  //
  if ((RHPortStatus & USBPORTSC_CSC) != 0) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
  }
  //
  // Port Enabled/Disabled Change
  //
  if ((RHPortStatus & USBPORTSC_PEDC) != 0) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
  }

  return EFI_SUCCESS;
}

/**
  Sets a feature for the specified root hub port.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS            The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.
  @retval EFI_TIMEOUT            The time out occurred.

**/
EFI_STATUS
EFIAPI
UhcSetRootHubPortFeature (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *This,
  IN UINT8                          PortNumber,
  IN EFI_USB_PORT_FEATURE           PortFeature
  )
{
  USB_UHC_DEV *UhcDev;
  UINT32      PSAddr;
  UINT32      CommandRegAddr;
  UINT16      RHPortControl;
  UINT8       TotalPortNumber;

  UhcGetRootHubPortNumber (PeiServices, This, &TotalPortNumber);
  if (PortNumber > TotalPortNumber) {
    return EFI_INVALID_PARAMETER;
  }

  UhcDev          = PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS (This);
  PSAddr          = UhcDev->UsbHostControllerBaseAddress + USBPORTSC1 + PortNumber * 2;
  CommandRegAddr  = UhcDev->UsbHostControllerBaseAddress + USBCMD;

  RHPortControl = USBReadPortW (UhcDev, PSAddr);

  switch (PortFeature) {

  case EfiUsbPortSuspend:
    if ((USBReadPortW (UhcDev, CommandRegAddr) & USBCMD_EGSM) == 0) {
      //
      // if global suspend is not active, can set port suspend
      //
      RHPortControl &= 0xfff5;
      RHPortControl |= USBPORTSC_SUSP;
    }
    break;

  case EfiUsbPortReset:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_PR;
    //
    // Set the reset bit
    //
    break;

  case EfiUsbPortPower:
    break;

  case EfiUsbPortEnable:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_PED;
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  USBWritePortW (UhcDev, PSAddr, RHPortControl);

  return EFI_SUCCESS;
}

/**
  Clears a feature for the specified root hub port.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS            The feature specified by PortFeature was cleared 
                                 for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.

**/
EFI_STATUS
EFIAPI
UhcClearRootHubPortFeature (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *This,
  IN UINT8                          PortNumber,
  IN EFI_USB_PORT_FEATURE           PortFeature
  )
{
  USB_UHC_DEV *UhcDev;
  UINT32      PSAddr;
  UINT16      RHPortControl;
  UINT8       TotalPortNumber;

  UhcGetRootHubPortNumber (PeiServices, This, &TotalPortNumber);

  if (PortNumber > TotalPortNumber) {
    return EFI_INVALID_PARAMETER;
  }

  UhcDev  = PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS (This);
  PSAddr  = UhcDev->UsbHostControllerBaseAddress + USBPORTSC1 + PortNumber * 2;

  RHPortControl = USBReadPortW (UhcDev, PSAddr);

  switch (PortFeature) {
  //
  // clear PORT_ENABLE feature means disable port.
  //
  case EfiUsbPortEnable:
    RHPortControl &= 0xfff5;
    RHPortControl &= ~USBPORTSC_PED;
    break;

  //
  // clear PORT_SUSPEND feature means resume the port.
  // (cause a resume on the specified port if in suspend mode)
  //
  case EfiUsbPortSuspend:
    RHPortControl &= 0xfff5;
    RHPortControl &= ~USBPORTSC_SUSP;
    break;

  //
  // no operation
  //
  case EfiUsbPortPower:
    break;

  //
  // clear PORT_RESET means clear the reset signal.
  //
  case EfiUsbPortReset:
    RHPortControl &= 0xfff5;
    RHPortControl &= ~USBPORTSC_PR;
    break;

  //
  // clear connect status change
  //
  case EfiUsbPortConnectChange:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_CSC;
    break;

  //
  // clear enable/disable status change
  //
  case EfiUsbPortEnableChange:
    RHPortControl &= 0xfff5;
    RHPortControl |= USBPORTSC_PEDC;
    break;

  //
  // root hub does not support this request
  //
  case EfiUsbPortSuspendChange:
    break;

  //
  // root hub does not support this request
  //
  case EfiUsbPortOverCurrentChange:
    break;

  //
  // root hub does not support this request
  //
  case EfiUsbPortResetChange:
    break;

  default:
    return EFI_INVALID_PARAMETER;
  }

  USBWritePortW (UhcDev, PSAddr, RHPortControl);

  return EFI_SUCCESS;
}

/**
  Initialize UHCI.

  @param  UhcDev                 UHCI Device.

  @retval EFI_SUCCESS            UHCI successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Resource can not be allocated.

**/
EFI_STATUS
InitializeUsbHC (
  IN USB_UHC_DEV          *UhcDev
  )
{
  EFI_STATUS  Status;
  UINT32      FrameListBaseAddrReg;
  UINT32      CommandReg;
  UINT16      Command;

  //
  // Create and Initialize Frame List For the Host Controller.
  //
  Status = CreateFrameList (UhcDev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FrameListBaseAddrReg  = UhcDev->UsbHostControllerBaseAddress + USBFLBASEADD;
  CommandReg            = UhcDev->UsbHostControllerBaseAddress + USBCMD;

  //
  // Set Frame List Base Address to the specific register to inform the hardware.
  //
  SetFrameListBaseAddress (UhcDev, FrameListBaseAddrReg, (UINT32) (UINTN) (UhcDev->FrameListEntry));

  Command = USBReadPortW (UhcDev, CommandReg);
  Command |= USBCMD_GRESET;
  USBWritePortW (UhcDev, CommandReg, Command);

  MicroSecondDelay (50 * 1000);


  Command &= ~USBCMD_GRESET;

  USBWritePortW (UhcDev, CommandReg, Command);

  //
  //UHCI spec page120 reset recovery time
  //
  MicroSecondDelay (20 * 1000);

  //
  // Set Run/Stop bit to 1.
  //
  Command = USBReadPortW (UhcDev, CommandReg);
  Command |= USBCMD_RS | USBCMD_MAXP;
  USBWritePortW (UhcDev, CommandReg, Command);

  return EFI_SUCCESS;
}

/**
  Create Frame List Structure.

  @param  UhcDev                 UHCI device.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
CreateFrameList (
  USB_UHC_DEV             *UhcDev
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  FrameListBaseAddr;
  FRAMELIST_ENTRY       *FrameListPtr;
  UINTN                 Index;

  //
  // The Frame List ocupies 4K bytes,
  // and must be aligned on 4-Kbyte boundaries.
  //
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             1,
             &FrameListBaseAddr
             );

  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  //Create Control QH and Bulk QH and link them into Framelist Entry
  //
  Status = CreateQH(UhcDev, &UhcDev->ConfigQH);
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = CreateQH(UhcDev, &UhcDev->BulkQH);
  if (Status != EFI_SUCCESS) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  //Set the corresponding QH pointer 
  //
  SetQHHorizontalLinkPtr(UhcDev->ConfigQH, UhcDev->BulkQH);
  SetQHHorizontalQHorTDSelect (UhcDev->ConfigQH, TRUE);
  SetQHHorizontalValidorInvalid (UhcDev->ConfigQH, TRUE);

  UhcDev->FrameListEntry = (FRAMELIST_ENTRY *) ((UINTN) FrameListBaseAddr);

  FrameListPtr = UhcDev->FrameListEntry;

  for (Index = 0; Index < 1024; Index++) {
    FrameListPtr->FrameListPtrTerminate = 0;
    FrameListPtr->FrameListPtr          = (UINT32)(UINTN)UhcDev->ConfigQH >> 4;
    FrameListPtr->FrameListPtrQSelect   = 1;
    FrameListPtr->FrameListRsvd         = 0;
    FrameListPtr ++;
  }

  return EFI_SUCCESS;
}

/**
  Read a 16bit width data from Uhc HC IO space register.
  
  @param  UhcDev  The UHCI device.
  @param  Port    The IO space address of the register.

  @retval the register content read.

**/
UINT16
USBReadPortW (
  IN  USB_UHC_DEV   *UhcDev,
  IN  UINT32        Port
  )
{
  return IoRead16 (Port);
}

/**
  Write a 16bit width data into Uhc HC IO space register.
  
  @param  UhcDev  The UHCI device.
  @param  Port    The IO space address of the register.
  @param  Data    The data written into the register.

**/
VOID
USBWritePortW (
  IN  USB_UHC_DEV   *UhcDev,
  IN  UINT32        Port,
  IN  UINT16        Data
  )
{
  IoWrite16 (Port, Data);
}

/**
  Write a 32bit width data into Uhc HC IO space register.
  
  @param  UhcDev  The UHCI device.
  @param  Port    The IO space address of the register.
  @param  Data    The data written into the register.

**/
VOID
USBWritePortDW (
  IN  USB_UHC_DEV   *UhcDev,
  IN  UINT32        Port,
  IN  UINT32        Data
  )
{
  IoWrite32 (Port, Data);
}

/**
  Clear the content of UHCI's Status Register.
  
  @param  UhcDev       The UHCI device.
  @param  StatusAddr   The IO space address of the register.

**/
VOID
ClearStatusReg (
  IN  USB_UHC_DEV   *UhcDev,
  IN  UINT32        StatusAddr
  )
{
  //
  // Clear the content of UHCI's Status Register
  //
  USBWritePortW (UhcDev, StatusAddr, 0x003F);
}

/**
  Check whether the host controller operates well.

  @param  UhcDev        The UHCI device.
  @param  StatusRegAddr The io address of status register.

  @retval TRUE          Host controller is working.
  @retval FALSE         Host controller is halted or system error.

**/
BOOLEAN
IsStatusOK (
  IN USB_UHC_DEV     *UhcDev,
  IN UINT32          StatusRegAddr
  )
{
  UINT16  StatusValue;

  StatusValue = USBReadPortW (UhcDev, StatusRegAddr);

  if ((StatusValue & (USBSTS_HCPE | USBSTS_HSE | USBSTS_HCH)) != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Get Current Frame Number.

  @param  UhcDev          The UHCI device.
  @param  FrameNumberAddr The address of frame list register.

  @retval The content of the frame list register.

**/
UINT16
GetCurrentFrameNumber (
  IN USB_UHC_DEV   *UhcDev,
  IN UINT32        FrameNumberAddr
  )
{
  //
  // Gets value in the USB frame number register.
  //
  return (UINT16) (USBReadPortW (UhcDev, FrameNumberAddr) & 0x03FF);
}

/**
  Set Frame List Base Address.

  @param  UhcDev           The UHCI device.
  @param  FrameListRegAddr The address of frame list register.
  @param  Addr             The address of frame list table.

**/
VOID
SetFrameListBaseAddress (
  IN USB_UHC_DEV   *UhcDev,
  IN UINT32        FrameListRegAddr,
  IN UINT32        Addr
  )
{
  //
  // Sets value in the USB Frame List Base Address register.
  //
  USBWritePortDW (UhcDev, FrameListRegAddr, (UINT32) (Addr & 0xFFFFF000));
}

/**
  Create QH and initialize.

  @param  UhcDev               The UHCI device.
  @param  PtrQH                Place to store QH_STRUCT pointer.

  @retval EFI_OUT_OF_RESOURCES Can't allocate memory resources.
  @retval EFI_SUCCESS        Success.

**/
EFI_STATUS
CreateQH (
  IN  USB_UHC_DEV   *UhcDev,
  OUT QH_STRUCT     **PtrQH
  )
{
  EFI_STATUS  Status;

  //
  // allocate align memory for QH_STRUCT
  //
  Status = AllocateTDorQHStruct (UhcDev, sizeof(QH_STRUCT), (void **)PtrQH);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // init each field of the QH_STRUCT
  //
  SetQHHorizontalValidorInvalid (*PtrQH, FALSE);
  SetQHVerticalValidorInvalid (*PtrQH, FALSE);

  return EFI_SUCCESS;
}

/**
  Set the horizontal link pointer in QH.

  @param  PtrQH               Place to store QH_STRUCT pointer.
  @param  PtrNext             Place to the next QH_STRUCT.

**/
VOID
SetQHHorizontalLinkPtr (
  IN QH_STRUCT  *PtrQH,
  IN VOID       *PtrNext
  )
{
  //
  // Since the QH_STRUCT is aligned on 16-byte boundaries,
  // Only the highest 28bit of the address is valid
  // (take 32bit address as an example).
  //
  PtrQH->QueueHead.QHHorizontalPtr = (UINT32) (UINTN) PtrNext >> 4;
}

/**
  Get the horizontal link pointer in QH.

  @param  PtrQH     Place to store QH_STRUCT pointer.

  @retval The horizontal link pointer in QH.

**/
VOID *
GetQHHorizontalLinkPtr (
  IN QH_STRUCT  *PtrQH
  )
{
  //
  // Restore the 28bit address to 32bit address
  // (take 32bit address as an example)
  //
  return (VOID *) (UINTN) ((PtrQH->QueueHead.QHHorizontalPtr) << 4);
}

/**
  Set a QH or TD horizontally to be connected with a specific QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsQH       Specify QH or TD is connected.

**/
VOID
SetQHHorizontalQHorTDSelect (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsQH
  )
{
  //
  // if QH is connected, the specified bit is set,
  // if TD is connected, the specified bit is cleared.
  //
  PtrQH->QueueHead.QHHorizontalQSelect = IsQH ? 1 : 0;
}

/**
  Set the horizontal validor bit in QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsValid    Specify the horizontal linker is valid or not.

**/
VOID
SetQHHorizontalValidorInvalid (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsValid
  )
{
  //
  // Valid means the horizontal link pointer is valid,
  // else, it's invalid.
  //
  PtrQH->QueueHead.QHHorizontalTerminate = IsValid ? 0 : 1;
}

/**
  Set the vertical link pointer in QH.

  @param  PtrQH       Place to store QH_STRUCT pointer.
  @param  PtrNext     Place to the next QH_STRUCT.

**/
VOID
SetQHVerticalLinkPtr (
  IN QH_STRUCT  *PtrQH,
  IN VOID       *PtrNext
  )
{
  //
  // Since the QH_STRUCT is aligned on 16-byte boundaries,
  // Only the highest 28bit of the address is valid
  // (take 32bit address as an example).
  //
  PtrQH->QueueHead.QHVerticalPtr = (UINT32) (UINTN) PtrNext >> 4;
}

/**
  Set a QH or TD vertically to be connected with a specific QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsQH       Specify QH or TD is connected.

**/
VOID
SetQHVerticalQHorTDSelect (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsQH
  )
{
  //
  // Set the specified bit if the Vertical Link Pointer pointing to a QH,
  // Clear the specified bit if the Vertical Link Pointer pointing to a TD.
  //
  PtrQH->QueueHead.QHVerticalQSelect = IsQH ? 1 : 0;
}

/**
  Set the vertical validor bit in QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsValid    Specify the vertical linker is valid or not.

**/
VOID
SetQHVerticalValidorInvalid (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsValid
  )
{
  //
  // If TRUE, meaning the Vertical Link Pointer field is valid,
  // else, the field is invalid.
  //
  PtrQH->QueueHead.QHVerticalTerminate = IsValid ? 0 : 1;
}

/**
  Get the vertical validor bit in QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.

  @retval The vertical linker is valid or not.

**/
BOOLEAN
GetQHHorizontalValidorInvalid (
  IN QH_STRUCT  *PtrQH
  )
{
  //
  // If TRUE, meaning the Horizontal Link Pointer field is valid,
  // else, the field is invalid.
  //
  return (BOOLEAN) (!(PtrQH->QueueHead.QHHorizontalTerminate));
}

/**
  Allocate TD or QH Struct.

  @param  UhcDev                 The UHCI device.
  @param  Size                   The size of allocation.
  @param  PtrStruct              Place to store TD_STRUCT pointer.

  @return EFI_SUCCESS            Allocate successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resource.

**/
EFI_STATUS
AllocateTDorQHStruct (
  IN  USB_UHC_DEV     *UhcDev,
  IN  UINT32          Size,
  OUT VOID            **PtrStruct
  )
{
  EFI_STATUS  Status;

  Status      = EFI_SUCCESS;
  *PtrStruct  = NULL;

  Status = UhcAllocatePool (
            UhcDev,
            (UINT8 **) PtrStruct,
            Size
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (*PtrStruct, Size);

  return Status;
}

/**
  Create a TD Struct.

  @param  UhcDev                 The UHCI device.
  @param  PtrTD                  Place to store TD_STRUCT pointer.

  @return EFI_SUCCESS            Allocate successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resource.

**/
EFI_STATUS
CreateTD (
  IN  USB_UHC_DEV     *UhcDev,
  OUT TD_STRUCT       **PtrTD
  )
{
  EFI_STATUS  Status;
  //
  // create memory for TD_STRUCT, and align the memory.
  //
  Status = AllocateTDorQHStruct (UhcDev, sizeof(TD_STRUCT), (void **)PtrTD);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make TD ready.
  //
  SetTDLinkPtrValidorInvalid (*PtrTD, FALSE);

  return EFI_SUCCESS;
}

/**
  Generate Setup Stage TD.

  @param  UhcDev       The UHCI device.
  @param  DevAddr      Device address.
  @param  Endpoint     Endpoint number.
  @param  DeviceSpeed  Device Speed.
  @param  DevRequest   Device reuquest.
  @param  RequestLen   Request length.
  @param  PtrTD        TD_STRUCT generated.

  @return EFI_SUCCESS            Generate setup stage TD successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resource.

**/
EFI_STATUS
GenSetupStageTD (
  IN  USB_UHC_DEV     *UhcDev,
  IN  UINT8           DevAddr,
  IN  UINT8           Endpoint,
  IN  UINT8           DeviceSpeed,
  IN  UINT8           *DevRequest,
  IN  UINT8           RequestLen,
  OUT TD_STRUCT       **PtrTD
  )
{
  TD_STRUCT   *TdStruct;
  EFI_STATUS  Status;

  Status = CreateTD (UhcDev, &TdStruct);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetTDLinkPtr (TdStruct, NULL);

  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (TdStruct, TRUE);

  //
  // initialize as the last TD in the QH context,
  // this field will be updated in the TD linkage process.
  //
  SetTDLinkPtrValidorInvalid (TdStruct, FALSE);

  //
  // Disable Short Packet Detection by default
  //
  EnableorDisableTDShortPacket (TdStruct, FALSE);

  //
  // Max error counter is 3, retry 3 times when error encountered.
  //
  SetTDControlErrorCounter (TdStruct, 3);

  //
  // set device speed attribute
  // (TRUE - Slow Device; FALSE - Full Speed Device)
  //
  switch (DeviceSpeed) {
  case USB_SLOW_SPEED_DEVICE:
    SetTDLoworFullSpeedDevice (TdStruct, TRUE);
    break;

  case USB_FULL_SPEED_DEVICE:
    SetTDLoworFullSpeedDevice (TdStruct, FALSE);
    break;
  }
  //
  // Non isochronous transfer TD
  //
  SetTDControlIsochronousorNot (TdStruct, FALSE);

  //
  // Interrupt On Complete bit be set to zero,
  // Disable IOC interrupt.
  //
  SetorClearTDControlIOC (TdStruct, FALSE);

  //
  // Set TD Active bit
  //
  SetTDStatusActiveorInactive (TdStruct, TRUE);

  SetTDTokenMaxLength (TdStruct, RequestLen);

  SetTDTokenDataToggle0 (TdStruct);

  SetTDTokenEndPoint (TdStruct, Endpoint);

  SetTDTokenDeviceAddress (TdStruct, DevAddr);

  SetTDTokenPacketID (TdStruct, SETUP_PACKET_ID);

  TdStruct->PtrTDBuffer      = (UINT8 *) DevRequest;
  TdStruct->TDBufferLength = RequestLen;
  SetTDDataBuffer (TdStruct);

  *PtrTD = TdStruct;

  return EFI_SUCCESS;
}

/**
  Generate Data Stage TD.

  @param  UhcDev       The UHCI device.
  @param  DevAddr      Device address.
  @param  Endpoint     Endpoint number.
  @param  PtrData      Data buffer.
  @param  Len          Data length.
  @param  PktID        PacketID.
  @param  Toggle       Data toggle value.
  @param  DeviceSpeed  Device Speed.
  @param  PtrTD        TD_STRUCT generated.

  @return EFI_SUCCESS            Generate data stage TD successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resource.

**/
EFI_STATUS
GenDataTD (
  IN  USB_UHC_DEV     *UhcDev,
  IN  UINT8           DevAddr,
  IN  UINT8           Endpoint,
  IN  UINT8           *PtrData,
  IN  UINT8           Len,
  IN  UINT8           PktID,
  IN  UINT8           Toggle,
  IN  UINT8           DeviceSpeed,
  OUT TD_STRUCT       **PtrTD
  )
{
  TD_STRUCT   *TdStruct;
  EFI_STATUS  Status;

  Status = CreateTD (UhcDev, &TdStruct);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetTDLinkPtr (TdStruct, NULL);

  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (TdStruct, TRUE);

  //
  // Link pointer pointing to TD struct
  //
  SetTDLinkPtrQHorTDSelect (TdStruct, FALSE);

  //
  // initialize as the last TD in the QH context,
  // this field will be updated in the TD linkage process.
  //
  SetTDLinkPtrValidorInvalid (TdStruct, FALSE);

  //
  // Disable short packet detect
  //
  EnableorDisableTDShortPacket (TdStruct, FALSE);
  //
  // Max error counter is 3
  //
  SetTDControlErrorCounter (TdStruct, 3);

  //
  // set device speed attribute
  // (TRUE - Slow Device; FALSE - Full Speed Device)
  //
  switch (DeviceSpeed) {
  case USB_SLOW_SPEED_DEVICE:
    SetTDLoworFullSpeedDevice (TdStruct, TRUE);
    break;

  case USB_FULL_SPEED_DEVICE:
    SetTDLoworFullSpeedDevice (TdStruct, FALSE);
    break;
  }
  //
  // Non isochronous transfer TD
  //
  SetTDControlIsochronousorNot (TdStruct, FALSE);

  //
  // Disable Interrupt On Complete
  // Disable IOC interrupt.
  //
  SetorClearTDControlIOC (TdStruct, FALSE);

  //
  // Set Active bit
  //
  SetTDStatusActiveorInactive (TdStruct, TRUE);

  SetTDTokenMaxLength (TdStruct, Len);

  if (Toggle != 0) {
    SetTDTokenDataToggle1 (TdStruct);
  } else {
    SetTDTokenDataToggle0 (TdStruct);
  }

  SetTDTokenEndPoint (TdStruct, Endpoint);

  SetTDTokenDeviceAddress (TdStruct, DevAddr);

  SetTDTokenPacketID (TdStruct, PktID);

  TdStruct->PtrTDBuffer      = (UINT8 *) PtrData;
  TdStruct->TDBufferLength = Len;
  SetTDDataBuffer (TdStruct);

  *PtrTD = TdStruct;

  return EFI_SUCCESS;
}

/**
  Generate Status Stage TD.

  @param  UhcDev       The UHCI device.
  @param  DevAddr      Device address.
  @param  Endpoint     Endpoint number.
  @param  PktID        PacketID.
  @param  DeviceSpeed  Device Speed.
  @param  PtrTD        TD_STRUCT generated.

  @return EFI_SUCCESS            Generate status stage TD successfully.
  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resource.

**/
EFI_STATUS
CreateStatusTD (
  IN  USB_UHC_DEV     *UhcDev,
  IN  UINT8           DevAddr,
  IN  UINT8           Endpoint,
  IN  UINT8           PktID,
  IN  UINT8           DeviceSpeed,
  OUT TD_STRUCT       **PtrTD
  )
{
  TD_STRUCT   *PtrTDStruct;
  EFI_STATUS  Status;

  Status = CreateTD (UhcDev, &PtrTDStruct);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SetTDLinkPtr (PtrTDStruct, NULL);

  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (PtrTDStruct, TRUE);

  //
  // initialize as the last TD in the QH context,
  // this field will be updated in the TD linkage process.
  //
  SetTDLinkPtrValidorInvalid (PtrTDStruct, FALSE);

  //
  // Disable short packet detect
  //
  EnableorDisableTDShortPacket (PtrTDStruct, FALSE);

  //
  // Max error counter is 3
  //
  SetTDControlErrorCounter (PtrTDStruct, 3);

  //
  // set device speed attribute
  // (TRUE - Slow Device; FALSE - Full Speed Device)
  //
  switch (DeviceSpeed) {
  case USB_SLOW_SPEED_DEVICE:
    SetTDLoworFullSpeedDevice (PtrTDStruct, TRUE);
    break;

  case USB_FULL_SPEED_DEVICE:
    SetTDLoworFullSpeedDevice (PtrTDStruct, FALSE);
    break;
  }
  //
  // Non isochronous transfer TD
  //
  SetTDControlIsochronousorNot (PtrTDStruct, FALSE);

  //
  // Disable Interrupt On Complete
  // Disable IOC interrupt.
  //
  SetorClearTDControlIOC (PtrTDStruct, FALSE);

  //
  // Set TD Active bit
  //
  SetTDStatusActiveorInactive (PtrTDStruct, TRUE);

  SetTDTokenMaxLength (PtrTDStruct, 0);

  SetTDTokenDataToggle1 (PtrTDStruct);

  SetTDTokenEndPoint (PtrTDStruct, Endpoint);

  SetTDTokenDeviceAddress (PtrTDStruct, DevAddr);

  SetTDTokenPacketID (PtrTDStruct, PktID);

  PtrTDStruct->PtrTDBuffer      = NULL;
  PtrTDStruct->TDBufferLength = 0;
  SetTDDataBuffer (PtrTDStruct);

  *PtrTD = PtrTDStruct;

  return EFI_SUCCESS;
}

/**
  Set the link pointer validor bit in TD.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  IsValid      Specify the linker pointer is valid or not.

**/
VOID
SetTDLinkPtrValidorInvalid (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsValid
  )
{
  //
  // Valid means the link pointer is valid,
  // else, it's invalid.
  //
  PtrTDStruct->TDData.TDLinkPtrTerminate = (IsValid ? 0 : 1);
}

/**
  Set the Link Pointer pointing to a QH or TD.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  IsQH          Specify QH or TD is connected.

**/
VOID
SetTDLinkPtrQHorTDSelect (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsQH
  )
{
  //
  // Indicate whether the Link Pointer pointing to a QH or TD
  //
  PtrTDStruct->TDData.TDLinkPtrQSelect = (IsQH ? 1 : 0);
}

/**
  Set the traverse is depth-first or breadth-first.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  IsDepth       Specify the traverse is depth-first or breadth-first.

**/
VOID
SetTDLinkPtrDepthorBreadth (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsDepth
  )
{
  //
  // If TRUE, indicating the host controller should process in depth first fashion,
  // else, the host controller should process in breadth first fashion
  //
  PtrTDStruct->TDData.TDLinkPtrDepthSelect = (IsDepth ? 1 : 0);
}

/**
  Set TD Link Pointer in TD.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  PtrNext      Place to the next TD_STRUCT.

**/
VOID
SetTDLinkPtr (
  IN  TD_STRUCT *PtrTDStruct,
  IN  VOID      *PtrNext
  )
{
  //
  // Set TD Link Pointer. Since QH,TD align on 16-byte boundaries,
  // only the highest 28 bits are valid. (if take 32bit address as an example)
  //
  PtrTDStruct->TDData.TDLinkPtr = (UINT32) (UINTN) PtrNext >> 4;
}

/**
  Get TD Link Pointer.

  @param  PtrTDStruct     Place to store TD_STRUCT pointer.

  @retval Get TD Link Pointer in TD.

**/
VOID *
GetTDLinkPtr (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Get TD Link Pointer. Restore it back to 32bit
  // (if take 32bit address as an example)
  //
  return (VOID *) (UINTN) ((PtrTDStruct->TDData.TDLinkPtr) << 4);
}

/**
  Get the information about whether the Link Pointer field pointing to
  a QH or a TD.

  @param  PtrTDStruct     Place to store TD_STRUCT pointer.

  @retval whether the Link Pointer field pointing to a QH or a TD.

**/
BOOLEAN
IsTDLinkPtrQHOrTD (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Get the information about whether the Link Pointer field pointing to
  // a QH or a TD.
  //
  return (BOOLEAN) (PtrTDStruct->TDData.TDLinkPtrQSelect);
}

/**
  Enable/Disable short packet detection mechanism.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  IsEnable     Enable or disable short packet detection mechanism.

**/
VOID
EnableorDisableTDShortPacket (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsEnable
  )
{
  //
  // TRUE means enable short packet detection mechanism.
  //
  PtrTDStruct->TDData.TDStatusSPD = (IsEnable ? 1 : 0);
}

/**
  Set the max error counter in TD.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  MaxErrors    The number of allowable error.

**/
VOID
SetTDControlErrorCounter (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINT8     MaxErrors
  )
{
  //
  // valid value of MaxErrors is 0,1,2,3
  //
  if (MaxErrors > 3) {
    MaxErrors = 3;
  }

  PtrTDStruct->TDData.TDStatusErr = MaxErrors;
}

/**
  Set the TD is targeting a low-speed device or not.

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsLowSpeedDevice  Whether The device is low-speed.

**/
VOID
SetTDLoworFullSpeedDevice (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsLowSpeedDevice
  )
{
  //
  // TRUE means the TD is targeting at a Low-speed device
  //
  PtrTDStruct->TDData.TDStatusLS = (IsLowSpeedDevice ? 1 : 0);
}

/**
  Set the TD is isochronous transfer type or not.

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsIsochronous     Whether the transaction isochronous transfer type.

**/
VOID
SetTDControlIsochronousorNot (
  IN  TD_STRUCT   *PtrTDStruct,
  IN  BOOLEAN     IsIsochronous
  )
{
  //
  // TRUE means the TD belongs to Isochronous transfer type.
  //
  PtrTDStruct->TDData.TDStatusIOS = (IsIsochronous ? 1 : 0);
}

/**
  Set if UCHI should issue an interrupt on completion of the frame
  in which this TD is executed

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsSet             Whether HC should issue an interrupt on completion.

**/
VOID
SetorClearTDControlIOC (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsSet
  )
{
  //
  // If this bit is set, it indicates that the host controller should issue
  // an interrupt on completion of the frame in which this TD is executed.
  //
  PtrTDStruct->TDData.TDStatusIOC = IsSet ? 1 : 0;
}

/**
  Set if the TD is active and can be executed.

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsActive          Whether the TD is active and can be executed.

**/
VOID
SetTDStatusActiveorInactive (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsActive
  )
{
  //
  // If this bit is set, it indicates that the TD is active and can be
  // executed.
  //
  if (IsActive) {
    PtrTDStruct->TDData.TDStatus |= 0x80;
  } else {
    PtrTDStruct->TDData.TDStatus &= 0x7F;
  }
}

/**
  Specifies the maximum number of data bytes allowed for the transfer.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  MaxLen        The maximum number of data bytes allowed.

  @retval The allowed maximum number of data.
**/
UINT16
SetTDTokenMaxLength (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINT16    MaxLen
  )
{
  //
  // Specifies the maximum number of data bytes allowed for the transfer.
  // the legal value extent is 0 ~ 0x500.
  //
  if (MaxLen > 0x500) {
    MaxLen = 0x500;
  }

  PtrTDStruct->TDData.TDTokenMaxLen = MaxLen - 1;

  return MaxLen;
}

/**
  Set the data toggle bit to DATA1.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

**/
VOID
SetTDTokenDataToggle1 (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Set the data toggle bit to DATA1
  //
  PtrTDStruct->TDData.TDTokenDataToggle = 1;
}

/**
  Set the data toggle bit to DATA0.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

**/
VOID
SetTDTokenDataToggle0 (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Set the data toggle bit to DATA0
  //
  PtrTDStruct->TDData.TDTokenDataToggle = 0;
}

/**
  Set EndPoint Number the TD is targeting at.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  EndPoint      The Endport number of the target.

**/
VOID
SetTDTokenEndPoint (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINTN     EndPoint
  )
{
  //
  // Set EndPoint Number the TD is targeting at.
  //
  PtrTDStruct->TDData.TDTokenEndPt = (UINT8) EndPoint;
}

/**
  Set Device Address the TD is targeting at.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  DevAddr       The Device Address of the target.

**/
VOID
SetTDTokenDeviceAddress (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINTN     DevAddr
  )
{
  //
  // Set Device Address the TD is targeting at.
  //
  PtrTDStruct->TDData.TDTokenDevAddr = (UINT8) DevAddr;
}

/**
  Set Packet Identification the TD is targeting at.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  PacketID      The Packet Identification of the target.

**/
VOID
SetTDTokenPacketID (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINT8     PacketID
  )
{
  //
  // Set the Packet Identification to be used for this transaction.
  //
  PtrTDStruct->TDData.TDTokenPID = PacketID;
}

/**
  Set the beginning address of the data buffer that will be used
  during the transaction.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

**/
VOID
SetTDDataBuffer (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Set the beginning address of the data buffer that will be used
  // during the transaction.
  //
  PtrTDStruct->TDData.TDBufferPtr = (UINT32) (UINTN) (PtrTDStruct->PtrTDBuffer);
}

/**
  Detect whether the TD is active.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The TD is active or not.

**/
BOOLEAN
IsTDStatusActive (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether the TD is active.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x80);
}

/**
  Detect whether the TD is stalled.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The TD is stalled or not.

**/
BOOLEAN
IsTDStatusStalled (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether the device/endpoint addressed by this TD is stalled.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x40);
}

/**
  Detect whether Data Buffer Error is happened.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The Data Buffer Error is happened or not.

**/
BOOLEAN
IsTDStatusBufferError (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether Data Buffer Error is happened.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x20);
}

/**
  Detect whether Babble Error is happened.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The Babble Error is happened or not.

**/
BOOLEAN
IsTDStatusBabbleError (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether Babble Error is happened.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x10);
}

/**
  Detect whether NAK is received.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The NAK is received or not.

**/
BOOLEAN
IsTDStatusNAKReceived (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether NAK is received.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x08);
}

/**
  Detect whether CRC/Time Out Error is encountered.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The CRC/Time Out Error is encountered or not.

**/
BOOLEAN
IsTDStatusCRCTimeOutError (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether CRC/Time Out Error is encountered.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x04);
}

/**
  Detect whether Bitstuff Error is received.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The Bitstuff Error is received or not.

**/
BOOLEAN
IsTDStatusBitStuffError (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  UINT8 TDStatus;

  //
  // Detect whether Bitstuff Error is received.
  //
  TDStatus = (UINT8) (PtrTDStruct->TDData.TDStatus);
  return (BOOLEAN) (TDStatus & 0x02);
}

/**
  Retrieve the actual number of bytes that were tansferred.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The actual number of bytes that were tansferred.

**/
UINT16
GetTDStatusActualLength (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Retrieve the actual number of bytes that were tansferred.
  // the value is encoded as n-1. so return the decoded value.
  //
  return (UINT16) ((PtrTDStruct->TDData.TDStatusActualLength) + 1);
}

/**
  Retrieve the information of whether the Link Pointer field is valid or not.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The linker pointer field is valid or not.

**/
BOOLEAN
GetTDLinkPtrValidorInvalid (
  IN  TD_STRUCT *PtrTDStruct
  )
{
  //
  // Retrieve the information of whether the Link Pointer field
  // is valid or not.
  //
  if ((PtrTDStruct->TDData.TDLinkPtrTerminate & BIT0) != 0) {
    return FALSE;
  } else {
    return TRUE;
  }

}

/**
  Count TD Number from PtrFirstTD.

  @param  PtrFirstTD   Place to store TD_STRUCT pointer.

  @retval The queued TDs number.

**/
UINTN
CountTDsNumber (
  IN  TD_STRUCT *PtrFirstTD
  )
{
  UINTN     Number;
  TD_STRUCT *Ptr;

  //
  // Count the queued TDs number.
  //
  Number  = 0;
  Ptr     = PtrFirstTD;
  while (Ptr != 0) {
    Ptr = (TD_STRUCT *) Ptr->PtrNextTD;
    Number++;
  }

  return Number;
}

/**
  Link TD To QH.

  @param  PtrQH   Place to store QH_STRUCT pointer.
  @param  PtrTD   Place to store TD_STRUCT pointer.

**/
VOID
LinkTDToQH (
  IN  QH_STRUCT *PtrQH,
  IN  TD_STRUCT *PtrTD
  )
{
  if (PtrQH == NULL || PtrTD == NULL) {
    return ;
  }
  //
  //  Validate QH Vertical Ptr field
  //
  SetQHVerticalValidorInvalid (PtrQH, TRUE);

  //
  //  Vertical Ptr pointing to TD structure
  //
  SetQHVerticalQHorTDSelect (PtrQH, FALSE);

  SetQHVerticalLinkPtr (PtrQH, (VOID *) PtrTD);

  PtrQH->PtrDown = (VOID *) PtrTD;
}

/**
  Link TD To TD.

  @param  PtrPreTD  Place to store TD_STRUCT pointer.
  @param  PtrTD     Place to store TD_STRUCT pointer.

**/
VOID
LinkTDToTD (
  IN  TD_STRUCT *PtrPreTD,
  IN  TD_STRUCT *PtrTD
  )
{
  if (PtrPreTD == NULL || PtrTD == NULL) {
    return ;
  }
  //
  // Depth first fashion
  //
  SetTDLinkPtrDepthorBreadth (PtrPreTD, TRUE);

  //
  // Link pointer pointing to TD struct
  //
  SetTDLinkPtrQHorTDSelect (PtrPreTD, FALSE);

  //
  // Validate the link pointer valid bit
  //
  SetTDLinkPtrValidorInvalid (PtrPreTD, TRUE);

  SetTDLinkPtr (PtrPreTD, PtrTD);

  PtrPreTD->PtrNextTD = (VOID *) PtrTD;

  PtrTD->PtrNextTD    = NULL;
}

/**
  Execute Control Transfer.

  @param  UhcDev            The UCHI device.
  @param  PtrTD             A pointer to TD_STRUCT data.
  @param  ActualLen         Actual transfer Length.
  @param  TimeOut           TimeOut value.
  @param  TransferResult    Transfer Result.

  @return EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @return EFI_TIMEOUT       The transfer failed due to time out.
  @return EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
ExecuteControlTransfer (
  IN  USB_UHC_DEV *UhcDev,
  IN  TD_STRUCT   *PtrTD,
  OUT UINTN       *ActualLen,
  IN  UINTN       TimeOut,
  OUT UINT32      *TransferResult
  )
{
  UINTN   ErrTDPos;
  UINTN   Delay;

  ErrTDPos          = 0;
  *TransferResult   = EFI_USB_NOERROR;
  *ActualLen        = 0;

  Delay = (TimeOut * STALL_1_MILLI_SECOND / 200) + 1;

  do {

    CheckTDsResults (PtrTD, TransferResult, &ErrTDPos, ActualLen);

    //
    // TD is inactive, means the control transfer is end.
    //
    if ((*TransferResult & EFI_USB_ERR_NOTEXECUTE) != EFI_USB_ERR_NOTEXECUTE) {
      break;
    }
    MicroSecondDelay (200);
    Delay--;

  } while (Delay != 0);


  if (*TransferResult != EFI_USB_NOERROR) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Execute Bulk Transfer.

  @param  UhcDev            The UCHI device.
  @param  PtrTD             A pointer to TD_STRUCT data.
  @param  ActualLen         Actual transfer Length.
  @param  DataToggle        DataToggle value.
  @param  TimeOut           TimeOut value.
  @param  TransferResult    Transfer Result.

  @return EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @return EFI_TIMEOUT       The transfer failed due to time out.
  @return EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
ExecBulkTransfer (
  IN     USB_UHC_DEV *UhcDev,
  IN     TD_STRUCT   *PtrTD,
  IN OUT UINTN       *ActualLen,
  IN     UINT8       *DataToggle,
  IN     UINTN       TimeOut,
  OUT    UINT32      *TransferResult
  )
{
  UINTN   ErrTDPos;
  UINTN   ScrollNum;
  UINTN   Delay;

  ErrTDPos          = 0;
  *TransferResult   = EFI_USB_NOERROR;
  *ActualLen        = 0;

  Delay = (TimeOut * STALL_1_MILLI_SECOND / 200) + 1;

  do {

    CheckTDsResults (PtrTD, TransferResult, &ErrTDPos, ActualLen);
    //
    // TD is inactive, thus meaning bulk transfer's end.
    //
    if ((*TransferResult & EFI_USB_ERR_NOTEXECUTE) != EFI_USB_ERR_NOTEXECUTE) {
      break;
    }
    MicroSecondDelay (200);
    Delay--;

  } while (Delay != 0);

  //
  // has error
  //
  if (*TransferResult != EFI_USB_NOERROR) {
    //
    // scroll the Data Toggle back to the last success TD
    //
    ScrollNum = CountTDsNumber (PtrTD) - ErrTDPos;
    if ((ScrollNum % 2) != 0) {
      *DataToggle ^= 1;
    }

  //
  // If error, wait 100ms to retry by upper layer
  //
    MicroSecondDelay (100 * 1000);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Delete Queued TDs.

  @param  UhcDev       The UCHI device.
  @param  PtrFirstTD   Place to store TD_STRUCT pointer.

**/
VOID
DeleteQueuedTDs (
  IN USB_UHC_DEV     *UhcDev,
  IN TD_STRUCT       *PtrFirstTD
  )
{
  TD_STRUCT *Tptr1;

  TD_STRUCT *Tptr2;

  Tptr1 = PtrFirstTD;
  //
  // Delete all the TDs in a queue.
  //
  while (Tptr1 != NULL) {

    Tptr2 = Tptr1;

    if (!GetTDLinkPtrValidorInvalid (Tptr2)) {
      Tptr1 = NULL;
    } else {
      //
      // has more than one TD in the queue.
      //
      Tptr1 = GetTDLinkPtr (Tptr2);
    }

    UhcFreePool (UhcDev, (UINT8 *) Tptr2, sizeof (TD_STRUCT));
  }

  return ;
}

/**
  Check TDs Results.

  @param  PtrTD               A pointer to TD_STRUCT data.
  @param  Result              The result to return.
  @param  ErrTDPos            The Error TD position.
  @param  ActualTransferSize  Actual transfer size.

  @retval The TD is executed successfully or not.

**/
BOOLEAN
CheckTDsResults (
  IN  TD_STRUCT               *PtrTD,
  OUT UINT32                  *Result,
  OUT UINTN                   *ErrTDPos,
  OUT UINTN                   *ActualTransferSize
  )
{
  UINTN Len;

  *Result   = EFI_USB_NOERROR;
  *ErrTDPos = 0;

  //
  // Init to zero.
  //
  *ActualTransferSize = 0;

  while (PtrTD != NULL) {

    if (IsTDStatusActive (PtrTD)) {
      *Result |= EFI_USB_ERR_NOTEXECUTE;
    }

    if (IsTDStatusStalled (PtrTD)) {
      *Result |= EFI_USB_ERR_STALL;
    }

    if (IsTDStatusBufferError (PtrTD)) {
      *Result |= EFI_USB_ERR_BUFFER;
    }

    if (IsTDStatusBabbleError (PtrTD)) {
      *Result |= EFI_USB_ERR_BABBLE;
    }

    if (IsTDStatusNAKReceived (PtrTD)) {
      *Result |= EFI_USB_ERR_NAK;
    }

    if (IsTDStatusCRCTimeOutError (PtrTD)) {
      *Result |= EFI_USB_ERR_TIMEOUT;
    }

    if (IsTDStatusBitStuffError (PtrTD)) {
      *Result |= EFI_USB_ERR_BITSTUFF;
    }
    //
    // Accumulate actual transferred data length in each TD.
    //
    Len = GetTDStatusActualLength (PtrTD) & 0x7FF;
    *ActualTransferSize += Len;

    //
    // if any error encountered, stop processing the left TDs.
    //
    if ((*Result) != 0) {
      return FALSE;
    }

    PtrTD = (TD_STRUCT *) (PtrTD->PtrNextTD);
    //
    // Record the first Error TD's position in the queue,
    // this value is zero-based.
    //
    (*ErrTDPos)++;
  }

  return TRUE;
}

/**
  Create Memory Block.

  @param  UhcDev                   The UCHI device.
  @param  MemoryHeader             The Pointer to allocated memory block.
  @param  MemoryBlockSizeInPages   The page size of memory block to be allocated.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
CreateMemoryBlock (
  IN  USB_UHC_DEV           *UhcDev,
  OUT MEMORY_MANAGE_HEADER  **MemoryHeader,
  IN  UINTN                 MemoryBlockSizeInPages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  TempPtr;
  UINTN                 MemPages;
  UINT8                 *Ptr;

  //
  // Memory Block uses MemoryBlockSizeInPages pages,
  // memory management header and bit array use 1 page
  //
  MemPages = MemoryBlockSizeInPages + 1;
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             MemPages,
             &TempPtr
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ptr = (UINT8 *) ((UINTN) TempPtr);

  ZeroMem (Ptr, MemPages * EFI_PAGE_SIZE);

  *MemoryHeader = (MEMORY_MANAGE_HEADER *) Ptr;
  //
  // adjust Ptr pointer to the next empty memory
  //
  Ptr += sizeof (MEMORY_MANAGE_HEADER);
  //
  // Set Bit Array initial address
  //
  (*MemoryHeader)->BitArrayPtr  = Ptr;

  (*MemoryHeader)->Next         = NULL;

  //
  // Memory block initial address
  //
  Ptr = (UINT8 *) ((UINTN) TempPtr);
  Ptr += EFI_PAGE_SIZE;
  (*MemoryHeader)->MemoryBlockPtr = Ptr;
  //
  // set Memory block size
  //
  (*MemoryHeader)->MemoryBlockSizeInBytes = MemoryBlockSizeInPages * EFI_PAGE_SIZE;
  //
  // each bit in Bit Array will manage 32byte memory in memory block
  //
  (*MemoryHeader)->BitArraySizeInBytes = ((*MemoryHeader)->MemoryBlockSizeInBytes / 32) / 8;

  return EFI_SUCCESS;
}

/**
  Initialize UHCI memory management.

  @param  UhcDev                 The UCHI device.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
InitializeMemoryManagement (
  IN USB_UHC_DEV           *UhcDev
  )
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  EFI_STATUS            Status;
  UINTN                 MemPages;

  MemPages  = NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES;
  Status    = CreateMemoryBlock (UhcDev, &MemoryHeader, MemPages);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UhcDev->Header1 = MemoryHeader;

  return EFI_SUCCESS;
}

/**
  Initialize UHCI memory management.

  @param  UhcDev           The UCHI device.
  @param  Pool             Buffer pointer to store the buffer pointer.
  @param  AllocSize        The size of the pool to be allocated.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
UhcAllocatePool (
  IN  USB_UHC_DEV     *UhcDev,
  OUT UINT8           **Pool,
  IN  UINTN           AllocSize
  )
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;
  MEMORY_MANAGE_HEADER  *NewMemoryHeader;
  UINTN                 RealAllocSize;
  UINTN                 MemoryBlockSizeInPages;
  EFI_STATUS            Status;

  *Pool = NULL;

  MemoryHeader = UhcDev->Header1;

  //
  // allocate unit is 32 byte (align on 32 byte)
  //
  if ((AllocSize & 0x1F) != 0) {
    RealAllocSize = (AllocSize / 32 + 1) * 32;
  } else {
    RealAllocSize = AllocSize;
  }

  Status = EFI_NOT_FOUND;
  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {

    Status = AllocMemInMemoryBlock (
              TempHeaderPtr,
              (VOID **) Pool,
              RealAllocSize / 32
              );
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }
  //
  // There is no enough memory,
  // Create a new Memory Block
  //
  //
  // if pool size is larger than NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES,
  // just allocate a large enough memory block.
  //
  if (RealAllocSize > (NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES * EFI_PAGE_SIZE)) {
    MemoryBlockSizeInPages = RealAllocSize / EFI_PAGE_SIZE + 1;
  } else {
    MemoryBlockSizeInPages = NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES;
  }

  Status = CreateMemoryBlock (UhcDev, &NewMemoryHeader, MemoryBlockSizeInPages);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Link the new Memory Block to the Memory Header list
  //
  InsertMemoryHeaderToList (MemoryHeader, NewMemoryHeader);

  Status = AllocMemInMemoryBlock (
            NewMemoryHeader,
            (VOID **) Pool,
            RealAllocSize / 32
            );
  return Status;
}

/**
  Alloc Memory In MemoryBlock.

  @param  MemoryHeader           The pointer to memory manage header.
  @param  Pool                   Buffer pointer to store the buffer pointer.
  @param  NumberOfMemoryUnit     The size of the pool to be allocated.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
AllocMemInMemoryBlock (
  IN  MEMORY_MANAGE_HEADER  *MemoryHeader,
  OUT VOID                  **Pool,
  IN  UINTN                 NumberOfMemoryUnit
  )
{
  UINTN TempBytePos;
  UINTN FoundBytePos;
  UINT8 Index;
  UINT8 FoundBitPos;
  UINT8 ByteValue;
  UINT8 BitValue;
  UINTN NumberOfZeros;
  UINTN Count;

  FoundBytePos  = 0;
  FoundBitPos   = 0;

  ByteValue     = MemoryHeader->BitArrayPtr[0];
  NumberOfZeros = 0;
  Index             = 0;
  for (TempBytePos = 0; TempBytePos < MemoryHeader->BitArraySizeInBytes;) {
    //
    // Pop out BitValue from a byte in TempBytePos.
    //
    BitValue = (UINT8)(ByteValue & 0x1);

    if (BitValue == 0) {
      //
      // Found a free bit, the NumberOfZeros only record the number of those consecutive zeros
      //
      NumberOfZeros++;
      //
      // Found enough consecutive free space, break the loop
      //
      if (NumberOfZeros >= NumberOfMemoryUnit) {
        break;
      }
    } else {
      //
      // Encountering a '1', meant the bit is ocupied.
      //
      if (NumberOfZeros >= NumberOfMemoryUnit) {
        //
        // Found enough consecutive free space,break the loop
        //
        break;
      } else {
        //
        // the NumberOfZeros only record the number of those consecutive zeros,
        // so reset the NumberOfZeros to 0 when encountering '1' before finding
        // enough consecutive '0's
        //
        NumberOfZeros = 0;
        //
        // reset the (FoundBytePos,FoundBitPos) to the position of '1'
        //
        FoundBytePos  = TempBytePos;
        FoundBitPos   = Index;
      }
    }
    //
    // right shift the byte
    //
    ByteValue /= 2;

    //
    // step forward a bit
    //
    Index++;
    if (Index == 8) {
      //
      // step forward a byte, getting the byte value,
      // and reset the bit pos.
      //
      TempBytePos += 1;
      ByteValue = MemoryHeader->BitArrayPtr[TempBytePos];
      Index     = 0;
    }
  }

  if (NumberOfZeros < NumberOfMemoryUnit) {
    return EFI_NOT_FOUND;
  }
  //
  // Found enough free space.
  //
  //
  // The values recorded in (FoundBytePos,FoundBitPos) have two conditions:
  //  1)(FoundBytePos,FoundBitPos) record the position
  //    of the last '1' before the consecutive '0's, it must
  //    be adjusted to the start position of the consecutive '0's.
  //  2)the start address of the consecutive '0's is just the start of
  //    the bitarray. so no need to adjust the values of (FoundBytePos,FoundBitPos).
  //
  if ((MemoryHeader->BitArrayPtr[0] & BIT0) != 0) {
    FoundBitPos += 1;
  }
  //
  // Have the (FoundBytePos,FoundBitPos) make sense.
  //
  if (FoundBitPos > 7) {
    FoundBytePos += 1;
    FoundBitPos -= 8;
  }
  //
  // Set the memory as allocated
  //
  for (TempBytePos = FoundBytePos, Index = FoundBitPos, Count = 0; Count < NumberOfMemoryUnit; Count++) {

    MemoryHeader->BitArrayPtr[TempBytePos] = (UINT8) (MemoryHeader->BitArrayPtr[TempBytePos] | (1 << Index));
    Index++;
    if (Index == 8) {
      TempBytePos += 1;
      Index = 0;
    }
  }

  *Pool = MemoryHeader->MemoryBlockPtr + (FoundBytePos * 8 + FoundBitPos) * 32;

  return EFI_SUCCESS;
}

/**
  Uhci Free Pool.

  @param  UhcDev                 The UHCI device.
  @param  Pool                   A pointer to store the buffer address.
  @param  AllocSize              The size of the pool to be freed.

**/
VOID
UhcFreePool (
  IN USB_UHC_DEV     *UhcDev,
  IN UINT8           *Pool,
  IN UINTN           AllocSize
  )
{
  MEMORY_MANAGE_HEADER  *MemoryHeader;
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;
  UINTN                 StartBytePos;
  UINTN                 Index;
  UINT8                 StartBitPos;
  UINT8                 Index2;
  UINTN                 Count;
  UINTN                 RealAllocSize;

  MemoryHeader = UhcDev->Header1;

  //
  // allocate unit is 32 byte (align on 32 byte)
  //
  if ((AllocSize & 0x1F) != 0) {
    RealAllocSize = (AllocSize / 32 + 1) * 32;
  } else {
    RealAllocSize = AllocSize;
  }

  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL;
       TempHeaderPtr = TempHeaderPtr->Next) {

    if ((Pool >= TempHeaderPtr->MemoryBlockPtr) &&
        ((Pool + RealAllocSize) <= (TempHeaderPtr->MemoryBlockPtr +
                                    TempHeaderPtr->MemoryBlockSizeInBytes))) {

      //
      // Pool is in the Memory Block area,
      // find the start byte and bit in the bit array
      //
      StartBytePos  = ((Pool - TempHeaderPtr->MemoryBlockPtr) / 32) / 8;
      StartBitPos   = (UINT8) (((Pool - TempHeaderPtr->MemoryBlockPtr) / 32) % 8);

      //
      // reset associated bits in bit arry
      //
      for (Index = StartBytePos, Index2 = StartBitPos, Count = 0; Count < (RealAllocSize / 32); Count++) {

        TempHeaderPtr->BitArrayPtr[Index] = (UINT8) (TempHeaderPtr->BitArrayPtr[Index] ^ (1 << Index2));
        Index2++;
        if (Index2 == 8) {
          Index += 1;
          Index2 = 0;
        }
      }
      //
      // break the loop
      //
      break;
    }
  }

}

/**
  Insert a new memory header into list.

  @param  MemoryHeader         A pointer to the memory header list.
  @param  NewMemoryHeader      A new memory header to be inserted into the list.

**/
VOID
InsertMemoryHeaderToList (
  IN MEMORY_MANAGE_HEADER  *MemoryHeader,
  IN MEMORY_MANAGE_HEADER  *NewMemoryHeader
  )
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  for (TempHeaderPtr = MemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {
    if (TempHeaderPtr->Next == NULL) {
      TempHeaderPtr->Next = NewMemoryHeader;
      break;
    }
  }
}

/**
  Judge the memory block in the memory header is empty or not.

  @param  MemoryHeaderPtr   A pointer to the memory header list.

  @retval Whether the memory block in the memory header is empty or not.

**/
BOOLEAN
IsMemoryBlockEmptied (
  IN MEMORY_MANAGE_HEADER  *MemoryHeaderPtr
  )
{
  UINTN Index;

  for (Index = 0; Index < MemoryHeaderPtr->BitArraySizeInBytes; Index++) {
    if (MemoryHeaderPtr->BitArrayPtr[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  remove a memory header from list.

  @param  FirstMemoryHeader   A pointer to the memory header list.
  @param  FreeMemoryHeader    A memory header to be removed into the list.

**/
VOID
DelinkMemoryBlock (
  IN MEMORY_MANAGE_HEADER    *FirstMemoryHeader,
  IN MEMORY_MANAGE_HEADER    *FreeMemoryHeader
  )
{
  MEMORY_MANAGE_HEADER  *TempHeaderPtr;

  if ((FirstMemoryHeader == NULL) || (FreeMemoryHeader == NULL)) {
    return ;
  }

  for (TempHeaderPtr = FirstMemoryHeader; TempHeaderPtr != NULL; TempHeaderPtr = TempHeaderPtr->Next) {

    if (TempHeaderPtr->Next == FreeMemoryHeader) {
      //
      // Link the before and after
      //
      TempHeaderPtr->Next = FreeMemoryHeader->Next;
      break;
    }
  }
}
