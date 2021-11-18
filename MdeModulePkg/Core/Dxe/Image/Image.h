/** @file
  Data structure and functions to load and unload PeImage.

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IMAGE_H_
#define _IMAGE_H_

//
// Private Data Types
//
#define IMAGE_FILE_HANDLE_SIGNATURE  SIGNATURE_32('i','m','g','f')
typedef struct {
  UINTN      Signature;
  BOOLEAN    FreeBuffer;
  VOID       *Source;
  UINTN      SourceSize;
} IMAGE_FILE_HANDLE;

#endif
