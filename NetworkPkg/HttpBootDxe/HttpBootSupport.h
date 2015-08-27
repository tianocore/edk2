/** @file
  Support functions declaration for UEFI HTTP boot driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_HTTP_BOOT_SUPPORT_H__
#define __EFI_HTTP_BOOT_SUPPORT_H__

/**
  Get the Nic handle using any child handle in the IPv4 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv4.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp4Children (
  IN EFI_HANDLE                 ControllerHandle
  );

/**
  This function is to convert UINTN to ASCII string with the required formatting.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         The pointer to the buffer for ASCII string.
  @param[in]  Length         The length of the required format.

**/
VOID
HttpBootUintnToAscDecWithFormat (
  IN UINTN                       Number,
  IN UINT8                       *Buffer,
  IN INTN                        Length
  );


/**
  This function is to display the IPv4 address.

  @param[in]  Ip        The pointer to the IPv4 address.

**/
VOID
HttpBootShowIp4Addr (
  IN EFI_IPv4_ADDRESS   *Ip
  );

//
// A wrapper structure to hold the HTTP headers.
//
typedef struct {
  UINTN                       MaxHeaderCount;
  UINTN                       HeaderCount;
  EFI_HTTP_HEADER             *Headers;
} HTTP_IO_HEADER;

/**
  Create a HTTP_IO_HEADER to hold the HTTP header items.

  @param[in]  MaxHeaderCount         The maximun number of HTTP header in this holder.

  @return    A pointer of the HTTP header holder or NULL if failed.
  
**/
HTTP_IO_HEADER *
HttpBootCreateHeader (
  IN  UINTN                MaxHeaderCount
  );

/**
  Destroy the HTTP_IO_HEADER and release the resouces. 

  @param[in]  HttpIoHeader       Point to the HTTP header holder to be destroyed.

**/
VOID
HttpBootFreeHeader (
  IN  HTTP_IO_HEADER       *HttpIoHeader
  );

/**
  Set or update a HTTP header with the field name and corresponding value.

  @param[in]  HttpIoHeader       Point to the HTTP header holder.
  @param[in]  FieldName          Null terminated string which describes a field name. 
  @param[in]  FieldValue         Null terminated string which describes the corresponding field value.

  @retval  EFI_SUCCESS           The HTTP header has been set or updated.
  @retval  EFI_INVALID_PARAMETER Any input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES  Insufficient resource to complete the operation.
  @retval  Other                 Unexpected error happened.
  
**/
EFI_STATUS
HttpBootSetHeader (
  IN  HTTP_IO_HEADER       *HttpIoHeader,
  IN  CHAR8                *FieldName,
  IN  CHAR8                *FieldValue
  );

//
// HTTP_IO configuration data for IPv4
//
typedef struct {
  EFI_HTTP_VERSION          HttpVersion;
  UINT32                    RequestTimeOut;  // In milliseconds.
  UINT32                    ResponseTimeOut; // In milliseconds.
  BOOLEAN                   UseDefaultAddress;
  EFI_IPv4_ADDRESS          LocalIp;
  EFI_IPv4_ADDRESS          SubnetMask;
  UINT16                    LocalPort;
} HTTP4_IO_CONFIG_DATA;

//
// HTTP_IO configuration
//
typedef union {
  HTTP4_IO_CONFIG_DATA       Config4;
} HTTP_IO_CONFIG_DATA;

//
// HTTO_IO wrapper of the EFI HTTP service.
//
typedef struct {
  UINT8                     IpVersion;
  EFI_HANDLE                Image;
  EFI_HANDLE                Controller;
  EFI_HANDLE                Handle;
  
  EFI_HTTP_PROTOCOL         *Http;

  EFI_HTTP_TOKEN            ReqToken;
  EFI_HTTP_MESSAGE          ReqMessage;
  EFI_HTTP_TOKEN            RspToken;
  EFI_HTTP_MESSAGE          RspMessage;

  BOOLEAN                   IsTxDone;
  BOOLEAN                   IsRxDone;
} HTTP_IO;

//
// A wrapper structure to hold the received HTTP response data.
//
typedef struct {
  EFI_HTTP_RESPONSE_DATA      Response;
  UINTN                       HeaderCount;
  EFI_HTTP_HEADER             *Headers;
  UINTN                       BodyLength;
  CHAR8                       *Body;
} HTTP_IO_RESOPNSE_DATA;

/**
  Create a HTTP_IO to access the HTTP service. It will create and configure
  a HTTP child handle.

  @param[in]  Image          The handle of the driver image.
  @param[in]  Controller     The handle of the controller.
  @param[in]  IpVersion      IP_VERSION_4 or IP_VERSION_6.
  @param[in]  ConfigData     The HTTP_IO configuration data.
  @param[out] HttpIo         The HTTP_IO.
  
  @retval EFI_SUCCESS            The HTTP_IO is created and configured.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Failed to create the HTTP_IO or configure it.

**/
EFI_STATUS
HttpIoCreateIo (
  IN EFI_HANDLE             Image,
  IN EFI_HANDLE             Controller,
  IN UINT8                  IpVersion,
  IN HTTP_IO_CONFIG_DATA    *ConfigData,
  OUT HTTP_IO               *HttpIo
  );

/**
  Destroy the HTTP_IO and release the resouces. 

  @param[in]  HttpIo          The HTTP_IO which wraps the HTTP service to be destroyed.

**/
VOID
HttpIoDestroyIo (
  IN HTTP_IO                *HttpIo
  );

/**
  Synchronously send a HTTP REQUEST message to the server.
  
  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   Request          A pointer to storage such data as URL and HTTP method.
  @param[in]   HeaderCount      Number of HTTP header structures in Headers list. 
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   BodyLength       Length in bytes of the HTTP body.
  @param[in]   Body             Body associated with the HTTP request. 
  
  @retval EFI_SUCCESS            The HTTP request is trasmitted.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoSendRequest (
  IN  HTTP_IO                *HttpIo,
  IN  EFI_HTTP_REQUEST_DATA  *Request,      OPTIONAL
  IN  UINTN                  HeaderCount,
  IN  EFI_HTTP_HEADER        *Headers,      OPTIONAL
  IN  UINTN                  BodyLength,
  IN  VOID                   *Body          OPTIONAL
  );

/**
  Synchronously receive a HTTP RESPONSE message from the server.
  
  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   RecvMsgHeader    TRUE to receive a new HTTP response (from message header).
                                FALSE to continue receive the previous response message.
  @param[out]  ResponseData     Point to a wrapper of the received response data.
  
  @retval EFI_SUCCESS            The HTTP resopnse is received.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoRecvResponse (
  IN      HTTP_IO                  *HttpIo,
  IN      BOOLEAN                  RecvMsgHeader,
     OUT  HTTP_IO_RESOPNSE_DATA    *ResponseData
  );

#endif
