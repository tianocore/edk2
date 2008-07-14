/** @file
  Library Routines to create IFR independent of string data - assume tokens already exist
  Primarily to be used for exporting op-codes at a label in pre-defined forms.

Copyright (c) 2007 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LibraryInternal.h"

STATIC EFI_GUID mIfrVendorGuid = EFI_IFR_TIANO_GUID;

/**
  Create GUIDed opcode for banner. Banner opcode
  EFI_IFR_EXTEND_OP_BANNER is extended opcode specific
  to Intel's implementation.

  @param  Title                  String ID for title
  @param  LineNumber             Line number for this banner
  @param  Alignment              Alignment for this banner, left, center or right
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateBannerOpCode (
  IN      EFI_STRING_ID       Title,
  IN      UINT16              LineNumber,
  IN      UINT8               Alignment,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_GUID_BANNER         Banner;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (Data->Offset + sizeof (EFI_IFR_GUID_BANNER) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Banner.Header.OpCode  = EFI_IFR_GUID_OP;
  Banner.Header.Length  = sizeof (EFI_IFR_GUID_BANNER);
  Banner.Header.Scope   = 0;
  CopyMem (&Banner.Guid, &mIfrVendorGuid, sizeof (EFI_IFR_GUID));
  Banner.ExtendOpCode   = EFI_IFR_EXTEND_OP_BANNER;
  Banner.Title          = Title;
  Banner.LineNumber     = LineNumber;
  Banner.Alignment      = Alignment;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  CopyMem (LocalBuffer, &Banner, sizeof (EFI_IFR_GUID_BANNER));
  Data->Offset += sizeof (EFI_IFR_GUID_BANNER);

  return EFI_SUCCESS;
}

