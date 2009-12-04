/** @file
  DiskIo driver that layers it's self on every Block IO protocol in the system.
  DiskIo converts a block oriented device to a byte oriented device.

  Copyright (c) 2006 - 2009, Intel Corporation                                              
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _DISK_IO_H
#define _DISK_IO_H

#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DriverConfiguration.h>
#include <Protocol/DriverConfiguration2.h>
#include <Protocol/DriverHealth.h>
#include <Protocol/DiskIo.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiString.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include "NVDataStruc.h"

#define DISK_IO_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('d', 's', 'k', 'I') 

#define DISK_IO_CONTROLLER_STATE_SIGNATURE  SIGNATURE_32 ('c', 't', 's', 'S')   

#define DATA_BUFFER_BLOCK_NUM           (64)

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  UINTN                            Signature;
  EFI_DISK_IO_PROTOCOL             DiskIo;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_HANDLE                       Handle;
  //
  // Consumed protocol
  //
  EFI_HII_DATABASE_PROTOCOL        *HiiDatabase;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;

  DISK_IO_NV_DATA                  NVdata;
  EFI_UNICODE_STRING_TABLE         *ControllerNameTable;
  UINT8                            ControllerIndex;
} DISK_IO_PRIVATE_DATA;

typedef struct {
  BOOLEAN                          StartState;
  UINTN                            CurrentState;
  UINTN                            NextState;
  EFI_STRING_ID                    StringId;
  BOOLEAN                          RepairNotify;
  EFI_DRIVER_HEALTH_STATUS         HealthStatus;
} DEVICE_STATE;

typedef struct {
  UINTN                            Signature;
  LIST_ENTRY                       Link;

  EFI_HANDLE                       ControllerHandle;
  UINTN                            ControllerIndex;
  EFI_HANDLE                       ChildHandle;
  UINTN                            DeviceStateNum;
} CONTROLLER_STATE;

#define DISK_IO_PRIVATE_DATA_FROM_THIS(a) CR (a, DISK_IO_PRIVATE_DATA, DiskIo, DISK_IO_PRIVATE_DATA_SIGNATURE)
#define DISK_IO_PRIVATE_DATA_FROM_CONFIG_ACCESS(a) CR (a, DISK_IO_PRIVATE_DATA, ConfigAccess, DISK_IO_PRIVATE_DATA_SIGNATURE)

#define DISK_IO_CONTROLLER_STATE_FROM_HANDLE(a) CR (a, CONTROLLER_STATE, ControllerHandle, DISK_IO_CONTROLLER_STATE_SIGNATURE)
#define DISK_IO_CONTROLLER_STATE_FROM_LINK(a) CR (a, CONTROLLER_STATE, Link, DISK_IO_CONTROLLER_STATE_SIGNATURE)
//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL         gDiskIoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL         gDiskIoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL        gDiskIoComponentName2;
extern EFI_DRIVER_HEALTH_PROTOCOL          gDiskIoDriverHealth;

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  DriverHealthDxeStrings[];
extern UINT8  DriverHealthVfrBin[];


//
// Prototypes
// Driver model protocol interface
//
/**
  Test to see if this driver supports ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle by opening a Block IO protocol and
  installing a Disk IO protocol on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle by removing Disk IO protocol and closing
  the Block IO protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

//
// Disk I/O Protocol Interface
//
/**
  Read BufferSize bytes from Offset into Buffer.
  Reads may support reads that are not aligned on
  sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the read request is
               less than a sector in length.
    Aligned  - A read of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary.

  @param  This                  Protocol instance pointer.
  @param  MediaId               Id of the media, changes every time the media is replaced.
  @param  Offset                The starting byte offset to read from
  @param  BufferSize            Size of Buffer
  @param  Buffer                Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  OUT VOID                 *Buffer
  );

/**
  Writes BufferSize bytes from Buffer into Offset.
  Writes may require a read modify write to support writes that are not
  aligned on sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the write request
               is less than a sector in length. Read modify write is required.
    Aligned  - A write of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary. Read modified write
               required.

  @param  This       Protocol instance pointer.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Offset     The starting byte offset to read from
  @param  BufferSize Size of Buffer
  @param  Buffer     Buffer containing read data

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not
                                 valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  );

//
// EFI Component Name Functions
//
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
DiskIoComponentNameGetDriverName (
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

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.

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
DiskIoComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

//
// EFI Driver Health Functions
//
/**
  Retrieves the health status of a controller in the platform.  This function can also 
  optionally return warning messages, error messages, and a set of HII Forms that may 
  be repair a controller that is not proper configured. 
  
  @param  This             A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.

  @param  ControllerHandle The handle of the controller to retrieve the health status 
                           on.  This is an optional parameter that may be NULL.  If 
                           this parameter is NULL, then the value of ChildHandle is 
                           ignored, and the combined health status of all the devices 
                           that the driver is managing is returned.

  @param  ChildHandle      The handle of the child controller to retrieve the health 
                           status on.  This is an optional parameter that may be NULL.  
                           This parameter is ignored of ControllerHandle is NULL.  It 
                           will be NULL for device drivers.  It will also be NULL for 
                           bus drivers when an attempt is made to collect the health 
                           status of the bus controller.  If will not be NULL when an 
                           attempt is made to collect the health status for a child 
                           controller produced by the driver.

  @param  HealthStatus     A pointer to the health status that is returned by this 
                           function.  This is an optional parameter that may be NULL.  
                           This parameter is ignored of ControllerHandle is NULL.  
                           The health status for the controller specified by 
                           ControllerHandle and ChildHandle is returned. 

  @param  MessageList      A pointer to an array of warning or error messages associated 
                           with the controller specified by ControllerHandle and 
                           ChildHandle.  This is an optional parameter that may be NULL.  
                           MessageList is allocated by this function with the EFI Boot 
                           Service AllocatePool(), and it is the caller's responsibility 
                           to free MessageList with the EFI Boot Service FreePool().  
                           Each message is specified by tuple of an EFI_HII_HANDLE and 
                           an EFI_STRING_ID.  The array of messages is terminated by tuple 
                           containing a EFI_HII_HANDLE with a value of NULL.  The 
                           EFI_HII_STRING_PROTOCOL.GetString() function can be used to 
                           retrieve the warning or error message as a Null-terminated 
                           Unicode string in a specific language.  Messages may be 
                           returned for any of the HealthStatus values except 
                           EfiDriverHealthStatusReconnectRequired and 
                           EfiDriverHealthStatusRebootRequired.

  @param  FormHiiHandle    A pointer to the HII handle for an HII form associated with the 
                           controller specified by ControllerHandle and ChildHandle.  
                           This is an optional parameter that may be NULL.  An HII form 
                           is specified by a combination of an EFI_HII_HANDLE and an 
                           EFI_GUID that identifies the Form Set GUID.  The 
                           EFI_FORM_BROWSER2_PROTOCOL.SendForm() function can be used 
                           to display and allow the user to make configuration changes 
                           to the HII Form.  An HII form may only be returned with a 
                           HealthStatus value of EfiDriverHealthStatusConfigurationRequired.

  @retval EFI_SUCCESS           ControllerHandle is NULL, and all the controllers 
                                managed by this driver specified by This have a health 
                                status of EfiDriverHealthStatusHealthy with no warning 
                                messages to be returned.  The ChildHandle, HealthStatus, 
                                MessageList, and FormList parameters are ignored.

  @retval EFI_DEVICE_ERROR      ControllerHandle is NULL, and one or more of the 
                                controllers managed by this driver specified by This 
                                do not have a health status of EfiDriverHealthStatusHealthy.  
                                The ChildHandle, HealthStatus, MessageList, and 
                                FormList parameters are ignored.

  @retval EFI_DEVICE_ERROR      ControllerHandle is NULL, and one or more of the 
                                controllers managed by this driver specified by This 
                                have one or more warning and/or error messages.  
                                The ChildHandle, HealthStatus, MessageList, and 
                                FormList parameters are ignored.

  @retval EFI_SUCCESS           ControllerHandle is not NULL and the health status 
                                of the controller specified by ControllerHandle and 
                                ChildHandle was returned in HealthStatus.  A list 
                                of warning and error messages may be optionally 
                                returned in MessageList, and a list of HII Forms 
                                may be optionally returned in FormList.

  @retval EFI_UNSUPPORTED	      ControllerHandle is not NULL, and the controller 
                                specified by ControllerHandle and ChildHandle is not 
                                currently being managed by the driver specified by This.

  @retval EFI_INVALID_PARAMETER	HealthStatus is NULL.

  @retval EFI_OUT_OF_RESOURCES	MessageList is not NULL, and there are not enough 
                                resource available to allocate memory for MessageList.

**/
EFI_STATUS
DiskIoDriverHealthGetHealthStatus (
  IN  EFI_DRIVER_HEALTH_PROTOCOL       *This,
  IN  EFI_HANDLE                       ControllerHandle  OPTIONAL,
  IN  EFI_HANDLE                       ChildHandle       OPTIONAL,
  OUT EFI_DRIVER_HEALTH_STATUS         *HealthStatus,
  OUT EFI_DRIVER_HEALTH_HII_MESSAGE    **MessageList     OPTIONAL,
  OUT EFI_HII_HANDLE                   *FormHiiHandle    OPTIONAL
  );

/**
  Performs a repair operation on a controller in the platform.  This function can 
  optionally report repair progress information back to the platform. 
  
  @param  This             A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param  ControllerHandle The handle of the controller to repair.
  @param  ChildHandle      The handle of the child controller to repair.  This is 
                           an optional parameter that may be NULL.  It will be NULL 
                           for device drivers.  It will also be NULL for bus 
                           drivers when an attempt is made to repair a bus controller.
                           If will not be NULL when an attempt is made to repair a 
                           child controller produced by the driver.
  @param  RepairNotify     A notification function that may be used by a driver to 
                           report the progress of the repair operation.  This is 
                           an optional parameter that may be NULL.  


  @retval EFI_SUCCESS	          An attempt to repair the controller specified by 
                                ControllerHandle and ChildHandle was performed.  
                                The result of the repair operation can be 
                                determined by calling GetHealthStatus().
  @retval EFI_UNSUPPORTED	      The driver specified by This is not currently 
                                managing the controller specified by ControllerHandle 
                                and ChildHandle.
  @retval EFI_OUT_OF_RESOURCES	There are not enough resources to perform the 
                                repair operation.

*/
EFI_STATUS
DiskIoDriverHealthRepair (
  IN  EFI_DRIVER_HEALTH_PROTOCOL                *This,
  IN  EFI_HANDLE                                ControllerHandle,
  IN  EFI_HANDLE                                ChildHandle       OPTIONAL,
  IN  EFI_DRIVER_HEALTH_REPAIR_PROGRESS_NOTIFY  RepairNotify      OPTIONAL
  );

/**
  Initialize the serial configuration form.

  @retval EFI_SUCCESS              The serial configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Other errors as indicated.
**/
EFI_STATUS
DiskIoConfigFormInit (
  VOID
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
DummyExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
DummyRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DummyDriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

/**
  Add the ISO639-2 and RFC4646 component name both for the Disk IO device

  @param DiskIoDevice     A pointer to the DISK_IO_PRIVATE_DATA instance.

**/
VOID
AddName (
  IN  DISK_IO_PRIVATE_DATA                     *DiskIoDevice
  );

#endif
