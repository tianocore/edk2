/** @file

  Definitions used internal to the PrmPkg implementation for PRM module image context.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <IndustryStandard/PeImage.h>
#include <Library/PeCoffLib.h>

#include <PrmExportDescriptor.h>

#pragma pack(push, 1)

typedef struct {
  PE_COFF_LOADER_IMAGE_CONTEXT           PeCoffImageContext;
  EFI_IMAGE_EXPORT_DIRECTORY             *ExportDirectory;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT    *ExportDescriptor;
} PRM_MODULE_IMAGE_CONTEXT;

#pragma pack(pop)
