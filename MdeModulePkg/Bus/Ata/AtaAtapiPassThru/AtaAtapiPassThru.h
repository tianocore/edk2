/** @file
  Header file for ATA/ATAPI PASS THRU driver.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __ATA_ATAPI_PASS_THRU_H__
#define __ATA_ATAPI_PASS_THRU_H__

#include <Uefi.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Atapi.h>
#include <IndustryStandard/Scsi.h>

#include <Protocol/PciIo.h>
#include <Protocol/IdeControllerInit.h>
#include <Protocol/AtaPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/AtaAtapiPolicy.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PciLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DevicePathLib.h>

#include "IdeMode.h"
#include "AhciMode.h"

extern EFI_DRIVER_BINDING_PROTOCOL  gAtaAtapiPassThruDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gAtaAtapiPassThruComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL gAtaAtapiPassThruComponentName2;

extern EDKII_ATA_ATAPI_POLICY_PROTOCOL *mAtaAtapiPolicy;

#define ATA_ATAPI_PASS_THRU_SIGNATURE  SIGNATURE_32 ('a', 'a', 'p', 't')
#define ATA_ATAPI_DEVICE_SIGNATURE     SIGNATURE_32 ('a', 'd', 'e', 'v')
#define ATA_NONBLOCKING_TASK_SIGNATURE  SIGNATURE_32 ('a', 't', 's', 'k')

typedef struct _ATA_NONBLOCK_TASK ATA_NONBLOCK_TASK;

typedef enum {
  EfiAtaIdeMode,
  EfiAtaAhciMode,
  EfiAtaRaidMode,
  EfiAtaUnknownMode
} EFI_ATA_HC_WORK_MODE;

typedef enum {
  EfiIdeCdrom,                  /* ATAPI CDROM */
  EfiIdeHarddisk,               /* Hard Disk */
  EfiPortMultiplier,            /* Port Multiplier */
  EfiIdeUnknown
} EFI_ATA_DEVICE_TYPE;

//
// Ahci mode device info
//
typedef struct {
  UINT32                            Signature;
  LIST_ENTRY                        Link;

  UINT16                            Port;
  UINT16                            PortMultiplier;
  EFI_ATA_DEVICE_TYPE               Type;

  EFI_IDENTIFY_DATA                 *IdentifyData;
} EFI_ATA_DEVICE_INFO;

typedef struct {
  UINT32                            Signature;

  EFI_HANDLE                        ControllerHandle;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  EFI_IDE_CONTROLLER_INIT_PROTOCOL  *IdeControllerInit;

  EFI_ATA_PASS_THRU_MODE            AtaPassThruMode;
  EFI_ATA_PASS_THRU_PROTOCOL        AtaPassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE       ExtScsiPassThruMode;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL   ExtScsiPassThru;

  EFI_ATA_HC_WORK_MODE              Mode;

  EFI_IDE_REGISTERS                 IdeRegisters[EfiIdeMaxChannel];
  EFI_AHCI_REGISTERS                AhciRegisters;

  //
  // The attached device list
  //
  LIST_ENTRY                        DeviceList;
  UINT64                            EnabledPciAttributes;
  UINT64                            OriginalPciAttributes;

  //
  // For AtaPassThru protocol, using the following bytes to record the previous call in
  // GetNextPort()/GetNextDevice().
  //
  UINT16                            PreviousPort;
  UINT16                            PreviousPortMultiplier;
  //
  // For ExtScsiPassThru protocol, using the following bytes to record the previous call in
  // GetNextTarget()/GetNextTargetLun().
  //
  UINT16                            PreviousTargetId;
  UINT64                            PreviousLun;

  //
  // For Non-blocking.
  //
  EFI_EVENT                         TimerEvent;
  LIST_ENTRY                        NonBlockingTaskList;
} ATA_ATAPI_PASS_THRU_INSTANCE;

//
// Task for Non-blocking mode.
//
struct _ATA_NONBLOCK_TASK {
  UINT32                            Signature;
  LIST_ENTRY                        Link;

  UINT16                            Port;
  UINT16                            PortMultiplier;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  *Packet;
  BOOLEAN                           IsStart;
  EFI_EVENT                         Event;
  UINT64                            RetryTimes;
  BOOLEAN                           InfiniteWait;
  VOID                              *Map;            // Pointer to map.
  VOID                              *TableMap;       // Pointer to PRD table map.
  EFI_ATA_DMA_PRD                   *MapBaseAddress; //  Pointer to range Base address for Map.
  UINTN                             PageCount;       //  The page numbers used by PCIO freebuffer.
};

//
// Timeout value which uses 100ns as a unit.
// It means 3 second span.
//
#define ATA_ATAPI_TIMEOUT           EFI_TIMER_PERIOD_SECONDS(3)
#define ATA_SPINUP_TIMEOUT          EFI_TIMER_PERIOD_SECONDS(10)

#define IS_ALIGNED(addr, size)      (((UINTN) (addr) & (size - 1)) == 0)

#define ATA_PASS_THRU_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      ATA_ATAPI_PASS_THRU_INSTANCE, \
      AtaPassThru, \
      ATA_ATAPI_PASS_THRU_SIGNATURE \
      )

#define EXT_SCSI_PASS_THRU_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      ATA_ATAPI_PASS_THRU_INSTANCE, \
      ExtScsiPassThru, \
      ATA_ATAPI_PASS_THRU_SIGNATURE \
      )

#define ATA_ATAPI_DEVICE_INFO_FROM_THIS(a) \
  CR (a, \
      EFI_ATA_DEVICE_INFO, \
      Link, \
      ATA_ATAPI_DEVICE_SIGNATURE \
      );

#define ATA_NON_BLOCK_TASK_FROM_ENTRY(a) \
  CR (a, \
      ATA_NONBLOCK_TASK, \
      Link, \
      ATA_NONBLOCKING_TASK_SIGNATURE \
      );

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
AtaAtapiPassThruComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
AtaAtapiPassThruComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
AtaAtapiPassThruSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN EFI_HANDLE                        Controller,
  IN EFI_DEVICE_PATH_PROTOCOL          *RemainingDevicePath
  );

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
AtaAtapiPassThruStart (
  IN EFI_DRIVER_BINDING_PROTOCOL        *This,
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  );

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
AtaAtapiPassThruStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN  EFI_HANDLE                        Controller,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer
  );

/**
  Traverse the attached ATA devices list to find out the device to access.

  @param[in]  Instance            A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.
  @param[in]  Port                The port number of the ATA device to send the command.
  @param[in]  PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                  If there is no port multiplier, then specify 0xFFFF.
  @param[in]  DeviceType          The device type of the ATA device.

  @retval     The pointer to the data structure of the device info to access.

**/
LIST_ENTRY *
EFIAPI
SearchDeviceInfoList (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN  UINT16                         Port,
  IN  UINT16                         PortMultiplier,
  IN  EFI_ATA_DEVICE_TYPE            DeviceType
  );

/**
  Allocate device info data structure to contain device info.
  And insert the data structure to the tail of device list for tracing.

  @param[in]  Instance            A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.
  @param[in]  Port                The port number of the ATA device to send the command.
  @param[in]  PortMultiplierPort  The port multiplier port number of the ATA device to send the command.
                                  If there is no port multiplier, then specify 0xFFFF.
  @param[in]  DeviceType          The device type of the ATA device.
  @param[in]  IdentifyData        The data buffer to store the output of the IDENTIFY cmd.

  @retval EFI_SUCCESS             Successfully insert the ata device to the tail of device list.
  @retval EFI_OUT_OF_RESOURCES    Can not allocate enough resource for use.

**/
EFI_STATUS
EFIAPI
CreateNewDeviceInfo (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN  UINT16                         Port,
  IN  UINT16                         PortMultiplier,
  IN  EFI_ATA_DEVICE_TYPE            DeviceType,
  IN  EFI_IDENTIFY_DATA              *IdentifyData
  );

/**
  Destroy all attached ATA devices info.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

**/
VOID
EFIAPI
DestroyDeviceInfoList (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE  *Instance
  );

/**
  Destroy all pending non blocking tasks.

  @param[in]  Instance    A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.
  @param[in]  IsSigEvent  Indicate whether signal the task event when remove the
                          task.

**/
VOID
EFIAPI
DestroyAsynTaskList (
  IN ATA_ATAPI_PASS_THRU_INSTANCE *Instance,
  IN BOOLEAN                       IsSigEvent
  );

/**
  Enumerate all attached ATA devices at IDE mode or AHCI mode separately.

  The function is designed to enumerate all attached ATA devices.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

  @retval EFI_SUCCESS           Successfully enumerate attached ATA devices.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
EnumerateAttachedDevice (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE      *Instance
  );

/**
  Call back function when the timer event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the
                        Event.

**/
VOID
EFIAPI
AsyncNonBlockingTransferRoutine (
  EFI_EVENT  Event,
  VOID*      Context
  );

/**
  Sends an ATA command to an ATA device that is attached to the ATA controller. This function
  supports both blocking I/O and non-blocking I/O. The blocking I/O functionality is required,
  and the non-blocking I/O functionality is optional.

  @param[in]      This               A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]      Port               The port number of the ATA device to send the command.
  @param[in]      PortMultiplierPort The port multiplier port number of the ATA device to send the command.
                                     If there is no port multiplier, then specify 0xFFFF.
  @param[in, out] Packet             A pointer to the ATA command to send to the ATA device specified by Port
                                     and PortMultiplierPort.
  @param[in]      Event              If non-blocking I/O is not supported then Event is ignored, and blocking
                                     I/O is performed. If Event is NULL, then blocking I/O is performed. If
                                     Event is not NULL and non blocking I/O is supported, then non-blocking
                                     I/O is performed, and Event will be signaled when the ATA command completes.

  @retval EFI_SUCCESS                The ATA command was sent by the host. For bi-directional commands,
                                     InTransferLength bytes were transferred from InDataBuffer. For write and
                                     bi-directional commands, OutTransferLength bytes were transferred by OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE        The ATA command was not executed. The number of bytes that could be transferred
                                     is returned in InTransferLength. For write and bi-directional commands,
                                     OutTransferLength bytes were transferred by OutDataBuffer.
  @retval EFI_NOT_READY              The ATA command could not be sent because there are too many ATA commands
                                     already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting to send the ATA command.
  @retval EFI_INVALID_PARAMETER      Port, PortMultiplierPort, or the contents of Acb are invalid. The ATA
                                     command was not sent, so no additional status information is available.

**/
EFI_STATUS
EFIAPI
AtaPassThruPassThru (
  IN     EFI_ATA_PASS_THRU_PROTOCOL       *This,
  IN     UINT16                           Port,
  IN     UINT16                           PortMultiplierPort,
  IN OUT EFI_ATA_PASS_THRU_COMMAND_PACKET *Packet,
  IN     EFI_EVENT                        Event OPTIONAL
  );

/**
  Used to retrieve the list of legal port numbers for ATA devices on an ATA controller.
  These can either be the list of ports where ATA devices are actually present or the
  list of legal port numbers for the ATA controller. Regardless, the caller of this
  function must probe the port number returned to see if an ATA device is actually
  present at that location on the ATA controller.

  The GetNextPort() function retrieves the port number on an ATA controller. If on input
  Port is 0xFFFF, then the port number of the first port on the ATA controller is returned
  in Port and EFI_SUCCESS is returned.

  If Port is a port number that was returned on a previous call to GetNextPort(), then the
  port number of the next port on the ATA controller is returned in Port, and EFI_SUCCESS
  is returned. If Port is not 0xFFFF and Port was not returned on a previous call to
  GetNextPort(), then EFI_INVALID_PARAMETER is returned.

  If Port is the port number of the last port on the ATA controller, then EFI_NOT_FOUND is
  returned.

  @param[in]      This          A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in, out] Port          On input, a pointer to the port number on the ATA controller.
                                On output, a pointer to the next port number on the ATA
                                controller. An input value of 0xFFFF retrieves the first port
                                number on the ATA controller.

  @retval EFI_SUCCESS           The next port number on the ATA controller was returned in Port.
  @retval EFI_NOT_FOUND         There are no more ports on this ATA controller.
  @retval EFI_INVALID_PARAMETER Port is not 0xFFFF and Port was not returned on a previous call
                                to GetNextPort().

**/
EFI_STATUS
EFIAPI
AtaPassThruGetNextPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN OUT UINT16                 *Port
  );

/**
  Used to retrieve the list of legal port multiplier port numbers for ATA devices on a port of an ATA
  controller. These can either be the list of port multiplier ports where ATA devices are actually
  present on port or the list of legal port multiplier ports on that port. Regardless, the caller of this
  function must probe the port number and port multiplier port number returned to see if an ATA
  device is actually present.

  The GetNextDevice() function retrieves the port multiplier port number of an ATA device
  present on a port of an ATA controller.

  If PortMultiplierPort points to a port multiplier port number value that was returned on a
  previous call to GetNextDevice(), then the port multiplier port number of the next ATA device
  on the port of the ATA controller is returned in PortMultiplierPort, and EFI_SUCCESS is
  returned.

  If PortMultiplierPort points to 0xFFFF, then the port multiplier port number of the first
  ATA device on port of the ATA controller is returned in PortMultiplierPort and
  EFI_SUCCESS is returned.

  If PortMultiplierPort is not 0xFFFF and the value pointed to by PortMultiplierPort
  was not returned on a previous call to GetNextDevice(), then EFI_INVALID_PARAMETER
  is returned.

  If PortMultiplierPort is the port multiplier port number of the last ATA device on the port of
  the ATA controller, then EFI_NOT_FOUND is returned.

  @param[in]      This               A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]      Port               The port number present on the ATA controller.
  @param[in, out] PortMultiplierPort On input, a pointer to the port multiplier port number of an
                                     ATA device present on the ATA controller.
                                     If on input a PortMultiplierPort of 0xFFFF is specified,
                                     then the port multiplier port number of the first ATA device
                                     is returned. On output, a pointer to the port multiplier port
                                     number of the next ATA device present on an ATA controller.

  @retval EFI_SUCCESS                The port multiplier port number of the next ATA device on the port
                                     of the ATA controller was returned in PortMultiplierPort.
  @retval EFI_NOT_FOUND              There are no more ATA devices on this port of the ATA controller.
  @retval EFI_INVALID_PARAMETER      PortMultiplierPort is not 0xFFFF, and PortMultiplierPort was not
                                     returned on a previous call to GetNextDevice().

**/
EFI_STATUS
EFIAPI
AtaPassThruGetNextDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port,
  IN OUT UINT16                 *PortMultiplierPort
  );

/**
  Used to allocate and build a device path node for an ATA device on an ATA controller.

  The BuildDevicePath() function allocates and builds a single device node for the ATA
  device specified by Port and PortMultiplierPort. If the ATA device specified by Port and
  PortMultiplierPort is not present on the ATA controller, then EFI_NOT_FOUND is returned.
  If DevicePath is NULL, then EFI_INVALID_PARAMETER is returned. If there are not enough
  resources to allocate the device path node, then EFI_OUT_OF_RESOURCES is returned.

  Otherwise, DevicePath is allocated with the boot service AllocatePool(), the contents of
  DevicePath are initialized to describe the ATA device specified by Port and PortMultiplierPort,
  and EFI_SUCCESS is returned.

  @param[in]      This               A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]      Port               Port specifies the port number of the ATA device for which a
                                     device path node is to be allocated and built.
  @param[in]      PortMultiplierPort The port multiplier port number of the ATA device for which a
                                     device path node is to be allocated and built. If there is no
                                     port multiplier, then specify 0xFFFF.
  @param[in, out] DevicePath         A pointer to a single device path node that describes the ATA
                                     device specified by Port and PortMultiplierPort. This function
                                     is responsible for allocating the buffer DevicePath with the
                                     boot service AllocatePool(). It is the caller's responsibility
                                     to free DevicePath when the caller is finished with DevicePath.
  @retval EFI_SUCCESS                The device path node that describes the ATA device specified by
                                     Port and PortMultiplierPort was allocated and returned in DevicePath.
  @retval EFI_NOT_FOUND              The ATA device specified by Port and PortMultiplierPort does not
                                     exist on the ATA controller.
  @retval EFI_INVALID_PARAMETER      DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources to allocate DevicePath.

**/
EFI_STATUS
EFIAPI
AtaPassThruBuildDevicePath (
  IN     EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN     UINT16                     Port,
  IN     UINT16                     PortMultiplierPort,
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath
  );

/**
  Used to translate a device path node to a port number and port multiplier port number.

  The GetDevice() function determines the port and port multiplier port number associated with
  the ATA device described by DevicePath. If DevicePath is a device path node type that the
  ATA Pass Thru driver supports, then the ATA Pass Thru driver will attempt to translate the contents
  DevicePath into a port number and port multiplier port number.

  If this translation is successful, then that port number and port multiplier port number are returned
  in Port and PortMultiplierPort, and EFI_SUCCESS is returned.

  If DevicePath, Port, or PortMultiplierPort are NULL, then EFI_INVALID_PARAMETER is returned.

  If DevicePath is not a device path node type that the ATA Pass Thru driver supports, then
  EFI_UNSUPPORTED is returned.

  If DevicePath is a device path node type that the ATA Pass Thru driver supports, but there is not
  a valid translation from DevicePath to a port number and port multiplier port number, then
  EFI_NOT_FOUND is returned.

  @param[in]  This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]  DevicePath          A pointer to the device path node that describes an ATA device on the
                                  ATA controller.
  @param[out] Port                On return, points to the port number of an ATA device on the ATA controller.
  @param[out] PortMultiplierPort  On return, points to the port multiplier port number of an ATA device
                                  on the ATA controller.

  @retval EFI_SUCCESS             DevicePath was successfully translated to a port number and port multiplier
                                  port number, and they were returned in Port and PortMultiplierPort.
  @retval EFI_INVALID_PARAMETER   DevicePath is NULL.
  @retval EFI_INVALID_PARAMETER   Port is NULL.
  @retval EFI_INVALID_PARAMETER   PortMultiplierPort is NULL.
  @retval EFI_UNSUPPORTED         This driver does not support the device path node type in DevicePath.
  @retval EFI_NOT_FOUND           A valid translation from DevicePath to a port number and port multiplier
                                  port number does not exist.

**/
EFI_STATUS
EFIAPI
AtaPassThruGetDevice (
  IN  EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  OUT UINT16                     *Port,
  OUT UINT16                     *PortMultiplierPort
  );

/**
  Resets a specific port on the ATA controller. This operation also resets all the ATA devices
  connected to the port.

  The ResetChannel() function resets an a specific port on an ATA controller. This operation
  resets all the ATA devices connected to that port. If this ATA controller does not support
  a reset port operation, then EFI_UNSUPPORTED is returned.

  If a device error occurs while executing that port reset operation, then EFI_DEVICE_ERROR is
  returned.

  If a timeout occurs during the execution of the port reset operation, then EFI_TIMEOUT is returned.

  If the port reset operation is completed, then EFI_SUCCESS is returned.

  @param[in]  This          A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in]  Port          The port number on the ATA controller.

  @retval EFI_SUCCESS       The ATA controller port was reset.
  @retval EFI_UNSUPPORTED   The ATA controller does not support a port reset operation.
  @retval EFI_DEVICE_ERROR  A device error occurred while attempting to reset the ATA port.
  @retval EFI_TIMEOUT       A timeout occurred while attempting to reset the ATA port.

**/
EFI_STATUS
EFIAPI
AtaPassThruResetPort (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port
  );

/**
  Resets an ATA device that is connected to an ATA controller.

  The ResetDevice() function resets the ATA device specified by Port and PortMultiplierPort.
  If this ATA controller does not support a device reset operation, then EFI_UNSUPPORTED is
  returned.

  If Port or PortMultiplierPort are not in a valid range for this ATA controller, then
  EFI_INVALID_PARAMETER is returned.

  If a device error occurs while executing that device reset operation, then EFI_DEVICE_ERROR
  is returned.

  If a timeout occurs during the execution of the device reset operation, then EFI_TIMEOUT is
  returned.

  If the device reset operation is completed, then EFI_SUCCESS is returned.

  @param[in] This                A pointer to the EFI_ATA_PASS_THRU_PROTOCOL instance.
  @param[in] Port                Port represents the port number of the ATA device to be reset.
  @param[in] PortMultiplierPort  The port multiplier port number of the ATA device to reset.
                                 If there is no port multiplier, then specify 0xFFFF.
  @retval EFI_SUCCESS            The ATA device specified by Port and PortMultiplierPort was reset.
  @retval EFI_UNSUPPORTED        The ATA controller does not support a device reset operation.
  @retval EFI_INVALID_PARAMETER  Port or PortMultiplierPort are invalid.
  @retval EFI_DEVICE_ERROR       A device error occurred while attempting to reset the ATA device
                                 specified by Port and PortMultiplierPort.
  @retval EFI_TIMEOUT            A timeout occurred while attempting to reset the ATA device
                                 specified by Port and PortMultiplierPort.

**/
EFI_STATUS
EFIAPI
AtaPassThruResetDevice (
  IN EFI_ATA_PASS_THRU_PROTOCOL *This,
  IN UINT16                     Port,
  IN UINT16                     PortMultiplierPort
  );

/**
  Sends a SCSI Request Packet to a SCSI device that is attached to the SCSI channel. This function
  supports both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the
  nonblocking I/O functionality is optional.

  @param  This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target  The Target is an array of size TARGET_MAX_BYTES and it represents
                  the id of the SCSI device to send the SCSI Request Packet. Each
                  transport driver may choose to utilize a subset of this size to suit the needs
                  of transport target representation. For example, a Fibre Channel driver
                  may use only 8 bytes (WWN) to represent an FC target.
  @param  Lun     The LUN of the SCSI device to send the SCSI Request Packet.
  @param  Packet  A pointer to the SCSI Request Packet to send to the SCSI device
                  specified by Target and Lun.
  @param  Event   If nonblocking I/O is not supported then Event is ignored, and blocking
                  I/O is performed. If Event is NULL, then blocking I/O is performed. If
                  Event is not NULL and non blocking I/O is supported, then
                  nonblocking I/O is performed, and Event will be signaled when the
                  SCSI Request Packet completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE   The SCSI Request Packet was not executed. The number of bytes that
                                could be transferred is returned in InTransferLength. For write
                                and bi-directional commands, OutTransferLength bytes were
                                transferred by OutDataBuffer.
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because there are too many
                                SCSI Request Packets already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_INVALID_PARAMETER Target, Lun, or the contents of ScsiRequestPacket are invalid.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet is not supported
                                by the host adapter. This includes the case of Bi-directional SCSI
                                commands not supported by the implementation. The SCSI Request
                                Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                    *This,
  IN UINT8                                              *Target,
  IN UINT64                                             Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET     *Packet,
  IN EFI_EVENT                                          Event OPTIONAL
  );

/**
  Used to retrieve the list of legal Target IDs and LUNs for SCSI devices on a SCSI channel. These
  can either be the list SCSI devices that are actually present on the SCSI channel, or the list of legal
  Target Ids and LUNs for the SCSI channel. Regardless, the caller of this function must probe the
  Target ID and LUN returned to see if a SCSI device is actually present at that location on the SCSI
  channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target On input, a pointer to the Target ID (an array of size
                 TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.
  @param  Lun    On input, a pointer to the LUN of a SCSI device present on the SCSI
                 channel. On output, a pointer to the LUN of the next SCSI device present
                 on a SCSI channel.

  @retval EFI_SUCCESS           The Target ID and LUN of the next SCSI device on the SCSI
                                channel was returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER Target array is not all 0xF, and Target and Lun were
                                not returned on a previous call to GetNextTargetLun().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruGetNextTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN OUT UINT8                           **Target,
  IN OUT UINT64                          *Lun
  );

/**
  Used to allocate and build a device path node for a SCSI device on a SCSI channel.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target     The Target is an array of size TARGET_MAX_BYTES and it specifies the
                     Target ID of the SCSI device for which a device path node is to be
                     allocated and built. Transport drivers may chose to utilize a subset of
                     this size to suit the representation of targets. For example, a Fibre
                     Channel driver may use only 8 bytes (WWN) in the array to represent a
                     FC target.
  @param  Lun        The LUN of the SCSI device for which a device path node is to be
                     allocated and built.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     specified by Target and Lun. This function is responsible for
                     allocating the buffer DevicePath with the boot service
                     AllocatePool(). It is the caller's responsibility to free
                     DevicePath when the caller is finished with DevicePath.

  @retval EFI_SUCCESS           The device path node that describes the SCSI device specified by
                                Target and Lun was allocated and returned in
                                DevicePath.
  @retval EFI_INVALID_PARAMETER DevicePath is NULL.
  @retval EFI_NOT_FOUND         The SCSI devices specified by Target and Lun does not exist
                                on the SCSI channel.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to allocate DevicePath.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN     UINT8                              *Target,
  IN     UINT64                             Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL           **DevicePath
  );

/**
  Used to translate a device path node to a Target ID and LUN.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     on the SCSI channel.
  @param  Target     A pointer to the Target Array which represents the ID of a SCSI device
                     on the SCSI channel.
  @param  Lun        A pointer to the LUN of a SCSI device on the SCSI channel.

  @retval EFI_SUCCESS           DevicePath was successfully translated to a Target ID and
                                LUN, and they were returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER DevicePath or Target or Lun is NULL.
  @retval EFI_NOT_FOUND         A valid translation from DevicePath to a Target ID and LUN
                                does not exist.
  @retval EFI_UNSUPPORTED       This driver does not support the device path node type in
                                 DevicePath.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN  EFI_DEVICE_PATH_PROTOCOL           *DevicePath,
  OUT UINT8                              **Target,
  OUT UINT64                             *Lun
  );

/**
  Resets a SCSI channel. This operation resets all the SCSI devices connected to the SCSI channel.

  @param  This A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @retval EFI_SUCCESS      The SCSI channel was reset.
  @retval EFI_DEVICE_ERROR A device error occurred while attempting to reset the SCSI channel.
  @retval EFI_TIMEOUT      A timeout occurred while attempting to reset the SCSI channel.
  @retval EFI_UNSUPPORTED  The SCSI channel does not support a channel reset operation.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruResetChannel (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL   *This
  );

/**
  Resets a SCSI logical unit that is connected to a SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target The Target is an array of size TARGET_MAX_BYTE and it represents the
                 target port ID of the SCSI device containing the SCSI logical unit to
                 reset. Transport drivers may chose to utilize a subset of this array to suit
                 the representation of their targets.
  @param  Lun    The LUN of the SCSI device to reset.

  @retval EFI_SUCCESS           The SCSI device specified by Target and Lun was reset.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           A timeout occurred while attempting to reset the SCSI device
                                specified by Target and Lun.
  @retval EFI_UNSUPPORTED       The SCSI channel does not support a target reset operation.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to reset the SCSI device
                                 specified by Target and Lun.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN UINT8                              *Target,
  IN UINT64                             Lun
  );

/**
  Used to retrieve the list of legal Target IDs for SCSI devices on a SCSI channel. These can either
  be the list SCSI devices that are actually present on the SCSI channel, or the list of legal Target IDs
  for the SCSI channel. Regardless, the caller of this function must probe the Target ID returned to
  see if a SCSI device is actually present at that location on the SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target (TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.

  @retval EFI_SUCCESS           The Target ID of the next SCSI device on the SCSI
                                channel was returned in Target.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           Target array is not all 0xF, and Target was not
                                returned on a previous call to GetNextTarget().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
ExtScsiPassThruGetNextTarget (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *This,
  IN OUT UINT8                           **Target
  );

/**
  Initialize ATA host controller at IDE mode.

  The function is designed to initialize ATA host controller.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

**/
EFI_STATUS
EFIAPI
IdeModeInitialization (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE    *Instance
  );

/**
  Initialize ATA host controller at AHCI mode.

  The function is designed to initialize ATA host controller.

  @param[in]  Instance          A pointer to the ATA_ATAPI_PASS_THRU_INSTANCE instance.

**/
EFI_STATUS
EFIAPI
AhciModeInitialization (
  IN  ATA_ATAPI_PASS_THRU_INSTANCE    *Instance
  );

/**
  Start a non data transfer on specific port.

  @param[in]       PciIo               The PCI IO protocol instance.
  @param[in]       AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtapiCommand        The atapi command will be used for the
                                       transfer.
  @param[in]       AtapiCommandLength  The length of the atapi command.
  @param[in]       AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
                                       used by non-blocking mode.

  @retval EFI_DEVICE_ERROR    The non data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The non data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciNonDataTransfer (
  IN     EFI_PCI_IO_PROTOCOL           *PciIo,
  IN     EFI_AHCI_REGISTERS            *AhciRegisters,
  IN     UINT8                         Port,
  IN     UINT8                         PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND        *AtapiCommand OPTIONAL,
  IN     UINT8                         AtapiCommandLength,
  IN     EFI_ATA_COMMAND_BLOCK         *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock,
  IN     UINT64                        Timeout,
  IN     ATA_NONBLOCK_TASK             *Task
  );

/**
  Start a DMA data transfer on specific port

  @param[in]       Instance            The ATA_ATAPI_PASS_THRU_INSTANCE protocol instance.
  @param[in]       AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtapiCommand        The atapi command will be used for the
                                       transfer.
  @param[in]       AtapiCommandLength  The length of the atapi command.
  @param[in]       Read                The transfer direction.
  @param[in]       AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param[in, out]  MemoryAddr          The pointer to the data buffer.
  @param[in]       DataCount           The data count to be transferred.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
                                       used by non-blocking mode.

  @retval EFI_DEVICE_ERROR    The DMA data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The DMA data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciDmaTransfer (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE *Instance,
  IN     EFI_AHCI_REGISTERS           *AhciRegisters,
  IN     UINT8                        Port,
  IN     UINT8                        PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND       *AtapiCommand OPTIONAL,
  IN     UINT8                        AtapiCommandLength,
  IN     BOOLEAN                      Read,
  IN     EFI_ATA_COMMAND_BLOCK        *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK         *AtaStatusBlock,
  IN OUT VOID                         *MemoryAddr,
  IN     UINT32                       DataCount,
  IN     UINT64                       Timeout,
  IN     ATA_NONBLOCK_TASK            *Task
  );

/**
  Start a PIO data transfer on specific port.

  @param[in]       PciIo               The PCI IO protocol instance.
  @param[in]       AhciRegisters       The pointer to the EFI_AHCI_REGISTERS.
  @param[in]       Port                The number of port.
  @param[in]       PortMultiplier      The timeout value of stop.
  @param[in]       AtapiCommand        The atapi command will be used for the
                                       transfer.
  @param[in]       AtapiCommandLength  The length of the atapi command.
  @param[in]       Read                The transfer direction.
  @param[in]       AtaCommandBlock     The EFI_ATA_COMMAND_BLOCK data.
  @param[in, out]  AtaStatusBlock      The EFI_ATA_STATUS_BLOCK data.
  @param[in, out]  MemoryAddr          The pointer to the data buffer.
  @param[in]       DataCount           The data count to be transferred.
  @param[in]       Timeout             The timeout value of non data transfer, uses 100ns as a unit.
  @param[in]       Task                Optional. Pointer to the ATA_NONBLOCK_TASK
                                       used by non-blocking mode.

  @retval EFI_DEVICE_ERROR    The PIO data transfer abort with error occurs.
  @retval EFI_TIMEOUT         The operation is time out.
  @retval EFI_UNSUPPORTED     The device is not ready for transfer.
  @retval EFI_SUCCESS         The PIO data transfer executes successfully.

**/
EFI_STATUS
EFIAPI
AhciPioTransfer (
  IN     EFI_PCI_IO_PROTOCOL        *PciIo,
  IN     EFI_AHCI_REGISTERS         *AhciRegisters,
  IN     UINT8                      Port,
  IN     UINT8                      PortMultiplier,
  IN     EFI_AHCI_ATAPI_COMMAND     *AtapiCommand OPTIONAL,
  IN     UINT8                      AtapiCommandLength,
  IN     BOOLEAN                    Read,
  IN     EFI_ATA_COMMAND_BLOCK      *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK       *AtaStatusBlock,
  IN OUT VOID                       *MemoryAddr,
  IN     UINT32                     DataCount,
  IN     UINT64                     Timeout,
  IN     ATA_NONBLOCK_TASK          *Task
  );

/**
  Send ATA command into device with NON_DATA protocol

  @param[in]      PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE
                                   data structure.
  @param[in]      IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param[in]      AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data
                                   structure.
  @param[in, out] AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
  @param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
                                   used by non-blocking mode.

  @retval  EFI_SUCCESS Reading succeed
  @retval  EFI_ABORTED Command failed
  @retval  EFI_DEVICE_ERROR Device status error.

**/
EFI_STATUS
EFIAPI
AtaNonDataCommandIn (
  IN     EFI_PCI_IO_PROTOCOL       *PciIo,
  IN     EFI_IDE_REGISTERS         *IdeRegisters,
  IN     EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK      *AtaStatusBlock,
  IN     UINT64                    Timeout,
  IN     ATA_NONBLOCK_TASK         *Task
  );

/**
  Perform an ATA Udma operation (Read, ReadExt, Write, WriteExt).

  @param[in]      Instance         A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data
                                   structure.
  @param[in]      IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param[in]      Read             Flag used to determine the data transfer
                                   direction. Read equals 1, means data transferred
                                   from device to host;Read equals 0, means data
                                   transferred from host to device.
  @param[in]      DataBuffer       A pointer to the source buffer for the data.
  @param[in]      DataLength       The length of  the data.
  @param[in]      AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param[in, out] AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
  @param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
                                   used by non-blocking mode.

  @retval EFI_SUCCESS          the operation is successful.
  @retval EFI_OUT_OF_RESOURCES Build PRD table failed
  @retval EFI_UNSUPPORTED      Unknown channel or operations command
  @retval EFI_DEVICE_ERROR     Ata command execute failed

**/
EFI_STATUS
EFIAPI
AtaUdmaInOut (
  IN     ATA_ATAPI_PASS_THRU_INSTANCE  *Instance,
  IN     EFI_IDE_REGISTERS             *IdeRegisters,
  IN     BOOLEAN                       Read,
  IN     VOID                          *DataBuffer,
  IN     UINT64                        DataLength,
  IN     EFI_ATA_COMMAND_BLOCK         *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK          *AtaStatusBlock,
  IN     UINT64                        Timeout,
  IN     ATA_NONBLOCK_TASK             *Task
  );

/**
  This function is used to send out ATA commands conforms to the PIO Data In Protocol.

  @param[in]      PciIo            A pointer to ATA_ATAPI_PASS_THRU_INSTANCE data
                                   structure.
  @param[in]      IdeRegisters     A pointer to EFI_IDE_REGISTERS data structure.
  @param[in, out] Buffer           A pointer to the source buffer for the data.
  @param[in]      ByteCount        The length of  the data.
  @param[in] Read                  Flag used to determine the data transfer direction.
                                   Read equals 1, means data transferred from device
                                   to host;Read equals 0, means data transferred
                                   from host to device.
  @param[in]      AtaCommandBlock  A pointer to EFI_ATA_COMMAND_BLOCK data structure.
  @param[in, out] AtaStatusBlock   A pointer to EFI_ATA_STATUS_BLOCK data structure.
  @param[in]      Timeout          The time to complete the command, uses 100ns as a unit.
  @param[in]      Task             Optional. Pointer to the ATA_NONBLOCK_TASK
                                   used by non-blocking mode.

  @retval EFI_SUCCESS      send out the ATA command and device send required data successfully.
  @retval EFI_DEVICE_ERROR command sent failed.

**/
EFI_STATUS
EFIAPI
AtaPioDataInOut (
  IN     EFI_PCI_IO_PROTOCOL       *PciIo,
  IN     EFI_IDE_REGISTERS         *IdeRegisters,
  IN OUT VOID                      *Buffer,
  IN     UINT64                    ByteCount,
  IN     BOOLEAN                   Read,
  IN     EFI_ATA_COMMAND_BLOCK     *AtaCommandBlock,
  IN OUT EFI_ATA_STATUS_BLOCK      *AtaStatusBlock,
  IN     UINT64                    Timeout,
  IN     ATA_NONBLOCK_TASK         *Task
  );

#endif

