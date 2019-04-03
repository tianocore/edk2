/** @file
  The platform device manager reference implement

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DEVICE_MANAGER_H_
#define _DEVICE_MANAGER_H_

#include "Bds.h"
#include "FrontPage.h"
#include "DeviceManagerVfr.h"
#include <Protocol/PciIo.h>

#define DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE       SIGNATURE_32 ('D', 'M', 'C', 'B')
#define DEVICE_MANAGER_DRIVER_HEALTH_INFO_SIGNATURE  SIGNATURE_32 ('D', 'M', 'D', 'H')


typedef struct {
  UINTN                           Signature;

  ///
  /// Device Manager HII relative handles
  ///
  EFI_HII_HANDLE                  HiiHandle;

  ///
  /// Driver Health HII relative handles
  ///
  EFI_HII_HANDLE                  DriverHealthHiiHandle;

  EFI_HANDLE                      DriverHandle;
  EFI_HANDLE                      DriverHealthHandle;

  ///
  /// Device Manager Produced protocols
  ///
  EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;

  ///
  /// Driver Health Produced protocols
  ///
  EFI_HII_CONFIG_ACCESS_PROTOCOL  DriverHealthConfigAccess;

  ///
  /// Configuration data
  ///
  UINT8                           VideoBios;
} DEVICE_MANAGER_CALLBACK_DATA;


typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                      Link;

  ///
  /// HII relative handles
  ///
  EFI_HII_HANDLE                  HiiHandle;

  ///
  /// Driver relative handles
  ///
  EFI_HANDLE                      DriverHandle;
  EFI_HANDLE                      ControllerHandle;
  EFI_HANDLE                      ChildHandle;

  EFI_DRIVER_HEALTH_PROTOCOL      *DriverHealth;
  ///
  /// Driver health messages of the specify Driver
  ///
  EFI_DRIVER_HEALTH_HII_MESSAGE   *MessageList;

  ///
  /// Driver Health status
  ///
  EFI_DRIVER_HEALTH_STATUS        HealthStatus;
} DRIVER_HEALTH_INFO;

typedef struct {
  EFI_STRING_ID    PromptId;
  EFI_QUESTION_ID  QuestionId;
}MENU_INFO_ITEM;

typedef struct {
  UINTN           CurListLen;
  UINTN           MaxListLen;
  MENU_INFO_ITEM  *NodeList;
} MAC_ADDRESS_NODE_LIST;

#define DEVICE_MANAGER_HEALTH_INFO_FROM_LINK(a) \
  CR (a, \
      DRIVER_HEALTH_INFO, \
      Link, \
      DEVICE_MANAGER_DRIVER_HEALTH_INFO_SIGNATURE \
      )

#define DEVICE_MANAGER_CALLBACK_DATA_FROM_THIS(a) \
  CR (a, \
      DEVICE_MANAGER_CALLBACK_DATA, \
      ConfigAccess, \
      DEVICE_MANAGER_CALLBACK_DATA_SIGNATURE \
      )
typedef struct {
  EFI_STRING_ID  StringId;
  UINT16         Class;
} DEVICE_MANAGER_MENU_ITEM;

/**
  This function is invoked if user selected a interactive opcode from Device Manager's
  Formset. The decision by user is saved to gCallbackKey for later processing. If
  user set VBIOS, the new value is saved to EFI variable.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
DeviceManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

/**
  This function is invoked if user selected a interactive opcode from Driver Health's
  Formset. The decision by user is saved to gCallbackKey for later processing.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
DriverHealthCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );


/**

  This function registers HII packages to HII database.

  @retval  EFI_SUCCESS           HII packages for the Device Manager were registered successfully.
  @retval  EFI_OUT_OF_RESOURCES  HII packages for the Device Manager failed to be registered.

**/
EFI_STATUS
InitializeDeviceManager (
  VOID
  );

/**

  Call the browser and display the device manager to allow user
  to configure the platform.

  This function create the dynamic content for device manager. It includes
  section header for all class of devices, one-of opcode to set VBIOS.

  @retval  EFI_SUCCESS             Operation is successful.
  @retval  Other values if failed to clean up the dynamic content from HII
           database.

**/
EFI_STATUS
CallDeviceManager (
  VOID
  );


/**
  Check the Driver Health status of a single controller and try to process it if not healthy.

  This function called by CheckAllControllersHealthStatus () function in order to process a specify
  contoller's health state.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health information.
  @param DriverHandle       The handle of driver.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health
                            status on.  This is an optional parameter that may be NULL.
  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param HealthStatus       The health status of the controller.

  @retval EFI_INVALID_PARAMETER   HealthStatus or DriverHealth is NULL.
  @retval HealthStatus            The Health status of specify controller.
  @retval EFI_OUT_OF_RESOURCES    The list of Driver Health Protocol handles can not be retrieved.
  @retval EFI_NOT_FOUND           No controller in the platform install Driver Health Protocol.
  @retval EFI_SUCCESS             The Health related operation has been taken successfully.

**/
EFI_STATUS
EFIAPI
GetSingleControllerHealthStatus (
  IN OUT LIST_ENTRY                   *DriverHealthList,
  IN EFI_HANDLE                       DriverHandle,
  IN EFI_HANDLE                       ControllerHandle,  OPTIONAL
  IN EFI_HANDLE                       ChildHandle,       OPTIONAL
  IN EFI_DRIVER_HEALTH_PROTOCOL       *DriverHealth,
  IN EFI_DRIVER_HEALTH_STATUS         *HealthStatus
  );

/**
  Collects all the EFI Driver Health Protocols currently present in the EFI Handle Database,
  and queries each EFI Driver Health Protocol to determine if one or more of the controllers
  managed by each EFI Driver Health Protocol instance are not healthy.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health
                            information.

  @retval    EFI_NOT_FOUND         No controller in the platform install Driver Health Protocol.
  @retval    EFI_SUCCESS           All the controllers in the platform are healthy.
  @retval    EFI_OUT_OF_RESOURCES  The list of Driver Health Protocol handles can not be retrieved.

**/
EFI_STATUS
GetAllControllersHealthStatus (
  IN OUT LIST_ENTRY  *DriverHealthList
  );

/**
  Check the healthy status of the platform, this function will return immediately while found one driver
  in the platform are not healthy.

  @retval FALSE      at least one driver in the platform are not healthy.
  @retval TRUE       No controller install Driver Health Protocol,
                     or all controllers in the platform are in healthy status.
**/
BOOLEAN
PlaformHealthStatusCheck (
  VOID
  );

/**
  Repair the whole platform.

  This function is the main entry for user choose "Repair All" in the front page.
  It will try to do recovery job till all the driver health protocol installed modules
  reach a terminal state.

  @param DriverHealthList   A Pointer to the list contain all of the platform driver health
                            information.

**/
VOID
PlatformRepairAll (
  IN LIST_ENTRY  *DriverHealthList
  );

/**
  Processes a single controller using the EFI Driver Health Protocol associated with
  that controller. This algorithm continues to query the GetHealthStatus() service until
  one of the legal terminal states of the EFI Driver Health Protocol is reached. This may
  require the processing of HII Messages, HII Form, and invocation of repair operations.

  @param DriverHealth       A pointer to the EFI_DRIVER_HEALTH_PROTOCOL instance.
  @param ControllerHandle   The class guid specifies which form set will be displayed.
  @param ChildHandle        The handle of the child controller to retrieve the health
                            status on.  This is an optional parameter that may be NULL.
  @param HealthStatus       The health status of the controller.
  @param MessageList        An array of warning or error messages associated
                            with the controller specified by ControllerHandle and
                            ChildHandle.  This is an optional parameter that may be NULL.
  @param FormHiiHandle      The HII handle for an HII form associated with the
                            controller specified by ControllerHandle and ChildHandle.
  @param RebootRequired     Indicate whether a reboot is required to repair the controller.
**/
VOID
ProcessSingleControllerHealth (
  IN  EFI_DRIVER_HEALTH_PROTOCOL         *DriverHealth,
  IN  EFI_HANDLE                         ControllerHandle, OPTIONAL
  IN  EFI_HANDLE                         ChildHandle,      OPTIONAL
  IN  EFI_DRIVER_HEALTH_STATUS           HealthStatus,
  IN  EFI_DRIVER_HEALTH_HII_MESSAGE      **MessageList,    OPTIONAL
  IN  EFI_HII_HANDLE                     FormHiiHandle,
  IN OUT BOOLEAN                         *RebootRequired
  );

/**
  Reports the progress of a repair operation.

  @param[in]  Value             A value between 0 and Limit that identifies the current
                                progress of the repair operation.

  @param[in]  Limit             The maximum value of Value for the current repair operation.
                                For example, a driver that wants to specify progress in
                                percent would use a Limit value of 100.

  @retval EFI_SUCCESS           The progress of a repair operation is reported successfully.

**/
EFI_STATUS
EFIAPI
RepairNotify (
  IN  UINTN Value,
  IN  UINTN Limit
  );

/**
  Processes a set of messages returned by the GetHealthStatus ()
  service of the EFI Driver Health Protocol

  @param    MessageList  The MessageList point to messages need to processed.

**/
VOID
ProcessMessages (
  IN  EFI_DRIVER_HEALTH_HII_MESSAGE      *MessageList
  );


/**
  Collect and display the platform's driver health relative information, allow user to do interactive
  operation while the platform is unhealthy.

  This function display a form which divided into two parts. The one list all modules which has installed
  driver health protocol. The list usually contain driver name, controller name, and it's health info.
  While the driver name can't be retrieved, will use device path as backup. The other part of the form provide
  a choice to the user to repair all platform.

**/
VOID
CallDriverHealth (
  VOID
  );

/**

  Select the best matching language according to front page policy for best user experience.

  This function supports both ISO 639-2 and RFC 4646 language codes, but language
  code types may not be mixed in a single call to this function.

  @param  SupportedLanguages   A pointer to a Null-terminated ASCII string that
                               contains a set of language codes in the format
                               specified by Iso639Language.
  @param  Iso639Language       If TRUE, then all language codes are assumed to be
                               in ISO 639-2 format.  If FALSE, then all language
                               codes are assumed to be in RFC 4646 language format.

  @retval NULL                 The best matching language could not be found in SupportedLanguages.
  @retval NULL                 There are not enough resources available to return the best matching
                               language.
  @retval Other                A pointer to a Null-terminated ASCII string that is the best matching
                               language in SupportedLanguages.
**/
CHAR8 *
DriverHealthSelectBestLanguage (
  IN CHAR8        *SupportedLanguages,
  IN BOOLEAN      Iso639Language
  );

/**

  This is an internal worker function to get the Component Name (2) protocol interface
  and the language it supports.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ComponentName        A pointer to the Component Name (2) protocol interface.
  @param  SupportedLanguage    The best suitable language that matches the SupportedLangues interface for the
                               located Component Name (2) instance.

  @retval EFI_SUCCESS          The Component Name (2) protocol instance is successfully located and we find
                               the best matching language it support.
  @retval EFI_UNSUPPORTED      The input Language is not supported by the Component Name (2) protocol.
  @retval Other                Some error occurs when locating Component Name (2) protocol instance or finding
                               the supported language.

**/
EFI_STATUS
GetComponentNameWorker (
  IN  EFI_GUID                    *ProtocolGuid,
  IN  EFI_HANDLE                  DriverBindingHandle,
  OUT EFI_COMPONENT_NAME_PROTOCOL **ComponentName,
  OUT CHAR8                       **SupportedLanguage
  );

/**

  This is an internal worker function to get driver name from Component Name (2) protocol interface.


  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
GetDriverNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  );

/**

  This function gets driver name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the driver name.
  If the attempt fails, it then gets the driver name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  DriverName           A pointer to the Unicode string to return. This Unicode string is the name
                               of the driver specified by This.

  @retval EFI_SUCCESS          The driver name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The driver name cannot be retrieved from Component Name (2) protocol
                               interface.

**/
EFI_STATUS
DriverHealthGetDriverName (
  IN  EFI_HANDLE  DriverBindingHandle,
  OUT CHAR16      **DriverName
  );

/**
  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  ProtocolGuid         A pointer to an EFI_GUID. It points to Component Name (2) protocol GUID.
  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval  EFI_SUCCESS         The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval  Other               The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
GetControllerNameWorker (
  IN  EFI_GUID    *ProtocolGuid,
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  );

/**
  This function gets controller name from Component Name 2 protocol interface and Component Name protocol interface
  in turn. It first tries UEFI 2.0 Component Name 2 protocol interface and try to get the controller name.
  If the attempt fails, it then gets the controller name from EFI 1.1 Component Name protocol for backward
  compatibility support.

  @param  DriverBindingHandle  The handle on which the Component Name (2) protocol instance is retrieved.
  @param  ControllerHandle     The handle of a controller that the driver specified by This is managing.
                               This handle specifies the controller whose name is to be returned.
  @param  ChildHandle          The handle of the child controller to retrieve the name of. This is an
                               optional parameter that may be NULL. It will be NULL for device drivers.
                               It will also be NULL for bus drivers that attempt to retrieve the name
                               of the bus controller. It will not be NULL for a bus driver that attempts
                               to retrieve the name of a child controller.
  @param  ControllerName       A pointer to the Unicode string to return. This Unicode string
                               is the name of the controller specified by ControllerHandle and ChildHandle.

  @retval EFI_SUCCESS          The controller name is successfully retrieved from Component Name (2) protocol
                               interface.
  @retval Other                The controller name cannot be retrieved from Component Name (2) protocol.

**/
EFI_STATUS
DriverHealthGetControllerName (
  IN  EFI_HANDLE  DriverBindingHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  EFI_HANDLE  ChildHandle,
  OUT CHAR16      **ControllerName
  );

#endif
