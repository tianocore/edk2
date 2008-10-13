/** @file
  EFI_AUTHENTICATION_INFO_PROTOCOL as defined in UEFI 2.0.
  This protocol is used on any device handle to obtain authentication information 
  associated with the physical or logical device.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __AUTHENTICATION_INFO_H__
#define __AUTHENTICATION_INFO_H__

#define EFI_AUTHENTICATION_INFO_PROTOCOL_GUID \
  { \
    0x7671d9d0, 0x53db, 0x4173, {0xaa, 0x69, 0x23, 0x27, 0xf2, 0x1f, 0x0b, 0xc7 } \
  }
  
#define EFI_AUTHENTICATION_CHAP_RADIUS_GUID \
  { \
    0xd6062b50, 0x15ca, 0x11da, {0x92, 0x19, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d } \
  }

#define EFI_AUTHENTICATION_CHAP_LOCAL_GUID \
  { \
    0xc280c73e, 0x15ca, 0x11da, {0xb0, 0xca, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d } \
  }

typedef struct _EFI_AUTHENTICATION_INFO_PROTOCOL EFI_AUTHENTICATION_INFO_PROTOCOL;

typedef struct {
  EFI_GUID         Guid;
  UINT16           Length;
} AUTH_NODE_HEADER;

typedef struct {
  AUTH_NODE_HEADER Header;
  EFI_IPv6_ADDRESS RadiusIpAddr;             ///< IPv4 or IPv6 address
  UINT16           Reserved;
  EFI_IPv6_ADDRESS NasIpAddr;                ///< IPv4 or IPv6 address
  UINT16           NasSecretLength; 
  UINT8            *NasSecret;      
  UINT16           ChapSecretLength;
  UINT8            *ChapSecret;
  UINT16           ChapNameLength;
  UINT8            *ChapName;
} CHAP_RADIUS_AUTH_NODE;

typedef struct {
  AUTH_NODE_HEADER Header;
  UINT16           Reserved;
  UINT16           UserSecretLength;
  UINT8            *UserSecret;     
  UINT16           UserNameLength;
  UINT8            *UserName;
  UINT16           ChapSecretLength;
  UINT8            *ChapSecret;
  UINT16           ChapNameLength;
  UINT8            *ChapName;
} CHAP_LOCAL_AUTH_NODE;

/**
  Retrieves the Authentication information associated with a particular controller handle.

  @param  This                  Pointer to the EFI_AUTHENTICATION_INFO_PROTOCOL
  @param  ControllerHandle      Handle to the Controller
  @param  Buffer                Pointer to the authentication information.

  @retval EFI_SUCCESS           Successfully retrieved Authentication information for the given ControllerHandle
  @retval EFI_INVALID_PARAMETER No matching Authentication information found for the given ControllerHandle
  @retval EFI_DEVICE_ERROR      The authentication information could not be retrieved due to a
                                hardware error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUTHENTICATION_PROTOCOL_INFO_GET)(
  IN  EFI_AUTHENTICATION_INFO_PROTOCOL *This,
  IN  EFI_HANDLE                       *ControllerHandle,
  OUT VOID                             *Buffer
  );  

/**
  Set the Authentication information for a given controller handle.

  @param  This                 Pointer to the EFI_AUTHENTICATION_INFO_PROTOCOL
  @param  ControllerHandle     Handle to the Controller
  @param  Buffer               Pointer to the authentication information.
                                
  @retval EFI_SUCCESS          Successfully set Authentication information for the given ControllerHandle
  @retval EFI_UNSUPPORTED      If the platform policies do not allow setting of the Authentication
                               information.
  @retval EFI_DEVICE_ERROR     The authentication information could not be configured due to a
                               hardware error.
  @retval EFI_OUT_OF_RESOURCES Not enough storage is available to hold the data.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUTHENTICATION_PROTOCOL_INFO_SET)(
  IN EFI_AUTHENTICATION_INFO_PROTOCOL  *This,
  IN EFI_HANDLE                        *ControllerHandle,
  IN VOID                              *Buffer
  );  

///
/// This protocol is used on any device handle to obtain authentication 
/// information associated with the physical or logical device.
///
struct _EFI_AUTHENTICATION_INFO_PROTOCOL {
  EFI_AUTHENTICATION_PROTOCOL_INFO_GET Get;
  EFI_AUTHENTICATION_PROTOCOL_INFO_SET Set;
};

extern EFI_GUID gEfiAuthenticationInfoProtocolGuid;
extern EFI_GUID gEfiAuthenticationChapRadiusGuid;
extern EFI_GUID gEfiAuthenticationChapLocalGuid;

#endif
