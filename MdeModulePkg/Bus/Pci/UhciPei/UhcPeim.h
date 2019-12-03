/** @file
Private Header file for Usb Host Controller PEIM

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RECOVERY_UHC_H_
#define _RECOVERY_UHC_H_


#include <PiPei.h>

#include <Ppi/UsbController.h>
#include <Ppi/UsbHostController.h>
#include <Ppi/IoMmu.h>
#include <Ppi/EndOfPeiPhase.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Library/PeiServicesLib.h>

#define USB_SLOW_SPEED_DEVICE 0x01
#define USB_FULL_SPEED_DEVICE 0x02

//
// One memory block uses 16 page
//
#define NORMAL_MEMORY_BLOCK_UNIT_IN_PAGES 16

#define USBCMD                            0       /* Command Register Offset 00-01h */
#define USBCMD_RS                         BIT0    /* Run/Stop */
#define USBCMD_HCRESET                    BIT1    /* Host reset */
#define USBCMD_GRESET                     BIT2    /* Global reset */
#define USBCMD_EGSM                       BIT3    /* Global Suspend Mode */
#define USBCMD_FGR                        BIT4    /* Force Global Resume */
#define USBCMD_SWDBG                      BIT5    /* SW Debug mode */
#define USBCMD_CF                         BIT6    /* Config Flag (sw only) */
#define USBCMD_MAXP                       BIT7    /* Max Packet (0 = 32, 1 = 64) */

/* Status register */
#define USBSTS        2       /* Status Register Offset 02-03h */
#define USBSTS_USBINT BIT0    /* Interrupt due to IOC */
#define USBSTS_ERROR  BIT1    /* Interrupt due to error */
#define USBSTS_RD     BIT2    /* Resume Detect */
#define USBSTS_HSE    BIT3    /* Host System Error - basically PCI problems */
#define USBSTS_HCPE   BIT4    /* Host Controller Process Error - the scripts were buggy */
#define USBSTS_HCH    BIT5    /* HC Halted */

/* Interrupt enable register */
#define USBINTR         4       /* Interrupt Enable Register 04-05h */
#define USBINTR_TIMEOUT BIT0    /* Timeout/CRC error enable */
#define USBINTR_RESUME  BIT1    /* Resume interrupt enable */
#define USBINTR_IOC     BIT2    /* Interrupt On Complete enable */
#define USBINTR_SP      BIT3    /* Short packet interrupt enable */

/* Frame Number Register Offset 06-08h */
#define USBFRNUM  6

/* Frame List Base Address Register Offset 08-0Bh */
#define USBFLBASEADD  8

/* Start of Frame Modify Register Offset 0Ch */
#define USBSOF  0x0c

/* USB port status and control registers */
#define USBPORTSC1            0x10      /*Port 1 offset 10-11h */
#define USBPORTSC2            0x12      /*Port 2 offset 12-13h */

#define USBPORTSC_CCS         BIT0      /* Current Connect Status ("device present") */
#define USBPORTSC_CSC         BIT1      /* Connect Status Change */
#define USBPORTSC_PED         BIT2      /* Port Enable / Disable */
#define USBPORTSC_PEDC        BIT3      /* Port Enable / Disable Change */
#define USBPORTSC_LSL         BIT4      /* Line Status Low bit*/
#define USBPORTSC_LSH         BIT5      /* Line Status High bit*/
#define USBPORTSC_RD          BIT6      /* Resume Detect */
#define USBPORTSC_LSDA        BIT8      /* Low Speed Device Attached */
#define USBPORTSC_PR          BIT9      /* Port Reset */
#define USBPORTSC_SUSP        BIT12     /* Suspend */

#define SETUP_PACKET_ID       0x2D
#define INPUT_PACKET_ID       0x69
#define OUTPUT_PACKET_ID      0xE1
#define ERROR_PACKET_ID       0x55

#define STALL_1_MICRO_SECOND  1
#define STALL_1_MILLI_SECOND  1000


#pragma pack(1)

typedef struct {
  UINT32  FrameListPtrTerminate : 1;
  UINT32  FrameListPtrQSelect : 1;
  UINT32  FrameListRsvd : 2;
  UINT32  FrameListPtr : 28;
} FRAMELIST_ENTRY;

typedef struct {
  UINT32  QHHorizontalTerminate : 1;
  UINT32  QHHorizontalQSelect : 1;
  UINT32  QHHorizontalRsvd : 2;
  UINT32  QHHorizontalPtr : 28;
  UINT32  QHVerticalTerminate : 1;
  UINT32  QHVerticalQSelect : 1;
  UINT32  QHVerticalRsvd : 2;
  UINT32  QHVerticalPtr : 28;
} QUEUE_HEAD;

typedef struct {
  QUEUE_HEAD  QueueHead;
  UINT32      Reserved1;
  UINT32      Reserved2;
  VOID        *PtrNext;
  VOID        *PtrDown;
  VOID        *Reserved3;
  UINT32      Reserved4;
} QH_STRUCT;

typedef struct {
  UINT32  TDLinkPtrTerminate : 1;
  UINT32  TDLinkPtrQSelect : 1;
  UINT32  TDLinkPtrDepthSelect : 1;
  UINT32  TDLinkPtrRsvd : 1;
  UINT32  TDLinkPtr : 28;
  UINT32  TDStatusActualLength : 11;
  UINT32  TDStatusRsvd : 5;
  UINT32  TDStatus : 8;
  UINT32  TDStatusIOC : 1;
  UINT32  TDStatusIOS : 1;
  UINT32  TDStatusLS : 1;
  UINT32  TDStatusErr : 2;
  UINT32  TDStatusSPD : 1;
  UINT32  TDStatusRsvd2 : 2;
  UINT32  TDTokenPID : 8;
  UINT32  TDTokenDevAddr : 7;
  UINT32  TDTokenEndPt : 4;
  UINT32  TDTokenDataToggle : 1;
  UINT32  TDTokenRsvd : 1;
  UINT32  TDTokenMaxLen : 11;
  UINT32  TDBufferPtr;
} TD;

typedef struct {
  TD      TDData;
  UINT8   *PtrTDBuffer;
  VOID    *PtrNextTD;
  VOID    *PtrNextQH;
  UINT16  TDBufferLength;
  UINT16  Reserved;
} TD_STRUCT;

#pragma pack()

typedef struct _MEMORY_MANAGE_HEADER MEMORY_MANAGE_HEADER;

struct _MEMORY_MANAGE_HEADER {
  UINT8                         *BitArrayPtr;
  UINTN                         BitArraySizeInBytes;
  UINT8                         *MemoryBlockPtr;
  UINTN                         MemoryBlockSizeInBytes;
  MEMORY_MANAGE_HEADER          *Next;
};

#define USB_UHC_DEV_SIGNATURE SIGNATURE_32 ('p', 'u', 'h', 'c')
typedef struct {
  UINTN                       Signature;
  PEI_USB_HOST_CONTROLLER_PPI UsbHostControllerPpi;
  EDKII_IOMMU_PPI             *IoMmu;
  EFI_PEI_PPI_DESCRIPTOR      PpiDescriptor;
  //
  // EndOfPei callback is used to stop the UHC DMA operation
  // after exit PEI phase.
  //
  EFI_PEI_NOTIFY_DESCRIPTOR   EndOfPeiNotifyList;

  UINT32                      UsbHostControllerBaseAddress;
  FRAMELIST_ENTRY             *FrameListEntry;
  QH_STRUCT                   *ConfigQH;
  QH_STRUCT                   *BulkQH;
  //
  // Header1 used for QH,TD memory blocks management
  //
  MEMORY_MANAGE_HEADER        *Header1;

} USB_UHC_DEV;

#define PEI_RECOVERY_USB_UHC_DEV_FROM_UHCI_THIS(a)  CR (a, USB_UHC_DEV, UsbHostControllerPpi, USB_UHC_DEV_SIGNATURE)
#define PEI_RECOVERY_USB_UHC_DEV_FROM_THIS_NOTIFY(a) CR (a, USB_UHC_DEV, EndOfPeiNotifyList, USB_UHC_DEV_SIGNATURE)

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
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    * This,
  IN     UINT8                      DeviceAddress,
  IN     UINT8                      DeviceSpeed,
  IN     UINT8                      MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST     * Request,
  IN     EFI_USB_DATA_DIRECTION     TransferDirection,
  IN OUT VOID                       *Data OPTIONAL,
  IN OUT UINTN                      *DataLength OPTIONAL,
  IN     UINTN                      TimeOut,
  OUT    UINT32                     *TransferResult
  );

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
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI    *This,
  IN  UINT8                         DeviceAddress,
  IN  UINT8                         EndPointAddress,
  IN  UINT8                         MaximumPacketLength,
  IN OUT VOID                       *Data,
  IN OUT UINTN                      *DataLength,
  IN OUT UINT8                      *DataToggle,
  IN  UINTN                         TimeOut,
  OUT UINT32                        *TransferResult
  );

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
  );

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
  );

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
  );

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
  );

/**
  Initialize UHCI.

  @param  UhcDev                 UHCI Device.

  @retval EFI_SUCCESS            UHCI successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Resource can not be allocated.

**/
EFI_STATUS
InitializeUsbHC (
  IN USB_UHC_DEV          *UhcDev
  );

/**
  Create Frame List Structure.

  @param  UhcDev                 UHCI device.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
CreateFrameList (
  USB_UHC_DEV             *UhcDev
  );

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
  );

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
  );

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
  );

/**
  Clear the content of UHCI's Status Register.

  @param  UhcDev       The UHCI device.
  @param  StatusAddr   The IO space address of the register.

**/
VOID
ClearStatusReg (
  IN  USB_UHC_DEV   *UhcDev,
  IN  UINT32        StatusAddr
  );

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
  );

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
  );

/**
  Create QH and initialize.

  @param  UhcDev               The UHCI device.
  @param  PtrQH                Place to store QH_STRUCT pointer.

  @retval EFI_OUT_OF_RESOURCES Can't allocate memory resources.
  @retval EFI_SUCCESS        Success.

**/
EFI_STATUS
CreateQH (
  IN  USB_UHC_DEV  *UhcDev,
  OUT QH_STRUCT    **PtrQH
  );

/**
  Set the horizontal link pointer in QH.

  @param  PtrQH               Place to store QH_STRUCT pointer.
  @param  PtrNext             Place to the next QH_STRUCT.

**/
VOID
SetQHHorizontalLinkPtr (
  IN QH_STRUCT  *PtrQH,
  IN VOID       *PtrNext
  );

/**
  Set a QH or TD horizontally to be connected with a specific QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsQH       Specify QH or TD is connected.

**/
VOID
SetQHHorizontalQHorTDSelect (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsQH
  );

/**
  Set the horizontal validor bit in QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsValid    Specify the horizontal linker is valid or not.

**/
VOID
SetQHHorizontalValidorInvalid (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsValid
  );

/**
  Set the vertical link pointer in QH.

  @param  PtrQH       Place to store QH_STRUCT pointer.
  @param  PtrNext     Place to the next QH_STRUCT.

**/
VOID
SetQHVerticalLinkPtr (
  IN QH_STRUCT  *PtrQH,
  IN VOID       *PtrNext
  );

/**
  Set a QH or TD vertically to be connected with a specific QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsQH       Specify QH or TD is connected.

**/
VOID
SetQHVerticalQHorTDSelect (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsQH
  );

/**
  Set the vertical validor bit in QH.

  @param  PtrQH      Place to store QH_STRUCT pointer.
  @param  IsValid    Specify the vertical linker is valid or not.

**/
VOID
SetQHVerticalValidorInvalid (
  IN QH_STRUCT  *PtrQH,
  IN BOOLEAN    IsValid
  );


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
  IN  USB_UHC_DEV   *UhcDev,
  IN  UINT32        Size,
  OUT VOID          **PtrStruct
  );

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
  );

/**
  Generate Setup Stage TD.

  @param  UhcDev       The UHCI device.
  @param  DevAddr      Device address.
  @param  Endpoint     Endpoint number.
  @param  DeviceSpeed  Device Speed.
  @param  DevRequest   CPU memory address of request structure buffer to transfer.
  @param  RequestPhy   PCI memory address of request structure buffer to transfer.
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
  IN  UINT8           *RequestPhy,
  IN  UINT8           RequestLen,
  OUT TD_STRUCT       **PtrTD
  );

/**
  Generate Data Stage TD.

  @param  UhcDev       The UHCI device.
  @param  DevAddr      Device address.
  @param  Endpoint     Endpoint number.
  @param  PtrData      CPU memory address of user data buffer to transfer.
  @param  DataPhy      PCI memory address of user data buffer to transfer.
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
  IN  UINT8           *DataPhy,
  IN  UINT8           Len,
  IN  UINT8           PktID,
  IN  UINT8           Toggle,
  IN  UINT8           DeviceSpeed,
  OUT TD_STRUCT       **PtrTD
  );

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
  );

/**
  Set the link pointer validor bit in TD.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  IsValid      Specify the linker pointer is valid or not.

**/
VOID
SetTDLinkPtrValidorInvalid (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsValid
  );

/**
  Set the Link Pointer pointing to a QH or TD.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  IsQH          Specify QH or TD is connected.

**/
VOID
SetTDLinkPtrQHorTDSelect (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsQH
  );

/**
  Set the traverse is depth-first or breadth-first.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  IsDepth       Specify the traverse is depth-first or breadth-first.

**/
VOID
SetTDLinkPtrDepthorBreadth (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsDepth
  );

/**
  Set TD Link Pointer in TD.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  PtrNext      Place to the next TD_STRUCT.

**/
VOID
SetTDLinkPtr (
  IN  TD_STRUCT *PtrTDStruct,
  IN  VOID      *PtrNext
  );

/**
  Get TD Link Pointer.

  @param  PtrTDStruct     Place to store TD_STRUCT pointer.

  @retval Get TD Link Pointer in TD.

**/
VOID*
GetTDLinkPtr (
  IN  TD_STRUCT *PtrTDStruct
  );


/**
  Enable/Disable short packet detection mechanism.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  IsEnable     Enable or disable short packet detection mechanism.

**/
VOID
EnableorDisableTDShortPacket (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsEnable
  );

/**
  Set the max error counter in TD.

  @param  PtrTDStruct  Place to store TD_STRUCT pointer.
  @param  MaxErrors    The number of allowable error.

**/
VOID
SetTDControlErrorCounter (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINT8     MaxErrors
  );

/**
  Set the TD is targeting a low-speed device or not.

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsLowSpeedDevice  Whether The device is low-speed.

**/
VOID
SetTDLoworFullSpeedDevice (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsLowSpeedDevice
  );

/**
  Set the TD is isochronous transfer type or not.

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsIsochronous     Whether the transaction isochronous transfer type.

**/
VOID
SetTDControlIsochronousorNot (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsIsochronous
  );

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
  );

/**
  Set if the TD is active and can be executed.

  @param  PtrTDStruct       Place to store TD_STRUCT pointer.
  @param  IsActive          Whether the TD is active and can be executed.

**/
VOID
SetTDStatusActiveorInactive (
  IN  TD_STRUCT *PtrTDStruct,
  IN  BOOLEAN   IsActive
  );

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
  );

/**
  Set the data toggle bit to DATA1.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

**/
VOID
SetTDTokenDataToggle1 (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Set the data toggle bit to DATA0.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

**/
VOID
SetTDTokenDataToggle0 (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Set EndPoint Number the TD is targeting at.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  EndPoint      The Endport number of the target.

**/
VOID
SetTDTokenEndPoint (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINTN     EndPoint
  );

/**
  Set Device Address the TD is targeting at.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  DevAddr       The Device Address of the target.

**/
VOID
SetTDTokenDeviceAddress (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINTN     DevAddr
  );

/**
  Set Packet Identification the TD is targeting at.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.
  @param  PacketID      The Packet Identification of the target.

**/
VOID
SetTDTokenPacketID (
  IN  TD_STRUCT *PtrTDStruct,
  IN  UINT8     PacketID
  );

/**
  Set the beginning address of the data buffer that will be used
  during the transaction.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

**/
VOID
SetTDDataBuffer (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether the TD is active.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The TD is active or not.

**/
BOOLEAN
IsTDStatusActive (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether the TD is stalled.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The TD is stalled or not.

**/
BOOLEAN
IsTDStatusStalled (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether Data Buffer Error is happened.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The Data Buffer Error is happened or not.

**/
BOOLEAN
IsTDStatusBufferError (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether Babble Error is happened.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The Babble Error is happened or not.

**/
BOOLEAN
IsTDStatusBabbleError (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether NAK is received.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The NAK is received or not.

**/
BOOLEAN
IsTDStatusNAKReceived (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether CRC/Time Out Error is encountered.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The CRC/Time Out Error is encountered or not.

**/
BOOLEAN
IsTDStatusCRCTimeOutError (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Detect whether Bitstuff Error is received.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The Bitstuff Error is received or not.

**/
BOOLEAN
IsTDStatusBitStuffError (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Retrieve the actual number of bytes that were tansferred.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The actual number of bytes that were tansferred.

**/
UINT16
GetTDStatusActualLength (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Retrieve the information of whether the Link Pointer field is valid or not.

  @param  PtrTDStruct   Place to store TD_STRUCT pointer.

  @retval The linker pointer field is valid or not.

**/
BOOLEAN
GetTDLinkPtrValidorInvalid (
  IN  TD_STRUCT *PtrTDStruct
  );

/**
  Count TD Number from PtrFirstTD.

  @param  PtrFirstTD   Place to store TD_STRUCT pointer.

  @retval The queued TDs number.

**/
UINTN
CountTDsNumber (
  IN  TD_STRUCT *PtrFirstTD
  );

/**
  Link TD To QH.

  @param  PtrQH   Place to store QH_STRUCT pointer.
  @param  PtrTD   Place to store TD_STRUCT pointer.

**/
VOID
LinkTDToQH (
  IN  QH_STRUCT *PtrQH,
  IN  TD_STRUCT *PtrTD
  );

/**
  Link TD To TD.

  @param  PtrPreTD  Place to store TD_STRUCT pointer.
  @param  PtrTD     Place to store TD_STRUCT pointer.

**/
VOID
LinkTDToTD (
  IN  TD_STRUCT *PtrPreTD,
  IN  TD_STRUCT *PtrTD
  );

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
  );

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
  IN OUT UINTN     *ActualLen,
  IN     UINT8     *DataToggle,
  IN     UINTN     TimeOut,
  OUT    UINT32    *TransferResult
  );

/**
  Delete Queued TDs.

  @param  UhcDev       The UCHI device.
  @param  PtrFirstTD   Place to store TD_STRUCT pointer.

**/
VOID
DeleteQueuedTDs (
  IN USB_UHC_DEV     *UhcDev,
  IN TD_STRUCT       *PtrFirstTD
  );

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
  );

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
  );

/**
  Initialize UHCI memory management.

  @param  UhcDev                 The UCHI device.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
InitializeMemoryManagement (
  IN USB_UHC_DEV           *UhcDev
  );

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
  );

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
  );

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
  );

/**
  Insert a new memory header into list.

  @param  MemoryHeader         A pointer to the memory header list.
  @param  NewMemoryHeader      A new memory header to be inserted into the list.

**/
VOID
InsertMemoryHeaderToList (
  IN MEMORY_MANAGE_HEADER  *MemoryHeader,
  IN MEMORY_MANAGE_HEADER  *NewMemoryHeader
  );


/**
  Map address of request structure buffer.

  @param  Uhc                The UHCI device.
  @param  Request            The user request buffer.
  @param  MappedAddr         Mapped address of request.
  @param  Map                Identificaion of this mapping to return.

  @return EFI_SUCCESS        Success.
  @return EFI_DEVICE_ERROR   Fail to map the user request.

**/
EFI_STATUS
UhciMapUserRequest (
  IN  USB_UHC_DEV         *Uhc,
  IN  OUT VOID            *Request,
  OUT UINT8               **MappedAddr,
  OUT VOID                **Map
  );

/**
  Map address of user data buffer.

  @param  Uhc                The UHCI device.
  @param  Direction          Direction of the data transfer.
  @param  Data               The user data buffer.
  @param  Len                Length of the user data.
  @param  PktId              Packet identificaion.
  @param  MappedAddr         Mapped address to return.
  @param  Map                Identificaion of this mapping to return.

  @return EFI_SUCCESS        Success.
  @return EFI_DEVICE_ERROR   Fail to map the user data.

**/
EFI_STATUS
UhciMapUserData (
  IN  USB_UHC_DEV             *Uhc,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  VOID                    *Data,
  IN  OUT UINTN               *Len,
  OUT UINT8                   *PktId,
  OUT UINT8                   **MappedAddr,
  OUT VOID                    **Map
  );

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Operation              Indicates if the bus master is going to read or write to system memory.
  @param HostAddress            The system memory address to map to the PCI controller.
  @param NumberOfBytes          On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
IoMmuMap (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN EDKII_IOMMU_OPERATION  Operation,
  IN VOID                   *HostAddress,
  IN OUT UINTN              *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param IoMmu              Pointer to IOMMU PPI.
  @param Mapping            The mapping value returned from Map().

**/
VOID
IoMmuUnmap (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN VOID                  *Mapping
  );

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Pages                  The number of pages to allocate.
  @param HostAddress            A pointer to store the base system memory address of the
                                allocated range.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN UINTN                  Pages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );


/**
  Initialize IOMMU.

  @param IoMmu              Pointer to pointer to IOMMU PPI.

**/
VOID
IoMmuInit (
  OUT EDKII_IOMMU_PPI       **IoMmu
  );

#endif
