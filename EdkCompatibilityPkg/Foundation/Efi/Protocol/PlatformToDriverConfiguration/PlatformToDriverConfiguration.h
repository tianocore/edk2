/*++

Copyright (c) 2008-2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatformToDriverConfiguration.h

Abstract:

    UEFI Platform to Driver Configuration Protocol

Revision History:

--*/

#ifndef _EFI_PLATFORM_TO_DRIVER_CONFIGURATION_H_
#define _EFI_PLATFORM_TO_DRIVER_CONFIGURATION_H_

//
// Global ID for the Platform to Driver Configuration Protocol
//
#define EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL_GUID \
  { \
    0x642cd590, 0x8059, 0x4c0a, {0xa9, 0x58, 0xc5, 0xec, 0x07, 0xd2, 0x3c, 0x4b} \
  }

#define EFI_PLATFORM_TO_DRIVER_CONFIGURATION_CLP_GUID \
  { \
    0x345ecc0e, 0x0cb6, 0x4b75, {0xbb, 0x57, 0x1b, 0x12, 0x9c, 0x47, 0x33, 0x3e} \
  }

EFI_FORWARD_DECLARATION (EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL);

typedef enum {
  EfiPlatformConfigurationActionNone               = 0,
  EfiPlatformConfigurationActionStopController     = 1,
  EfiPlatformConfigurationActionRestartController  = 2,
  EfiPlatformConfigurationActionRestartPlatform    = 3,
  EfiPlatformConfigurationActionNvramFailed        = 4,
  EfiPlatformConfigurationActionMaximum
} EFI_PLATFORM_CONFIGURATION_ACTION;

typedef struct {
  CHAR8         *CLPCommand;
  UINT32        CLPCommandLength;
  CHAR8         *CLPReturnString;
  UINT32        CLPReturnStringLength;
  UINT8         CLPCmdStatus;
  UINT8         CLPErrorValue;
  UINT16        CLPMsgCode;
} EFI_CONFIGURE_CLP_PARAMETER_BLK;

typedef
EFI_STATUS
(EFIAPI *EFI_PLATFORM_TO_DRIVER_CONFIGURATION_QUERY) (
  IN  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                     ControllerHandle,
  IN  EFI_HANDLE                                     ChildHandle,  OPTIONAL
  IN UINTN                                           *Instance,
  OUT EFI_GUID                                       **ParameterTypeGuid,
  OUT VOID                                           **ParameterBlock,
  OUT UINTN                                          *ParameterBlockSize
  );
/*++

  Routine Description:
    Allows the UEFI driver to query the platform for configuration information
    needed to complete the drivers Start() operation.

  Arguments:
    This               - A pointer to the EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL instance.
    ControllerHandle   - The handle the platform will return configuration
                         information about.
    ChildHandle        - The handle of the child controller to set options on.
                         This is an optional parameter that may be NULL.  It will
                         be NULL for device drivers, and for a bus drivers that
                         wish to set options for the bus controller.  It will not
                         be NULL for a bus driver that wishes to set options for
                         one of its child controllers.
    Instance           - Pointer to the Instance value. Zero means return the first
                         query data.  The caller should increment this value by one
                         each time to retrieve successive data.
    ParameterTypeGuid  - An EFI_GUID that defines the contents of ParameterBlock.
                         UEFI drivers must use the ParameterTypeGuid to determine
                         how to parse the ParameterBlock. The caller should not
                         attempt to free ParameterTypeGuid.
    ParameterBlock     - The platform returns a pointer to the ParameterBlock
                         structure which contains details about the configuration
                         parameters specific to the ParameterTypeGuid. This structure
                         is defined based on the protocol and may be different for
                         different protocols. UEFI driver decodes this structure
                         and its contents based on ParameterTypeGuid. ParameterBlock
                         is allocated by the platform and the platform is responsible
                         for freeing the ParameterBlock after Response is called.
    ParameterBlockSize - The platform returns the size of the ParameterBlock in bytes.

  Returns:
    EFI_SUCCESS           - The platform return parameter information for ControllerHandle.
    EFI_NOT_FOUND         - No more unread Instance exists.
    EFI_INVALID_PARAMETER - ControllerHandle is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - Instance is NULL.
    EFI_DEVICE_ERROR      - A device error occurred while attempting to return
                            parameter block information for the controller specified
                            by ControllerHandle and ChildHandle.
    EFI_OUT_RESOURCES     - There are not enough resources available to set the
                            configuration options for the controller specified
                            by ControllerHandle and ChildHandle.

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_PLATFORM_TO_DRIVER_CONFIGURATION_RESPONSE) (
  IN  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL  *This,
  IN  EFI_HANDLE                                     ControllerHandle,
  IN  EFI_HANDLE                                     ChildHandle  OPTIONAL,
  IN  UINTN                                          *Instance,
  IN  EFI_GUID                                       *ParameterTypeGuid,
  IN  VOID                                           *ParameterBlock,
  IN  UINTN                                          ParameterBlockSize ,
  IN  EFI_PLATFORM_CONFIGURATION_ACTION              ConfigurationAction
  );
/*++

  Routine Description:
    Tell the platform what actions where taken by the driver after processing
    the data returned from Query.

  Arguments:
    This               - A pointer to the EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL instance.
    ControllerHandle   - The handle the platform will return configuration
                         information about.
    ChildHandle        - The handle of the child controller to set options on.
                         This is an optional parameter that may be NULL.  It will
                         be NULL for device drivers, and for a bus drivers that
                         wish to set options for the bus controller.  It will not
                         be NULL for a bus driver that wishes to set options for
                         one of its child controllers.
    Instance           - Instance data returned from  Query.
    ParameterTypeGuid  - ParameterTypeGuid returned from Query.
    ParameterBlock     - ParameterBlock returned from Query.
    ParameterBlockSize - The ParameterBlock size returned from Query.
    ConfigurationAction- The driver tells the platform what action is required
                         for ParameterBlock to take effect.

  Returns:
    EFI_SUCCESS           - The platform return parameter information for ControllerHandle.
    EFI_NOT_FOUND         - Instance was not found.
    EFI_INVALID_PARAMETER - ControllerHandle is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - Instance is zero.

--*/

//
// Interface structure for the Platform to Driver Configuration Protocol
//
struct _EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL {
  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_QUERY      Query;
  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_RESPONSE   Response;
};

extern EFI_GUID gEfiPlatformToDriverConfigurationProtocolGuid;
extern EFI_GUID gEfiPlatformToDriverConfigurationClpGuid;

#endif
