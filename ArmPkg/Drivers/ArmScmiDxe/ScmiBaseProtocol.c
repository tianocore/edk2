/** @file

  Copyright (c) 2017-2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ArmScmiBaseProtocol.h>

#include "ArmScmiBaseProtocolPrivate.h"
#include "ScmiPrivate.h"

/** Return version of the Base protocol supported by SCP firmware.

  @param[in]  This     A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] Version  Version of the supported SCMI Base protocol.

  @retval EFI_SUCCESS       The version of the protocol is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
BaseGetVersion (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT32              *Version
  )
{
  return ScmiGetProtocolVersion (ScmiProtocolIdBase, Version);
}

/** Return total number of SCMI protocols supported by the SCP firmware.

  @param[in]  This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] TotalProtocols Total number of SCMI protocols supported.

  @retval EFI_SUCCESS       Total number of protocols supported are returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
BaseGetTotalProtocols (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT32              *TotalProtocols
  )
{
  EFI_STATUS  Status;
  UINT32      *ReturnValues;

  Status = ScmiGetProtocolAttributes (ScmiProtocolIdBase, &ReturnValues);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *TotalProtocols = SCMI_TOTAL_PROTOCOLS (ReturnValues[0]);

  return EFI_SUCCESS;
}

/** Common function which returns vendor details.

  @param[in] MessageId       ScmiMessageIdBaseDiscoverVendor
                             OR
                             ScmiMessageIdBaseDiscoverSubVendor

  @param[out] VendorIdentifier ASCII name of the vendor/subvendor.

  @retval EFI_SUCCESS       VendorIdentifier is returned.
  @retval EFI_DEVICE_ERROR  SCP returns an SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
BaseDiscoverVendorDetails (
  IN  SCMI_MESSAGE_ID_BASE  MessageId,
  OUT UINT8                 VendorIdentifier[SCMI_MAX_STR_LEN]
  )
{
  EFI_STATUS    Status;
  UINT32        *ReturnValues;
  SCMI_COMMAND  Cmd;
  UINT32        PayloadLength;

  Cmd.ProtocolId = ScmiProtocolIdBase;
  Cmd.MessageId  = MessageId;

  PayloadLength = 0;

  Status = ScmiCommandExecute (
             &Cmd,
             &PayloadLength,
             &ReturnValues
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AsciiStrCpyS (
    (CHAR8*)VendorIdentifier,
    SCMI_MAX_STR_LEN,
    (CONST CHAR8*)ReturnValues
    );

  return EFI_SUCCESS;
}

/** Return vendor name.

  @param[in] This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] VendorIdentifier Null terminated ASCII string of up to
                               16 bytes with a vendor name.

  @retval EFI_SUCCESS       VendorIdentifier is returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
BaseDiscoverVendor (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT8               VendorIdentifier[SCMI_MAX_STR_LEN]
  )
{
  return BaseDiscoverVendorDetails (
           ScmiMessageIdBaseDiscoverVendor,
           VendorIdentifier
           );
}

/** Return sub vendor name.

  @param[in]  This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] VendorIdentifier Null terminated ASCII string of up to
                               16 bytes with a sub vendor name.

  @retval EFI_SUCCESS       VendorIdentifier is returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
EFI_STATUS
BaseDiscoverSubVendor (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT8               VendorIdentifier[SCMI_MAX_STR_LEN]
  )
{
  return BaseDiscoverVendorDetails (
           ScmiMessageIdBaseDiscoverSubVendor,
           VendorIdentifier
           );
}

/** Return implementation version.

  @param[in] This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] ImplementationVersion Vendor specific implementation version.

  @retval EFI_SUCCESS       Implementation version is returned.
  @retval EFI_DEVICE_ERROR  SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)    Other errors.
**/
STATIC
EFI_STATUS
BaseDiscoverImplVersion (
  IN  SCMI_BASE_PROTOCOL  *This,
  OUT UINT32              *ImplementationVersion
  )
{
  EFI_STATUS    Status;
  UINT32        *ReturnValues;
  SCMI_COMMAND  Cmd;
  UINT32        PayloadLength;

  Cmd.ProtocolId = ScmiProtocolIdBase;
  Cmd.MessageId  = ScmiMessageIdBaseDiscoverImplementationVersion;

  PayloadLength = 0;

  Status = ScmiCommandExecute (
             &Cmd,
             &PayloadLength,
             &ReturnValues
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *ImplementationVersion = ReturnValues[0];

  return EFI_SUCCESS;
}

/** Return list of protocols.

  @param[in]  This           A Pointer to SCMI_BASE_PROTOCOL Instance.

  @param[out] ProtocolListSize  Size of the ProtocolList.

  @param[out] ProtocolList   Protocol list.

  @retval EFI_SUCCESS          List of protocols is returned.
  @retval EFI_BUFFER_TOO_SMALL ProtocolListSize is too small for the result.
                                It has been updated to the size needed.
  @retval EFI_DEVICE_ERROR     SCP returns a SCMI error.
  @retval !(EFI_SUCCESS)       Other errors.
**/
STATIC
EFI_STATUS
BaseDiscoverListProtocols (
  IN     SCMI_BASE_PROTOCOL  *This,
  IN OUT UINT32              *ProtocolListSize,
  OUT    UINT8               *ProtocolList
  )
{
  EFI_STATUS          Status;
  UINT32              TotalProtocols;
  UINT32              *MessageParams;
  BASE_DISCOVER_LIST  *DiscoverList;
  UINT32              Skip;
  UINT32              Index;
  SCMI_COMMAND        Cmd;
  UINT32              PayloadLength;
  UINT32              RequiredSize;

  Status = BaseGetTotalProtocols (This, &TotalProtocols);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ScmiCommandGetPayload (&MessageParams);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  RequiredSize = sizeof (UINT8) * TotalProtocols;
  if (*ProtocolListSize < RequiredSize) {
    *ProtocolListSize = RequiredSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  Cmd.ProtocolId = ScmiProtocolIdBase;
  Cmd.MessageId  = ScmiMessageIdBaseDiscoverListProtocols;

  Skip = 0;

  while (Skip < TotalProtocols) {

    *MessageParams = Skip;

    // Note PayloadLength is a IN/OUT parameter.
    PayloadLength = sizeof (Skip);

    Status = ScmiCommandExecute (
               &Cmd,
               &PayloadLength,
               (UINT32**)&DiscoverList
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    for (Index = 0; Index < DiscoverList->NumProtocols; Index++) {
      ProtocolList[Skip++] = DiscoverList->Protocols[Index];
    }
  }

  *ProtocolListSize = RequiredSize;

  return EFI_SUCCESS;
}

// Instance of the SCMI Base protocol.
STATIC CONST SCMI_BASE_PROTOCOL BaseProtocol = {
  BaseGetVersion,
  BaseGetTotalProtocols,
  BaseDiscoverVendor,
  BaseDiscoverSubVendor,
  BaseDiscoverImplVersion,
  BaseDiscoverListProtocols
};

/** Initialize Base protocol and install protocol on a given handle.

   @param[in] Handle              Handle to install Base protocol.

   @retval EFI_SUCCESS            Base protocol interface installed
                                  successfully.
**/
EFI_STATUS
ScmiBaseProtocolInit (
  IN OUT EFI_HANDLE* Handle
  )
{
  return gBS->InstallMultipleProtocolInterfaces (
                Handle,
                &gArmScmiBaseProtocolGuid,
                &BaseProtocol,
                NULL
                );
}
