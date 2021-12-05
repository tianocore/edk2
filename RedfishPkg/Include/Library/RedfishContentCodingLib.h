/** @file
  Definitinos of RedfishContentCodingLib.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_CONTENT_CODING_LIB_H_
#define REDFISH_CONTENT_CODING_LIB_H_

/**
  This is the function to encode the content use the
  algorithm indicated in ContentEncodedValue. The naming of
  ContentEncodedValue is follow HTTP spec or could be a
  platform-specific value.

  @param[in]   ContentEncodedValue   HTTP conent encoded value.
                                     The value could be one of below
                                     or any which is platform-specific.
                                       - HTTP_CONTENT_ENCODING_IDENTITY "identity"
                                       - HTTP_CONTENT_ENCODING_GZIP     "gzip"
                                       - HTTP_CONTENT_ENCODING_COMPRESS "compress"
                                       - HTTP_CONTENT_ENCODING_DEFLATE  "deflate"
                                       - HTTP_CONTENT_ENCODING_BROTLI   "br"
  @param[in]   OriginalContent       Original content.
  @param[in]   OriginalContentLength The length of original content.
  @param[out]  EncodedContentPointer Pointer to receive the encoded content pointer.
  @param[out]  EncodedContentLength  Length of encoded content.

  @retval EFI_SUCCESS              Content is encoded successfully.
  @retval EFI_UNSUPPORTED          No supported encoding funciton,
  @retval EFI_INVALID_PARAMETER    One of the given parameter is invalid.

**/

EFI_STATUS
RedfishContentEncode  (
  IN CHAR8   *ContentEncodedValue,
  IN CHAR8   *OriginalContent,
  IN UINTN   OriginalContentLength,
  OUT VOID   **EncodedContentPointer,
  OUT UINTN  *EncodedLength
  );

/**
  This is the function to decode the content use the
  algorithm indicated in ContentEncodedValue. The naming of
  ContentEncodedValue is follow HTTP spec or could be a
  platform-specific value.

  @param[in]   ContentDecodedValue   HTTP conent decoded value.
                                     The value could be one of below
                                     or any which is platform-specific.
                                       - HTTP_CONTENT_ENCODING_IDENTITY "identity"
                                       - HTTP_CONTENT_ENCODING_GZIP     "gzip"
                                       - HTTP_CONTENT_ENCODING_COMPRESS "compress"
                                       - HTTP_CONTENT_ENCODING_DEFLATE  "deflate"
                                       - HTTP_CONTENT_ENCODING_BROTLI   "br"
  @param[in]   ContentPointer        Original content.
  @param[in]   ContentLength         The length of original content.
  @param[out]  DecodedContentPointer Pointer to receive decoded content pointer.
  @param[out]  DecodedContentLength  Length of decoded content.

  @retval EFI_SUCCESS              Content is decoded successfully.
  @retval EFI_UNSUPPORTED          No supported decoding funciton,
  @retval EFI_INVALID_PARAMETER    One of the given parameter is invalid.

**/
EFI_STATUS
RedfishContentDecode (
  IN CHAR8   *ContentEncodedValue,
  IN VOID    *ContentPointer,
  IN UINTN   ContentLength,
  OUT VOID   **DecodedContentPointer,
  OUT UINTN  *DecodedLength
  );

#endif
