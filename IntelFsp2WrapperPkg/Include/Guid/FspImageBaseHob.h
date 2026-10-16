/** @file
  HOB with the FSP_INFO_HEADER.ImageBase value of an FSP component as
  found before the FSP wrapper wrote the load address into that field.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FSP_IMAGE_BASE_HOB_H_
#define FSP_IMAGE_BASE_HOB_H_

#define FSP_IMAGE_BASE_HOB_GUID \
  { 0x5acef3e9, 0x597b, 0x435d, { 0x93, 0xd2, 0x91, 0x1c, 0x11, 0xe6, 0x52, 0x5c } }

typedef struct {
  ///
  /// Base address of the FSP component in memory. This is the value the
  /// FSP wrapper wrote into FSP_INFO_HEADER.ImageBase.
  ///
  UINT32    FspBase;
  ///
  /// FSP_INFO_HEADER.ImageBase before the FSP wrapper wrote FspBase into it.
  ///
  UINT32    OriginalImageBase;
} FSP_IMAGE_BASE_HOB;

extern EFI_GUID  gFspImageBaseHobGuid;

#endif
