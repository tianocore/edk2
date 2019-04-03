/** @file

Copyright (c) 1999 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BIOS_SNP_16_H_
#define _BIOS_SNP_16_H_

#include <Uefi.h>

#include <Protocol/LegacyBios.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/PciIo.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/EventGroup.h>

#include <IndustryStandard/Pci.h>

#include "Pxe.h"

//
// BIOS Simple Network Protocol Device Structure
//
#define EFI_SIMPLE_NETWORK_DEV_SIGNATURE    SIGNATURE_32 ('s', 'n', '1', '6')

#define INIT_PXE_STATUS                     0xabcd

#define EFI_SIMPLE_NETWORK_MAX_TX_FIFO_SIZE 64

typedef struct {
  UINT32  First;
  UINT32  Last;
  VOID *  Data[EFI_SIMPLE_NETWORK_MAX_TX_FIFO_SIZE];
} EFI_SIMPLE_NETWORK_DEV_FIFO;

typedef struct {
  UINTN                                     Signature;
  EFI_HANDLE                                Handle;
  EFI_SIMPLE_NETWORK_PROTOCOL               SimpleNetwork;
  EFI_SIMPLE_NETWORK_MODE                   SimpleNetworkMode;
  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL Nii;
  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath;
  EFI_PCI_IO_PROTOCOL                       *PciIo;
  EFI_LEGACY_BIOS_PROTOCOL                  *LegacyBios;

  //
  // Local Data for Simple Network Protocol interface goes here
  //
  BOOLEAN                                   UndiLoaded;
  EFI_EVENT                                 EfiBootEvent;
  EFI_EVENT                                 LegacyBootEvent;
  UINT16                                    PxeEntrySegment;
  UINT16                                    PxeEntryOffset;
  EFI_SIMPLE_NETWORK_DEV_FIFO               TxBufferFifo;
  EFI_DEVICE_PATH_PROTOCOL                  *BaseDevicePath;
  PXE_T                                     *Pxe;                   ///< Pointer to !PXE structure
  PXENV_UNDI_GET_INFORMATION_T              GetInformation;         ///< Data from GET INFORMATION
  PXENV_UNDI_GET_NIC_TYPE_T                 GetNicType;             ///< Data from GET NIC TYPE
  PXENV_UNDI_GET_NDIS_INFO_T                GetNdisInfo;            ///< Data from GET NDIS INFO
  BOOLEAN                                   IsrValid;               ///< TRUE if Isr contains valid data
  PXENV_UNDI_ISR_T                          Isr;                    ///< Data from ISR
  PXENV_UNDI_TBD_T                          *Xmit;                  //
  VOID                                      *TxRealModeMediaHeader; ///< < 1 MB Size = 0x100
  VOID                                      *TxRealModeDataBuffer;  ///< < 1 MB Size = GetInformation.MaxTranUnit
  VOID                                      *TxDestAddr;            ///< < 1 MB Size = 16
  UINT8                                     InterruptStatus;        ///< returned/cleared by GetStatus, set in ISR
  UINTN                                     UndiLoaderTablePages;
  UINTN                                     DestinationDataSegmentPages;
  UINTN                                     DestinationStackSegmentPages;
  UINTN                                     DestinationCodeSegmentPages;
  VOID                                      *UndiLoaderTable;
  VOID                                      *DestinationDataSegment;
  VOID                                      *DestinationStackSegment;
  VOID                                      *DestinationCodeSegment;
} EFI_SIMPLE_NETWORK_DEV;

#define EFI_SIMPLE_NETWORK_DEV_FROM_THIS(a) \
  CR (a, \
      EFI_SIMPLE_NETWORK_DEV, \
      SimpleNetwork, \
      EFI_SIMPLE_NETWORK_DEV_SIGNATURE \
      )

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gBiosSnp16DriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gBiosSnp16ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gBiosSnp16ComponentName2;


//
// Driver Binding Protocol functions
//
/**
  Tests to see if this driver supports a given controller.

  @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param Controller           The handle of the controller to test.
  @param RemainingDevicePath  A pointer to the remaining portion of a device path.

  @retval EFI_SUCCESS    The driver supports given controller.
  @retval EFI_UNSUPPORT  The driver doesn't support given controller.
  @retval Other          Other errors prevent driver finishing to test
                         if the driver supports given controller.
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
;

/**
  Starts the Snp device controller

  @param This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param Controller           The handle of the controller to test.
  @param RemainingDevicePath  A pointer to the remaining portion of a device path.

  @retval  EFI_SUCCESS          - The device was started.
  @retval  EFI_DEVICE_ERROR     - The device could not be started due to a device error.
  @retval  EFI_OUT_OF_RESOURCES - The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
;

/**
  Stops the device by given device controller.

  @param This               A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param Controller         The handle of the controller to test.
  @param NumberOfChildren   The number of child device handles in ChildHandleBuffer.
  @param ChildHandleBuffer  An array of child handles to be freed. May be NULL if
                            NumberOfChildren is 0.

  @retval  EFI_SUCCESS      - The device was stopped.
  @retval  EFI_DEVICE_ERROR - The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
BiosSnp16DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
;

//
// Simple Network Protocol functions
//
/**
  Call 16 bit UNDI ROM to start the network interface

  @param This       A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.

  @retval EFI_DEVICE_ERROR Network interface has not be initialized.
  @retval EFI_DEVICE_ERROR Fail to execute 16 bit ROM call.
  @retval EFI_SUCESS       Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStart (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
;

/**
  Call 16 bit UNDI ROM to stop the network interface

  @param This       A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.

  @retval EFI_DEVICE_ERROR Network interface has not be initialized.
  @retval EFI_DEVICE_ERROR Fail to execute 16 bit ROM call.
  @retval EFI_SUCESS       Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
;

/**
  Initialize network interface

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param ExtraRxBufferSize    The size of extra request receive buffer.
  @param ExtraTxBufferSize    The size of extra request transmit buffer.

  @retval EFI_DEVICE_ERROR Fail to execute 16 bit ROM call.
  @retval EFI_SUCESS       Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkInitialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL                     *This,
  IN UINTN                                           ExtraRxBufferSize  OPTIONAL,
  IN UINTN                                           ExtraTxBufferSize  OPTIONAL
  )
;

/**
  Reset network interface.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param ExtendedVerification Need extended verfication.

  @retval EFI_INVALID_PARAMETER Invalid This parameter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkReset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  )
;

/**
  Shutdown network interface.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.

  @retval EFI_INVALID_PARAMETER Invalid This parameter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
;

/**
  Reset network interface.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param Enable               Enable mask value
  @param Disable              Disable mask value
  @param ResetMCastFilter     Whether reset multi cast filter or not
  @param MCastFilterCnt       Count of mutli cast filter for different MAC address
  @param MCastFilter          Buffer for mustli cast filter for different MAC address.

  @retval EFI_INVALID_PARAMETER Invalid This parameter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL                     * This,
  IN UINT32                                          Enable,
  IN UINT32                                          Disable,
  IN BOOLEAN                                         ResetMCastFilter,
  IN UINTN                                           MCastFilterCnt     OPTIONAL,
  IN EFI_MAC_ADDRESS                                 * MCastFilter OPTIONAL
  )
;

/**
  Set new MAC address.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param Reset                Whether reset station MAC address to permanent address
  @param New                  A pointer to New address

  @retval EFI_INVALID_PARAMETER Invalid This parameter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  IN BOOLEAN                      Reset,
  IN EFI_MAC_ADDRESS              * New OPTIONAL
  )
;

/**
  Collect statistics.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param Reset                Whether cleanup old statistics data.
  @param StatisticsSize       The buffer of statistics table.
  @param StatisticsTable      A pointer to statistics buffer.

  @retval EFI_INVALID_PARAMETER Invalid This parameter.
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkStatistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  IN BOOLEAN                      Reset,
  IN OUT UINTN                    *StatisticsSize OPTIONAL,
  OUT EFI_NETWORK_STATISTICS      * StatisticsTable OPTIONAL
  )
;

/**
  Translate IP address to MAC address.

  @param This                 A pointer to EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @param IPv6                 IPv6 or IPv4
  @param IP                   A pointer to given Ip address.
  @param MAC                  On return, translated MAC address.

  @retval EFI_INVALID_PARAMETER Invalid This parameter.
  @retval EFI_INVALID_PARAMETER Invalid IP address.
  @retval EFI_INVALID_PARAMETER Invalid return buffer for holding MAC address.
  @retval EFI_UNSUPPORTED       Do not support IPv6
  @retval EFI_DEVICE_ERROR      Network device has not been initialized.
  @retval EFI_NOT_STARTED       Network device has been stopped.
  @retval EFI_DEVICE_ERROR      Invalid status for network device
  @retval EFI_SUCCESS           Success operation.
**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkMCastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      IPv6,
  IN EFI_IP_ADDRESS               *IP,
  OUT EFI_MAC_ADDRESS             *MAC
  )
;

/**
  Performs read and write operations on the NVRAM device attached to a
  network interface.

  @param  This       The protocol instance pointer.
  @param  ReadWrite  TRUE for read operations, FALSE for write operations.
  @param  Offset     Byte offset in the NVRAM device at which to start the read or
                     write operation. This must be a multiple of NvRamAccessSize and
                     less than NvRamSize.
  @param  BufferSize The number of bytes to read or write from the NVRAM device.
                     This must also be a multiple of NvramAccessSize.
  @param  Buffer     A pointer to the data buffer.

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkNvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      Write,
  IN UINTN                        Offset,
  IN UINTN                        BufferSize,
  IN OUT VOID                     *Buffer
  )
;

/**
  Reads the current interrupt status and recycled transmit buffer status from
  a network interface.

  @param  This            The protocol instance pointer.
  @param  InterruptStatus A pointer to the bit mask of the currently active interrupts
                          If this is NULL, the interrupt status will not be read from
                          the device. If this is not NULL, the interrupt status will
                          be read from the device. When the  interrupt status is read,
                          it will also be cleared. Clearing the transmit  interrupt
                          does not empty the recycled transmit buffer array.
  @param  TxBuf           Recycled transmit buffer address. The network interface will
                          not transmit if its internal recycled transmit buffer array
                          is full. Reading the transmit buffer does not clear the
                          transmit interrupt. If this is NULL, then the transmit buffer
                          status will not be read. If there are no transmit buffers to
                          recycle and TxBuf is not NULL, * TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkGetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  * This,
  OUT UINT32                      *InterruptStatus OPTIONAL,
  OUT VOID                        **TxBuf OPTIONAL
  )
;

/**
  Places a packet in the transmit queue of a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header to be filled in by
                     the Transmit() function. If HeaderSize is non-zero, then it
                     must be equal to This->Mode->MediaHeaderSize and the DestAddr
                     and Protocol parameters must not be NULL.
  @param  BufferSize The size, in bytes, of the entire packet (media header and
                     data) to be transmitted through the network interface.
  @param  Buffer     A pointer to the packet (media header followed by data) to be
                     transmitted. This parameter cannot be NULL. If HeaderSize is zero,
                     then the media header in Buffer must already be filled in by the
                     caller. If HeaderSize is non-zero, then the media header will be
                     filled in by the Transmit() function.
  @param  SrcAddr    The source HW MAC address. If HeaderSize is zero, then this parameter
                     is ignored. If HeaderSize is non-zero and SrcAddr is NULL, then
                     This->Mode->CurrentAddress is used for the source HW MAC address.
  @param  DestAddr   The destination HW MAC address. If HeaderSize is zero, then this
                     parameter is ignored.
  @param  Protocol   The type of header to build. If HeaderSize is zero, then this
                     parameter is ignored. See RFC 1700, section "Ether Types", for
                     examples.

  @retval EFI_SUCCESS           The packet was placed on the transmit queue.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this transmit request.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkTransmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        HeaderSize,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer,
  IN EFI_MAC_ADDRESS              *SrcAddr OPTIONAL,
  IN EFI_MAC_ADDRESS              *DestAddr OPTIONAL,
  IN UINT16                       *Protocol OPTIONAL
  )
;

/**
  Receives a packet from a network interface.

  @param  This       The protocol instance pointer.
  @param  HeaderSize The size, in bytes, of the media header received on the network
                     interface. If this parameter is NULL, then the media header size
                     will not be returned.
  @param  BufferSize On entry, the size, in bytes, of Buffer. On exit, the size, in
                     bytes, of the packet that was received on the network interface.
  @param  Buffer     A pointer to the data buffer to receive both the media header and
                     the data.
  @param  SrcAddr    The source HW MAC address. If this parameter is NULL, the
                     HW MAC source address will not be extracted from the media
                     header.
  @param  DestAddr   The destination HW MAC address. If this parameter is NULL,
                     the HW MAC destination address will not be extracted from the
                     media header.
  @param  Protocol   The media header type. If this parameter is NULL, then the
                     protocol will not be extracted from the media header. See
                     RFC 1700 section "Ether Types" for examples.

  @retval  EFI_SUCCESS           The received data was stored in Buffer, and BufferSize has
                                 been updated to the number of bytes received.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
Undi16SimpleNetworkReceive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  OUT UINTN                       *HeaderSize OPTIONAL,
  IN OUT UINTN                    *BufferSize,
  OUT VOID                        *Buffer,
  OUT EFI_MAC_ADDRESS             *SrcAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  OUT UINT16                      *Protocol OPTIONAL
  )
;

/**
  wait for a packet to be received.

  @param Event      Event used with WaitForEvent() to wait for a packet to be received.
  @param Context    Event Context

**/
VOID
EFIAPI
Undi16SimpleNetworkWaitForPacket (
  IN EFI_EVENT               Event,
  IN VOID                    *Context
  )
;

/**
  Check whether packet is ready for receive.

  @param This The protocol instance pointer.

  @retval  EFI_SUCCESS           Receive data is ready.
  @retval  EFI_NOT_STARTED       The network interface has not been started.
  @retval  EFI_NOT_READY         The network interface is too busy to accept this transmit
                                 request.
  @retval  EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval  EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval  EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval  EFI_UNSUPPORTED       This function is not supported by the network interface.
**/
EFI_STATUS
Undi16SimpleNetworkCheckForPacket (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
;

/**
 Cache Interrupt verctor address converted from IVT number.

 @param VectorNumber  IVT number

 @retval EFI_SUCCESS Success to operation.
**/
EFI_STATUS
CacheVectorAddress (
  UINT8   VectorNumber
  )
;

/**
 Get interrupt vector address according to IVT number.

 @param VectorNumber    Given IVT number

 @return cached interrupt vector address.
**/
EFI_STATUS
RestoreCachedVectorAddress (
  UINT8   VectorNumber
  )
;

/**
  If available, launch the BaseCode from a NIC option ROM.
  This should install the !PXE and PXENV+ structures in memory for
  subsequent use.


  @param SimpleNetworkDevice    Simple network device instance
  @param RomAddress             The ROM base address for NIC rom.

  @retval EFI_NOT_FOUND         The check sum does not match
  @retval EFI_NOT_FOUND         Rom ID offset is wrong
  @retval EFI_NOT_FOUND         No Rom ID structure is found
**/
EFI_STATUS
LaunchBaseCode (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  UINTN                   RomAddress
  )
;

/**
  PXE
  START UNDI
  Op-Code: PXENV_START_UNDI (0000h)
  Input: Far pointer to a PXENV_START_UNDI_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This service is used to pass the BIOS parameter registers to the UNDI driver. The UNDI driver is
  responsible for saving the information it needs to communicate with the hardware.
  This service is also responsible for hooking the Int 1Ah service routine
  Note: This API service must be called only once during UNDI Option ROM boot.
  The UNDI driver is responsible for saving this information and using it every time
  PXENV_UNDI_STARTUP is called.
  Service cannot be used in protected mode.
  typedef struct  {
      PXENV_STATUS Status;
      UINT16 AX;
      UINT16 BX;
      UINT16 DX;
      UINT16 DI;
      UINT16 ES;
  } PXENV_START_UNDI_T;
  Set before calling API service
  AX, BX, DX, DI, ES: BIOS initialization parameter registers. These
  fields should contain the same information passed to the option ROM
  initialization routine by the Host System BIOS. Information about the
  contents of these registers can be found in the [PnP], [PCI] and
  [BBS] specifications.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeStartUndi (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_START_UNDI_T               *PxeUndiTable
  )
;

/**
  PXE
  UNDI STARTUP
  Op-Code: PXENV_UNDI_STARTUP (0001h)
  Input: Far pointer to a PXENV_UNDI_STARTUP_T parameter structure that has been initialized by the
  caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This API is responsible for initializing the contents of the UNDI code & data segment for proper
  operation. Information from the !PXE structure and the first PXENV_START_UNDI API call is used
  to complete this initialization. The rest of the UNDI APIs will not be available until this call has
  been completed.
  Note: PXENV_UNDI_STARTUP must not be called again without first calling
  PXENV_UNDI_SHUTDOWN.
  PXENV_UNDI_STARTUP and PXENV_UNDI_SHUTDOWN are no longer responsible for
  chaining interrupt 1Ah. This must be done by the PXENV_START_UNDI and
  PXENV_STOP_UNDI API calls.
  This service cannot be used in protected mode.
  typedef struct
  {
      PXENV_STATUS Status;
  } PXENV_UNDI_STARTUP_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiStartup (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_STARTUP_T             *PxeUndiTable
  )
;

/**
  PXE
  UNDI CLEANUP
  Op-Code: PXENV_UNDI_CLEANUP (0002h)
  Input: Far pointer to a PXENV_UNDI_CLEANUP_T parameter structure.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field
  in the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call will prepare the network adapter driver to be unloaded from memory. This call must be
  made just before unloading the Universal NIC Driver. The rest of the API will not be available
  after this call executes.
  This service cannot be used in protected mode.
  typedef struct {
      PXENX_STATUS Status;
  } PXENV_UNDI_CLEANUP_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiCleanup (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_CLEANUP_T             *PxeUndiTable
  )
;

/**
  PXE
  UNDI INITIALIZE
  Op-Code: PXENV_UNDI_INITIALIZE (0003h)
  Input: Far pointer to a PXENV_UNDI_INITIALIZE_T parameter structure that has been initialized by the
  caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets the adapter and programs it with default parameters. The default parameters used
  are those supplied to the most recent UNDI_STARTUP call. This routine does not enable the
  receive and transmit units of the network adapter to readily receive or transmit packets. The
  application must call PXENV_UNDI_OPEN to logically connect the network adapter to the network.
  This call must be made by an application to establish an interface to the network adapter driver.
  Note: When the PXE code makes this call to initialize the network adapter, it passes a NULL pointer for
  the Protocol field in the parameter structure.
  typedef struct {
    PXENV_STATUS Status;
    ADDR32 ProtocolIni;
    UINT8 reserved[8];
  } PXENV_UNDI_INITIALIZE_T;
  Set before calling API service
  ProtocolIni: Physical address of a memory copy of the driver
  module from the protocol.ini file obtained from the protocol manager
  driver (refer to the NDIS 2.0 specification). This parameter is
  supported for the universal NDIS driver to pass the information
  contained in the protocol.ini file to the NIC driver for any specific
  configuration of the NIC. (Note that the module identification in the
  protocol.ini file was done by NDIS.) This value can be NULL for any
  other application interfacing to the universal NIC driver
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance.
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiInitialize (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_INITIALIZE_T          *PxeUndiTable
  )
;

/**
  Wrapper routine for reset adapter.

  PXE
  UNDI RESET ADAPTER
  Op-Code: PXENV_UNDI_RESET_ADAPTER (0004h)
  Input: Far pointer to a PXENV_UNDI_RESET_ADAPTER_t parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets and reinitializes the network adapter with the same set of parameters supplied to
  Initialize Routine. Unlike Initialize, this call opens the adapter that is, it connects logically to the
  network. This routine cannot be used to replace Initialize or Shutdown calls.
  typedef struct {
    PXENV_STATUS Status;
    PXENV_UNDI_MCAST_ADDRESS_t    R_Mcast_Buf;
  } PXENV_UNDI_RESET_T;

  #define MAXNUM_MCADDR 8

  typedef struct {
    UINT16 MCastAddrCount;
    MAC_ADDR McastAddr[MAXNUM_MCADDR];
  } PXENV_UNDI_MCAST_ADDRESS_t;

  Set before calling API service
  R_Mcast_Buf: This is a structure of MCastAddrCount and
  McastAddr.
  MCastAddrCount: Number of multicast MAC addresses in the
  buffer.
  McastAddr: List of up to MAXNUM_MCADDR multicast MAC
  addresses.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance.
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.
  @param  RxFilter             Filter setting mask value for PXE recive .

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiResetNic (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_RESET_T               *PxeUndiTable,
  IN UINT16                               RxFilter
  )
;

/**
  PXE
  UNDI SHUTDOWN
  Op-Code: PXENV_UNDI_SHUTDOWN (0005h)
  Input: Far pointer to a PXENV_UNDI_SHUTDOWN_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call resets the network adapter and leaves it in a safe state for another driver to program it.
  Note: The contents of the PXENV_UNDI_STARTUP parameter structure need to be saved by the
  Universal NIC Driver in case PXENV_UNDI_INITIALIZE is called again.
  typedef struct
  {
    PXENV_STATUS Status;
  } PXENV_UNDI_SHUTDOWN_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiShutdown (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_SHUTDOWN_T            *PxeUndiTable
  )
;

/**
  PXE
  UNDI OPEN
  Op-Code: PXENV_UNDI_OPEN (0006h)
  Input: Far pointer to a PXENV_UNDI_OPEN_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call activates the adapter network connection and sets the adapter ready to accept packets
  for transmit and receive.
  typedef struct {
    PXENV_STATUS Status;
    UINT16 OpenFlag;
    UINT16 PktFilter;
      #define FLTR_DIRECTED 0x0001
      #define FLTR_BRDCST 0x0002
      #define FLTR_PRMSCS 0x0004
      #define FLTR_SRC_RTG 0x0008
    PXENV_UNDI_MCAST_ADDRESS_t R_Mcast_Buf;
  } PXENV_UNDI_OPEN_T;
  Set before calling API service
  OpenFlag: This is an adapter specific input parameter. This is
  supported for the universal NDIS 2.0 driver to pass in the open flags
  provided by the protocol driver. (See the NDIS 2.0 specification.)
  This can be zero.
  PktFilter: Filter for receiving packets. This can be one, or more, of
  the FLTR_xxx constants. Multiple values are arithmetically or-ed
  together.
  directed packets are packets that may come to your MAC address
  or the multicast MAC address.
  R_Mcast_Buf: See definition in UNDI RESET ADAPTER (0004h).
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiOpen (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_OPEN_T                *PxeUndiTable
  )
;

/**
  PXE
  UNDI CLOSE
  Op-Code: PXENV_UNDI_CLOSE (0007h)
  Input: Far pointer to a PXENV_UNDI_CLOSE_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call disconnects the network adapter from the network. Packets cannot be transmitted or
  received until the network adapter is open again.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_UNDI_CLOSE_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiClose (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_CLOSE_T               *PxeUndiTable
  )
;

/**
  PXE
  UNDI TRANSMIT PACKET
  Op-Code: PXENV_UNDI_TRANSMIT (0008h)
  Input: Far pointer to a PXENV_UNDI_TRANSMIT_T parameter structure that
  has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX.
  The status code must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call transmits a buffer to the network. The media header
  for the packet can be filled by the calling protocol, but it might not be.
  The network adapter driver will fill it if required by the values in the
  parameter block. The packet is buffered for transmission provided there is
  an available buffer, and the function returns PXENV_EXIT_SUCCESS. If no
  buffer is available the function returns PXENV_EXIT_FAILURE with a status
  code of PXE_UNDI_STATUS__OUT OF_RESOURCE. The number of buffers is
  implementation-dependent. An interrupt is generated on completion of the
  transmission of one or more packets. A call to PXENV_UNDI_TRANSMIT is
  permitted in the context of a transmit complete interrupt.

  typedef struct {
    PXENV_STATUS Status;
    UINT8 Protocol;
      #define P_UNKNOWN 0
      #define P_IP 1
      #define P_ARP 2
      #define P_RARP 3
    UINT8 XmitFlag;
      #define XMT_DESTADDR 0x0000
      #define XMT_BROADCAST 0x0001
    SEGOFF16 DestAddr;
    SEGOFF16 TBD;
    UINT32 Reserved[2];
  } t_PXENV_UNDI_TRANSMIT;

  #define MAX_DATA_BLKS 8

  typedef struct {
    UINT16 ImmedLength;
    SEGOFF16 Xmit;
    UINT16 DataBlkCount;
    struct DataBlk {
      UINT8 TDPtrType;
      UINT8 TDRsvdByte;
      UINT16 TDDataLen;
      SEGOFF16 TDDataPtr;
    } DataBlock[MAX_DATA_BLKS];
  } PXENV_UNDI_TBD_T

  Set before calling API service
  Protocol: This is the protocol of the upper layer that is calling UNDI
  TRANSMIT call. If the upper layer has filled the media header, this
  field must be P_UNKNOWN.
  XmitFlag: If this flag is XMT_DESTADDR, the NIC driver expects a
  pointer to the destination media address in the field DestAddr. If
  XMT_BROADCAST, the NIC driver fills the broadcast address for the
  destination.
  TBD: Segment:Offset address of the transmit buffer descriptor.
  ImmedLength: Length of the immediate transmit buffer: Xmit.
  Xmit: Segment:Offset of the immediate transmit buffer.
  DataBlkCount: Number of blocks in this transmit buffer.
  TDPtrType:
  0 => 32-bit physical address in TDDataPtr (not supported in this
  version of PXE)
  1 => segment:offset in TDDataPtr which can be a real mode or 16-bit
  protected mode pointer
  TDRsvdByte: Reserved must be zero.
  TDDatalen: Data block length in bytes.
  TDDataPtr: Segment:Offset of the transmit block.
  DataBlock: Array of transmit data blocks.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiTransmit (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_TRANSMIT_T            *PxeUndiTable
  )
;


/**
  PXE
  UNDI SET STATION ADDRESS
  Op-Code: PXENV_UNDI_SET_STATION_ADDRESS (000Ah)
  Input: Far pointer to a PXENV_UNDI_SET_STATION_ADDRESS_t parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call sets the MAC address to be the input value and is called before opening the network
  adapter. Later, the open call uses this variable as a temporary MAC address to program the
  adapter individual address registers.
  typedef struct {
    PXENV_STATUS Status;
    MAC_ADDR StationAddress;
  } PXENV_UNDI_SET_STATION_ADDR_T;
  Set before calling API service
  StationAddress: Temporary MAC address to be used for
  transmit and receive.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiSetStationAddr (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_SET_STATION_ADDR_T    *PxeUndiTable
  )
;


/**
  PXE
  UNDI GET INFORMATION
  Op-Code: PXENV_UNDI_GET_INFORMATION (000Ch)
  Input: Far pointer to a PXENV_UNDI_GET_INFORMATION_T parameter structure that has been
  initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call copies the network adapter variables, including the MAC address, into the input buffer.
  Note: The PermNodeAddress field must be valid after PXENV_START_UNDI and
  PXENV_UNDI_STARTUP have been issued. All other fields must be valid after
  PXENV_START_UNDI, PXENV_UNDI_STARTUP and PXENV_UNDI_INITIALIZE have been
  called.
  typedef struct {
    PXENV_STATUS Status;
    UINT16 BaseIo;
    UINT16 IntNumber;
    UINT16 MaxTranUnit;
    UINT16 HwType;
      #define ETHER_TYPE 1
      #define EXP_ETHER_TYPE 2
      #define IEEE_TYPE 6
      #define ARCNET_TYPE 7
    UINT16 HwAddrLen;
    MAC_ADDR CurrentNodeAddress;
    MAC_ADDR PermNodeAddress;
    SEGSEL ROMAddress;
    UINT16 RxBufCt;
    UINT16 TxBufCt;
  } PXENV_UNDI_GET_INFORMATION_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  BaseIO: Adapter base I/O address.
  IntNumber: Adapter IRQ number.
  MaxTranUnit: Adapter maximum transmit unit.
  HWType: Type of protocol at the hardware level.
  HWAddrLen: Length of the hardware address.
  CurrentNodeAddress: Current hardware address.
  PermNodeAddress: Permanent hardware address.
  ROMAddress: Real mode ROM segment address.
  RxBufCnt: Receive queue length.
  TxBufCnt: Transmit queue length.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiGetInformation (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_INFORMATION_T     *PxeUndiTable
  )
;

/**
  PXE
  UNDI GET STATISTICS
  Op-Code: PXENV_UNDI_GET_STATISTICS (000Dh)
  Input: Far pointer to a PXENV_UNDI_GET_STATISTICS_T parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call reads statistical information from the network adapter, and returns.
  typedef struct {
    PXENV_STATUS Status;
    UINT32 XmtGoodFrames;
    UINT32 RcvGoodFrames;
    UINT32 RcvCRCErrors;
    UINT32 RcvResourceErrors;
  } PXENV_UNDI_GET_STATISTICS_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  XmtGoodFrames: Number of successful transmissions.
  RcvGoodFrames: Number of good frames received.
  RcvCRCErrors: Number of frames received with CRC
  error.
  RcvResourceErrors: Number of frames discarded
  because receive queue was full.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiGetStatistics (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_STATISTICS_T      *PxeUndiTable
  )
;

/**
  PXE
  UNDI CLEAR STATISTICS
  Op-Code: PXENV_UNDI_CLEAR_STATISTICS (000Eh)
  Input: Far pointer to a PXENV_UNDI_CLEAR_STATISTICS_T parameter.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the
  PXENV_STATUS_xxx constants.
  Description: This call clears the statistical information from the network adapter.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_UNDI_CLEAR_STATISTICS_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiClearStatistics (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_CLEAR_STATISTICS_T    *PxeUndiTable
  )
;


/**
  PXE
  UNDI GET MULTICAST ADDRESS
  Op-Code: PXENV_UNDI_GET_MCAST_ADDRESS (0011h)
  Input: Far pointer to a PXENV_GET_MCAST_ADDRESS_t parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This call converts the given IP multicast address to a hardware multicast address.
  typedef struct  {
    PXENV_STATUS Status;
    IP4 InetAddr;
    MAC_ADDR MediaAddr;
  } PXENV_UNDI_GET_MCAST_ADDR_T;
  Set before calling API service
  InetAddr: IP multicast address.
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  MediaAddr: MAC multicast address.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiGetMcastAddr (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_MCAST_ADDR_T      *PxeUndiTable
  )
;

/**
  PXE
  UNDI GET NIC TYPE
  Op-Code: PXENV_UNDI_GET_NIC_TYPE (0012h)
  Input: Far pointer to a PXENV_UNDI_GET_NIC_TYPE_T parameter structure that has been initialized by
  the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants. If the PXENV_EXIT_SUCCESS is returned the parameter structure must contain the
  NIC information.
  Description: This call, if successful, provides the NIC-specific information necessary to identify the network
  adapter that is used to boot the system.
  Note: The application first gets the DHCPDISCOVER packet using GET_CACHED_INFO and checks if
  the UNDI is supported before making this call. If the UNDI is not supported, the NIC-specific
  information can be obtained from the DHCPDISCOVER packet itself.
  PXENV_START_UNDI, PXENV_UNDI_STARTUP and PXENV_UNDI_INITIALIZE must be called
  before the information provided is valid.
  typedef {
    PXENV_STATUS Status;
    UINT8 NicType;
      #define PCI_NIC 2
      #define PnP_NIC 3
      #define CardBus_NIC 4
    Union {
      Struct {
        UINT16 Vendor_ID;
        UINT16 Dev_ID;
        UINT8 Base_Class;
        UINT8 Sub_Class;
        UINT8 Prog_Intf;
        UINT8 Rev;
        UINT16 BusDevFunc;
        UINT16 SubVendor_ID;
        UINT16 SubDevice_ID;
      } pci, cardbus;
      struct {
        UINT32 EISA_Dev_ID;
        UINT8 Base_Class;
        UINT8 Sub_Class;
        UINT8 Prog_Intf;
        UINT16 CardSelNum;
      } pnp;
    } info;
  } PXENV_UNDI_GET_NIC_TYPE_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  NICType: Type of NIC information stored in the parameter
  structure.
  Info: Information about the fields in this union can be found
  in the [PnP] and [PCI] specifications

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiGetNicType (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_NIC_TYPE_T        *PxeUndiTable
  )
;

/**
  PXE
  UNDI GET IFACE INFO
  Op-Code: PXENV_UNDI_GET_IFACE_INFO (0013h)
  Input: Far pointer to a PXENV_UNDI_GET_IFACE_INFO_t parameter structure that has been initialized
  by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants. If the PXENV_EXIT_SUCCESS is returned, the parameter structure must contain the
  interface specific information.
  Description: This call, if successful, provides the network interface specific information such as the interface
  type at the link layer (Ethernet, Tokenring) and the link speed. This information can be used in the
  universal drivers such as NDIS or Miniport to communicate to the upper protocol modules.
  Note: UNDI follows the NDIS2 specification in giving this information. It is the responsibility of the
  universal driver to translate/convert this information into a format that is required in its specification
  or to suit the expectation of the upper level protocol modules.
  PXENV_START_UNDI, PXENV_UNDI_STARTUP and PXENV_UNDI_INITIALIZE must be called
  before the information provided is valid.
  typedef struct {
    PXENV_STATUS Status
    UINT8 IfaceType[16];
    UINT32 LinkSpeed;
    UINT32 ServiceFlags;
    UINT32 Reserved[4];
  } PXENV_UNDI_GET_NDIS_INFO_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.
  IfaceType: Name of MAC type in ASCIIZ format. This is
  used by the universal NDIS driver to specify its driver type
  to the protocol driver.
  LinkSpeed: Defined in the NDIS 2.0 specification.
  ServiceFlags: Defined in the NDIS 2.0 specification.
  Reserved: Must be zero.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiGetNdisInfo (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_GET_NDIS_INFO_T       *PxeUndiTable
  )
;

/**
  PXE
  UNDI ISR
  Op-Code: PXENV_UNDI_ISR (0014h)
  Input: Far pointer to a PXENV_UNDI_ISR_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This API function will be called at different levels of processing the interrupt. The FuncFlag field in
  the parameter block indicates the operation to be performed for the call. This field is filled with the
  status of that operation on return.
  Note: Interrupt Service Routine Operation:
  In this design the UNDI does not hook the interrupt for the Network Interface. Instead, the
  application or the protocol driver hooks the interrupt and calls UNDI with the PXENV_UNDI_ISR
  API call for interrupt verification (PXENV_UNDI_ISR_IN_START) and processing
  (PXENV_UNDI_ISR_IN_PROCESS and PXENV_UNDI_ISR_GET_NEXT).
  When the Network Interface HW generates an interrupt the protocol driver interrupt service
  routine (ISR) gets control and takes care of the interrupt processing at the PIC level. The ISR then
  calls the UNDI using the PXENV_UNDI_ISR API with the value PXENV_UNDI_ISR_IN_START for
  the FuncFlag parameter. At this time UNDI must disable the interrupts at the Network Interface
  level and read any status values required to further process the interrupt. UNDI must return as
  quickly as possible with one of the two values, PXENV_UNDI_ISR_OUT_OURS or
  PXENV_UNDI_ISR_OUT_NOT_OURS, for the parameter FuncFlag depending on whether the
  interrupt was generated by this particular Network Interface or not.
  If the value returned in FuncFlag is PXENV_UNDI_ISR_OUT_NOT_OURS, then the interrupt was
  not generated by our NIC, and interrupt processing is complete.
  If the value returned in FuncFlag is PXENV_UNDI_ISR_OUT_OURS, the protocol driver must start
  a handler thread and send an end-of-interrupt (EOI) command to the PIC. Interrupt processing is
  now complete.
  The protocol driver strategy routine will call UNDI using this same API with FuncFlag equal to
  PXENV_UNDI_ISR_IN_PROCESS. At this time UNDI must find the cause of this interrupt and
  return the status in the FuncFlag. It first checks if there is a frame received and if so it returns the
  first buffer pointer of that frame in the parameter block.
  The protocol driver calls UNDI repeatedly with the FuncFlag equal to
  PXENV_UNDI_ISR_IN_GET_NEXT to get all the buffers in a frame and also all the received
  frames in the queue. On this call, UNDI must remember the previous buffer given to the protoco,l
  remove it from the receive queue and recycle it. In case of a multi-buffered frame, if the previous
  buffer is not the last buffer in the frame it must return the next buffer in the frame in the parameter
  block. Otherwise it must return the first buffer in the next frame.
  If there is no received frame pending to be processed, UNDI processes the transmit completes and
  if there is no other interrupt status to be processed, UNDI re-enables the interrupt at the
  NETWORK INTERFACE level and returns PXENV_UNDI_ISR_OUT_DONE in the FuncFlag.
  IMPORTANT: It is possible for the protocol driver to be interrupted again while in the
  strategy routine when the UNDI re-enables interrupts.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiIsr (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_UNDI_ISR_T                 *PxeUndiTable
  )
;

/**
  PXE
  STOP UNDI
  Op-Code: PXENV_STOP_UNDI (0015h)
  Input: Far pointer to a PXENV_STOP_UNDI_T parameter structure that has been initialized by the caller.
  Output: PXENV_EXIT_SUCCESS or PXENV_EXIT_FAILURE must be returned in AX. The status field in
  the parameter structure must be set to one of the values represented by the PXENV_STATUS_xxx
  constants.
  Description: This routine is responsible for unhooking the Int 1Ah service routine.
  Note: This API service must be called only once at the end of UNDI Option ROM boot. One of the valid
  status codes is PXENV_STATUS_KEEP. If this status is returned, UNDI must not be removed from
  base memory. Also, UNDI must not be removed from base memory if BC is not removed from base
  memory.
  Service cannot be used in protected mode.
  typedef struct {
    PXENV_STATUS Status;
  } PXENV_STOP_UNDI_T;
  Set before calling API service
  N/A
  Returned from API service
  Status: See the PXENV_STATUS_xxx constants.

  @param  SimpleNetworkDevice   Device instance
  @param  PxeUndiTable          Point to structure which hold parameter and return value
                                for option ROM call.

  @return Return value of PXE option ROM far call.
**/
EFI_STATUS
PxeUndiStop (
  IN EFI_SIMPLE_NETWORK_DEV               *SimpleNetworkDevice,
  IN OUT PXENV_STOP_UNDI_T                *PxeUndiTable
  )
;


/**
  Effect the Far Call into the PXE Layer

  Note: When using a 32-bit stack segment do not push 32-bit words onto the stack. The PXE API
  services will not work, unless there are three 16-bit parameters pushed onto the stack.
      push DS                                 ;Far pointer to parameter structure
      push offset pxe_data_call_struct        ;is pushed onto stack.
      push Index                              ;UINT16 is pushed onto stack.
      call dword ptr (s_PXE ptr es:[di]).EntryPointSP
      add sp, 6 ;Caller cleans up stack.

  @param SimpleNetworkDevice    Device instance for simple network
  @param Table                 Point to parameter/retun value table for legacy far call
  @param TableSize              The size of parameter/return value table
  @param CallIndex              The index of legacy call.

  @return EFI_STATUS
**/
EFI_STATUS
MakePxeCall (
  EFI_SIMPLE_NETWORK_DEV  *SimpleNetworkDevice,
  IN OUT VOID             *Table,
  IN UINTN                TableSize,
  IN UINT16               CallIndex
  )
;

/**
  Allocate buffer below 1M for real mode.

  @param NumPages     The number pages want to be allocated.
  @param Buffer       On return, allocated buffer.

  @return Status of allocating pages.
**/
EFI_STATUS
BiosSnp16AllocatePagesBelowOneMb (
  UINTN  NumPages,
  VOID   **Buffer
  )
;

#endif
