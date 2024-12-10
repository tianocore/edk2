/** @file
  FIT Load Image Support
Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FitLib.h"

PROPERTY_DATA  PropertyData32List[] = {
  { "data-offset", PAYLOAD_ENTRY_OFFSET_OFFSET  },
  { "data-size",   PAYLOAD_ENTRY_SIZE_OFFSET    },
  { "reloc-start", RELOCATE_TABLE_OFFSET_OFFSET }
};

PROPERTY_DATA  PropertyData64List[] = {
  { "entry", PAYLOAD_ENTRY_POINT_OFFSET },
  { "load",  PAYLOAD_LOAD_ADDR_OFFSET   }
};

/**
  Parse the target firmware image info in FIT.
  @param[in]  Fdt            Memory address of a fdt.
  @param[in]  Firmware       Target name of an image.
  @param[out] Context        The FIT image context pointer.
  @retval EFI_NOT_FOUND      FIT node dose not find.
  @retval EFI_SUCCESS        FIT binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
FitParseFirmwarePropertyData (
  IN   VOID               *Fdt,
  IN   CHAR8              *Firmware,
  OUT  FIT_IMAGE_CONTEXT  *Context
  )
{
  CONST FDT_PROPERTY  *PropertyPtr;
  INT32               ImageNode;
  INT32               TianoNode;
  INT32               TempLen;
  UINT32              *Data32;
  UINT64              *Data64;
  UINT32              *ContextOffset32;
  UINT64              *ContextOffset64;
  INT32               Index;

  ImageNode = FdtSubnodeOffsetNameLen (Fdt, 0, "images", (INT32)AsciiStrLen ("images"));
  if (ImageNode <= 0) {
    return EFI_NOT_FOUND;
  }

  TianoNode = FdtSubnodeOffsetNameLen (Fdt, ImageNode, Firmware, (INT32)AsciiStrLen (Firmware));
  if (TianoNode <= 0) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < sizeof (PropertyData32List) / sizeof (PROPERTY_DATA); Index++) {
    PropertyPtr      = FdtGetProperty (Fdt, TianoNode, PropertyData32List[Index].Name, &TempLen);
    Data32           = (UINT32 *)(PropertyPtr->Data);
    ContextOffset32  = (UINT32 *)((UINTN)Context + PropertyData32List[Index].Offset);
    *ContextOffset32 = Fdt32ToCpu (*Data32);
  }

  for (Index = 0; Index < sizeof (PropertyData64List)/sizeof (PROPERTY_DATA); Index++) {
    PropertyPtr      = FdtGetProperty (Fdt, TianoNode, PropertyData64List[Index].Name, &TempLen);
    Data64           = (UINT64 *)(PropertyPtr->Data);
    ContextOffset64  = (UINT64 *)((UINTN)Context + PropertyData64List[Index].Offset);
    *ContextOffset64 = Fdt64ToCpu (*Data64);
  }

  //
  // Need to load the FIT header (FDT) too. "load" property of tianocore entrypoint refers to image.
  // Since FDT is directly before tianocore, subtract its size from load.
  //
  Context->PayloadLoadAddress -= FdtTotalSize (Fdt);

  return EFI_SUCCESS;
}

/**
  Parse the FIT image info.
  @param[in]  ImageBase      Memory address of an image.
  @param[out] Context        The FIT image context pointer.
  @retval EFI_UNSUPPORTED         Unsupported binary type.
  @retval EFI_SUCCESS             FIT binary is loaded successfully.
**/
EFI_STATUS
EFIAPI
ParseFitImage (
  IN   VOID               *ImageBase,
  OUT  FIT_IMAGE_CONTEXT  *Context
  )
{
  VOID                *Fdt;
  INT32               ConfigNode;
  INT32               Config1Node;
  CONST FDT_PROPERTY  *PropertyPtr;
  INT32               TempLen;
  UINT32              *Data32;
  UINT64              Value;
  EFI_STATUS          Status;
  UINTN               UplSize;
  CHAR8               *Firmware;

  Status = FdtCheckHeader (ImageBase);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Fdt         = ImageBase;
  PropertyPtr = FdtGetProperty (Fdt, 0, "size", &TempLen);
  Data32      = (UINT32 *)(PropertyPtr->Data);
  UplSize     = Value = Fdt32ToCpu (*Data32);
  ConfigNode  = FdtSubnodeOffsetNameLen (Fdt, 0, "configurations", (INT32)AsciiStrLen ("configurations"));
  if (ConfigNode <= 0) {
    return EFI_NOT_FOUND;
  }

  Config1Node = FdtSubnodeOffsetNameLen (Fdt, ConfigNode, "conf-1", (INT32)AsciiStrLen ("conf-1"));
  if (Config1Node <= 0) {
    return EFI_NOT_FOUND;
  }

  PropertyPtr = FdtGetProperty (Fdt, Config1Node, "firmware", &TempLen);
  Firmware    = (CHAR8 *)(PropertyPtr->Data);

  FitParseFirmwarePropertyData (Fdt, Firmware, Context);

  Context->ImageBase          = (EFI_PHYSICAL_ADDRESS)ImageBase;
  Context->PayloadSize        = UplSize;
  Context->RelocateTableCount = (Context->PayloadEntrySize - (Context->RelocateTableOffset - Context->PayloadEntryOffset)) / sizeof (FIT_RELOCATE_ITEM);

  return EFI_SUCCESS;
}
