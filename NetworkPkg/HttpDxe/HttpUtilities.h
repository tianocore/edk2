/** @file
  The header files of HTTP helper functions for HttpDxe driver.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_HTTP_UTILITIES_H__
#define __EFI_HTTP_UTILITIES_H__

/**
  This function is used to manage the headers portion of an HTTP message by providing 
  the ability to add, remove, or replace HTTP headers.

  @param[in]   SeedMessageSize       Size in bytes of the initial HTTP header. This can be zero.  
  @param[in]   SeedMessage           Initial raw unformatted HTTP header to be used as a base for 
                                     building a new unformatted HTTP header. If NULL, SeedMessageSize 
                                     is ignored. The buffer containing this message will be allocated 
                                     and released by the caller.           
  @param[in]   DeleteCount           Number of null-terminated HTTP header field names in DeleteList.
  @param[in]   DeleteList            List of null-terminated HTTP header field names to remove from SeedMessage. 
                                     Only the field names are in this list because the field values are irrelevant 
                                     to this operation. If NULL, DeleteCount is ignored. The buffer containing the 
                                     list will be allocated and released by the caller.
  @param[in]   AppendCount           Number of header fields in AppendList. 
  @param[in]   AppendList            List of HTTP headers to populate NewMessage with. If SeedMessage is not NULL, 
                                     AppendList will be appended to the existing list from SeedMessage in NewMessage.
  @param[out]  NewMessageSize        Pointer to the size in bytes of the new unformatted HTTP header in NewMessage.       
  @param[out]  NewMessage            Pointer to a new unformatted HTTP header. The storage for this NewMessage is 
                                     allocated by the driver publishing this protocol, and must be freed by the caller. 
  
  @retval EFI_SUCCESS                Add, remove, and replace operations succeeded.
  @retval EFI_OUT_OF_RESOURCES       Could not allocate memory for NewMessage.
  
**/
EFI_STATUS
HttpUtilitiesBuild(
  IN     UINTN                       SeedMessageSize,
  IN     VOID                        *SeedMessage, OPTIONAL
  IN     UINTN                       DeleteCount,
  IN     CHAR8                       *DeleteList[], OPTIONAL
  IN     UINTN                       AppendCount,
  IN     EFI_HTTP_HEADER             *AppendList[], OPTIONAL
     OUT UINTN                       *NewMessageSize,
     OUT VOID                        **NewMessage
  );

/**
  This function is used to transform data stored in HttpMessage into a list of fields 
  paired with their corresponding values.

  @param[in]   HttpMessage           Contains raw unformatted HTTP header string. The buffer for this string will 
                                     be allocated and released by the caller.
  @param[in]   HttpMessageSize       Size in bytes of raw unformatted HTTP header.      
  @param[out]  HeaderFields          Array of key/value header pairs. The storage for all header pairs is allocated
                                     by the driver publishing this protocol, and must be freed by the caller. 
  @param[out]  FieldCount            Number of headers in HeaderFields.
  
  @retval EFI_SUCCESS                Parse HTTP header into array of key/value pairs succeeded.
  @retval EFI_OUT_OF_RESOURCES       Could not allocate memory for NewMessage.
  @retval EFI_INVALID_PARAMETER      One or more of the following conditions is TRUE:
                                     HttpMessage is NULL.
                                     HeaderFields is NULL.
                                     FieldCount is NULL.
  
**/
EFI_STATUS
HttpUtilitiesParse(
  IN  CHAR8                        *HttpMessage,
  IN  UINTN                        HttpMessageSize,
  OUT EFI_HTTP_HEADER              **HeaderFields,
  OUT UINTN                        *FieldCount
  );

#endif
