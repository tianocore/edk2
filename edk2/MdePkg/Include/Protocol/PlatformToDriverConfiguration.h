/** @file
  The file provides the protocol to retrieve configuration
  information for a device that a UEFI driver is about to start.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PlatformToDriverConfiguration.h

**/

#ifndef __PLATFORM_TO_DRIVER_CONFIGUARTION_H__
#define __PLATFORM_TO_DRIVER_CONFIGUARTION_H__

#define EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL_GUID  \
  { 0x642cd590, 0x8059, 0x4c0a, { 0xa9, 0x58, 0xc5, 0xec, 0x7, 0xd2, 0x3c, 0x4b } }


typedef struct _EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL;


/**
  The UEFI driver must call Query early in the Start() function
  before any time consuming operations are performed. If
  ChildHandle is NULL the driver is requesting information from
  the platform about the ControllerHandle that is being started.
  Information returned from Query may lead to the drivers Start()
  function failing. If the UEFI driver is a bus driver and
  producing a ChildHandle the driver must call Query after the
  child handle has been created and an EFI_DEVICE_PATH_PROTOCOL
  has been placed on that handle, but before any time consuming
  operation is performed. If information return by Query may lead
  the driver to decide to not create the ChildHandle. The driver
  must then cleanup and remove the ChildHandle from the system.
  The UEFI driver repeatedly calls Query, processes the
  information returned by the platform, and calls Response passing
  in the arguments returned from Query. The Instance value passed
  into Response must be the same value returned from the
  corresponding call to Query. The UEFI driver must continuously
  call Query and Response until EFI_NOT_FOUND is returned by
  Query. The only value of Instance that has meaning to the UEFI
  driver is zero. An Instance value of zero means return the first
  ParameterBlock in the set of unprocessed parameter blocks. If a
  ParameterBlock has been processed via a Query and corresponding
  Response call it must not be returned again via a Query call.

  @param This   A pointer to the
                EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOC OL
                instance.
  
  @param ControllerHandle   The handle the platform will return
                            configuration information about.
  
  @param ChildHandle  The handle of the child controller to
                      return information on. This is an optional
                      parameter that may be NULL. It will be
                      NULL for device drivers, and for bus
                      drivers that attempt to get options for
                      the bus controller. It will not be NULL
                      for a bus driver that attempts to get
                      options for one of its child controllers.
  
  
  @param Instance   Pointer to the Instance value. On output the
                    instance associated with the parameter data
                    return. On input zero means return the first
                    query data or pass in a valid instance
                    number returned from a previous call to
                    Query.

  @param ParameterTypeGuid  An EFI_GUID that defines the
                            contents of ParameterBlock. UEFI
                            drivers must use the
                            ParameterTypeGuid to determine how
                            to parser the ParameterBlock.

  @param ParameterBlock   The platform returns a pointer to the
                          ParameterBlock structure which
                          contains details about the
                          configuration parameters specific to
                          the ParameterTypeGuid. This structure
                          is defined based on the protocol and
                          may be different for different
                          protocols. UEFI driver decodes this
                          structure and its contents based on
                          ProtocolGuid. ParameterBlock is
                          allocated by the platform and the
                          platform is responsible for freeing
                          the ParameterBlock after Result is
                          called.

  @param ParameterBlockSize   The platform returns the size of
                              the ParameterBlock in bytes.


  @retval EFI_SUCCESS   The platform return parameter
                        information for ControllerHandle.

  @retval EFI_NOT_FOUND No more unread Instance exists.

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Instance is NULL.

  @retval EFI_DEVICE_ERROR  A device error occurred while
                            attempting to return parameter block
                            information for the controller
                            specified by ControllerHandle and
                            ChildHandle.

  @retval EFI_OUT_RESOURCES There are not enough resources
                            available to set the configuration
                            options for the controller specified
                            by ControllerHandle and ChildHandle.


**/
typedef
EFI_STATUS
(EFIAPI *EFI_PLATFORM_TO_DRIVER_CONFIGURATION_QUERY) (
  IN CONST  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL *This,
  IN CONST  EFI_HANDLE  ControllerHandle,
  IN CONST  EFI_HANDLE  ChildHandle OPTIONAL,
  IN OUT    UINTN       *Instance,
  IN OUT    EFI_GUID    **ParameterTypeGuid,
  IN OUT    VOID        **ParameterBlock,
  IN OUT    UINTN       *ParameterBlockSize
);

/**

  @param EfiPlatformConfigurationActionNone
          The controller specified by ControllerHandle is still
          in a usable state, it's configuration has been updated
          via parsing the ParameterBlock. If required by the
          parameter block and the module supports an NVRAM store
          the configuration information from PB was successfully
          saved to the NVRAM. No actions are required before
          this controller can be used again with the updated
          configuration settings.


  @param EfiPlatformConfigurationStopController 
          The driver has detected that the controller specified
          by ControllerHandle  is not in a usable state, and it
          needs to be stopped. The calling agent can use the
          DisconnectControservice to perform this operation, and
          it should be performed as soon as possible.

  @param EfiPlatformConfigurationRestartController
          This controller specified by ControllerHandle needs to
          be stopped and restarted before it can be used again.
          The calling agent can use the DisconnectController()
          and ConnectController() services to perform this
          operation. The restart operation can be delayed  until
          all of the configuratiooptions have been set.


  @param EfiPlatformConfigurationRestartPlatform
          A configuration change has been made that requires the
          platform to be restarted before the controller
          specified by ControllerHandle can be used again. The
          calling agent can use the ResetSystem() services to
          perform this operation. The restart operation can be
          delayed until all of the configuration options have
          been set.

  @param EfiPlatformConfigurationActionNvramFailed 
          The controller specified by ControllerHandle is still
          in a usable state; its configuration has been updated
          via parsing the ParameterBlock. The driver tried to
          update the driver's private NVRAM store with
          information from ParameterBlock and failed. No actions
          are required before this controller can be used again
          with the updated configuration settings, but these
          configuration settings are not guaranteed to persist
          after ControllerHandle is stopped.

**/
typedef enum {
  EfiPlatformConfigurationActionNone              = 0,
  EfiPlatformConfigurationActionStopController    = 1,
  EfiPlatformConfigurationActionRestartController = 2,
  EfiPlatformConfigurationActionRestartPlatform   = 3,
  EfiPlatformConfigurationActionNvramFailed       = 4,
  EfiPlatformConfigurationActionMaximum
} EFI_PLATFORM_CONFIGURATION_ACTION;


/**
  The UEFI driver repeatedly calls Query, processes the
  information returned by the platform, and calls Response passing
  in the arguments returned from Query. The UEFI driver must
  continuously call Query until EFI_NOT_FOUND is returned. For
  every call to Query that returns EFI_SUCCESS a corrisponding
  call to Response is required passing in the same
  ContollerHandle, ChildHandle, Instance, ParameterTypeGuid,
  ParameterBlock, and ParameterBlockSize. The UEFI driver may
  update values in ParameterBlock based on rules defined by
  ParameterTypeGuid. The platform is responsible for freeing
  ParameterBlock and the UEFI driver must not try to free it

  @param This   A pointer to the
                EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOC OL
                instance.

  @param ControllerHandle The handle the driver is returning
                          configuration information about.

  @param ChildHandle  The handle of the child controller to
                      return information on. This is an optional
                      parameter that may be NULL. It will be
                      NULL for device drivers, and for bus
                      drivers that attempt to get options for
                      the bus controller. It will not be NULL
                      for a bus driver that attempts to get
                      options for one of its child controllers.
                      Instance Instance data returned from
                      Query().

  @param ParameterTypeGuid ParameterTypeGuid returned from
                           Query.

  @param ParameterBlock ParameterBlock returned from Query.

  @param ParameterBlockSize The ParameterBlock size returned
                            from Query.

  @param Configuration  ActionThe driver tells the platform what
                        action is required for ParameterBlock to
                        take effect.
  
  
  @retval EFI_SUCCESS The platform return parameter information
                      for ControllerHandle.
  
  @retval EFI_NOT_FOUND Instance was not found.
  
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid
                                EFI_HANDLE.
  
  @retval EFI_INVALID_PARAMETER Instance is zero.
  
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PLATFORM_TO_DRIVER_CONFIGURATION_RESPONSE) (
  IN CONST  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL *This,
  IN CONST  EFI_HANDLE  ControllerHandle,
  IN CONST  EFI_HANDLE  ChildHandle OPTIONAL,
  IN CONST  UINTN       *Instance,
  IN CONST  EFI_GUID    *ParameterTypeGuid,
  IN CONST  VOID        *ParameterBlock,
  IN CONST  UINTN       ParameterBlockSize ,
  IN CONST  EFI_PLATFORM_CONFIGURATION_ACTION ConfigurationAction
);


/**
  The EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL is used by the
  UEFI driver to query the platform for configuration information.
  The UEFI driver calls Query() multiple times to get
  configuration information from the platform. For every call to
  Query() there must be a matching call to Response() so the
  UEFI driver can inform the platform how it used the
  information passed in from Query(). It¡¯s legal for a UEFI
  driver to use Response() to inform the platform it does not
  understand the data returned via Query() and thus no action was
  taken.

  @param  Query   Called by the UEFI Driver Start() function to
                  get configuration information from the
                  platform.
  
  @param  Response  Called by the UEFI Driver Start() function
                    to let the platform know how UEFI driver
                    processed the data return from Query.
  

**/
struct _EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL {
  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_QUERY    Query;
  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_RESPONSE Response;
};



#define EFI_PLATFORM_TO_DRIVER_CONFIGURATION_CLP_GUID   \
  {0x345ecc0e, 0xcb6, 0x4b75, { 0xbb, 0x57, 0x1b, 0x12, 0x9c, 0x47, 0x33,0x3e } }

/**
   
  ParameterTypeGuid provides the support for parameters
  communicated through the DMTF SM CLP Specification 1.0 Final
  Standard to be used to configure the UEFI driver. In this
  section the producer of the
  EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL is platform
  firmware and the consumer is the UEFI driver. Note: if future
  versions of the DMTF SM CLP Specification require changes to the
  parameter block definition, newer ParameterTypeGuid will be
  used.

  @param CLPCommand   A pointer to the DMTF SM CLP command line
                      null-terminated string that the driver is
                      required to parse and process when this
                      EFI_SUCCESS The platform return parameter
                      information for ControllerHandle.
                      EFI_NOT_FOUND Instance was not found.
                      EFI_INVALID_PARAMETER ControllerHandle is
                      not a valid EFI_HANDLE.
                      EFI_INVALID_PARAMETER Instance is zero.
                      function is called. See the DMTF SM CLP
                      Specification 1.0 Final Standard for
                      details on the format and syntax of the
                      CLP command line string. CLPCommand buffer
                      is allocated by the producer of the
                      EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOOL.

  @param CLPCommandLength   The length of the CLP Command in
                            bytes.

  @param CLPReturnString  A pointer to the CLP return status
                          string that the driver is required to
                          provide to the calling agent. The
                          calling agent may parse and/ or pass
                          this for processing and user feedback.
                          The SM CLP Command Response string
                          buffer is filled in by the UEFI driver
                          in the "keyword=value" format
                          described in the SM CLP Specification,
                          unless otherwise requested via the SM
                          CLP Coutput option in the Command Line
                          string buffer. UEFI driver's support
                          for this default "keyword=value"
                          output format is required if the UEFI
                          driver supports this protocol, while
                          support for other SM CLP output
                          formats is optional (the UEFI Driver
                          should return an EFI_UNSUPPORTED if
                          the SM CLP Coutput option requested by
                          the caller is not supported by the
                          UEFI Driver). CLPReturnString buffer
                          is allocated by the consumer of the
                          EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOC
                          OL and undefined prior to the call to
                          Response().

  @param CLPReturnStringLength  The length of the CLP return
                                status string in bytes.

  @param CLPReturnStatus  SM CLP Command Status (see DMTF SM CLP
                          Specification 1.0 Final Standard -
                          Table 4) CLPErrorValue SM CLP
                          Processing Error Value (see DMTF SM
                          CLP Specification 1.0 Final Standard -
                          Table 6). This field is filled in by
                          the consumer of the
                          EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOC 
                          OL and undefined prior to the call to
                          Response().

  @param CLPMessageCode   Bit 15: OEM Message Code Flag 0 =
                          Message Code is an SM CLP Probable
                          Cause Value. (see SM CLP Specification
                          Table 11) 1 = Message Code is OEM
                          Specific Bits 14-0: Message Code This
                          field is filled in by the consumer of
                          the
                          EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOC
                          OL and undefined prior to the call to
                          Response().

**/
typedef struct {
  CHAR8   *CLPCommand;
  UINT32  CLPCommandLength;
  CHAR8   *CLPReturnString;
  UINT32  CLPReturnStringLength;
  UINT8   CLPCmdStatus;
  UINT8   CLPErrorValue;
  UINT16  CLPMsgCode;
} EFI_CONFIGURE_CLP_PARAMETER_BLK;



extern EFI_GUID gEfiPlatformToDriverConfigurationClpGuid;

extern EFI_GUID gEfiPlatformToDriverConfigurationProtocolGuid;

#endif
