/** @file
  Header file for IDE Bus Driver.

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IDE_BUS_H_
#define _IDE_BUS_H_



#include <FrameworkDxe.h>

#include <Protocol/IdeControllerInit.h>
#include <Protocol/BlockIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include <Guid/EventGroup.h>

#include <IndustryStandard/Pci.h>
#include "IdeData.h"

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL      gIDEBusDriverBinding;
extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL  gIDEBusDriverDiagnostics;
extern EFI_DRIVER_DIAGNOSTICS2_PROTOCOL gIDEBusDriverDiagnostics2;

//
// Extra Definition to porting
//
#define MAX_IDE_DEVICE    4
#define MAX_IDE_CHANNELS  2
#define MAX_IDE_DRIVES    2

#define INVALID_DEVICE_TYPE 0xff
#define ATA_DEVICE_TYPE     0x00
#define ATAPI_DEVICE_TYPE   0x01

typedef struct {
  BOOLEAN HaveScannedDevice[MAX_IDE_DEVICE];
  BOOLEAN DeviceFound[MAX_IDE_DEVICE];
  BOOLEAN DeviceProcessed[MAX_IDE_DEVICE];
} IDE_BUS_DRIVER_PRIVATE_DATA;

#define IDE_BLK_IO_DEV_SIGNATURE  SIGNATURE_32 ('i', 'b', 'i', 'd')

typedef struct {
  UINT32                      Signature;

  EFI_HANDLE                  Handle;
  EFI_BLOCK_IO_PROTOCOL       BlkIo;
  EFI_BLOCK_IO_MEDIA          BlkMedia;
  EFI_DISK_INFO_PROTOCOL      DiskInfo;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  IDE_BUS_DRIVER_PRIVATE_DATA *IdeBusDriverPrivateData;

  //
  // Local Data for IDE interface goes here
  //
  EFI_IDE_CHANNEL             Channel;
  EFI_IDE_DEVICE              Device;
  UINT16                      Lun;
  IDE_DEVICE_TYPE             Type;

  IDE_BASE_REGISTERS          *IoPort;
  UINT16                      AtapiError;

  ATAPI_INQUIRY_DATA                *InquiryData;
  EFI_IDENTIFY_DATA           *IdData;
  ATA_PIO_MODE                PioMode;
  EFI_ATA_MODE                UdmaMode;
  CHAR8                       ModelName[41];
  ATAPI_REQUEST_SENSE_DATA          *SenseData;
  UINT8                       SenseDataNumber;
  UINT8                       *Cache;

  //
  // ExitBootService Event, it is used to clear pending IDE interrupt
  //
  EFI_EVENT                   ExitBootServiceEvent;

  EFI_UNICODE_STRING_TABLE    *ControllerNameTable;
} IDE_BLK_IO_DEV;

#include "ComponentName.h"

#define IDE_BLOCK_IO_DEV_FROM_THIS(a)           CR (a, IDE_BLK_IO_DEV, BlkIo, IDE_BLK_IO_DEV_SIGNATURE)
#define IDE_BLOCK_IO_DEV_FROM_DISK_INFO_THIS(a) CR (a, IDE_BLK_IO_DEV, DiskInfo, IDE_BLK_IO_DEV_SIGNATURE)

#include "Ide.h"


/**
  Supported function of Driver Binding protocol for this driver.

  @param This                A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param ControllerHandle    The handle of the controller to test.
  @param RemainingDevicePath A pointer to the remaining portion of a device path.

  @retval  EFI_SUCCESS Driver loaded.
  @retval  other       Driver not loaded.

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start function of Driver binding protocol which start this driver on Controller
  by detecting all disks and installing BlockIo protocol on them.

  @param  This                Protocol instance pointer.
  @param  Controller          Handle of device to bind driver to.
  @param  RemainingDevicePath produce all possible children.

  @retval  EFI_SUCCESS         This driver is added to ControllerHandle.
  @retval  EFI_ALREADY_STARTED This driver is already running on ControllerHandle.
  @retval  other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop function of Driver Binding Protocol which is to stop the driver on Controller Handle and all
  child handle attached to the controller handle if there are.

  @param  This Protocol instance pointer.
  @param  Controller Handle of device to stop driver on
  @param  NumberOfChildren Not used
  @param  ChildHandleBuffer Not used

  @retval  EFI_SUCCESS This driver is removed DeviceHandle
  @retval  other This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
IDEBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  );

//
// EFI Driver Configuration Functions
//
/**
  Allows the user to set controller specific options for a controller that a 
  driver is currently managing.

  @param  This              A pointer to the EFI_DRIVER_CONFIGURATION_ PROTOCOL instance.
  @param  ControllerHandle  The handle of the controller to set options on.
  @param  ChildHandle       The handle of the child controller to set options on.
                            This is an optional parameter that may be NULL.
                            It will be NULL for device drivers, and for a bus drivers
                            that wish to set options for the bus controller.
                            It will not be NULL for a bus driver that wishes to set
                            options for one of its child controllers.
  @param  Language          A pointer to a three character ISO 639-2 language identifier. 
                            This is the language of the user interface that should be presented 
                            to the user, and it must match one of the languages specified in 
                            SupportedLanguages. The number of languages supported by a driver is up to
                            the driver writer.
  @param  ActionRequired    A pointer to the action that the calling agent is required 
                            to perform when this function returns.
  

  @retval  EFI_SUCCESS           The driver specified by This successfully set the configuration 
                                 options for the controller specified by ControllerHandle..
  @retval  EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ActionRequired is NULL.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support setting configuration options for 
                                 the controller specified by ControllerHandle and ChildHandle.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support the language specified by Language.
  @retval  EFI_DEVICE_ERROR      A device error occurred while attempt to set the configuration options for the 
                                 controller specified by ControllerHandle and ChildHandle.
  @retval  EFI_OUT_RESOURCES     There are not enough resources available to set the configuration options for the 
                                 controller specified by ControllerHandle and ChildHandle
**/
EFI_STATUS
EFIAPI
IDEBusDriverConfigurationSetOptions (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  CHAR8                                                  *Language,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  );

/**
  Tests to see if a controller's current configuration options are valid.

  @param  This             A pointer to the EFI_DRIVER_CONFIGURATION_PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to test if it's current configuration options 
                           are valid.
  @param  ChildHandle      The handle of the child controller to test if it's current configuration 
                           options are valid.  This is an optional parameter that may be NULL. It will 
                           be NULL for device drivers.  It will also be NULL for a bus drivers that
                           wish to test the configuration options for the bus controller. It will 
                           not be NULL for a bus driver that wishes to test configuration options for 
                           one of its child controllers.
  @retval  EFI_SUCCESS           The controller specified by ControllerHandle and ChildHandle that is being
                                 managed by the driver specified by This has a valid set of  configuration
                                 options.
  @retval  EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_UNSUPPORTED       The driver specified by This is not currently  managing the controller 
                                 specified by ControllerHandle and ChildHandle.
  @retval  EFI_DEVICE_ERROR      The controller specified by ControllerHandle and ChildHandle that is being
                                 managed by the driver specified by This has an invalid set of configuration
                                 options.
**/
EFI_STATUS
EFIAPI
IDEBusDriverConfigurationOptionsValid (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL               *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle  OPTIONAL
  );

/**
  Forces a driver to set the default configuration options for a controller.

  @param  This             A pointer to the EFI_DRIVER_CONFIGURATION_ PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to force default configuration options on.
  @param  ChildHandle      The handle of the child controller to force default configuration 
                           options on  This is an optional parameter that may be NULL.  It 
                           will be NULL for device drivers. It will also be NULL for a bus 
                           drivers that wish to force default configuration options for the bus
                           controller.  It will not be NULL for a bus driver that wishes to force
                           default configuration options for one of its child controllers.
  @param  DefaultType      The type of default configuration options to force on the controller 
                           specified by ControllerHandle and ChildHandle. 
  @param  ActionRequired   A pointer to the action that the calling agent is required to perform 
                           when this function returns.

  @retval  EFI_SUCCESS           The driver specified by This successfully forced the 
                                 default configuration options on the controller specified by 
                                 ControllerHandle and ChildHandle.
  @retval  EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER ActionRequired is NULL.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support forcing the default 
                                 configuration options on the controller specified by ControllerHandle
                                 and ChildHandle.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support the configuration type 
                                 specified by DefaultType.
  @retval  EFI_DEVICE_ERROR      A device error occurred while attempt to force the default configuration 
                                 options on the controller specified by  ControllerHandle and ChildHandle.
  @retval  EFI_OUT_RESOURCES     There are not enough resources available to force the default configuration 
                                 options on the controller specified by ControllerHandle and ChildHandle.
**/
EFI_STATUS
EFIAPI
IDEBusDriverConfigurationForceDefaults (
  IN  EFI_DRIVER_CONFIGURATION_PROTOCOL                      *This,
  IN  EFI_HANDLE                                             ControllerHandle,
  IN  EFI_HANDLE                                             ChildHandle  OPTIONAL,
  IN  UINT32                                                 DefaultType,
  OUT EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED               *ActionRequired
  );

//
// EFI Driver Diagnostics Functions
//
/**
  Runs diagnostics on a controller.

  @param  This             A pointer to the EFI_DRIVER_DIAGNOSTICS_PROTOCOLinstance.
  @param  ControllerHandle The handle of the controller to run diagnostics on.
  @param  ChildHandle      The handle of the child controller to run diagnostics on
                           This is an optional parameter that may be NULL.  It will
                           be NULL for device drivers.  It will also be NULL for a
                           bus drivers that wish to run diagnostics on the bus controller. 
                           It will not be NULL for a bus driver that wishes to run 
                           diagnostics on one of its child controllers.
  @param  DiagnosticType   Indicates type of diagnostics to perform on the controller
                           specified by ControllerHandle and ChildHandle.
  @param  Language         A pointer to a three character ISO 639-2 language identifier. 
                           This is the language in which the optional error message should 
                           be returned in Buffer, and it must match one of the languages 
                           specified in SupportedLanguages. The number of languages supported by
                           a driver is up to the driver writer.
  @param  ErrorType        A GUID that defines the format of the data returned in Buffer.
  @param  BufferSize       The size, in bytes, of the data returned in Buffer.
  @param  Buffer           A buffer that contains a Null-terminated Unicode string
                           plus some additional data whose format is defined by ErrorType.  
                           Buffer is allocated by this function with AllocatePool(), and 
                           it is the caller's responsibility to free it with a call to FreePool().

  @retval  EFI_SUCCESS           The controller specified by ControllerHandle and ChildHandle passed 
                                 the diagnostic.
  @retval  EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval  EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.
  @retval  EFI_INVALID_PARAMETER Language is NULL.
  @retval  EFI_INVALID_PARAMETER ErrorType is NULL.
  @retval  EFI_INVALID_PARAMETER BufferType is NULL.
  @retval  EFI_INVALID_PARAMETER Buffer is NULL.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support running 
                                 diagnostics for the controller specified by ControllerHandle 
                                 and ChildHandle.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support the
                                 type of diagnostic specified by DiagnosticType.
  @retval  EFI_UNSUPPORTED       The driver specified by This does not support the language 
                                 specified by Language.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to complete the 
                                 diagnostics.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to return the 
                                 status information in ErrorType, BufferSize,and Buffer.
  @retval  EFI_DEVICE_ERROR      The controller specified by ControllerHandle and ChildHandle 
                                 did not pass the diagnostic.
**/
EFI_STATUS
EFIAPI
IDEBusDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL               *This,
  IN  EFI_HANDLE                                    ControllerHandle,
  IN  EFI_HANDLE                                    ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                    DiagnosticType,
  IN  CHAR8                                         *Language,
  OUT EFI_GUID                                      **ErrorType,
  OUT UINTN                                         *BufferSize,
  OUT CHAR16                                        **Buffer
  );

/**
  issue ATA or ATAPI command to reset a block IO device.
  @param  This                  Block IO protocol instance pointer.
  @param  ExtendedVerification  If FALSE,for ATAPI device, driver will only invoke ATAPI reset method
                                If TRUE, for ATAPI device, driver need invoke ATA reset method after
                                invoke ATAPI reset method

  @retval EFI_DEVICE_ERROR      When the device is neighther ATA device or ATAPI device.
  @retval EFI_SUCCESS           The device reset successfully

**/
EFI_STATUS
EFIAPI
IDEBlkIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  BOOLEAN                     ExtendedVerification
  );

/**
  Read data from a block IO device.

  @param  This       Block IO protocol instance pointer.
  @param  MediaId    The media ID of the device
  @param  Lba        Starting LBA address to read data
  @param  BufferSize The size of data to be read
  @param  Buffer     Caller supplied buffer to save data

  @retval EFI_DEVICE_ERROR  unknown device type
  @retval EFI_SUCCESS       read the data successfully.

**/
EFI_STATUS
EFIAPI
IDEBlkIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  UINT32                      MediaId,
  IN  EFI_LBA                     Lba,
  IN  UINTN                       BufferSize,
  OUT VOID                        *Buffer
  );

/**
  Write data to block io device

  @param  This       Protocol instance pointer.
  @param  MediaId    The media ID of the device
  @param  Lba        Starting LBA address to write data
  @param  BufferSize The size of data to be written
  @param  Buffer     Caller supplied buffer to save data

  @retval EFI_DEVICE_ERROR  unknown device type
  @retval other             write data status

**/
EFI_STATUS
EFIAPI
IDEBlkIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This,
  IN  UINT32                      MediaId,
  IN  EFI_LBA                     Lba,
  IN  UINTN                       BufferSize,
  IN  VOID                        *Buffer
  );

/**
  Flushes all modified data to a physical block devices

  @param  This  Indicates a pointer to the calling context which to sepcify a 
                sepcific block device

  @retval EFI_SUCCESS   Always return success.
**/
EFI_STATUS
EFIAPI
IDEBlkIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL       *This
  );
/**
  This function is used by the IDE bus driver to get inquiry data. 
  Data format of Identify data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param  InquiryData           Pointer to a buffer for the inquiry data.
  @param  InquiryDataSize       Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL  IntquiryDataSize not big enough 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoInquiry (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *InquiryDataSize
  );

/**
  This function is used by the IDE bus driver to get identify data. 
  Data format of Identify data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param  IdentifyData          Pointer to a buffer for the identify data.
  @param  IdentifyDataSize      Pointer to the value for the identify data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading IdentifyData from device 
  @retval EFI_BUFFER_TOO_SMALL  IdentifyDataSize not big enough 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoIdentify (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  );

/**
  This function is used by the IDE bus driver to get sense data. 
  Data format of Sense data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param  SenseData             Pointer to the SenseData. 
  @param  SenseDataSize         Size of SenseData in bytes. 
  @param  SenseDataNumber       Pointer to the value for the identify data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL  SenseDataSize not big enough 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoSenseData (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT UINT8                       *SenseDataNumber
  );

/**
  This function is used by the IDE bus driver to get controller information.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param  IdeChannel            Pointer to the Ide Channel number. Primary or secondary.
  @param  IdeDevice             Pointer to the Ide Device number. Master or slave.

  @retval EFI_SUCCESS           IdeChannel and IdeDevice are valid 
  @retval EFI_UNSUPPORTED       This is not an IDE device 

**/
EFI_STATUS
EFIAPI
IDEDiskInfoWhichIde (
  IN EFI_DISK_INFO_PROTOCOL       *This,
  OUT UINT32                      *IdeChannel,
  OUT UINT32                      *IdeDevice
  );
/**
  The is an event(generally the event is exitBootService event) call back function. 
  Clear pending IDE interrupt before OS loader/kernel take control of the IDE device.

  @param  Event   Pointer to this event
  @param  Context Event handler private data

**/
VOID
EFIAPI
ClearInterrupt (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );
#endif
