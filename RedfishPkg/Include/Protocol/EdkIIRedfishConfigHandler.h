/** @file
  This file defines the EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL interface.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_REDFISH_CONFIG_HANDLER_H_
#define EDKII_REDFISH_CONFIG_HANDLER_H_

typedef struct _EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL;

#define EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL_GUID \
    {  \
      0xbc0fe6bb, 0x2cc9, 0x463e, { 0x90, 0x82, 0xfa, 0x11, 0x76, 0xfc, 0x67, 0xde }  \
    }

typedef struct {
  EFI_HANDLE        RedfishServiceRestExHandle; ///< REST EX EFI handle associated with this Redfish service.
  UINTN             RedfishServiceVersion;      ///< Redfish service version.
  CHAR16            *RedfishServiceLocation;    ///< Redfish service location.
  CHAR16            *RedfishServiceUuid;        ///< Redfish service UUID.
  CHAR16            *RedfishServiceOs;          ///< Redfish service OS.
  CHAR16            *RedfishServiceOsVersion;   ///< Redfish service OS version.
  CHAR16            *RedfishServiceProduct;     ///< Redfish service product name.
  CHAR16            *RedfishServiceProductVer;  ///< Redfish service product version.
  BOOLEAN           RedfishServiceUseHttps;     ///< Redfish service uses HTTPS.
} REDFISH_CONFIG_SERVICE_INFORMATION;

/**
  Initialize a configure handler of EDKII Redfish feature driver.

  This function will be called by the EDKII Redfish config handler driver to
  initialize the configure handler of each EDKII Redfish feature driver.

  @param[in]   This                    Pointer to EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL instance.
  @param[in]   RedfishServiceinfo      Redfish service information.

  @retval EFI_SUCCESS                  The handler has been initialized successfully.
  @retval EFI_DEVICE_ERROR             Failed to create or configure the REST EX protocol instance.
  @retval EFI_ALREADY_STARTED          This handler has already been initialized.
  @retval Other                        Error happens during the initialization.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL_INIT) (
  IN  EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL *This,
  IN  REDFISH_CONFIG_SERVICE_INFORMATION  *RedfishServiceinfo
  );

/**
  Stop a Redfish configure handler of EDKII Redfish feature driver.

  @param[in]   This                Pointer to EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL instance.

  @retval EFI_SUCCESS              This handler has been stoped successfully.
  @retval Others                   Some error happened.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL_STOP) (
  IN     EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL    *This
  );

struct _EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL {
  EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL_INIT      Init;
  EDKII_REDFISH_CONFIG_HANDLER_PROTOCOL_STOP      Stop;
};


extern EFI_GUID gdkIIRedfishConfigHandlerProtocolGuid;

#endif
