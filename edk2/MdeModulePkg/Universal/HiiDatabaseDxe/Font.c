/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    Font.c

Abstract:

    Implementation for EFI_HII_FONT_PROTOCOL.

Revision History


**/


#include "HiiDatabase.h"

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL        mEfiColors[16] = {
  //
  // B     G     R
  //
  {0x00, 0x00, 0x00, 0x00},  // BLACK
  {0x98, 0x00, 0x00, 0x00},  // BLUE
  {0x00, 0x98, 0x00, 0x00},  // GREEN
  {0x98, 0x98, 0x00, 0x00},  // CYAN
  {0x00, 0x00, 0x98, 0x00},  // RED
  {0x98, 0x00, 0x98, 0x00},  // MAGENTA
  {0x00, 0x98, 0x98, 0x00},  // BROWN
  {0x98, 0x98, 0x98, 0x00},  // LIGHTGRAY
  {0x30, 0x30, 0x30, 0x00},  // DARKGRAY - BRIGHT BLACK
  {0xff, 0x00, 0x00, 0x00},  // LIGHTBLUE
  {0x00, 0xff, 0x00, 0x00},  // LIGHTGREEN
  {0xff, 0xff, 0x00, 0x00},  // LIGHTCYAN
  {0x00, 0x00, 0xff, 0x00},  // LIGHTRED
  {0xff, 0x00, 0xff, 0x00},  // LIGHTMAGENTA
  {0x00, 0xff, 0xff, 0x00},  // YELLOW
  {0xff, 0xff, 0xff, 0x00},  // WHITE
};


/**
  Insert a character cell information to the list specified by GlyphInfoList.

  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphInfoList           HII_GLYPH_INFO list head.
  @param  Cell                    Incoming character cell information.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
STATIC
EFI_STATUS
NewCell (
  IN  CHAR16                         CharValue,
  IN  LIST_ENTRY                     *GlyphInfoList,
  IN  EFI_HII_GLYPH_INFO             *Cell
  )
{
  HII_GLYPH_INFO           *GlyphInfo;

  ASSERT (Cell != NULL && GlyphInfoList != NULL);

  GlyphInfo = (HII_GLYPH_INFO *) AllocateZeroPool (sizeof (HII_GLYPH_INFO));
  if (GlyphInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // GlyphInfoList stores a list of default character cell information, each is
  // identified by "CharId".
  //
  GlyphInfo->Signature = HII_GLYPH_INFO_SIGNATURE;
  GlyphInfo->CharId    = CharValue;
  CopyMem (&GlyphInfo->Cell, Cell, sizeof (EFI_HII_GLYPH_INFO));
  InsertTailList (GlyphInfoList, &GlyphInfo->Entry);

  return EFI_SUCCESS;
}


/**
  Get a character cell information from the list specified by GlyphInfoList.

  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphInfoList           HII_GLYPH_INFO list head.
  @param  Cell                    Buffer which stores output character cell
                                  information.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_NOT_FOUND           The character info specified by CharValue does
                                  not exist.

**/
STATIC
EFI_STATUS
GetCell (
  IN  CHAR16                         CharValue,
  IN  LIST_ENTRY                     *GlyphInfoList,
  OUT EFI_HII_GLYPH_INFO             *Cell
  )
{
  HII_GLYPH_INFO           *GlyphInfo;
  LIST_ENTRY               *Link;

  ASSERT (Cell != NULL && GlyphInfoList != NULL);

  //
  // Since the EFI_HII_GIBT_DEFAULTS block won't increment CharValueCurrent,
  // the value of "CharId" of a default character cell which is used for a
  // EFI_HII_GIBT_GLYPH_DEFAULT or EFI_HII_GIBT_GLYPHS_DEFAULT should be
  // less or equal to the value of "CharValueCurrent" of this default block.
  //
  // For instance, if the CharId of a GlyphInfoList is {1, 3, 7}, a default glyph
  // with CharValue equals "7" uses the GlyphInfo with CharId = 7;
  // a default glyph with CharValue equals "6" uses the GlyphInfo with CharId = 3.
  //
  for (Link = GlyphInfoList->BackLink; Link != GlyphInfoList; Link = Link->BackLink) {
    GlyphInfo = CR (Link, HII_GLYPH_INFO, Entry, HII_GLYPH_INFO_SIGNATURE);
    if (GlyphInfo->CharId <= CharValue) {
      CopyMem (Cell, &GlyphInfo->Cell, sizeof (EFI_HII_GLYPH_INFO));
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Convert the glyph for a single character into a bitmap.

  @param  Private                 HII database driver private data.
  @param  Char                    Character to retrieve.
  @param  StringInfo              Points to the string font and color information
                                  or NULL  if the string should use the default
                                  system font and color.
  @param  GlyphBuffer             Buffer to store the retrieved bitmap data.
  @param  Cell                    Points to EFI_HII_GLYPH_INFO structure.
  @param  Attributes              If not NULL, output the glyph attributes if any.

  @retval EFI_SUCCESS             Glyph bitmap outputted.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate the output buffer GlyphBuffer.
  @retval EFI_NOT_FOUND           The glyph was unknown can not be found.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.

**/
STATIC
EFI_STATUS
GetGlyphBuffer (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  IN  CHAR16                         Char,
  IN  EFI_FONT_INFO                  *StringInfo,
  OUT UINT8                          **GlyphBuffer,
  OUT EFI_HII_GLYPH_INFO             *Cell,
  OUT UINT8                          *Attributes OPTIONAL
  )
{
  HII_DATABASE_RECORD                *Node;
  LIST_ENTRY                         *Link;
  HII_SIMPLE_FONT_PACKAGE_INSTANCE   *SimpleFont;
  LIST_ENTRY                         *Link1;
  UINT16                             Index;
  EFI_NARROW_GLYPH                   Narrow;
  EFI_WIDE_GLYPH                     Wide;
  HII_GLOBAL_FONT_INFO               *GlobalFont;
  UINTN                              HeaderSize;
  EFI_NARROW_GLYPH                   *NarrowPtr;
  EFI_WIDE_GLYPH                     *WidePtr;

  if (GlyphBuffer == NULL || Cell == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Private == NULL || Private->Signature != HII_DATABASE_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Cell, sizeof (EFI_HII_GLYPH_INFO));

  //
  // If StringInfo is not NULL, it must point to an existing EFI_FONT_INFO rather
  // than system default font and color.
  // If NULL, try to find the character in simplified font packages since
  // default system font is the fixed font (narrow or wide glyph).
  //
  if (StringInfo != NULL) {
    if(!IsFontInfoExisted (Private, StringInfo, NULL, NULL, &GlobalFont)) {
      return EFI_INVALID_PARAMETER;
    }
    if (Attributes != NULL) {
      *Attributes = PROPORTIONAL_GLYPH;
    }
    return FindGlyphBlock (GlobalFont->FontPackage, Char, GlyphBuffer, Cell, NULL);
  } else {
    HeaderSize = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR);

    for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
      Node = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
      for (Link1 = Node->PackageList->SimpleFontPkgHdr.ForwardLink;
           Link1 != &Node->PackageList->SimpleFontPkgHdr;
           Link1 = Link1->ForwardLink
          ) {
        SimpleFont = CR (Link1, HII_SIMPLE_FONT_PACKAGE_INSTANCE, SimpleFontEntry, HII_S_FONT_PACKAGE_SIGNATURE);
        //
        // Search the narrow glyph array
        //
        NarrowPtr = (EFI_NARROW_GLYPH *) ((UINT8 *) (SimpleFont->SimpleFontPkgHdr) + HeaderSize);
        for (Index = 0; Index < SimpleFont->SimpleFontPkgHdr->NumberOfNarrowGlyphs; Index++) {
          CopyMem (&Narrow, NarrowPtr + Index,sizeof (EFI_NARROW_GLYPH));
          if (Narrow.UnicodeWeight == Char) {
            *GlyphBuffer = (UINT8 *) AllocateZeroPool (EFI_GLYPH_HEIGHT);
            if (*GlyphBuffer == NULL) {
              return EFI_OUT_OF_RESOURCES;
            }
            Cell->Width    = EFI_GLYPH_WIDTH;
            Cell->Height   = EFI_GLYPH_HEIGHT;
            Cell->OffsetY  = NARROW_BASELINE;
            Cell->AdvanceX = Cell->Width;
            CopyMem (*GlyphBuffer, Narrow.GlyphCol1, Cell->Height);
            if (Attributes != NULL) {
              *Attributes = (UINT8) (Narrow.Attributes | NARROW_GLYPH);
            }
            return EFI_SUCCESS;
          }
        }
        //
        // Search the wide glyph array
        //
        WidePtr = (EFI_WIDE_GLYPH *) (NarrowPtr + SimpleFont->SimpleFontPkgHdr->NumberOfNarrowGlyphs);
        for (Index = 0; Index < SimpleFont->SimpleFontPkgHdr->NumberOfWideGlyphs; Index++) {
          CopyMem (&Wide, WidePtr + Index, sizeof (EFI_WIDE_GLYPH));
          if (Wide.UnicodeWeight == Char) {
            *GlyphBuffer    = (UINT8 *) AllocateZeroPool (EFI_GLYPH_HEIGHT * 2);
            if (*GlyphBuffer == NULL) {
              return EFI_OUT_OF_RESOURCES;
            }
            Cell->Width    = EFI_GLYPH_WIDTH * 2;
            Cell->Height   = EFI_GLYPH_HEIGHT;
            Cell->OffsetY  = WIDE_BASELINE;
            Cell->AdvanceX = Cell->Width;
            CopyMem (*GlyphBuffer, Wide.GlyphCol1, EFI_GLYPH_HEIGHT);
            CopyMem (*GlyphBuffer + EFI_GLYPH_HEIGHT, Wide.GlyphCol2, EFI_GLYPH_HEIGHT);
            if (Attributes != NULL) {
              *Attributes = (UINT8) (Wide.Attributes | EFI_GLYPH_WIDE);
            }
            return EFI_SUCCESS;
          }
        }
      }
    }
  }

  return EFI_NOT_FOUND;
}

STATIC
VOID
NarrowGlyphToBlt (
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINTN                         ImageWidth,
  IN     UINTN                         ImageHeight,
  IN     BOOLEAN                       Transparent,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Origin
  )
{
  UINT8                                X;
  UINT8                                Y;
  UINT8                                Height;
  UINT8                                Width;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *Buffer;

  ASSERT (GlyphBuffer != NULL && Origin != NULL && *Origin != NULL);

  Height = EFI_GLYPH_HEIGHT;
  Width  = EFI_GLYPH_WIDTH;

  ASSERT (Width <= ImageWidth && Height <= ImageHeight);

  Buffer = *Origin;

  for (Y = 0; Y < Height; Y++) {
    for (X = 0; X < Width; X++) {
      if ((GlyphBuffer[Y] & (1 << X)) != 0) {
        Buffer[Y * ImageWidth + (Width - X - 1)] = Foreground;
      } else {
        if (!Transparent) {
          Buffer[Y * ImageWidth + (Width - X - 1)] = Background;
        }
      }
    }
  }

  *Origin = Buffer + Width;
}


/**
  Convert bitmap data of the glyph to blt structure.

  @param  GlyphBuffer             Buffer points to bitmap data of glyph.
  @param  Foreground              The color of the "on" pixels in the glyph in the
                                  bitmap.
  @param  Background              The color of the "off" pixels in the glyph in the
                                  bitmap.
  @param  Width                   Width of the character or character cell, in
                                  pixels.
  @param  Height                  Height of the character or character cell, in
                                  pixels.
  @param  Transparent             If TRUE, the Background color is ignored and all
                                  "off" pixels in the character's drawn wil use the
                                  pixel value from BltBuffer.
  @param  BltBuffer               Points to the blt buffer.


**/
STATIC
VOID
GlyphToBlt (
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINTN                         ImageWidth,
  IN     UINTN                         ImageHeight,
  IN     BOOLEAN                       Transparent,
  IN     EFI_HII_GLYPH_INFO            Cell,
  IN     UINT8                         Attributes,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Origin
  )
{
  UINT8                                X;
  UINT8                                Y;
  UINT8                                Data;
  UINT8                                Index;
  UINTN                                OffsetY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *BltBuffer;

  ASSERT (GlyphBuffer != NULL && Origin != NULL && *Origin != NULL);
  ASSERT (Cell.Width <= ImageWidth && Cell.Height <= ImageHeight);

  BltBuffer = *Origin;

  //
  // Since non-spacing key will be printed OR'd with the previous glyph, don't
  // write 0.
  //
  if ((Attributes & EFI_GLYPH_NON_SPACING) == EFI_GLYPH_NON_SPACING) {
    Transparent = TRUE;
  }

  //
  // The glyph's upper left hand corner pixel is the most significant bit of the
  // first bitmap byte.
  //
  for (Y = 0; Y < Cell.Height; Y++) {
    OffsetY = BITMAP_LEN_1_BIT (Cell.Width, Y);

    //
    // All bits in these bytes are meaningful.
    //
    for (X = 0; X < Cell.Width / 8; X++) {
      Data  = *(GlyphBuffer + OffsetY + X);
      for (Index = 0; Index < 8; Index++) {
        if ((Data & (1 << Index)) != 0) {
          BltBuffer[Y * ImageWidth + X * 8 + (8 - Index - 1)] = Foreground;
        } else {
          if (!Transparent) {
            BltBuffer[Y * ImageWidth + X * 8 + (8 - Index - 1)] = Background;
          }
        }
      }
    }

    if (Cell.Width % 8 != 0) {
      //
      // There are some padding bits in this byte. Ignore them.
      //
      Data  = *(GlyphBuffer + OffsetY + X);
      for (Index = 0; Index < Cell.Width % 8; Index++) {
        if ((Data & (1 << (8 - Index - 1))) != 0) {
          BltBuffer[Y * ImageWidth + X * 8 + Index] = Foreground;
        } else {
          if (!Transparent) {
            BltBuffer[Y * ImageWidth + X * 8 + Index] = Background;
          }
        }
      }
    } // end of if (Width % 8...)

  } // end of for (Y=0...)

  *Origin = BltBuffer + Cell.Width;
}


/**
  Convert bitmap data of the glyph to blt structure.

  @param  GlyphBuffer             Buffer points to bitmap data of glyph.
  @param  Foreground              The color of the "on" pixels in the glyph in the
                                  bitmap.
  @param  Background              The color of the "off" pixels in the glyph in the
                                  bitmap.
  @param  Width                   Width of the character or character cell, in
                                  pixels.
  @param  Height                  Height of the character or character cell, in
                                  pixels.
  @param  Transparent             If TRUE, the Background color is ignored and all
                                  "off" pixels in the character's drawn wil use the
                                  pixel value from BltBuffer.
  @param  Cell                    Points to EFI_HII_GLYPH_INFO structure.
  @param  Attributes              The attribute of incoming glyph in GlyphBuffer.
  @param  Origin                  On input, points to the origin of the to be
                                  displayed character, on output, points to the
                                  next glyph's origin.

  @return Points to the address of next origin node in BltBuffer.

**/
STATIC
VOID
GlyphToImage (
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINTN                         ImageWidth,
  IN     UINTN                         ImageHeight,
  IN     BOOLEAN                       Transparent,
  IN     EFI_HII_GLYPH_INFO            Cell,
  IN     UINT8                         Attributes,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Origin
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL        *Buffer;

  ASSERT (GlyphBuffer != NULL && Origin != NULL && *Origin != NULL);
  ASSERT (Cell.Width <= ImageWidth && Cell.Height <= ImageHeight);

  Buffer = *Origin;

  if ((Attributes & EFI_GLYPH_NON_SPACING) == EFI_GLYPH_NON_SPACING) {
    //
    // This character is a non-spacing key, print it OR'd with the previous glyph.
    // without advancing cursor.
    //
    Buffer -= Cell.Width;
    GlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      ImageHeight,
      Transparent,
      Cell,
      Attributes,
      &Buffer
      );

  } else if ((Attributes & EFI_GLYPH_WIDE) == EFI_GLYPH_WIDE) {
    //
    // This character is wide glyph, i.e. 16 pixels * 19 pixels.
    // Draw it as two narrow glyphs.
    //
    NarrowGlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      ImageHeight,
      Transparent,
      Origin
      );

    NarrowGlyphToBlt (
      GlyphBuffer + EFI_GLYPH_HEIGHT,
      Foreground,
      Background,
      ImageWidth,
      ImageHeight,
      Transparent,
      Origin
      );

  } else if ((Attributes & NARROW_GLYPH) == NARROW_GLYPH) {
    //
    // This character is narrow glyph, i.e. 8 pixels * 19 pixels.
    //
    NarrowGlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      ImageHeight,
      Transparent,
      Origin
      );

  } else if ((Attributes & PROPORTIONAL_GLYPH) == PROPORTIONAL_GLYPH) {
    //
    // This character is proportional glyph, i.e. Cell.Width * Cell.Height pixels.
    //
    GlyphToBlt (
      GlyphBuffer,
      Foreground,
      Background,
      ImageWidth,
      ImageHeight,
      Transparent,
      Cell,
      Attributes,
      Origin
      );
  }
}


/**
  Write the output parameters of FindGlyphBlock().

  @param  BufferIn                Buffer which stores the bitmap data of the found
                                  block.
  @param  BufferLen               Length of BufferIn.
  @param  InputCell               Buffer which stores cell information of the
                                  encoded bitmap.
  @param  GlyphBuffer             Output the corresponding bitmap data of the found
                                  block. It is the caller's responsiblity to free
                                  this buffer.
  @param  Cell                    Output cell information of the encoded bitmap.
  @param  GlyphBufferLen          If not NULL, output the length of GlyphBuffer.

  @retval EFI_SUCCESS             The operation is performed successfully.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
STATIC
EFI_STATUS
WriteOutputParam (
  IN  UINT8                          *BufferIn,
  IN  UINTN                          BufferLen,
  IN  EFI_HII_GLYPH_INFO             *InputCell,
  OUT UINT8                          **GlyphBuffer, OPTIONAL
  OUT EFI_HII_GLYPH_INFO             *Cell, OPTIONAL
  OUT UINTN                          *GlyphBufferLen OPTIONAL
  )
{
  if (BufferIn == NULL || BufferLen < 1 || InputCell == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Cell != NULL) {
    CopyMem (Cell, InputCell, sizeof (EFI_HII_GLYPH_INFO));
  }

  if (GlyphBuffer != NULL) {
    *GlyphBuffer = (UINT8 *) AllocateZeroPool (BufferLen);
    if (*GlyphBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (*GlyphBuffer, BufferIn, BufferLen);
  }

  if (GlyphBufferLen != NULL) {
    *GlyphBufferLen = BufferLen;
  }

  return EFI_SUCCESS;
}


/**
  Parse all glyph blocks to find a glyph block specified by CharValue.
  If CharValue = (CHAR16) (-1), collect all default character cell information
  within this font package and backup its information.

  @param  FontPackage             Hii string package instance.
  @param  CharValue               Unicode character value, which identifies a glyph
                                  block.
  @param  GlyphBuffer             Output the corresponding bitmap data of the found
                                  block. It is the caller's responsiblity to free
                                  this buffer.
  @param  Cell                    Output cell information of the encoded bitmap.
  @param  GlyphBufferLen          If not NULL, output the length of GlyphBuffer.

  @retval EFI_SUCCESS             The bitmap data is retrieved successfully.
  @retval EFI_NOT_FOUND           The specified CharValue does not exist in current
                                  database.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
EFI_STATUS
FindGlyphBlock (
  IN  HII_FONT_PACKAGE_INSTANCE      *FontPackage,
  IN  CHAR16                         CharValue,
  OUT UINT8                          **GlyphBuffer, OPTIONAL
  OUT EFI_HII_GLYPH_INFO             *Cell, OPTIONAL
  OUT UINTN                          *GlyphBufferLen OPTIONAL
  )
{
  EFI_STATUS                          Status;
  UINT8                               *BlockPtr;
  UINT16                              CharCurrent;
  UINT16                              Length16;
  UINT32                              Length32;
  EFI_HII_GIBT_GLYPHS_BLOCK           Glyphs;
  UINTN                               BufferLen;
  UINT16                              Index;
  EFI_HII_GLYPH_INFO                  DefaultCell;
  EFI_HII_GLYPH_INFO                  LocalCell;

  ASSERT (FontPackage != NULL);
  ASSERT (FontPackage->Signature == HII_FONT_PACKAGE_SIGNATURE);

  if (CharValue == (CHAR16) (-1)) {
    //
    // Collect the cell information specified in font package fixed header.
    // Use CharValue =0 to represent this particular cell.
    //
    Status = NewCell (
               0,
               &FontPackage->GlyphInfoList,
               (EFI_HII_GLYPH_INFO *) ((UINT8 *) FontPackage->FontPkgHdr + 3 * sizeof (UINT32))
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  BlockPtr    = FontPackage->GlyphBlock;
  CharCurrent = 1;
  BufferLen   = 0;

  while (*BlockPtr != EFI_HII_GIBT_END) {
    switch (*BlockPtr) {
    case EFI_HII_GIBT_DEFAULTS:
      //
      // Collect all default character cell information specified by
      // EFI_HII_GIBT_DEFAULTS.
      //
      if (CharValue == (CHAR16) (-1)) {
        Status = NewCell (
                   CharCurrent,
                   &FontPackage->GlyphInfoList,
                   (EFI_HII_GLYPH_INFO *) (BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK))
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
      BlockPtr += sizeof (EFI_HII_GIBT_DEFAULTS_BLOCK);
      break;

    case EFI_HII_GIBT_DUPLICATE:
      if (CharCurrent == CharValue) {
        CopyMem (&CharValue, BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK), sizeof (CHAR16));
        CharCurrent = 1;
        BlockPtr    = FontPackage->GlyphBlock;
        continue;
      }
      CharCurrent++;
      BlockPtr += sizeof (EFI_HII_GIBT_DUPLICATE_BLOCK);
      break;

    case EFI_HII_GIBT_EXT1:
      BlockPtr += *(BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK) + sizeof (UINT8));
      break;
    case EFI_HII_GIBT_EXT2:
      CopyMem (
        &Length16,
        BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK) + sizeof (UINT8),
        sizeof (UINT16)
        );
      BlockPtr += Length16;
      break;
    case EFI_HII_GIBT_EXT4:
      CopyMem (
        &Length32,
        BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK) + sizeof (UINT8),
        sizeof (UINT32)
        );
      BlockPtr += Length32;
      break;

    case EFI_HII_GIBT_GLYPH:
      CopyMem (
        &LocalCell,
        BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK),
        sizeof (EFI_HII_GLYPH_INFO)
        );
      BufferLen = BITMAP_LEN_1_BIT (LocalCell.Width, LocalCell.Height);
      if (CharCurrent == CharValue) {
        return WriteOutputParam (
                 BlockPtr + sizeof (EFI_HII_GIBT_GLYPH_BLOCK) - sizeof (UINT8),
                 BufferLen,
                 &LocalCell,
                 GlyphBuffer,
                 Cell,
                 GlyphBufferLen
                 );
      }
      CharCurrent++;
      BlockPtr += sizeof (EFI_HII_GIBT_GLYPH_BLOCK) - sizeof (UINT8) + BufferLen;
      break;

    case EFI_HII_GIBT_GLYPHS:
      BlockPtr += sizeof (EFI_HII_GLYPH_BLOCK);
      CopyMem (&Glyphs.Cell, BlockPtr, sizeof (EFI_HII_GLYPH_INFO));
      BlockPtr += sizeof (EFI_HII_GLYPH_INFO);
      CopyMem (&Glyphs.Count, BlockPtr, sizeof (UINT16));
      BlockPtr += sizeof (UINT16);

      BufferLen = BITMAP_LEN_1_BIT (Glyphs.Cell.Width, Glyphs.Cell.Height);
      for (Index = 0; Index < Glyphs.Count; Index++) {
        if (CharCurrent + Index == CharValue) {
          return WriteOutputParam (
                   BlockPtr,
                   BufferLen,
                   &Glyphs.Cell,
                   GlyphBuffer,
                   Cell,
                   GlyphBufferLen
                   );
        }
        BlockPtr += BufferLen;
      }
      CharCurrent = (UINT16) (CharCurrent + Glyphs.Count);
      break;

    case EFI_HII_GIBT_GLYPH_DEFAULT:
      Status = GetCell (CharCurrent, &FontPackage->GlyphInfoList, &DefaultCell);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      BufferLen = BITMAP_LEN_1_BIT (DefaultCell.Width, DefaultCell.Height);

      if (CharCurrent == CharValue) {
        return WriteOutputParam (
                 BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK),
                 BufferLen,
                 &DefaultCell,
                 GlyphBuffer,
                 Cell,
                 GlyphBufferLen
                 );
      }
      CharCurrent++;
      BlockPtr += sizeof (EFI_HII_GLYPH_BLOCK) + BufferLen;
      break;

    case EFI_HII_GIBT_GLYPHS_DEFAULT:
      CopyMem (&Length16, BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK), sizeof (UINT16));
      Status = GetCell (CharCurrent, &FontPackage->GlyphInfoList, &DefaultCell);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      BufferLen = BITMAP_LEN_1_BIT (DefaultCell.Width, DefaultCell.Height);
      BlockPtr += sizeof (EFI_HII_GIBT_GLYPHS_DEFAULT_BLOCK) - sizeof (UINT8);
      for (Index = 0; Index < Length16; Index++) {
        if (CharCurrent + Index == CharValue) {
          return WriteOutputParam (
                   BlockPtr,
                   BufferLen,
                   &DefaultCell,
                   GlyphBuffer,
                   Cell,
                   GlyphBufferLen
                   );
        }
        BlockPtr += BufferLen;
      }
      CharCurrent = (UINT16) (CharCurrent + Length16);
      break;

    case EFI_HII_GIBT_SKIP1:
      CharCurrent = (UINT16) (CharCurrent + (UINT16) (*(BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK))));
      BlockPtr    += sizeof (EFI_HII_GIBT_SKIP1_BLOCK);
      break;
    case EFI_HII_GIBT_SKIP2:
      CopyMem (&Length16, BlockPtr + sizeof (EFI_HII_GLYPH_BLOCK), sizeof (UINT16));
      CharCurrent = (UINT16) (CharCurrent + Length16);
      BlockPtr    += sizeof (EFI_HII_GIBT_SKIP2_BLOCK);
      break;
    default:
      ASSERT (FALSE);
      break;
    }

    if (CharValue < CharCurrent) {
      return EFI_NOT_FOUND;
    }
  }

  if (CharValue == (CHAR16) (-1)) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


/**
  Copy a Font Name to a new created EFI_FONT_INFO structure.

  @param  FontName                NULL-terminated string.
  @param  FontInfo                a new EFI_FONT_INFO which stores the FontName.
                                  It's caller's responsibility to free this buffer.

  @retval EFI_SUCCESS             FontInfo is allocated and copied with FontName.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.

**/
STATIC
EFI_STATUS
SaveFontName (
  IN  EFI_STRING                       FontName,
  OUT EFI_FONT_INFO                    **FontInfo
  )
{
  UINTN         FontInfoLen;

  ASSERT (FontName != NULL && FontInfo != NULL);

  FontInfoLen = sizeof (EFI_FONT_INFO) - sizeof (CHAR16) + StrSize (FontName);
  *FontInfo = (EFI_FONT_INFO *) AllocateZeroPool (FontInfoLen);
  if (*FontInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrCpy ((*FontInfo)->FontName, FontName);
  return EFI_SUCCESS;
}


/**
  Retrieve system default font and color.

  @param  Private                 HII database driver private data.
  @param  FontInfo                Points to system default font output-related
                                  information. It's caller's responsibility to free
                                  this buffer.
  @param  FontInfoSize            If not NULL, output the size of buffer FontInfo.

  @retval EFI_SUCCESS             Cell information is added to the GlyphInfoList.
  @retval EFI_OUT_OF_RESOURCES    The system is out of resources to accomplish the
                                  task.
  @retval EFI_INVALID_PARAMETER   Any input parameter is invalid.

**/
EFI_STATUS
GetSystemFont (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  OUT EFI_FONT_DISPLAY_INFO          **FontInfo,
  OUT UINTN                          *FontInfoSize OPTIONAL
  )
{
  EFI_FONT_DISPLAY_INFO              *Info;
  UINTN                              InfoSize;

  if (Private == NULL || Private->Signature != HII_DATABASE_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
  if (FontInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The standard font always has the name "sysdefault".
  //
  InfoSize = sizeof (EFI_FONT_DISPLAY_INFO) - sizeof (CHAR16) + StrSize (L"sysdefault");
  Info = (EFI_FONT_DISPLAY_INFO *) AllocateZeroPool (InfoSize);
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Info->ForegroundColor    = mEfiColors[Private->Attribute & 0x0f];
  Info->BackgroundColor    = mEfiColors[Private->Attribute >> 4];
  Info->FontInfoMask       = EFI_FONT_INFO_SYS_FONT | EFI_FONT_INFO_SYS_SIZE | EFI_FONT_INFO_SYS_STYLE;
  Info->FontInfo.FontStyle = 0;
  Info->FontInfo.FontSize  = EFI_GLYPH_HEIGHT;
  StrCpy (Info->FontInfo.FontName, L"sysdefault");

  *FontInfo = Info;
  if (FontInfoSize != NULL) {
    *FontInfoSize = InfoSize;
  }
  return EFI_SUCCESS;
}


/**
  Check whether EFI_FONT_DISPLAY_INFO points to system default font and color.

  @param  Private                 HII database driver private data.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.
  @param  SystemInfo              If not NULL, points to system default font and
                                  color when incoming StringInfo does not match the
                                  default.  Points to NULL if matches. It's
                                  caller's reponsibility to free this buffer.
  @param  SystemInfoLen           If not NULL, output the length of default system
                                  info.

  @retval TRUE                    Yes, it points to system default.
  @retval FALSE                   No.

**/
STATIC
BOOLEAN
IsSystemFontInfo (
  IN  HII_DATABASE_PRIVATE_DATA      *Private,
  IN  EFI_FONT_DISPLAY_INFO          *StringInfo,
  OUT EFI_FONT_DISPLAY_INFO          **SystemInfo, OPTIONAL
  OUT UINTN                          *SystemInfoLen OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_FONT_DISPLAY_INFO               *SystemDefault;
  UINTN                               DefaultLen;

  ASSERT (Private != NULL && Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);

  if (StringInfo == NULL && SystemInfo == NULL) {
    return TRUE;
  }

  //
  // Check whether incoming string font and color matches system default.
  //
  Status = GetSystemFont (Private, &SystemDefault, &DefaultLen);
  ASSERT_EFI_ERROR (Status);

  if (SystemInfo != NULL) {
    *SystemInfo = SystemDefault;
  } else {
    SafeFreePool (SystemDefault);
  }

  if (SystemInfoLen != NULL) {
    *SystemInfoLen = DefaultLen;
  }

  if (StringInfo == NULL ||
      (StringInfo != NULL && CompareMem (SystemDefault, StringInfo, DefaultLen) == 0)) {
    return TRUE;
  }

  return FALSE;
}


/**
  This function checks whether EFI_FONT_INFO exists in current database. If
  FontInfoMask is specified, check what options can be used to make a match.
  Note that the masks relate to where the system default should be supplied
  are ignored by this function.

  @param  Private                 Hii database private structure.
  @param  FontInfo                Points to EFI_FONT_INFO structure.
  @param  FontInfoMask            If not NULL, describes what options can be used
                                  to make a match between the font requested and
                                  the font available. The caller must guarantee
                                  this mask is valid.
  @param  FontHandle              On entry, Points to the font handle returned by a
                                  previous  call to GetFontInfo() or NULL to start
                                  with the first font.
  @param  GlobalFontInfo          If not NULL, output the corresponding globa font
                                  info.

  @retval TRUE                    Existed
  @retval FALSE                   Not existed

**/
BOOLEAN
IsFontInfoExisted (
  IN  HII_DATABASE_PRIVATE_DATA *Private,
  IN  EFI_FONT_INFO             *FontInfo,
  IN  EFI_FONT_INFO_MASK        *FontInfoMask,   OPTIONAL
  IN  EFI_FONT_HANDLE           FontHandle,      OPTIONAL
  OUT HII_GLOBAL_FONT_INFO      **GlobalFontInfo OPTIONAL
  )
{
  HII_GLOBAL_FONT_INFO          *GlobalFont;
  HII_GLOBAL_FONT_INFO          *GlobalFontBackup1;
  HII_GLOBAL_FONT_INFO          *GlobalFontBackup2;
  LIST_ENTRY                    *Link;
  EFI_FONT_INFO_MASK            Mask;
  BOOLEAN                       Matched;
  BOOLEAN                       VagueMatched1;
  BOOLEAN                       VagueMatched2;

  ASSERT (Private != NULL && Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);
  ASSERT (FontInfo != NULL);

  //
  // Matched flag represents an exactly match; VagueMatched1 repensents a RESIZE
  // or RESTYLE match; VagueMatched2 represents a RESIZE | RESTYLE match.
  //
  Matched           = FALSE;
  VagueMatched1     = FALSE;
  VagueMatched2     = FALSE;

  Mask              = 0;
  GlobalFontBackup1 = NULL;
  GlobalFontBackup2 = NULL;

  // The process of where the system default should be supplied instead of
  // the specified font info beyonds this function's scope.
  //
  if (FontInfoMask != NULL) {
    Mask = *FontInfoMask & (~SYS_FONT_INFO_MASK);
  }

  //
  // If not NULL, FontHandle points to the next node of the last searched font
  // node by previous call.
  //
  if (FontHandle == NULL) {
    Link = Private->FontInfoList.ForwardLink;
  } else {
    Link = (LIST_ENTRY     *) FontHandle;
  }

  for (; Link != &Private->FontInfoList; Link = Link->ForwardLink) {
    GlobalFont = CR (Link, HII_GLOBAL_FONT_INFO, Entry, HII_GLOBAL_FONT_INFO_SIGNATURE);
    if (FontInfoMask == NULL) {
      if (CompareMem (GlobalFont->FontInfo, FontInfo, GlobalFont->FontInfoSize) == 0) {
        if (GlobalFontInfo != NULL) {
          *GlobalFontInfo = GlobalFont;
        }
        return TRUE;
      }
    } else {
      //
      // Check which options could be used to make a match.
      //
      switch (Mask) {
      case EFI_FONT_INFO_ANY_FONT:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle &&
            GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_STYLE:
        if (GlobalFont->FontInfo->FontSize == FontInfo->FontSize) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_SIZE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_SIZE | EFI_FONT_INFO_ANY_STYLE:
        Matched   = TRUE;
        break;
      //
      // If EFI_FONT_INFO_RESTYLE is specified, then the system may attempt to
      // remove some of the specified styles to meet the style requested.
      //
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_RESTYLE:
        if (GlobalFont->FontInfo->FontSize == FontInfo->FontSize) {
          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            Matched           = TRUE;
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      //
      // If EFI_FONT_INFO_RESIZE is specified, then the sytem may attempt to
      // stretch or shrink a font to meet the size requested.
      //
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_RESIZE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_RESTYLE | EFI_FONT_INFO_RESIZE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          } else {
            VagueMatched2     = TRUE;
            GlobalFontBackup2 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_STYLE | EFI_FONT_INFO_RESIZE:
        if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
          Matched           = TRUE;
        } else {
          VagueMatched1     = TRUE;
          GlobalFontBackup1 = GlobalFont;
        }
        break;
      case EFI_FONT_INFO_ANY_FONT | EFI_FONT_INFO_ANY_SIZE | EFI_FONT_INFO_RESTYLE:
        if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
          Matched           = TRUE;
        } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
          VagueMatched1     = TRUE;
          GlobalFontBackup1 = GlobalFont;
        }
        break;
      case EFI_FONT_INFO_ANY_STYLE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_STYLE | EFI_FONT_INFO_ANY_SIZE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_STYLE | EFI_FONT_INFO_RESIZE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_ANY_SIZE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontStyle  == FontInfo->FontStyle) {
          Matched = TRUE;
        }
        break;
      case EFI_FONT_INFO_ANY_SIZE | EFI_FONT_INFO_RESTYLE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            Matched           = TRUE;
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_RESTYLE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {

          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            Matched           = TRUE;
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_RESIZE:
        if ((CompareMem (
               GlobalFont->FontInfo->FontName,
               FontInfo->FontName,
               StrSize (FontInfo->FontName)
               ) == 0) &&
            GlobalFont->FontInfo->FontStyle  == FontInfo->FontStyle) {

          if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
            Matched           = TRUE;
          } else {
            VagueMatched1     = TRUE;
            GlobalFontBackup1 = GlobalFont;
          }
        }
        break;
      case EFI_FONT_INFO_RESIZE | EFI_FONT_INFO_RESTYLE:
        if (CompareMem (
              GlobalFont->FontInfo->FontName,
              FontInfo->FontName,
              StrSize (FontInfo->FontName)
              ) == 0) {
          if (GlobalFont->FontInfo->FontStyle == FontInfo->FontStyle) {
            if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
              Matched           = TRUE;
            } else {
              VagueMatched1     = TRUE;
              GlobalFontBackup1 = GlobalFont;
            }
          } else if ((GlobalFont->FontInfo->FontStyle & FontInfo->FontStyle) == FontInfo->FontStyle) {
            if (GlobalFont->FontInfo->FontSize  == FontInfo->FontSize) {
              VagueMatched1     = TRUE;
              GlobalFontBackup1 = GlobalFont;
            } else {
              VagueMatched2     = TRUE;
              GlobalFontBackup2 = GlobalFont;
            }
          }
        }
        break;
      default:
        break;
      }

      if (Matched) {
        if (GlobalFontInfo != NULL) {
          *GlobalFontInfo = GlobalFont;
        }
        return TRUE;
      }
    }
  }

  if (VagueMatched1) {
    if (GlobalFontInfo != NULL) {
      *GlobalFontInfo = GlobalFontBackup1;
    }
    return TRUE;
  } else if (VagueMatched2) {
    if (GlobalFontInfo != NULL) {
      *GlobalFontInfo = GlobalFontBackup2;
    }
    return TRUE;
  }

  return FALSE;
}


/**
  Check whether the unicode represents a line break or not.

  @param  Char                    Unicode character

  @retval 0                       Yes, it is a line break.
  @retval 1                       Yes, it is a hyphen that desires a line break
                                  after this character.
  @retval 2                       Yes, it is a dash that desires a line break
                                  before and after it.
  @retval -1                      No, it is not a link break.

**/
STATIC
INT8
IsLineBreak (
  IN  CHAR16    Char
  )
{
  UINT8         Byte1;
  UINT8         Byte2;

  //
  // In little endian, Byte1 is the low byte of Char, Byte2 is the high byte of Char.
  //
  Byte1 = *((UINT8 *) (&Char));
  Byte2 = *(((UINT8 *) (&Char) + 1));

  if (Byte2 == 0x20) {
    switch (Byte1) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x28:
    case 0x29:
    case 0x5F:
      return 0;
    case 0x10:
    case 0x12:
    case 0x13:
      return 1;
    case 0x14:
      //
      // BUGBUG: Does it really require line break before it and after it?
      //
      return 2;
    }
  } else if (Byte2 == 0x00) {
    switch (Byte1) {
    case 0x20:
    case 0x0C:
    case 0x0D:
      return 0;
    }
  }

  switch (Char) {
    case 0x1680:
      return 0;
    case 0x058A:
    case 0x0F0B:
    case 0x1361:
    case 0x17D5:
      return 1;
  }

  return -1;
}


/**
  Renders a string to a bitmap or to the display.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Flags                   Describes how the string is to be drawn.
  @param  String                  Points to the null-terminated string to be
                                  displayed.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.  If NULL, then the
                                  string will be output in the default system font
                                  and color.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels   wide
                                  and Height pixels high. The string will be drawn
                                  onto this image and
                                  EFI_HII_OUT_FLAG_CLIP is implied. If this points
                                  to a NULL on entry, then a              buffer
                                  will be allocated to hold the generated image and
                                  the pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX,BLTY               Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  RowInfoArray            If this is non-NULL on entry, then on exit, this
                                  will point to an allocated buffer    containing
                                  row information and RowInfoArraySize will be
                                  updated to contain the        number of elements.
                                  This array describes the characters which were at
                                  least partially drawn and the heights of the
                                  rows. It is the caller's responsibility to free
                                  this buffer.
  @param  RowInfoArraySize        If this is non-NULL on entry, then on exit it
                                  contains the number of elements in RowInfoArray.
  @param  ColumnInfoArray         If this is non-NULL, then on return it will be
                                  filled with the horizontal offset for each
                                  character in the string on the row where it is
                                  displayed. Non-printing characters will     have
                                  the offset ~0. The caller is responsible to
                                  allocate a buffer large enough so that    there
                                  is one entry for each character in the string,
                                  not including the null-terminator. It is possible
                                  when character display is normalized that some
                                  character cells overlap.

  @retval EFI_SUCCESS             The string was successfully rendered.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for
                                  RowInfoArray or Blt.
  @retval EFI_INVALID_PARAMETER   The String or Blt was NULL.

**/
EFI_STATUS
EFIAPI
HiiStringToImage (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  CONST EFI_STRING               String,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  UINT8                               **GlyphBuf;
  EFI_HII_GLYPH_INFO                  *Cell;
  UINT8                               *Attributes;
  EFI_IMAGE_OUTPUT                    *Image;
  EFI_STRING                          StringPtr;
  EFI_STRING                          StringTmp;
  EFI_HII_ROW_INFO                    *RowInfo;
  UINTN                               LineWidth;
  UINTN                               LineHeight;
  UINTN                               BaseLineOffset;
  UINT16                              MaxRowNum;
  UINT16                              RowIndex;
  UINTN                               Index;
  UINTN                               Index1;
  EFI_FONT_DISPLAY_INFO               *StringInfoOut;
  EFI_FONT_DISPLAY_INFO               *SystemDefault;
  EFI_FONT_HANDLE                     FontHandle;
  EFI_STRING                          StringIn;
  EFI_STRING                          StringIn2;
  UINT16                              Height;
  EFI_FONT_INFO                       *FontInfo;
  BOOLEAN                             SysFontFlag;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       Background;
  BOOLEAN                             Transparent;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BufferPtr;
  UINTN                               RowInfoSize;
  BOOLEAN                             LineBreak;

  //
  // Check incoming parameters.
  //

  if (This == NULL || String == NULL || Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (*Blt == NULL) {
    //
    // These two flag cannot be used if Blt is NULL upon entry.
    //
    if ((Flags & EFI_HII_OUT_FLAG_TRANSPARENT) == EFI_HII_OUT_FLAG_TRANSPARENT) {
      return EFI_INVALID_PARAMETER;
    }
    if ((Flags & EFI_HII_OUT_FLAG_CLIP) == EFI_HII_OUT_FLAG_CLIP) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // These two flags require that EFI_HII_OUT_FLAG_CLIP be also set.
  //
  if ((Flags & (EFI_HII_OUT_FLAG_CLIP | EFI_HII_OUT_FLAG_CLEAN_X)) ==  EFI_HII_OUT_FLAG_CLEAN_X) {
    return EFI_INVALID_PARAMETER;
  }
  if ((Flags & (EFI_HII_OUT_FLAG_CLIP | EFI_HII_OUT_FLAG_CLEAN_Y)) ==  EFI_HII_OUT_FLAG_CLEAN_Y) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // This flag cannot be used with EFI_HII_OUT_FLAG_CLEAN_X.
  //
  if ((Flags & (EFI_HII_OUT_FLAG_WRAP | EFI_HII_OUT_FLAG_CLEAN_X)) ==  (EFI_HII_OUT_FLAG_WRAP | EFI_HII_OUT_FLAG_CLEAN_X)) {
    return EFI_INVALID_PARAMETER;
  }

  GlyphBuf = (UINT8 **) AllocateZeroPool (MAX_STRING_LENGTH * sizeof (UINT8 *));
  ASSERT (GlyphBuf != NULL);
  Cell = (EFI_HII_GLYPH_INFO *) AllocateZeroPool (MAX_STRING_LENGTH * sizeof (EFI_HII_GLYPH_INFO));
  ASSERT (Cell != NULL);
  Attributes = (UINT8 *) AllocateZeroPool (MAX_STRING_LENGTH * sizeof (UINT8));
  ASSERT (Attributes != NULL);

  RowInfo       = NULL;
  Status        = EFI_SUCCESS;
  StringIn2     = NULL;
  SystemDefault = NULL;

  //
  // Calculate the string output information, including specified color and font .
  // If StringInfo does not points to system font info, it must indicate an existing
  // EFI_FONT_INFO.
  //
  StringInfoOut = NULL;
  FontHandle    = NULL;
  Private       = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  SysFontFlag   = IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfo, &SystemDefault, NULL);

  if (SysFontFlag) {
    FontInfo   = NULL;
    Height     = SystemDefault->FontInfo.FontSize;
    Foreground = SystemDefault->ForegroundColor;
    Background = SystemDefault->BackgroundColor;

  } else {
    Status = HiiGetFontInfo (This, &FontHandle, (EFI_FONT_DISPLAY_INFO *) StringInfo, &StringInfoOut, NULL);
    if (Status == EFI_NOT_FOUND) {
      //
      // The specified EFI_FONT_DISPLAY_INFO does not exist in current database.
      // Use the system font instead. Still use the color specified by StringInfo.
      //
      SysFontFlag = TRUE;
      FontInfo    = NULL;
      Height      = SystemDefault->FontInfo.FontSize;
      Foreground  = ((EFI_FONT_DISPLAY_INFO *) StringInfo)->ForegroundColor;
      Background  = ((EFI_FONT_DISPLAY_INFO *) StringInfo)->BackgroundColor;

    } else {
      FontInfo   = &StringInfoOut->FontInfo;
      Height     = StringInfoOut->FontInfo.FontSize;
      Foreground = StringInfoOut->ForegroundColor;
      Background = StringInfoOut->BackgroundColor;
    }
  }

  //
  // Parse the string to be displayed to drop some ignored characters.
  //

  StringPtr = String;
  StringIn  = NULL;

  //
  // Ignore line-break characters only. Hyphens or dash character will be displayed
  // without line-break opportunity.
  //
  if ((Flags & EFI_HII_IGNORE_LINE_BREAK) == EFI_HII_IGNORE_LINE_BREAK) {
    StringIn = AllocateZeroPool (StrSize (StringPtr));
    if (StringIn == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    StringTmp = StringIn;
    while (*StringPtr != 0) {
      if (IsLineBreak (*StringPtr) == 0) {
        StringPtr++;
      } else {
        *StringTmp++ = *StringPtr++;
      }
    }
    *StringTmp = 0;
    StringPtr  = StringIn;
  }
  //
  // If EFI_HII_IGNORE_IF_NO_GLYPH is set, then characters which have no glyphs
  // are not drawn. Otherwise they are replaced wth Unicode character 0xFFFD.
  //
  StringIn2  = AllocateZeroPool (StrSize (StringPtr));
  if (StringIn2 == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  Index     = 0;
  StringTmp = StringIn2;

  while (*StringPtr != 0 && Index < MAX_STRING_LENGTH) {
    Status = GetGlyphBuffer (Private, *StringPtr, FontInfo, &GlyphBuf[Index], &Cell[Index], &Attributes[Index]);
    if (Status == EFI_NOT_FOUND) {
      if ((Flags & EFI_HII_IGNORE_IF_NO_GLYPH) == EFI_HII_IGNORE_IF_NO_GLYPH) {
        SafeFreePool (GlyphBuf[Index]);
        GlyphBuf[Index] = NULL;
        StringPtr++;
      } else {
        //
        // Unicode 0xFFFD must exist in current hii database if this flag is not set.
        //
        Status = GetGlyphBuffer (
                   Private,
                   REPLACE_UNKNOWN_GLYPH,
                   FontInfo,
                   &GlyphBuf[Index],
                   &Cell[Index],
                   &Attributes[Index]
                   );
        if (EFI_ERROR (Status)) {
          Status = EFI_INVALID_PARAMETER;
          goto Exit;
        }
        *StringTmp++ = *StringPtr++;
        Index++;
      }
    } else if (EFI_ERROR (Status)) {
      goto Exit;
    } else {
      *StringTmp++ = *StringPtr++;
      Index++;
    }
  }
  *StringTmp = 0;
  StringPtr  = StringIn2;

  //
  // Draw the string according to the specified EFI_HII_OUT_FLAGS and Blt.
  // If Blt is not NULL, then EFI_HII_OUT_FLAG_CLIP is implied, render this string
  // to an existing image (bitmap or screen depending on flags) pointed by "*Blt".
  // Otherwise render this string to a new allocated image and output it.
  //
  if (*Blt != NULL) {
    Image     = *Blt;
    BufferPtr = Image->Image.Bitmap + Image->Width * BltY + BltX;
    MaxRowNum = (UINT16) (Image->Height / Height);
    if (Image->Height % Height != 0) {
      MaxRowNum++;
    }

    RowInfo = (EFI_HII_ROW_INFO *) AllocateZeroPool (MaxRowNum * sizeof (EFI_HII_ROW_INFO));
    if (RowInfo == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    //
    // Format the glyph buffer according to flags.
    //

    Transparent = (BOOLEAN) ((Flags & EFI_HII_OUT_FLAG_TRANSPARENT) == EFI_HII_OUT_FLAG_TRANSPARENT ? TRUE : FALSE);
    if ((Flags & EFI_HII_OUT_FLAG_CLEAN_Y) == EFI_HII_OUT_FLAG_CLEAN_Y) {
      //
      // Don't draw at all if there is only one row and
      // the row's bottom-most on pixel cannot fit.
      //
      if (MaxRowNum == 1 && SysFontFlag) {
        Status = EFI_SUCCESS;
        goto Exit;
      }
    }

    for (RowIndex = 0, Index = 0; RowIndex < MaxRowNum && StringPtr[Index] != 0; ) {
      LineWidth      = 0;
      LineHeight     = 0;
      BaseLineOffset = 0;
      LineBreak      = FALSE;

      //
      // Calculate how many characters there are in a row.
      //
      RowInfo[RowIndex].StartIndex = Index;

      while (LineWidth + BltX < Image->Width && StringPtr[Index] != 0) {
        LineWidth += (UINTN) Cell[Index].AdvanceX;
        if (LineHeight < Cell[Index].Height) {
          LineHeight = (UINTN) Cell[Index].Height;
        }
        BaseLineOffset += (UINTN) Cell[Index].OffsetY;

        if ((Flags & EFI_HII_IGNORE_LINE_BREAK) == 0 &&
            (Flags & EFI_HII_OUT_FLAG_WRAP) == 0 &&
             IsLineBreak (StringPtr[Index]) > 0) {
          //
          // It is a line break that ends this row.
          //
          Index++;
          break;
        }

        Index++;
      }

      //
      // If this character is the last character of a row, we need not
      // draw its (AdvanceX - Width) for next character.
      //
      Index--;
      if (!SysFontFlag) {
        LineWidth -= (UINTN) (Cell[Index].AdvanceX - Cell[Index].Width);
      }

      //
      // EFI_HII_OUT_FLAG_WRAP will wrap the text at the right-most line-break
      // opportunity prior to a character whose right-most extent would exceed Width.
      // Search the right-most line-break opportunity here.
      //
      if ((Flags & EFI_HII_OUT_FLAG_WRAP) == EFI_HII_OUT_FLAG_WRAP) {
        if ((Flags & EFI_HII_IGNORE_LINE_BREAK) == 0) {
          for (Index1 = RowInfo[RowIndex].EndIndex; Index1 >= RowInfo[RowIndex].StartIndex; Index1--) {
            if (IsLineBreak (StringPtr[Index1]) > 0) {
              LineBreak = TRUE;
              RowInfo[RowIndex].EndIndex = Index1 - 1;
              break;
            }
          }
        }
        //
        // If no line-break opportunity can be found, then the text will
        // behave as if EFI_HII_OUT_FLAG_CLEAN_X is set.
        //
        if (!LineBreak) {
          Flags &= (~ (EFI_HII_OUT_FLAGS) EFI_HII_OUT_FLAG_WRAP);
          Flags |= EFI_HII_OUT_FLAG_CLEAN_X;
        }
      }

      //
      // Clip the right-most character if cannot fit when EFI_HII_OUT_FLAG_CLEAN_X is set.
      //
      if (LineWidth + BltX <= Image->Width ||
          (LineWidth + BltX > Image->Width && (Flags & EFI_HII_OUT_FLAG_CLEAN_X) == 0)) {
        //
        // Record right-most character in RowInfo even if it is partially displayed.
        //
        RowInfo[RowIndex].EndIndex       = Index;
        RowInfo[RowIndex].LineWidth      = LineWidth;
        RowInfo[RowIndex].LineHeight     = LineHeight;
        RowInfo[RowIndex].BaselineOffset = BaseLineOffset;
      } else {
        //
        // When EFI_HII_OUT_FLAG_CLEAN_X is set, it will not draw a character
        // if its right-most on pixel cannot fit.
        //
        if (Index > 0) {
          RowInfo[RowIndex].EndIndex       = Index - 1;
          RowInfo[RowIndex].LineWidth      = LineWidth - Cell[Index].AdvanceX;
          RowInfo[RowIndex].BaselineOffset = BaseLineOffset - Cell[Index].OffsetY;
          if (LineHeight > Cell[Index - 1].Height) {
            LineHeight = Cell[Index - 1].Height;
          }
          RowInfo[RowIndex].LineHeight     = LineHeight;
        } else {
          //
          // There is only one column and it can not be drawn so that return directly.
          //
          Status = EFI_SUCCESS;
          goto Exit;
        }
      }

      //
      // Clip the final row if the row's bottom-most on pixel cannot fit when
      // EFI_HII_OUT_FLAG_CLEAN_Y is set.
      //
      if (RowIndex == MaxRowNum - 1 && Image->Height < LineHeight) {
        LineHeight = Image->Height;
        if ((Flags & EFI_HII_OUT_FLAG_CLEAN_Y) == EFI_HII_OUT_FLAG_CLEAN_Y) {
          //
          // Don't draw at all if the row's bottom-most on pixel cannot fit.
          //
          break;
        }
      }

      //
      // Draw it to screen or existing bitmap depending on whether
      // EFI_HII_DIRECT_TO_SCREEN is set.
      //
      if ((Flags & EFI_HII_DIRECT_TO_SCREEN) == EFI_HII_DIRECT_TO_SCREEN) {
        BltBuffer = AllocateZeroPool (RowInfo[RowIndex].LineWidth * RowInfo[RowIndex].LineHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        if (BltBuffer == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Exit;
        }
        BufferPtr = BltBuffer;
        for (Index1 = RowInfo[RowIndex].StartIndex; Index1 <= RowInfo[RowIndex].EndIndex; Index1++) {
          GlyphToImage (
            GlyphBuf[Index1],
            Foreground,
            Background,
            RowInfo[RowIndex].LineWidth,
            RowInfo[RowIndex].LineHeight,
            Transparent,
            Cell[Index1],
            Attributes[Index1],
            &BufferPtr
            );
          if (ColumnInfoArray != NULL) {
            if (Index1 == RowInfo[RowIndex].StartIndex) {
              *ColumnInfoArray = 0;
            } else {
              *ColumnInfoArray = Cell[Index1 -1].AdvanceX;
            }
            ColumnInfoArray++;
          }
        }
        Status = Image->Image.Screen->Blt (
                                        Image->Image.Screen,
                                        BltBuffer,
                                        EfiBltBufferToVideo,
                                        0,
                                        0,
                                        BltX,
                                        BltY,
                                        RowInfo[RowIndex].LineWidth,
                                        RowInfo[RowIndex].LineHeight,
                                        0
                                        );
        if (EFI_ERROR (Status)) {
          SafeFreePool (BltBuffer);
          goto Exit;
        }

        SafeFreePool (BltBuffer);

      } else {
        for (Index1 = RowInfo[RowIndex].StartIndex; Index1 <= RowInfo[RowIndex].EndIndex; Index1++) {
          GlyphToImage (
            GlyphBuf[Index1],
            Foreground,
            Background,
            Image->Width,
            Image->Height,
            Transparent,
            Cell[Index1],
            Attributes[Index1],
            &BufferPtr
            );
          if (ColumnInfoArray != NULL) {
            if (Index1 == RowInfo[RowIndex].StartIndex) {
              *ColumnInfoArray = 0;
            } else {
              *ColumnInfoArray = Cell[Index1 -1].AdvanceX;
            }
            ColumnInfoArray++;
          }
        }
        //
        // Jump to next row
        //
        BufferPtr += BltX + Image->Width * (LineHeight - 1);
      }

      Index++;
      RowIndex++;

    }

    //
    // Write output parameters.
    //
    RowInfoSize = RowIndex * sizeof (EFI_HII_ROW_INFO);
    if (RowInfoArray != NULL) {
      *RowInfoArray = AllocateZeroPool (RowInfoSize);
      if (*RowInfoArray == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
      }
      CopyMem (*RowInfoArray, RowInfo, RowInfoSize);
    }
    if (RowInfoArraySize != NULL) {
      *RowInfoArraySize = RowIndex;
    }

  } else {
    //
    // Create a new bitmap and draw the string onto this image.
    //
    Image = AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    if (Image == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Image->Width  = 800;
    Image->Height = 600;
    Image->Image.Bitmap = AllocateZeroPool (Image->Width * Image->Height *sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (Image->Image.Bitmap == NULL) {
      SafeFreePool (Image);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Other flags are not permitted when Blt is NULL.
    //
    Flags &= EFI_HII_OUT_FLAG_WRAP | EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_IGNORE_LINE_BREAK;
    Status = HiiStringToImage (
               This,
               Flags,
               String,
               StringInfo,
               &Image,
               BltX,
               BltY,
               RowInfoArray,
               RowInfoArraySize,
               ColumnInfoArray
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    *Blt = Image;
  }

  Status = EFI_SUCCESS;

Exit:

  for (Index = 0; Index < MAX_STRING_LENGTH; Index++) {
    SafeFreePool (GlyphBuf[Index]);
  }
  SafeFreePool (StringIn);
  SafeFreePool (StringIn2);
  SafeFreePool (StringInfoOut);
  SafeFreePool (RowInfo);
  SafeFreePool (SystemDefault);
  SafeFreePool (GlyphBuf);
  SafeFreePool (Cell);
  SafeFreePool (Attributes);

  return Status;
}


/**
  Render a string to a bitmap or the screen containing the contents of the specified string.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Flags                   Describes how the string is to be drawn.
  @param  PackageList             The package list in the HII database to search
                                  for the specified string.
  @param  StringId                The string's id, which is unique within
                                  PackageList.
  @param  Language                Points to the language for the retrieved string.
                                  If NULL, then the current system language is
                                  used.
  @param  StringInfo              Points to the string output information,
                                  including the color and font.  If NULL, then the
                                  string will be output in the default system font
                                  and color.
  @param  Blt                     If this points to a non-NULL on entry, this
                                  points to the image, which is Width pixels   wide
                                  and Height pixels high. The string will be drawn
                                  onto this image and
                                  EFI_HII_OUT_FLAG_CLIP is implied. If this points
                                  to a NULL on entry, then a              buffer
                                  will be allocated to hold the generated image and
                                  the pointer updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  BltX,BLTY               Specifies the offset from the left and top edge
                                  of the image of the first character cell in the
                                  image.
  @param  RowInfoArray            If this is non-NULL on entry, then on exit, this
                                  will point to an allocated buffer    containing
                                  row information and RowInfoArraySize will be
                                  updated to contain the        number of elements.
                                  This array describes the characters which were at
                                  least partially drawn and the heights of the
                                  rows. It is the caller's responsibility to free
                                  this buffer.
  @param  RowInfoArraySize        If this is non-NULL on entry, then on exit it
                                  contains the number of elements in RowInfoArray.
  @param  ColumnInfoArray         If this is non-NULL, then on return it will be
                                  filled with the horizontal offset for each
                                  character in the string on the row where it is
                                  displayed. Non-printing characters will     have
                                  the offset ~0. The caller is responsible to
                                  allocate a buffer large enough so that    there
                                  is one entry for each character in the string,
                                  not including the null-terminator. It is possible
                                  when character display is normalized that some
                                  character cells overlap.

  @retval EFI_SUCCESS             The string was successfully rendered.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for
                                  RowInfoArray or Blt.
  @retval EFI_INVALID_PARAMETER   The PackageList was NULL.

**/
EFI_STATUS
EFIAPI
HiiStringIdToImage (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  EFI_HII_OUT_FLAGS              Flags,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_STRING_ID                  StringId,
  IN  CONST CHAR8*                   Language,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo       OPTIONAL,
  IN  OUT EFI_IMAGE_OUTPUT           **Blt,
  IN  UINTN                          BltX,
  IN  UINTN                          BltY,
  OUT EFI_HII_ROW_INFO               **RowInfoArray    OPTIONAL,
  OUT UINTN                          *RowInfoArraySize OPTIONAL,
  OUT UINTN                          *ColumnInfoArray  OPTIONAL
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_STRING                          String;
  UINTN                               StringSize;

  if (This == NULL || PackageList == NULL || Blt == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  //
  // Get the string to be displayed.
  //

  StringSize = MAX_STRING_LENGTH;
  String = (EFI_STRING) AllocateZeroPool (StringSize);
  if (String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Private->HiiString.GetString (
                                &Private->HiiString,
                                Language,
                                PackageList,
                                StringId,
                                String,
                                &StringSize,
                                NULL
                                );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    SafeFreePool (String);
    String = (EFI_STRING) AllocateZeroPool (StringSize);
    if (String == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = Private->HiiString.GetString (
                                  &Private->HiiString,
                                  Language,
                                  PackageList,
                                  StringId,
                                  String,
                                  &StringSize,
                                  NULL
                                  );

  }

  if (EFI_ERROR (Status)) {
    SafeFreePool (String);
    return Status;
  }

  return HiiStringToImage (
           This,
           Flags,
           String,
           StringInfo,
           Blt,
           BltX,
           BltY,
           RowInfoArray,
           RowInfoArraySize,
           ColumnInfoArray
           );

}


/**
  Convert the glyph for a single character into a bitmap.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  Char                    Character to retrieve.
  @param  StringInfo              Points to the string font and color information
                                  or NULL if the string should use the default
                                  system font and color.
  @param  Blt                     Thus must point to a NULL on entry. A buffer will
                                  be allocated to hold the output and the pointer
                                  updated on exit. It is the caller's
                                  responsibility to free this buffer.
  @param  Baseline                Number of pixels from the bottom of the bitmap to
                                  the baseline.

  @retval EFI_SUCCESS             Glyph bitmap created.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate the output buffer Blt.
  @retval EFI_WARN_UNKNOWN_GLYPH  The glyph was unknown and was replaced with the
                                  glyph for Unicode character 0xFFFD.
  @retval EFI_INVALID_PARAMETER   Blt is NULL or *Blt is not NULL.

**/
EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  CHAR16                         Char,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfo,
  OUT EFI_IMAGE_OUTPUT               **Blt,
  OUT UINTN                          *Baseline OPTIONAL
  )
{
  EFI_STATUS                         Status;
  HII_DATABASE_PRIVATE_DATA          *Private;
  EFI_IMAGE_OUTPUT                   *Image;
  UINT8                              *GlyphBuffer;
  EFI_FONT_DISPLAY_INFO              *SystemDefault;
  EFI_FONT_DISPLAY_INFO              *StringInfoOut;
  BOOLEAN                            Default;
  EFI_FONT_HANDLE                    FontHandle;
  EFI_STRING                         String;
  EFI_HII_GLYPH_INFO                 Cell;
  EFI_FONT_INFO                      *FontInfo;
  UINT8                              Attributes;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer;

  if (This == NULL || Blt == NULL || *Blt != NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  Default       = FALSE;
  Image         = NULL;
  SystemDefault = NULL;
  FontHandle    = NULL;
  String        = NULL;
  GlyphBuffer   = NULL;
  StringInfoOut = NULL;
  FontInfo      = NULL;

  ZeroMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  ZeroMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

  Default = IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfo, &SystemDefault, NULL);

  if (!Default) {
    //
    // Find out a EFI_FONT_DISPLAY_INFO which could display the character in
    // the specified color and font.
    //
    String = (EFI_STRING) AllocateZeroPool (sizeof (CHAR16) * 2);
    if (String == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    *String = Char;
    *(String + 1) = 0;

    Status = HiiGetFontInfo (This, &FontHandle, StringInfo, &StringInfoOut, String);
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
    FontInfo   = &StringInfoOut->FontInfo;
    Foreground = StringInfoOut->ForegroundColor;
    Background = StringInfoOut->BackgroundColor;
  } else {
    Foreground = SystemDefault->ForegroundColor;
    Background = SystemDefault->BackgroundColor;
  }

  Status = GetGlyphBuffer (Private, Char, FontInfo, &GlyphBuffer, &Cell, &Attributes);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Image = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  if (Image == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  Image->Width   = Cell.Width;
  Image->Height  = Cell.Height;

  Image->Image.Bitmap = AllocateZeroPool (Image->Width * Image->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (Image->Image.Bitmap == NULL) {
    SafeFreePool (Image);
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  BltBuffer = Image->Image.Bitmap;
  GlyphToImage (
    GlyphBuffer,
    Foreground,
    Background,
    Image->Width,
    Image->Height,
    FALSE,
    Cell,
    Attributes,
    &BltBuffer
    );

  *Blt = Image;
  if (Baseline != NULL) {
    *Baseline = Cell.OffsetY;
  }

  Status = EFI_SUCCESS;

Exit:

  if (Status == EFI_NOT_FOUND) {
    //
    // Glyph is unknown and replaced with the glyph for unicode character 0xFFFD
    //
    if (Char != REPLACE_UNKNOWN_GLYPH) {
      Status = HiiGetGlyph (This, REPLACE_UNKNOWN_GLYPH, StringInfo, Blt, Baseline);
      if (!EFI_ERROR (Status)) {
        Status = EFI_WARN_UNKNOWN_GLYPH;
      }
    } else {
      Status = EFI_WARN_UNKNOWN_GLYPH;
    }
  }

  SafeFreePool (SystemDefault);
  SafeFreePool (StringInfoOut);
  SafeFreePool (String);
  SafeFreePool (GlyphBuffer);

  return Status;
}


/**
  This function iterates through fonts which match the specified font, using
  the specified criteria. If String is non-NULL, then all of the characters in
  the string must exist in order for a candidate font to be returned.

  @param  This                    A pointer to the EFI_HII_FONT_PROTOCOL instance.
  @param  FontHandle              On entry, points to the font handle returned by a
                                   previous call to GetFontInfo() or NULL to start
                                  with the  first font. On return, points to the
                                  returned font handle or points to NULL if there
                                  are no more matching fonts.
  @param  StringInfoIn            Upon entry, points to the font to return
                                  information about.
  @param  StringInfoOut           Upon return, contains the matching font's
                                  information.  If NULL, then no information is
                                  returned. It's caller's responsibility to free
                                  this buffer.
  @param  String                  Points to the string which will be tested to
                                  determine  if all characters are available. If
                                  NULL, then any font  is acceptable.

  @retval EFI_SUCCESS             Matching font returned successfully.
  @retval EFI_NOT_FOUND           No matching font was found.
  @retval EFI_INVALID_PARAMETER   StringInfoIn is NULL.
  @retval EFI_OUT_OF_RESOURCES    There were insufficient resources to complete the
                                  request.

**/
EFI_STATUS
EFIAPI
HiiGetFontInfo (
  IN  CONST EFI_HII_FONT_PROTOCOL    *This,
  IN  OUT   EFI_FONT_HANDLE          *FontHandle,
  IN  CONST EFI_FONT_DISPLAY_INFO    *StringInfoIn,
  OUT       EFI_FONT_DISPLAY_INFO    **StringInfoOut,
  IN  CONST EFI_STRING               String OPTIONAL
  )
{
  HII_DATABASE_PRIVATE_DATA          *Private;
  EFI_STATUS                         Status;
  EFI_FONT_DISPLAY_INFO              *SystemDefault;
  EFI_FONT_DISPLAY_INFO              InfoOut;
  UINTN                              StringInfoOutLen;
  EFI_FONT_INFO                      *FontInfo;
  HII_GLOBAL_FONT_INFO               *GlobalFont;
  EFI_STRING                         StringIn;
  EFI_FONT_HANDLE                    LocalFontHandle;

  if (This == NULL || StringInfoIn == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the font information mask to make sure it is valid.
  //
  if (((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_SYS_FONT  | EFI_FONT_INFO_ANY_FONT))  ==
       (EFI_FONT_INFO_SYS_FONT | EFI_FONT_INFO_ANY_FONT))   ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_SYS_SIZE  | EFI_FONT_INFO_ANY_SIZE))  ==
       (EFI_FONT_INFO_SYS_SIZE | EFI_FONT_INFO_ANY_SIZE))   ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_SYS_STYLE | EFI_FONT_INFO_ANY_STYLE)) ==
       (EFI_FONT_INFO_SYS_STYLE | EFI_FONT_INFO_ANY_STYLE)) ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_RESIZE    | EFI_FONT_INFO_ANY_SIZE))  ==
       (EFI_FONT_INFO_RESIZE | EFI_FONT_INFO_ANY_SIZE))     ||
      ((StringInfoIn->FontInfoMask & (EFI_FONT_INFO_RESTYLE   | EFI_FONT_INFO_ANY_STYLE)) ==
       (EFI_FONT_INFO_RESTYLE | EFI_FONT_INFO_ANY_STYLE))) {
    return EFI_INVALID_PARAMETER;
  }

  FontInfo        = NULL;
  LocalFontHandle = NULL;
  if (FontHandle != NULL) {
    LocalFontHandle = *FontHandle;
  }

  //
  // Get default system display info, if StringInfoIn points to
  // system display info, return it directly.
  //
  Private = HII_FONT_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  if (IsSystemFontInfo (Private, (EFI_FONT_DISPLAY_INFO *) StringInfoIn, &SystemDefault, &StringInfoOutLen)) {
    if (StringInfoOut != NULL) {
      *StringInfoOut = AllocateCopyPool (StringInfoOutLen, (EFI_FONT_DISPLAY_INFO *) StringInfoIn);
      if (*StringInfoOut == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        LocalFontHandle = NULL;
        goto Exit;
      }
    }

    LocalFontHandle = Private->FontInfoList.ForwardLink;
    Status = EFI_SUCCESS;
    goto Exit;
  }

  //
  // Parse the font information mask to find a matching font.
  //

  CopyMem (&InfoOut, (EFI_FONT_DISPLAY_INFO *) StringInfoIn, sizeof (EFI_FONT_DISPLAY_INFO));

  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_FONT) == EFI_FONT_INFO_SYS_FONT) {
    Status = SaveFontName (SystemDefault->FontInfo.FontName, &FontInfo);
  } else {
    Status = SaveFontName (((EFI_FONT_DISPLAY_INFO *) StringInfoIn)->FontInfo.FontName, &FontInfo);
  }
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_SIZE) == EFI_FONT_INFO_SYS_SIZE) {
    InfoOut.FontInfo.FontSize = SystemDefault->FontInfo.FontSize;
  } else if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_STYLE) == EFI_FONT_INFO_SYS_STYLE) {
    InfoOut.FontInfo.FontStyle = SystemDefault->FontInfo.FontStyle;
  } else if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_FORE_COLOR) == EFI_FONT_INFO_SYS_FORE_COLOR) {
    InfoOut.ForegroundColor = SystemDefault->ForegroundColor;
  } else if ((StringInfoIn->FontInfoMask & EFI_FONT_INFO_SYS_BACK_COLOR) == EFI_FONT_INFO_SYS_BACK_COLOR) {
    InfoOut.BackgroundColor = SystemDefault->BackgroundColor;
  }

  FontInfo->FontSize  = InfoOut.FontInfo.FontSize;
  FontInfo->FontStyle = InfoOut.FontInfo.FontStyle;

  if (IsFontInfoExisted (Private, FontInfo, &InfoOut.FontInfoMask, LocalFontHandle, &GlobalFont)) {
    if (String != NULL) {
      //
      // Test to guarantee all characters are available in the found font.
      //
      StringIn = String;
      while (*StringIn != 0) {
        Status = FindGlyphBlock (GlobalFont->FontPackage, *StringIn, NULL, NULL, NULL);
        if (EFI_ERROR (Status)) {
          LocalFontHandle = NULL;
          goto Exit;
        }
        StringIn++;
      }

      //
      // Write to output parameter
      //
      if (StringInfoOut != NULL) {
        StringInfoOutLen = sizeof (EFI_FONT_DISPLAY_INFO) - sizeof (EFI_FONT_INFO) + GlobalFont->FontInfoSize;
        *StringInfoOut   = (EFI_FONT_DISPLAY_INFO *) AllocateZeroPool (StringInfoOutLen);
        if (*StringInfoOut == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          LocalFontHandle = NULL;
          goto Exit;
        }
        CopyMem (*StringInfoOut, &InfoOut, sizeof (EFI_FONT_DISPLAY_INFO));
        CopyMem (&(*StringInfoOut)->FontInfo, GlobalFont->FontInfo, GlobalFont->FontInfoSize);
      }
      LocalFontHandle = GlobalFont->Entry.ForwardLink;

      Status = EFI_SUCCESS;
      goto Exit;
    }
  } else {
    LocalFontHandle = NULL;
  }

  Status = EFI_NOT_FOUND;

Exit:

  if (FontHandle != NULL) {
    *FontHandle = LocalFontHandle;
  }

  SafeFreePool (SystemDefault);
  SafeFreePool (FontInfo);
  return Status;
}

