/** @file
  The internal definitions of EFI REST Resource JSON to C structure convertor
  Protocol.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EFI_REST_JSON_STRUCTURE_INTERNAL_H_
#define EFI_REST_JSON_STRUCTURE_INTERNAL_H_

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

///
/// Internal structure to maintain the information of JSON to
/// C structure convertor.
///
typedef struct _REST_JSON_STRUCTURE_INSTANCE {
  LIST_ENTRY                                   NextRestJsonStructureInstance; ///< Next convertor instance
  UINTN                                        NumberOfNameSpaceToConvert;    ///< Number of resource type this convertor supports.
  EFI_REST_JSON_RESOURCE_TYPE_IDENTIFIER       *SupportedRsrcIndentifier;     ///< The supported resource type array.
  EFI_REST_JSON_STRUCTURE_TO_STRUCTURE         JsonToStructure;               ///< JSON to C structure function
  EFI_REST_JSON_STRUCTURE_TO_JSON              StructureToJson;               ///< C structure to JSON function
  EFI_REST_JSON_STRUCTURE_DESTORY_STRUCTURE    DestroyStructure;              ///< Destory C struture function.
} REST_JSON_STRUCTURE_INSTANCE;
#endif
