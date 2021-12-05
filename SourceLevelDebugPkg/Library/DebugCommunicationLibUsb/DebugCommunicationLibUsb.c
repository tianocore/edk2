/** @file
  Debug Port Library implementation based on usb debug port.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Usb.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugCommunicationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define SETUP_PID   0x2D
#define INPUT_PID   0x69
#define OUTPUT_PID  0xE1
#define ERROR_PID   0x55
#define DATA0_PID   0xC3
#define DATA1_PID   0x4B
#define DATA2_PID   0x87
#define MDATA_PID   0x0F
#define ACK_PID     0xD2
#define NAK_PID     0x5A
#define STALL_PID   0x1E
#define NYET_PID    0x96

#define PCI_CAPABILITY_ID_DEBUG_PORT    0x0A
#define USB_DEBUG_PORT_MAX_PACKET_SIZE  0x08

#define USB_DEBUG_PORT_IN_USE  BIT10
#define USB_DEBUG_PORT_ENABLE  BIT28
#define USB_DEBUG_PORT_OWNER   BIT30

#define USB_PORT_LINE_STATUS_LS    0x400
#define USB_PORT_LINE_STATUS_MASK  0xC00

//
// Usb debug device descriptor, which is defined at
// USB2 Debug Device Specification.
//
typedef struct _USB_DEBUG_PORT_DESCRIPTOR {
  UINT8    Length;
  UINT8    DescriptorType;
  UINT8    DebugInEndpoint;
  UINT8    DebugOutEndpoint;
} USB_DEBUG_PORT_DESCRIPTOR;

USB_DEVICE_REQUEST  mDebugCommunicationLibUsbGetDebugDescriptor = {
  0x80,
  USB_REQ_GET_DESCRIPTOR,
  (UINT16)(0x0A << 8),
  0x0000,
  sizeof (USB_DEBUG_PORT_DESCRIPTOR)
};

USB_DEVICE_REQUEST  mDebugCommunicationLibUsbSetDebugFeature = {
  0x0,
  USB_REQ_SET_FEATURE,
  (UINT16)(0x06),
  0x0000,
  0x0
};

USB_DEVICE_REQUEST  mDebugCommunicationLibUsbSetDebugAddress = {
  0x0,
  USB_REQ_SET_ADDRESS,
  (UINT16)(0x7F),
  0x0000,
  0x0
};

//
// Usb debug port register file, which is defined at
// EHCI Specification.
//
typedef struct _USB_DEBUG_PORT_REGISTER {
  UINT32    ControlStatus;
  UINT8     TokenPid;
  UINT8     SendPid;
  UINT8     ReceivedPid;
  UINT8     Reserved1;
  UINT8     DataBuffer[8];
  UINT8     UsbEndPoint;
  UINT8     UsbAddress;
  UINT8     Reserved2;
  UINT8     Reserved3;
} USB_DEBUG_PORT_REGISTER;

//
// The state machine of usb debug port
//
#define USBDBG_NO_DEV      0     // No device present at debug port
#define USBDBG_NO_DBG_CAB  1     // The device attached is not usb debug cable
#define USBDBG_DBG_CAB     2     // The device attached is usb debug cable
#define USBDBG_INIT_DONE   4     // The usb debug cable device is initialized
#define USBDBG_RESET       8     // The system is reset

#pragma pack(1)
//
// The internal data structure of DEBUG_PORT_HANDLE, which stores some
// important datum which are used across various phases.
//
typedef struct _USB_DEBUG_PORT_HANDLE {
  //
  // The usb debug port memory BAR number in EHCI configuration space.
  //
  UINT8     DebugPortBarNumber;
  UINT8     Initialized;
  //
  // The offset of usb debug port registers in EHCI memory range.
  //
  UINT16    DebugPortOffset;
  //
  // The usb debug port memory BAR address.
  //
  UINT32    UsbDebugPortMemoryBase;
  //
  // The EHCI memory BAR address.
  //
  UINT32    EhciMemoryBase;
  //
  // The usb debug device In endpoint.
  //
  UINT8     InEndpoint;
  //
  // The usb debug device Out endpoint.
  //
  UINT8     OutEndpoint;
  //
  // The Bulk In endpoint toggle bit.
  //
  UINT8     BulkInToggle;
  //
  // The Bulk Out endpoint toggle bit.
  //
  UINT8     BulkOutToggle;
  //
  // The available data length in the following data buffer.
  //
  UINT8     DataCount;
  //
  // The data buffer. Maximum length is 8 bytes.
  //
  UINT8     Data[8];
} USB_DEBUG_PORT_HANDLE;
#pragma pack()

//
// The global variable which can be used after memory is ready.
//
USB_DEBUG_PORT_HANDLE  mDebugCommunicationLibUsbDebugPortHandle;

/**
  Calculate the usb debug port bar address.

  @param  DebugPortOffset    Get usb debug port offset in the usb debug port memory space.
  @param  DebugPortBarNumbar Get the bar number at which usb debug port is located.

  @retval RETURN_UNSUPPORTED The usb host controller does not supported usb debug port capability.
  @retval RETURN_SUCCESS     Get bar and offset successfully.

**/
RETURN_STATUS
EFIAPI
CalculateUsbDebugPortBar (
  OUT UINT16  *DebugPortOffset,
  OUT UINT8   *DebugPortBarNumbar
  )
{
  UINT16  PciStatus;
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT8   ProgInterface;
  UINT8   SubClassCode;
  UINT8   BaseCode;
  UINT8   CapabilityPtr;
  UINT8   CapabilityId;

  VendorId = PciRead16 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_VENDOR_ID_OFFSET);
  DeviceId = PciRead16 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_DEVICE_ID_OFFSET);

  if ((VendorId == 0xFFFF) || (DeviceId == 0xFFFF)) {
    return RETURN_UNSUPPORTED;
  }

  ProgInterface = PciRead8 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_CLASSCODE_OFFSET);
  SubClassCode  = PciRead8 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_CLASSCODE_OFFSET + 1);
  BaseCode      = PciRead8 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_CLASSCODE_OFFSET + 2);

  if ((ProgInterface != PCI_IF_EHCI) || (SubClassCode != PCI_CLASS_SERIAL_USB) || (BaseCode != PCI_CLASS_SERIAL)) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Enable Ehci Host Controller MMIO Space.
  //
  PciStatus = PciRead16 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_PRIMARY_STATUS_OFFSET);

  if ((PciStatus & EFI_PCI_STATUS_CAPABILITY) == 0) {
    //
    // The Pci Device Doesn't Support Capability Pointer.
    //
    return RETURN_UNSUPPORTED;
  }

  //
  // Get Pointer To Capability List
  //
  CapabilityPtr = PciRead8 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_CAPBILITY_POINTER_OFFSET);

  //
  // Find Capability ID 0xA, Which Is For Debug Port
  //
  while (CapabilityPtr != 0) {
    CapabilityId = PciRead8 (PcdGet32 (PcdUsbEhciPciAddress) + CapabilityPtr);
    if (CapabilityId == PCI_CAPABILITY_ID_DEBUG_PORT) {
      break;
    }

    CapabilityPtr = PciRead8 (PcdGet32 (PcdUsbEhciPciAddress) + CapabilityPtr + 1);
  }

  //
  // No Debug Port Capability Found
  //
  if (CapabilityPtr == 0) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Get The Base Address Of Debug Port Register In Debug Port Capability Register
  //
  *DebugPortOffset    = (UINT16)(PciRead16 (PcdGet32 (PcdUsbEhciPciAddress) + CapabilityPtr + 2) & 0x1FFF);
  *DebugPortBarNumbar = (UINT8)((PciRead16 (PcdGet32 (PcdUsbEhciPciAddress) + CapabilityPtr + 2) >> 13) - 1);

  return RETURN_SUCCESS;
}

/**
  Do a usb IN transaction by usb debug port.

  @param  DebugPortRegister        Pointer to the base address of usb debug port register interface.
  @param  Buffer                   Pointer to the buffer receiving data.
  @param  Length                   Number of bytes of the received data.
  @param  Token                    The token PID for each USB transaction.
  @param  Addr                     The usb device address for usb transaction.
  @param  Ep                       The endpoint for usb transaction.
  @param  DataToggle               The toggle bit used at usb transaction.

  @retval RETURN_SUCCESS           The IN transaction is executed successfully.
  @retval RETURN_INVALID_PARAMETER The parameters passed in are invalid.
  @retval RETURN_DEVICE_ERROR      The IN transaction comes across error.

**/
RETURN_STATUS
EFIAPI
UsbDebugPortIn (
  IN      USB_DEBUG_PORT_REGISTER  *DebugPortRegister,
  IN  OUT UINT8                    *Buffer,
  OUT UINT8                        *Length,
  IN      UINT8                    Token,
  IN      UINT8                    Addr,
  IN      UINT8                    Ep,
  IN      UINT8                    DataToggle
  )
{
  UINTN  Index;

  if (Length == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  *Length = 0;

  DebugPortRegister->TokenPid = Token;
  if (DataToggle != 0) {
    DebugPortRegister->SendPid = DATA1_PID;
  } else {
    DebugPortRegister->SendPid = DATA0_PID;
  }

  DebugPortRegister->UsbAddress  = (UINT8)(Addr & 0x7F);
  DebugPortRegister->UsbEndPoint = (UINT8)(Ep & 0xF);

  //
  // Clearing W/R bit to indicate it's a READ operation
  //
  MmioAnd32 ((UINTN)&DebugPortRegister->ControlStatus, (UINT32) ~BIT4);

  //
  // Setting GO bit as well as clearing DONE bit
  //
  MmioOr32 ((UINTN)&DebugPortRegister->ControlStatus, (UINT32)BIT5);

  //
  // Wait for completing the request
  //
  while ((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus) & (UINT32)BIT16) == 0) {
    if ((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus) & (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_ENABLE))
        != (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_ENABLE))
    {
      return RETURN_DEVICE_ERROR;
    }
  }

  //
  // Clearing DONE bit by writing 1
  //
  MmioOr32 ((UINTN)&DebugPortRegister->ControlStatus, BIT16);

  //
  // Check if the request is executed successfully or not.
  //
  if ((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus)) & BIT6) {
    return RETURN_DEVICE_ERROR;
  }

  //
  // Make sure the received data are not beyond the allowable maximum length - 8 byte
  //
  if (((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus)) & 0xF) > USB_DEBUG_PORT_MAX_PACKET_SIZE) {
    return RETURN_DEVICE_ERROR;
  }

  *Length = (UINT8)(MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus) & 0xF);
  if (*Length > 8) {
    return RETURN_DEVICE_ERROR;
  }

  for (Index = 0; Index < *Length; Index++) {
    Buffer[Index] = DebugPortRegister->DataBuffer[Index];
  }

  return RETURN_SUCCESS;
}

/**
  Do a usb SETUP/OUT transaction by usb debug port.

  @param  DebugPortRegister        Pointer to the base address of usb debug port register interface.
  @param  Buffer                   Pointer to the buffer receiving data.
  @param  Length                   Number of bytes of the received data.
  @param  Token                    The token PID for each USB transaction.
  @param  Addr                     The usb device address for usb transaction.
  @param  Ep                       The endpoint for usb transaction.
  @param  DataToggle               The toggle bit used at usb transaction.

  @retval RETURN_SUCCESS           The IN transaction is executed successfully.
  @retval RETURN_INVALID_PARAMETER The parameters passed in are invalid.
  @retval RETURN_DEVICE_ERROR      The IN transaction comes across error.

**/
RETURN_STATUS
EFIAPI
UsbDebugPortOut (
  IN  USB_DEBUG_PORT_REGISTER  *DebugPortRegister,
  IN  UINT8                    *Buffer,
  IN  UINT8                    Length,
  IN  UINT8                    Token,
  IN  UINT8                    Addr,
  IN  UINT8                    Ep,
  IN  UINT8                    DataToggle
  )
{
  UINT8  Index;

  if (Length > 8) {
    return RETURN_INVALID_PARAMETER;
  }

  DebugPortRegister->TokenPid = Token;
  if (DataToggle != 0) {
    DebugPortRegister->SendPid = DATA1_PID;
  } else {
    DebugPortRegister->SendPid = DATA0_PID;
  }

  DebugPortRegister->UsbAddress  = (UINT8)(Addr & 0x7F);
  DebugPortRegister->UsbEndPoint = (UINT8)(Ep & 0xF);

  //
  // Fill in the data length and corresponding data.
  //
  MmioAnd32 ((UINTN)&DebugPortRegister->ControlStatus, (UINT32) ~0xF);
  MmioOr32 ((UINTN)&DebugPortRegister->ControlStatus, Length & 0xF);
  for (Index = 0; Index < Length; Index++) {
    DebugPortRegister->DataBuffer[Index] = Buffer[Index];
  }

  //
  // Setting W/R bit to indicate it's a WRITE operation
  //
  MmioOr32 ((UINTN)&DebugPortRegister->ControlStatus, BIT4);
  //
  // Setting GO bit as well as clearing DONE bit
  //
  MmioOr32 ((UINTN)&DebugPortRegister->ControlStatus, BIT5);

  //
  // Wait for completing the request
  //
  while ((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus) & BIT16) == 0) {
    if ((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus) & (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_ENABLE))
        != (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_ENABLE))
    {
      return RETURN_DEVICE_ERROR;
    }
  }

  //
  // Clearing DONE bit by writing 1
  //
  MmioOr32 ((UINTN)&DebugPortRegister->ControlStatus, BIT16);

  //
  // Check if the request is executed successfully or not.
  //
  if ((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus)) & BIT6) {
    return RETURN_DEVICE_ERROR;
  }

  //
  // Make sure the sent data are not beyond the allowable maximum length - 8 byte
  //
  if (((MmioRead32 ((UINTN)&DebugPortRegister->ControlStatus)) & 0xF) > USB_DEBUG_PORT_MAX_PACKET_SIZE) {
    return RETURN_DEVICE_ERROR;
  }

  return RETURN_SUCCESS;
}

/**
  Do a usb control transfer by usb debug port.

  @param  DebugPortRegister        Pointer to the base address of usb debug port register interface.
  @param  SetupPacket              The token PID for each USB transaction.
  @param  Addr                     The usb device address for usb transaction.
  @param  Ep                       The endpoint for usb transaction.
  @param  Data                     Pointer to the buffer receiving data.
  @param  DataLength               Number of bytes of the received data.

  @retval RETURN_SUCCESS           The IN transaction is executed successfully.
  @retval RETURN_INVALID_PARAMETER The parameters passed in are invalid.
  @retval RETURN_DEVICE_ERROR      The IN transaction comes across error.

**/
RETURN_STATUS
EFIAPI
UsbDebugPortControlTransfer (
  IN      USB_DEBUG_PORT_REGISTER  *DebugPortRegister,
  IN      USB_DEVICE_REQUEST       *SetupPacket,
  IN      UINT8                    Addr,
  IN      UINT8                    Ep,
  OUT UINT8                        *Data,
  IN  OUT UINT8                    *DataLength
  )
{
  RETURN_STATUS  Status;
  UINT8          Temp;
  UINT8          ReturnStatus[8];

  //
  // Setup Phase
  //
  Status = UsbDebugPortOut (DebugPortRegister, (UINT8 *)SetupPacket, (UINT8)sizeof (USB_DEVICE_REQUEST), SETUP_PID, Addr, Ep, 0);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Data Phase
  //
  if (DataLength != 0) {
    if ((SetupPacket->RequestType & BIT7) != 0) {
      //
      // Get Data From Device
      //
      Status = UsbDebugPortIn (DebugPortRegister, Data, DataLength, INPUT_PID, Addr, Ep, 1);
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    } else {
      //
      // Send Data To Device
      //
      Status = UsbDebugPortOut (DebugPortRegister, Data, *DataLength, OUTPUT_PID, Addr, Ep, 1);
      if (RETURN_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // Status Phase
  //
  if ((SetupPacket->RequestType & BIT7) != 0) {
    //
    // For READ operation, Data Toggle in Status Phase Should be 1.
    //
    Status = UsbDebugPortOut (DebugPortRegister, NULL, 0, OUTPUT_PID, Addr, Ep, 1);
  } else {
    //
    // For WRITE operation, Data Toggle in Status Phase Should be 1.
    //
    Status = UsbDebugPortIn (DebugPortRegister, ReturnStatus, &Temp, INPUT_PID, Addr, Ep, 1);
  }

  return Status;
}

/**
  Check if it needs to re-initialize usb debug port hardware.

  During different phases switch, such as SEC to PEI or PEI to DXE or DXE to SMM, we should check
  whether the usb debug port hardware configuration is changed. Such case can be triggered by
  Pci bus resource allocation and so on.

  @param  Handle           Debug port handle.

  @retval TRUE             The usb debug port hardware configuration is changed.
  @retval FALSE            The usb debug port hardware configuration is not changed.

**/
BOOLEAN
EFIAPI
NeedReinitializeHardware (
  IN USB_DEBUG_PORT_HANDLE  *Handle
  )
{
  UINT16                   PciCmd;
  UINT32                   UsbDebugPortMemoryBase;
  UINT32                   EhciMemoryBase;
  BOOLEAN                  Status;
  USB_DEBUG_PORT_REGISTER  *UsbDebugPortRegister;

  Status = FALSE;

  EhciMemoryBase = 0xFFFFFC00 & PciRead32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET);
  if (EhciMemoryBase != Handle->EhciMemoryBase) {
    Handle->EhciMemoryBase = EhciMemoryBase;
    Status                 = TRUE;
  }

  UsbDebugPortMemoryBase = 0xFFFFFC00 & PciRead32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET + Handle->DebugPortBarNumber * 4);
  if (UsbDebugPortMemoryBase != Handle->UsbDebugPortMemoryBase) {
    Handle->UsbDebugPortMemoryBase = UsbDebugPortMemoryBase;
    Status                         = TRUE;
  }

  //
  // Enable Ehci Memory Space Access
  //
  PciCmd = PciRead16 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_COMMAND_OFFSET);
  if (((PciCmd & EFI_PCI_COMMAND_MEMORY_SPACE) == 0) || ((PciCmd & EFI_PCI_COMMAND_BUS_MASTER) == 0)) {
    PciCmd |= EFI_PCI_COMMAND_MEMORY_SPACE | EFI_PCI_COMMAND_BUS_MASTER;
    PciWrite16 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_COMMAND_OFFSET, PciCmd);
    Status = TRUE;
  }

  //
  // If the owner and in_use bit is not set, it means system is doing cold/warm boot or EHCI host controller is reset by system software.
  //
  UsbDebugPortRegister = (USB_DEBUG_PORT_REGISTER *)((UINTN)Handle->UsbDebugPortMemoryBase + Handle->DebugPortOffset);
  if ((MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus) & (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_ENABLE | USB_DEBUG_PORT_IN_USE))
      != (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_ENABLE | USB_DEBUG_PORT_IN_USE))
  {
    Status = TRUE;
  }

  if (Handle->Initialized == USBDBG_RESET) {
    Status = TRUE;
  } else if (Handle->Initialized != USBDBG_INIT_DONE) {
    Status = TRUE;
  }

  return Status;
}

/**
  Initialize usb debug port hardware.

  1. reset ehci host controller.
  2. set right port to debug port.
  3. find a usb debug device is attached by getting debug device descriptor.
  4. set address for the usb debug device.
  5. configure the usb debug device to debug mode.

  @param  Handle           Debug port handle.

  @retval TRUE             The usb debug port hardware configuration is changed.
  @retval FALSE            The usb debug port hardware configuration is not changed.

**/
RETURN_STATUS
EFIAPI
InitializeUsbDebugHardware (
  IN USB_DEBUG_PORT_HANDLE  *Handle
  )
{
  RETURN_STATUS              Status;
  USB_DEBUG_PORT_REGISTER    *UsbDebugPortRegister;
  USB_DEBUG_PORT_DESCRIPTOR  UsbDebugPortDescriptor;
  UINT32                     *PortStatus;
  UINT32                     *UsbCmd;
  UINT32                     *UsbStatus;
  UINT32                     *UsbHCSParam;
  UINT8                      DebugPortNumber;
  UINT8                      Length;

  UsbDebugPortRegister = (USB_DEBUG_PORT_REGISTER *)((UINTN)Handle->UsbDebugPortMemoryBase + Handle->DebugPortOffset);
  UsbHCSParam          = (UINT32 *)((UINTN)Handle->EhciMemoryBase + 0x04);
  UsbCmd               = (UINT32 *)((UINTN)Handle->EhciMemoryBase + 0x20);
  UsbStatus            = (UINT32 *)((UINTN)Handle->EhciMemoryBase + 0x24);

  //
  // Check if the debug port is enabled and owned by myself.
  //
  if (((MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus) & (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE))
       != (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE)) || (Handle->Initialized == USBDBG_RESET))
  {
    DEBUG ((
      DEBUG_INFO,
      "UsbDbg: Need to reset the host controller. ControlStatus = %08x\n",
      MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus)
      ));
    //
    // If the host controller is halted, then reset and restart it.
    //
    if ((MmioRead32 ((UINTN)UsbStatus) & BIT12) != 0) {
      DEBUG ((DEBUG_INFO, "UsbDbg: Reset the host controller.\n"));
      //
      // reset the host controller.
      //
      MmioOr32 ((UINTN)UsbCmd, BIT1);
      //
      // ensure that the host controller is reset.
      //
      while ((MmioRead32 ((UINTN)UsbCmd) & BIT1) != 0) {
      }

      MmioOr32 ((UINTN)UsbCmd, BIT0);
      // ensure that the host controller is started (HALTED bit must be cleared)
      while ((MmioRead32 ((UINTN)UsbStatus) & BIT12) != 0) {
      }
    }

    //
    // First get the ownership of port 0.
    //
    MmioOr32 ((UINTN)&UsbDebugPortRegister->ControlStatus, USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE);

    MicroSecondDelay (200000);
  }

  //
  // Find out which port is used as debug port.
  //
  DebugPortNumber = (UINT8)((MmioRead32 ((UINTN)UsbHCSParam) & 0x00F00000) >> 20);
  //
  // Should find a device is connected at debug port
  //
  PortStatus = (UINT32 *)((UINTN)Handle->EhciMemoryBase + 0x64 + (DebugPortNumber - 1) * 4);
  if (!(MmioRead32 ((UINTN)PortStatus) & BIT0)) {
    Handle->Initialized = USBDBG_NO_DEV;
    return RETURN_NOT_FOUND;
  }

  if ((Handle->Initialized != USBDBG_INIT_DONE) ||
      ((MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus) & USB_DEBUG_PORT_ENABLE) == 0))
  {
    DEBUG ((DEBUG_INFO, "UsbDbg: Reset the debug port.\n"));
    //
    // Reset the debug port
    //
    MmioOr32 ((UINTN)PortStatus, BIT8);
    MicroSecondDelay (500000);
    MmioAnd32 ((UINTN)PortStatus, (UINT32) ~BIT8);
    while (MmioRead32 ((UINTN)PortStatus) & BIT8) {
    }

    //
    // The port enabled bit should be set by HW.
    //
    if ((MmioRead32 ((UINTN)PortStatus) & BIT2) == 0) {
      Handle->Initialized = USBDBG_NO_DBG_CAB;
      return RETURN_DEVICE_ERROR;
    }

    //
    // Enable Usb Debug Port Capability
    //
    MmioOr32 ((UINTN)&UsbDebugPortRegister->ControlStatus, USB_DEBUG_PORT_ENABLE);

    //
    // initialize the data toggle used by bulk in/out endpoint.
    //
    Handle->BulkInToggle  = 0;
    Handle->BulkOutToggle = 0;

    //
    // set usb debug device address as 0x7F.
    //
    Status = UsbDebugPortControlTransfer (UsbDebugPortRegister, &mDebugCommunicationLibUsbSetDebugAddress, 0x0, 0x0, NULL, NULL);
    if (RETURN_ERROR (Status)) {
      //
      // The device can not work well.
      //
      Handle->Initialized = USBDBG_NO_DBG_CAB;
      return Status;
    }

    //
    // Start to communicate with Usb Debug Device to see if the attached device is usb debug device or not.
    //
    Length = (UINT8)sizeof (USB_DEBUG_PORT_DESCRIPTOR);

    //
    // Get debug descriptor.
    //
    Status = UsbDebugPortControlTransfer (UsbDebugPortRegister, &mDebugCommunicationLibUsbGetDebugDescriptor, 0x7F, 0x0, (UINT8 *)&UsbDebugPortDescriptor, &Length);
    if (RETURN_ERROR (Status)) {
      //
      // The device is not a usb debug device.
      //
      Handle->Initialized = USBDBG_NO_DBG_CAB;
      return Status;
    }

    if (Length != sizeof (USB_DEBUG_PORT_DESCRIPTOR)) {
      Handle->Initialized = USBDBG_NO_DBG_CAB;
      return RETURN_DEVICE_ERROR;
    }

    //
    // Determine the usb debug device endpoints.
    //
    Handle->InEndpoint  = UsbDebugPortDescriptor.DebugInEndpoint;
    Handle->OutEndpoint = UsbDebugPortDescriptor.DebugOutEndpoint;

    //
    // enable the usb debug feature.
    //
    Status = UsbDebugPortControlTransfer (UsbDebugPortRegister, &mDebugCommunicationLibUsbSetDebugFeature, 0x7F, 0x0, NULL, NULL);
    if (RETURN_ERROR (Status)) {
      //
      // The device can not work well.
      //
      Handle->Initialized = USBDBG_NO_DBG_CAB;
      return Status;
    }

    Handle->Initialized = USBDBG_DBG_CAB;
  }

  //
  // Set initialized flag
  //
  Handle->Initialized = USBDBG_INIT_DONE;

  return RETURN_SUCCESS;
}

/**
  Read data from debug device and save the datas in buffer.

  Reads NumberOfBytes data bytes from a debug device into the buffer
  specified by Buffer. The number of bytes actually read is returned.
  If the return value is less than NumberOfBytes, then the rest operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Handle           Debug port handle.
  @param  Buffer           Pointer to the data buffer to store the data read from the debug device.
  @param  NumberOfBytes    Number of bytes which will be read.
  @param  Timeout          Timeout value for reading from debug device. It unit is Microsecond.

  @retval 0                Read data failed, no data is to be read.
  @retval >0               Actual number of bytes read from debug device.

**/
UINTN
EFIAPI
DebugPortReadBuffer (
  IN   DEBUG_PORT_HANDLE  Handle,
  IN   UINT8              *Buffer,
  IN   UINTN              NumberOfBytes,
  IN   UINTN              Timeout
  )
{
  USB_DEBUG_PORT_HANDLE  *UsbDebugPortHandle;
  RETURN_STATUS          Status;
  UINT8                  Index;

  if ((NumberOfBytes != 1) || (Buffer == NULL) || (Timeout != 0)) {
    return 0;
  }

  //
  // If Handle is NULL, it means memory is ready for use.
  // Use global variable to store handle value.
  //
  if (Handle == NULL) {
    UsbDebugPortHandle = &mDebugCommunicationLibUsbDebugPortHandle;
  } else {
    UsbDebugPortHandle = (USB_DEBUG_PORT_HANDLE *)Handle;
  }

  if (NeedReinitializeHardware (UsbDebugPortHandle)) {
    Status = InitializeUsbDebugHardware (UsbDebugPortHandle);
    if (RETURN_ERROR (Status)) {
      return 0;
    }
  }

  //
  // Read data from buffer
  //
  if (UsbDebugPortHandle->DataCount < 1) {
    return 0;
  } else {
    *Buffer = UsbDebugPortHandle->Data[0];
    for (Index = 0; Index < UsbDebugPortHandle->DataCount - 1; Index++) {
      if ((Index + 1) >= USB_DEBUG_PORT_MAX_PACKET_SIZE) {
        return 0;
      }

      UsbDebugPortHandle->Data[Index] = UsbDebugPortHandle->Data[Index + 1];
    }

    UsbDebugPortHandle->DataCount = (UINT8)(UsbDebugPortHandle->DataCount - 1);
    return 1;
  }
}

/**
  Write data from buffer to debug device.

  Writes NumberOfBytes data bytes from Buffer to the debug device.
  The number of bytes actually written to the debug device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If NumberOfBytes is zero, then return 0.

  @param  Handle           Debug port handle.
  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the debug device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the debug device.
                           If this value is less than NumberOfBytes, then the read operation failed.

**/
UINTN
EFIAPI
DebugPortWriteBuffer (
  IN   DEBUG_PORT_HANDLE  Handle,
  IN   UINT8              *Buffer,
  IN   UINTN              NumberOfBytes
  )
{
  USB_DEBUG_PORT_HANDLE    *UsbDebugPortHandle;
  USB_DEBUG_PORT_REGISTER  *UsbDebugPortRegister;
  RETURN_STATUS            Status;
  UINT8                    Sent;
  UINTN                    Total;
  UINT8                    ReceivedPid;

  if ((NumberOfBytes == 0) || (Buffer == NULL)) {
    return 0;
  }

  Sent  = 0;
  Total = 0;

  //
  // If Handle is NULL, it means memory is ready for use.
  // Use global variable to store handle value.
  //
  if (Handle == NULL) {
    UsbDebugPortHandle = &mDebugCommunicationLibUsbDebugPortHandle;
  } else {
    UsbDebugPortHandle = (USB_DEBUG_PORT_HANDLE *)Handle;
  }

  if (NeedReinitializeHardware (UsbDebugPortHandle)) {
    Status = InitializeUsbDebugHardware (UsbDebugPortHandle);
    if (RETURN_ERROR (Status)) {
      return 0;
    }
  }

  UsbDebugPortRegister = (USB_DEBUG_PORT_REGISTER *)((UINTN)UsbDebugPortHandle->UsbDebugPortMemoryBase + UsbDebugPortHandle->DebugPortOffset);

  while ((Total < NumberOfBytes)) {
    if (NumberOfBytes - Total > USB_DEBUG_PORT_MAX_PACKET_SIZE) {
      Sent = USB_DEBUG_PORT_MAX_PACKET_SIZE;
    } else {
      Sent = (UINT8)(NumberOfBytes - Total);
    }

    Status = UsbDebugPortOut (UsbDebugPortRegister, Buffer + Total, Sent, OUTPUT_PID, 0x7F, UsbDebugPortHandle->OutEndpoint, UsbDebugPortHandle->BulkOutToggle);

    if (RETURN_ERROR (Status)) {
      return Total;
    }

    ReceivedPid = (MmioRead8 ((UINTN)&UsbDebugPortRegister->ReceivedPid));
    //
    // If received a NAK_PID on write transaction, it means the usb debug device is busy and can not handle this transaction.
    // should send the packet again.
    //
    if (ReceivedPid == NAK_PID) {
      Sent = 0;
    } else {
      UsbDebugPortHandle->BulkOutToggle ^= 1;
    }

    Total += Sent;
  }

  return Total;
}

/**
  Polls a debug device to see if there is any data waiting to be read.

  Polls a debug device to see if there is any data waiting to be read.
  If there is data waiting to be read from the debug device, then TRUE is returned.
  If there is no data waiting to be read from the debug device, then FALSE is returned.

  @param  Handle           Debug port handle.

  @retval TRUE             Data is waiting to be read from the debug device.
  @retval FALSE            There is no data waiting to be read from the serial device.

**/
BOOLEAN
EFIAPI
DebugPortPollBuffer (
  IN DEBUG_PORT_HANDLE  Handle
  )
{
  USB_DEBUG_PORT_HANDLE    *UsbDebugPortHandle;
  USB_DEBUG_PORT_REGISTER  *UsbDebugPortRegister;
  UINT8                    Length;
  UINT8                    Index;
  RETURN_STATUS            Status;

  //
  // If Handle is NULL, it means memory is ready for use.
  // Use global variable to store handle value.
  //
  if (Handle == NULL) {
    UsbDebugPortHandle = &mDebugCommunicationLibUsbDebugPortHandle;
  } else {
    UsbDebugPortHandle = (USB_DEBUG_PORT_HANDLE *)Handle;
  }

  if (NeedReinitializeHardware (UsbDebugPortHandle)) {
    Status = InitializeUsbDebugHardware (UsbDebugPortHandle);
    if (RETURN_ERROR (Status)) {
      return FALSE;
    }
  }

  //
  // If the data buffer is not empty, then return TRUE directly.
  // else initialize a usb read transaction and read data to the data buffer.
  //
  if (UsbDebugPortHandle->DataCount != 0) {
    return TRUE;
  }

  UsbDebugPortRegister = (USB_DEBUG_PORT_REGISTER *)((UINTN)UsbDebugPortHandle->UsbDebugPortMemoryBase + UsbDebugPortHandle->DebugPortOffset);

  UsbDebugPortRegister->TokenPid = INPUT_PID;
  if (UsbDebugPortHandle->BulkInToggle == 0) {
    UsbDebugPortRegister->SendPid = DATA0_PID;
  } else {
    UsbDebugPortRegister->SendPid = DATA1_PID;
  }

  UsbDebugPortRegister->UsbAddress  = 0x7F;
  UsbDebugPortRegister->UsbEndPoint = UsbDebugPortHandle->InEndpoint & 0x0F;

  //
  // Clearing W/R bit to indicate it's a READ operation
  //
  MmioAnd32 ((UINTN)&UsbDebugPortRegister->ControlStatus, (UINT32) ~BIT4);
  //
  // Setting GO bit as well as clearing DONE bit
  //
  MmioOr32 ((UINTN)&UsbDebugPortRegister->ControlStatus, (UINT32)BIT5);

  //
  // Wait for completing the request
  //
  while ((MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus) & (UINT32)BIT16) == 0) {
    if ((MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus) & (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_ENABLE))
        != (USB_DEBUG_PORT_OWNER | USB_DEBUG_PORT_IN_USE | USB_DEBUG_PORT_ENABLE))
    {
      return FALSE;
    }
  }

  if ((MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus)) & BIT6) {
    return FALSE;
  }

  Length = (UINT8)(MmioRead32 ((UINTN)&UsbDebugPortRegister->ControlStatus) & 0xF);

  if (Length > 8) {
    return FALSE;
  }

  UsbDebugPortHandle->BulkInToggle ^= 1;

  if (Length == 0) {
    return FALSE;
  }

  for (Index = 0; Index < Length; Index++) {
    UsbDebugPortHandle->Data[Index] = UsbDebugPortRegister->DataBuffer[Index];
  }

  UsbDebugPortHandle->DataCount = Length;

  return TRUE;
}

/**
  Initialize the debug port.

  If Function is not NULL, Debug Communication Library will call this function
  by passing in the Context to be the first parameter. If needed, Debug Communication
  Library will create one debug port handle to be the second argument passing in
  calling the Function, otherwise it will pass NULL to be the second argument of
  Function.

  If Function is NULL, and Context is not NULL, the Debug Communication Library could
    a) Return the same handle as passed in (as Context parameter).
    b) Ignore the input Context parameter and create new handle to be returned.

  If parameter Function is NULL and Context is NULL, Debug Communication Library could
  created a new handle if needed and return it, otherwise it will return NULL.

  @param[in] Context      Context needed by callback function; it was optional.
  @param[in] Function     Continue function called by Debug Communication library;
                          it was optional.

  @return  The debug port handle created by Debug Communication Library if Function
           is not NULL.

**/
DEBUG_PORT_HANDLE
EFIAPI
DebugPortInitialize (
  IN VOID                 *Context,
  IN DEBUG_PORT_CONTINUE  Function
  )
{
  RETURN_STATUS          Status;
  USB_DEBUG_PORT_HANDLE  Handle;

  //
  // Validate the PCD PcdDebugPortHandleBufferSize value
  //
  ASSERT (PcdGet16 (PcdDebugPortHandleBufferSize) == sizeof (USB_DEBUG_PORT_HANDLE));

  if ((Function == NULL) && (Context != NULL)) {
    return (DEBUG_PORT_HANDLE *)Context;
  }

  ZeroMem (&Handle, sizeof (USB_DEBUG_PORT_HANDLE));

  Status = CalculateUsbDebugPortBar (&Handle.DebugPortOffset, &Handle.DebugPortBarNumber);
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbDbg: the pci device pointed by PcdUsbEhciPciAddress is not EHCI host controller or does not support debug port capability!\n"));
    goto Exit;
  }

  Handle.EhciMemoryBase = 0xFFFFFC00 & PciRead32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET);

  if (Handle.EhciMemoryBase == 0) {
    //
    // Usb Debug Port MMIO Space Is Not Enabled. Assumption here that DebugPortBase is zero
    //
    PciWrite32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET, PcdGet32 (PcdUsbEhciMemorySpaceBase));
    Handle.EhciMemoryBase = 0xFFFFFC00 & PciRead32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET);
  }

  Handle.UsbDebugPortMemoryBase = 0xFFFFFC00 & PciRead32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET + Handle.DebugPortBarNumber * 4);

  if (Handle.UsbDebugPortMemoryBase == 0) {
    //
    // Usb Debug Port MMIO Space Is Not Enabled. Assumption here that DebugPortBase is zero
    //
    PciWrite32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET + Handle.DebugPortBarNumber * 4, PcdGet32 (PcdUsbDebugPortMemorySpaceBase));
    Handle.UsbDebugPortMemoryBase = 0xFFFFFC00 & PciRead32 (PcdGet32 (PcdUsbEhciPciAddress) + PCI_BASE_ADDRESSREG_OFFSET + Handle.DebugPortBarNumber * 4);
  }

  Handle.Initialized = USBDBG_RESET;

  if (NeedReinitializeHardware (&Handle)) {
    DEBUG ((DEBUG_ERROR, "UsbDbg: Start EHCI debug port initialization!\n"));
    Status = InitializeUsbDebugHardware (&Handle);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbDbg: Failed, please check if USB debug cable is plugged into EHCI debug port correctly!\n"));
      goto Exit;
    }
  }

Exit:

  if (Function != NULL) {
    Function (Context, &Handle);
  } else {
    CopyMem (&mDebugCommunicationLibUsbDebugPortHandle, &Handle, sizeof (USB_DEBUG_PORT_HANDLE));
  }

  return (DEBUG_PORT_HANDLE)(UINTN)&mDebugCommunicationLibUsbDebugPortHandle;
}
