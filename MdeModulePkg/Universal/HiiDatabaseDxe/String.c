/** @file
Implementation for EFI_HII_STRING_PROTOCOL.


Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

CHAR16 mLanguageWindow[16] = {
  0x0000, 0x0080, 0x0100, 0x0300,
  0x2000, 0x2080, 0x2100, 0x3000,
  0x0080, 0x00C0, 0x0400, 0x0600,
  0x0900, 0x3040, 0x30A0, 0xFF00
};


/**
  This function checks whether a global font info is referred by local
  font info list or not. (i.e. HII_FONT_INFO is generated.) If not, create
  a HII_FONT_INFO to refer it locally.

  This is a internal function.


  @param  Private                Hii database private structure.
  @param  StringPackage          HII string package instance.
  @param  FontId                Font identifer, which must be unique within the string package.
  @param  DuplicateEnable        If true, duplicate HII_FONT_INFO which refers to
                                 the same EFI_FONT_INFO is permitted. Otherwise it
                                 is not allowed.
  @param  GlobalFontInfo         Input a global font info which specify a
                                 EFI_FONT_INFO.
  @param  LocalFontInfo          Output a local font info which refers to a
                                 EFI_FONT_INFO.

  @retval TRUE                   Already referred before calling this function.
  @retval FALSE                  Not referred before calling this function.

**/
BOOLEAN
ReferFontInfoLocally (
  IN  HII_DATABASE_PRIVATE_DATA   *Private,
  IN  HII_STRING_PACKAGE_INSTANCE *StringPackage,
  IN  UINT8                       FontId,
  IN  BOOLEAN                     DuplicateEnable,
  IN  HII_GLOBAL_FONT_INFO        *GlobalFontInfo,
  OUT HII_FONT_INFO               **LocalFontInfo
  )
{
  HII_FONT_INFO                 *LocalFont;
  LIST_ENTRY                    *Link;

  ASSERT (Private != NULL && StringPackage != NULL && GlobalFontInfo != NULL && LocalFontInfo != NULL);

  if (!DuplicateEnable) {
    for (Link = StringPackage->FontInfoList.ForwardLink;
         Link != &StringPackage->FontInfoList;
         Link = Link->ForwardLink
        ) {
      LocalFont = CR (Link, HII_FONT_INFO, Entry, HII_FONT_INFO_SIGNATURE);
      if (LocalFont->GlobalEntry == &GlobalFontInfo->Entry) {
        //
        // Already referred by local font info list, return directly.
        //
        *LocalFontInfo = LocalFont;
        return TRUE;
      }
    }
  }
  // FontId identifies EFI_FONT_INFO in local string package uniquely.
  // GlobalEntry points to a HII_GLOBAL_FONT_INFO which identifies
  // EFI_FONT_INFO uniquely in whole hii database.
  //
  LocalFont = (HII_FONT_INFO *) AllocateZeroPool (sizeof (HII_FONT_INFO));
  ASSERT (LocalFont != NULL);

  LocalFont->Signature   = HII_FONT_INFO_SIGNATURE;
  LocalFont->FontId      = FontId;
  LocalFont->GlobalEntry = &GlobalFontInfo->Entry;
  InsertTailList (&StringPackage->FontInfoList, &LocalFont->Entry);

  *LocalFontInfo = LocalFont;
  return FALSE;
}


/**
  Convert Ascii string text to unicode string test.

  This is a internal function.


  @param  StringDest             Buffer to store the string text. If it is NULL,
                                 only the size will be returned.
  @param  StringSrc              Points to current null-terminated string.
  @param  BufferSize             Length of the buffer.

  @retval EFI_SUCCESS            The string text was outputed successfully.
  @retval EFI_BUFFER_TOO_SMALL   Buffer is insufficient to store the found string
                                 text. BufferSize is updated to the required buffer
                                 size.

**/
EFI_STATUS
ConvertToUnicodeText (
  OUT EFI_STRING       StringDest,
  IN  CHAR8            *StringSrc,
  IN  OUT UINTN        *BufferSize
  )
{
  UINTN  StringSize;
  UINTN  Index;

  ASSERT (StringSrc != NULL && BufferSize != NULL);

  StringSize = AsciiStrSize (StringSrc) * 2;
  if (*BufferSize < StringSize) {
    *BufferSize = StringSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  for (Index = 0; Index < AsciiStrLen (StringSrc); Index++) {
    StringDest[Index] = (CHAR16) StringSrc[Index];
  }

  StringDest[Index] = 0;
  return EFI_SUCCESS;
}


/**
  Calculate the size of StringSrc and output it. If StringDest is not NULL,
  copy string text from src to dest.

  This is a internal function.

  @param  StringDest             Buffer to store the string text. If it is NULL,
                                 only the size will be returned.
  @param  StringSrc              Points to current null-terminated string.
  @param  BufferSize             Length of the buffer.

  @retval EFI_SUCCESS            The string text was outputed successfully.
  @retval EFI_BUFFER_TOO_SMALL   Buffer is insufficient to store the found string
                                 text. BufferSize is updated to the required buffer
                                 size.

**/
EFI_STATUS
GetUnicodeStringTextOrSize (
  OUT EFI_STRING       StringDest, OPTIONAL
  IN  UINT8            *StringSrc,
  IN  OUT UINTN        *BufferSize
  )
{
  UINTN  StringSize;
  UINT8  *StringPtr;

  ASSERT (StringSrc != NULL && BufferSize != NULL);

  StringSize = sizeof (CHAR16);
  StringPtr  = StringSrc;
  while (ReadUnaligned16 ((UINT16 *) StringPtr) != 0) {
    StringSize += sizeof (CHAR16);
    StringPtr += sizeof (CHAR16);
  }

  if (*BufferSize < StringSize) {
    *BufferSize = StringSize;
    return EFI_BUFFER_TOO_SMALL;
  }
  if (StringDest != NULL) {
    CopyMem (StringDest, StringSrc, StringSize);
  }

  *BufferSize = StringSize;
  return EFI_SUCCESS;
}


/**
  Copy string font info to a buffer.

  This is a internal function.

  @param  StringPackage          Hii string package instance.
  @param  FontId                 Font identifier which is unique in a string
                                 package.
  @param  StringFontInfo         Buffer to record the output font info. It's
                                 caller's responsibility to free this buffer.

  @retval EFI_SUCCESS            The string font is outputed successfully.
  @retval EFI_NOT_FOUND          The specified font id does not exist.

**/
EFI_STATUS
GetStringFontInfo (
  IN  HII_STRING_PACKAGE_INSTANCE     *StringPackage,
  IN  UINT8                           FontId,
  OUT EFI_FONT_INFO                   **StringFontInfo
  )
{
  LIST_ENTRY                           *Link;
  HII_FONT_INFO                        *FontInfo;
  HII_GLOBAL_FONT_INFO                 *GlobalFont;

  ASSERT (StringFontInfo != NULL && StringPackage != NULL);

  for (Link = StringPackage->FontInfoList.ForwardLink; Link != &StringPackage->FontInfoList; Link = Link->ForwardLink) {
    FontInfo = CR (Link, HII_FONT_INFO, Entry, HII_FONT_INFO_SIGNATURE);
    if (FontInfo->FontId == FontId) {
      GlobalFont = CR (FontInfo->GlobalEntry, HII_GLOBAL_FONT_INFO, Entry, HII_GLOBAL_FONT_INFO_SIGNATURE);
      *StringFontInfo = (EFI_FONT_INFO *) AllocateZeroPool (GlobalFont->FontInfoSize);
      if (*StringFontInfo == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      CopyMem (*StringFontInfo, GlobalFont->FontInfo, GlobalFont->FontInfoSize);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Parse all string blocks to find a String block specified by StringId.
  If StringId = (EFI_STRING_ID) (-1), find out all EFI_HII_SIBT_FONT blocks
  within this string package and backup its information.
  If StringId = 0, output the string id of last string block (EFI_HII_SIBT_END).

  @param  Private                Hii database private structure.
  @param  StringPackage          Hii string package instance.
  @param  StringId               The string's id, which is unique within
                                 PackageList.
  @param  BlockType              Output the block type of found string block.
  @param  StringBlockAddr        Output the block address of found string block.
  @param  StringTextOffset       Offset, relative to the found block address, of
                                 the  string text information.
  @param  LastStringId           Output the last string id when StringId = 0.

  @retval EFI_SUCCESS            The string text and font is retrieved
                                 successfully.
  @retval EFI_NOT_FOUND          The specified text or font info can not be found
                                 out.
  @retval EFI_OUT_OF_RESOURCES   The system is out of resources to accomplish the
                                 task.

**/
EFI_STATUS
FindStringBlock (
  IN HII_DATABASE_PRIVATE_DATA        *Private,
  IN  HII_STRING_PACKAGE_INSTANCE     *StringPackage,
  IN  EFI_STRING_ID                   StringId,
  OUT UINT8                           *BlockType, OPTIONAL
  OUT UINT8                           **StringBlockAddr, OPTIONAL
  OUT UINTN                           *StringTextOffset, OPTIONAL
  OUT EFI_STRING_ID                   *LastStringId OPTIONAL
  )
{
  UINT8                                *BlockHdr;
  EFI_STRING_ID                        CurrentStringId;
  UINTN                                BlockSize;
  UINTN                                Index;
  UINT8                                *StringTextPtr;
  UINTN                                Offset;
  HII_FONT_INFO                        *LocalFont;
  EFI_FONT_INFO                        *FontInfo;
  HII_GLOBAL_FONT_INFO                 *GlobalFont;
  UINTN                                FontInfoSize;
  UINT16                               StringCount;
  UINT16                               SkipCount;
  EFI_HII_FONT_STYLE                   FontStyle;
  UINT16                               FontSize;
  UINT8                                Length8;
  EFI_HII_SIBT_EXT2_BLOCK              Ext2;
  UINT8                                FontId;
  UINT32                               Length32;
  UINTN                                StringSize;
  CHAR16                               Zero;

  ASSERT (StringPackage != NULL);
  ASSERT (StringPackage->Signature == HII_STRING_PACKAGE_SIGNATURE);

  CurrentStringId = 1;

  if (StringId != (EFI_STRING_ID) (-1) && StringId != 0) {
    ASSERT (BlockType != NULL && StringBlockAddr != NULL && StringTextOffset != NULL);
  } else {
    ASSERT (Private != NULL && Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);
  }

  ZeroMem (&Zero, sizeof (CHAR16));

  //
  // Parse the string blocks to get the string text and font.
  //
  BlockHdr  = StringPackage->StringBlock;
  BlockSize = 0;
  Offset    = 0;
  while (*BlockHdr != EFI_HII_SIBT_END) {
    switch (*BlockHdr) {
    case EFI_HII_SIBT_STRING_SCSU:
      Offset = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset + AsciiStrSize ((CHAR8 *) StringTextPtr);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_SCSU_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK) - sizeof (UINT8);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset + AsciiStrSize ((CHAR8 *) StringTextPtr);
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_SCSU:
      CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      StringTextPtr = BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_BLOCK) - sizeof (UINT8);
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        BlockSize += AsciiStrSize ((CHAR8 *) StringTextPtr);
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringBlockAddr  = BlockHdr;
          *StringTextOffset = StringTextPtr - BlockHdr;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *) StringTextPtr);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
      CopyMem (
        &StringCount,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT16)
        );
      StringTextPtr = BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK) - sizeof (UINT8);
      BlockSize += StringTextPtr - BlockHdr;

      for (Index = 0; Index < StringCount; Index++) {
        BlockSize += AsciiStrSize ((CHAR8 *) StringTextPtr);
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringBlockAddr  = BlockHdr;
          *StringTextOffset = StringTextPtr - BlockHdr;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *) StringTextPtr);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRING_UCS2:
      Offset        = sizeof (EFI_HII_STRING_BLOCK);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StringSize to store the size of the specified string, including the NULL
      // terminator.
      //
      GetUnicodeStringTextOrSize (NULL, StringTextPtr, &StringSize);
      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRING_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK)  - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      //
      // Use StrSize to store the size of the specified string, including the NULL
      // terminator.
      //
      GetUnicodeStringTextOrSize (NULL, StringTextPtr, &StringSize);
      BlockSize += Offset + StringSize;
      CurrentStringId++;
      break;

    case EFI_HII_SIBT_STRINGS_UCS2:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      for (Index = 0; Index < StringCount; Index++) {
        GetUnicodeStringTextOrSize (NULL, StringTextPtr, &StringSize);
        BlockSize += StringSize;
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringBlockAddr  = BlockHdr;
          *StringTextOffset = StringTextPtr - BlockHdr;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      Offset = sizeof (EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK) - sizeof (CHAR16);
      StringTextPtr = BlockHdr + Offset;
      BlockSize += Offset;
      CopyMem (
        &StringCount,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT16)
        );
      for (Index = 0; Index < StringCount; Index++) {
        GetUnicodeStringTextOrSize (NULL, StringTextPtr, &StringSize);
        BlockSize += StringSize;
        if (CurrentStringId == StringId) {
          *BlockType        = *BlockHdr;
          *StringBlockAddr  = BlockHdr;
          *StringTextOffset = StringTextPtr - BlockHdr;
          return EFI_SUCCESS;
        }
        StringTextPtr = StringTextPtr + StringSize;
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_DUPLICATE:
      if (CurrentStringId == StringId) {
        //
        // Incoming StringId is an id of a duplicate string block.
        // Update the StringId to be the previous string block.
        // Go back to the header of string block to search.
        //
        CopyMem (
          &StringId,
          BlockHdr + sizeof (EFI_HII_STRING_BLOCK),
          sizeof (EFI_STRING_ID)
          );
        ASSERT (StringId != CurrentStringId);
        CurrentStringId = 1;
        BlockSize       = 0;
      } else {
        BlockSize       += sizeof (EFI_HII_SIBT_DUPLICATE_BLOCK);
        CurrentStringId++;
      }
      break;

    case EFI_HII_SIBT_SKIP1:
      SkipCount = (UINT16) (*(BlockHdr + sizeof (EFI_HII_STRING_BLOCK)));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP1_BLOCK);
      break;

    case EFI_HII_SIBT_SKIP2:
      CopyMem (&SkipCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
      CurrentStringId = (UINT16) (CurrentStringId + SkipCount);
      BlockSize       +=  sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
      break;

    case EFI_HII_SIBT_EXT1:
      CopyMem (
        &Length8,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT8)
        );
      BlockSize += Length8;
      break;

    case EFI_HII_SIBT_EXT2:
      CopyMem (&Ext2, BlockHdr, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
      if (Ext2.BlockType2 == EFI_HII_SIBT_FONT && StringId == (EFI_STRING_ID) (-1)) {
        //
        // Find the relationship between global font info and the font info of
        // this EFI_HII_SIBT_FONT block then backup its information in local package.
        //
        BlockHdr += sizeof (EFI_HII_SIBT_EXT2_BLOCK);
        CopyMem (&FontId, BlockHdr, sizeof (UINT8));
        BlockHdr += sizeof (UINT8);
        CopyMem (&FontSize, BlockHdr, sizeof (UINT16));
        BlockHdr += sizeof (UINT16);
        CopyMem (&FontStyle, BlockHdr, sizeof (EFI_HII_FONT_STYLE));
        BlockHdr += sizeof (EFI_HII_FONT_STYLE);
        GetUnicodeStringTextOrSize (NULL, BlockHdr, &StringSize);

        FontInfoSize = sizeof (EFI_FONT_INFO) - sizeof (CHAR16) + StringSize;
        FontInfo = (EFI_FONT_INFO *) AllocateZeroPool (FontInfoSize);
        if (FontInfo == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
        FontInfo->FontStyle = FontStyle;
        FontInfo->FontSize  = FontSize;
        CopyMem (FontInfo->FontName, BlockHdr, StringSize);

        //
        // If find the corresponding global font info, save the relationship.
        // Otherwise ignore this EFI_HII_SIBT_FONT block.
        //
        if (IsFontInfoExisted (Private, FontInfo, NULL, NULL, &GlobalFont)) {
          ReferFontInfoLocally (Private, StringPackage, FontId, TRUE, GlobalFont, &LocalFont);
        }

        //
        // Since string package tool set FontId initially to 0 and increases it
        // progressively by one, StringPackage->FondId always represents an unique
        // and available FontId.
        //        
        StringPackage->FontId++;

        FreePool (FontInfo);
      }

      BlockSize += Ext2.Length;

      break;

    case EFI_HII_SIBT_EXT4:
      CopyMem (
        &Length32,
        BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8),
        sizeof (UINT32)
        );

      BlockSize += Length32;
      break;

    default:
      break;
    }

    if (StringId > 0) {
      if (StringId == CurrentStringId - 1) {
        *BlockType        = *BlockHdr;
        *StringBlockAddr  = BlockHdr;
        *StringTextOffset = Offset;
        return EFI_SUCCESS;
      }

      if (StringId < CurrentStringId - 1) {
        return EFI_NOT_FOUND;
      }
    }
    BlockHdr  = StringPackage->StringBlock + BlockSize;

  }

  if (StringId == (EFI_STRING_ID) (-1)) {
    return EFI_SUCCESS;
  }

  if (StringId == 0 && LastStringId != NULL) {
    *LastStringId = CurrentStringId;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}


/**
  Parse all string blocks to get a string specified by StringId.

  This is a internal function.

  @param  Private                Hii database private structure.
  @param  StringPackage          Hii string package instance.
  @param  StringId               The string's id, which is unique within
                                 PackageList.
  @param  String                 Points to retrieved null-terminated string.
  @param  StringSize             On entry, points to the size of the buffer pointed
                                 to by String, in bytes. On return, points to the
                                 length of the string, in bytes.
  @param  StringFontInfo         If not NULL, allocate a buffer to record the
                                 output font info. It's caller's responsibility to
                                 free this buffer.

  @retval EFI_SUCCESS            The string text and font is retrieved
                                 successfully.
  @retval EFI_NOT_FOUND          The specified text or font info can not be found
                                 out.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by StringSize is too small to
                                 hold the string.

**/
EFI_STATUS
GetStringWorker (
  IN HII_DATABASE_PRIVATE_DATA        *Private,
  IN  HII_STRING_PACKAGE_INSTANCE     *StringPackage,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize,
  OUT EFI_FONT_INFO                   **StringFontInfo OPTIONAL
  )
{
  UINT8                                *StringTextPtr;
  UINT8                                BlockType;
  UINT8                                *StringBlockAddr;
  UINTN                                StringTextOffset;
  EFI_STATUS                           Status;
  UINT8                                FontId;

  ASSERT (StringPackage != NULL && StringSize != NULL);
  ASSERT (Private != NULL && Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);

  //
  // Find the specified string block
  //
  Status = FindStringBlock (
             Private,
             StringPackage,
             StringId,
             &BlockType,
             &StringBlockAddr,
             &StringTextOffset,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the string text.
  //
  StringTextPtr = StringBlockAddr + StringTextOffset;
  switch (BlockType) {
  case EFI_HII_SIBT_STRING_SCSU:
  case EFI_HII_SIBT_STRING_SCSU_FONT:
  case EFI_HII_SIBT_STRINGS_SCSU:
  case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    Status = ConvertToUnicodeText (String, (CHAR8 *) StringTextPtr, StringSize);
    break;
  case EFI_HII_SIBT_STRING_UCS2:
  case EFI_HII_SIBT_STRING_UCS2_FONT:
  case EFI_HII_SIBT_STRINGS_UCS2:
  case EFI_HII_SIBT_STRINGS_UCS2_FONT:
    Status = GetUnicodeStringTextOrSize (String, StringTextPtr, StringSize);
    break;
  default:
    return EFI_NOT_FOUND;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the string font. The FontId 0 is the default font for those string blocks which 
  // do not specify a font identifier. If default font is not specified, return NULL.
  //
  if (StringFontInfo != NULL) {
    switch (BlockType) {
    case EFI_HII_SIBT_STRING_SCSU_FONT:
    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    case EFI_HII_SIBT_STRING_UCS2_FONT:
    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      FontId = *(StringBlockAddr + sizeof (EFI_HII_STRING_BLOCK));
      break;
    default:
      FontId = 0;
    }
    Status = GetStringFontInfo (StringPackage, FontId, StringFontInfo);
    if (Status == EFI_NOT_FOUND) {
        *StringFontInfo = NULL;
    }
  }

  return EFI_SUCCESS;
}


/**
  Parse all string blocks to set a String specified by StringId.

  This is a internal function.

  @param  Private                HII database driver private structure.
  @param  StringPackage          HII string package instance.
  @param  StringId               The string's id, which is unique within
                                 PackageList.
  @param  String                 Points to the new null-terminated string.
  @param  StringFontInfo         Points to the input font info.

  @retval EFI_SUCCESS            The string was updated successfully.
  @retval EFI_NOT_FOUND          The string specified by StringId is not in the
                                 database.
  @retval EFI_INVALID_PARAMETER  The String or Language was NULL.
  @retval EFI_INVALID_PARAMETER  The specified StringFontInfo does not exist in
                                 current database.
  @retval EFI_OUT_OF_RESOURCES   The system is out of resources to accomplish the
                                 task.

**/
EFI_STATUS
SetStringWorker (
  IN  HII_DATABASE_PRIVATE_DATA       *Private,
  IN OUT HII_STRING_PACKAGE_INSTANCE  *StringPackage,
  IN  EFI_STRING_ID                   StringId,
  IN  EFI_STRING                      String,
  IN  EFI_FONT_INFO                   *StringFontInfo OPTIONAL
  )
{
  UINT8                                *StringTextPtr;
  UINT8                                BlockType;
  UINT8                                *StringBlockAddr;
  UINTN                                StringTextOffset;
  EFI_STATUS                           Status;
  UINT8                                *Block;
  UINT8                                *BlockPtr;
  UINTN                                BlockSize;
  UINTN                                OldBlockSize;
  HII_FONT_INFO                        *LocalFont;
  HII_GLOBAL_FONT_INFO                 *GlobalFont;
  BOOLEAN                              Referred;
  EFI_HII_SIBT_EXT2_BLOCK              Ext2;
  UINTN                                StringSize;
  UINTN                                TmpSize;


  ASSERT (Private != NULL && StringPackage != NULL && String != NULL);
  ASSERT (Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);
  //
  // Find the specified string block
  //
  Status = FindStringBlock (
             Private,
             StringPackage,
             StringId,
             &BlockType,
             &StringBlockAddr,
             &StringTextOffset,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  LocalFont  = NULL;
  GlobalFont = NULL;
  Referred   = FALSE;

  //
  // The input StringFontInfo should exist in current database if specified.
  //
  if (StringFontInfo != NULL) {
    if (!IsFontInfoExisted (Private, StringFontInfo, NULL, NULL, &GlobalFont)) {
      return EFI_INVALID_PARAMETER;
    } else {
      Referred = ReferFontInfoLocally (
                   Private, 
                   StringPackage, 
                   StringPackage->FontId, 
                   FALSE, 
                   GlobalFont, 
                   &LocalFont
                   );
      if (!Referred) {
        StringPackage->FontId++;
      }
    }
    //
    // Update the FontId of the specified string block to input font info.
    //
    switch (BlockType) {
    case EFI_HII_SIBT_STRING_SCSU_FONT:  
    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    case EFI_HII_SIBT_STRING_UCS2_FONT:
    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      *(StringBlockAddr + sizeof (EFI_HII_STRING_BLOCK)) = LocalFont->FontId;
      break;
    default:
      //
      // When modify the font info of these blocks, the block type should be updated
      // to contain font info thus the whole structure should be revised.
      // It is recommended to use tool to modify the block type not in the code.
      //      
      return EFI_UNSUPPORTED;
    }
  }

  OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;

  //
  // Set the string text and font.
  //
  StringTextPtr = StringBlockAddr + StringTextOffset;
  switch (BlockType) {
  case EFI_HII_SIBT_STRING_SCSU:
  case EFI_HII_SIBT_STRING_SCSU_FONT:
  case EFI_HII_SIBT_STRINGS_SCSU:
  case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    BlockSize = OldBlockSize + StrLen (String);
    BlockSize -= AsciiStrLen ((CHAR8 *) StringTextPtr);
    Block = AllocateZeroPool (BlockSize);
    if (Block == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Block, StringPackage->StringBlock, StringTextPtr - StringPackage->StringBlock);
    BlockPtr = Block + (StringTextPtr - StringPackage->StringBlock);

    while (*String != 0) {
      *BlockPtr++ = (CHAR8) *String++;
    }
    *BlockPtr++ = 0;

    
    TmpSize = OldBlockSize - (StringTextPtr - StringPackage->StringBlock) - AsciiStrSize ((CHAR8 *) StringTextPtr);
    CopyMem (
      BlockPtr,
      StringTextPtr + AsciiStrSize ((CHAR8 *)StringTextPtr),
      TmpSize
      );

    FreePool (StringPackage->StringBlock);
    StringPackage->StringBlock = Block;
    StringPackage->StringPkgHdr->Header.Length += (UINT32) (BlockSize - OldBlockSize);
    break;

  case EFI_HII_SIBT_STRING_UCS2:
  case EFI_HII_SIBT_STRING_UCS2_FONT:
  case EFI_HII_SIBT_STRINGS_UCS2:
  case EFI_HII_SIBT_STRINGS_UCS2_FONT:
    //
    // Use StrSize to store the size of the specified string, including the NULL
    // terminator.
    //
    GetUnicodeStringTextOrSize (NULL, StringTextPtr, &StringSize);

    BlockSize = OldBlockSize + StrSize (String) - StringSize;
    Block = AllocateZeroPool (BlockSize);
    if (Block == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Block, StringPackage->StringBlock, StringTextPtr - StringPackage->StringBlock);
    BlockPtr = Block + (StringTextPtr - StringPackage->StringBlock);

    CopyMem (BlockPtr, String, StrSize (String));
    BlockPtr += StrSize (String);

    CopyMem (
      BlockPtr,
      StringTextPtr + StringSize,
      OldBlockSize - (StringTextPtr - StringPackage->StringBlock) - StringSize
      );

    FreePool (StringPackage->StringBlock);
    StringPackage->StringBlock = Block;
    StringPackage->StringPkgHdr->Header.Length += (UINT32) (BlockSize - OldBlockSize);
    break;

  default:
    return EFI_NOT_FOUND;
  }

  //
  // Insert a new EFI_HII_SIBT_FONT_BLOCK to the header of string block, if incoming
  // StringFontInfo does not exist in current string package.
  //
  // This new block does not impact on the value of StringId.
  //
  //
  if (StringFontInfo == NULL || Referred) {
    return EFI_SUCCESS;
  }

  OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;
  BlockSize = OldBlockSize + sizeof (EFI_HII_SIBT_FONT_BLOCK) - sizeof (CHAR16) +
              StrSize (GlobalFont->FontInfo->FontName);

  Block = AllocateZeroPool (BlockSize);
  if (Block == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlockPtr = Block;
  Ext2.Header.BlockType = EFI_HII_SIBT_EXT2;
  Ext2.BlockType2       = EFI_HII_SIBT_FONT;
  Ext2.Length           = (UINT16) (BlockSize - OldBlockSize);
  CopyMem (BlockPtr, &Ext2, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
  BlockPtr += sizeof (EFI_HII_SIBT_EXT2_BLOCK);

  *BlockPtr = LocalFont->FontId;
  BlockPtr += sizeof (UINT8);
  CopyMem (BlockPtr, &GlobalFont->FontInfo->FontSize, sizeof (UINT16));
  BlockPtr += sizeof (UINT16);
  CopyMem (BlockPtr, &GlobalFont->FontInfo->FontStyle, sizeof (UINT32));
  BlockPtr += sizeof (UINT32);
  CopyMem (
    BlockPtr,
    GlobalFont->FontInfo->FontName,
    StrSize (GlobalFont->FontInfo->FontName)
    );
  BlockPtr += StrSize (GlobalFont->FontInfo->FontName);

  CopyMem (BlockPtr, StringPackage->StringBlock, OldBlockSize);

  FreePool (StringPackage->StringBlock);
  StringPackage->StringBlock = Block;
  StringPackage->StringPkgHdr->Header.Length += Ext2.Length;

  return EFI_SUCCESS;

}


/**
  This function adds the string String to the group of strings owned by PackageList, with the
  specified font information StringFontInfo and returns a new string id.

  @param  This                   A pointer to the EFI_HII_STRING_PROTOCOL instance.
  @param  PackageList            Handle of the package list where this string will
                                 be added.
  @param  StringId               On return, contains the new strings id, which is
                                 unique within PackageList.
  @param  Language               Points to the language for the new string.
  @param  LanguageName           Points to the printable language name to associate
                                 with the passed in  Language field.If LanguageName
                                 is not NULL and the string package header's
                                 LanguageName  associated with a given Language is
                                 not zero, the LanguageName being passed  in will
                                 be ignored.
  @param  String                 Points to the new null-terminated string.
  @param  StringFontInfo         Points to the new string's font information or
                                 NULL if the string should have the default system
                                 font, size and style.

  @retval EFI_SUCCESS            The new string was added successfully.
  @retval EFI_NOT_FOUND          The specified PackageList could not be found in
                                 database.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.
  @retval EFI_INVALID_PARAMETER  String is NULL or StringId is NULL or Language is
                                 NULL.
  @retval EFI_INVALID_PARAMETER  The specified StringFontInfo does not exist in
                                 current database.

**/
EFI_STATUS
EFIAPI
HiiNewString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST CHAR8                     *Language,
  IN  CONST CHAR16                    *LanguageName, OPTIONAL
  IN  CONST EFI_STRING                String,
  IN  CONST EFI_FONT_INFO             *StringFontInfo OPTIONAL
  )
{
  EFI_STATUS                          Status;
  LIST_ENTRY                          *Link;
  BOOLEAN                             Matched;
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_RECORD                 *DatabaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  UINT32                              HeaderSize;
  UINT32                              BlockSize;
  UINT32                              OldBlockSize;
  UINT8                               *StringBlock;
  UINT8                               *BlockPtr;
  UINT32                              Ucs2BlockSize;
  UINT32                              FontBlockSize;
  UINT32                              Ucs2FontBlockSize;
  EFI_HII_SIBT_EXT2_BLOCK             Ext2;
  HII_FONT_INFO                       *LocalFont;
  HII_GLOBAL_FONT_INFO                *GlobalFont;

  if (This == NULL || String == NULL || StringId == NULL || Language == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private    = HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  GlobalFont = NULL;

  //
  // If StringFontInfo specify a paritcular font, it should exist in current database.
  //
  if (StringFontInfo != NULL) {
    if (!IsFontInfoExisted (Private, (EFI_FONT_INFO *) StringFontInfo, NULL, NULL, &GlobalFont)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Get the matching package list.
  //
  PackageListNode = NULL;
  for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
    DatabaseRecord = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (DatabaseRecord->Handle == PackageList) {
      PackageListNode = DatabaseRecord->PackageList;
      break;
    }
  }
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Try to get the matching string package. Create a new string package when failed.
  //
  StringPackage = NULL;
  Matched       = FALSE;
  for (Link = PackageListNode->StringPkgHdr.ForwardLink;
       Link != &PackageListNode->StringPkgHdr;
       Link = Link->ForwardLink
      ) {
    StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
    if (R8_EfiLibCompareLanguage (StringPackage->StringPkgHdr->Language, (CHAR8 *) Language)) {
      Matched = TRUE;
      break;
    }
  }

  if (!Matched) {
    //
    // LanguageName is required to create a new string package.
    //
    if (LanguageName == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    StringPackage = AllocateZeroPool (sizeof (HII_STRING_PACKAGE_INSTANCE));
    if (StringPackage == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    StringPackage->Signature = HII_STRING_PACKAGE_SIGNATURE;
    StringPackage->FontId    = 0;
    InitializeListHead (&StringPackage->FontInfoList);

    //
    // Fill in the string package header
    //
    HeaderSize = (UINT32) (AsciiStrSize ((CHAR8 *) Language) - 1 + sizeof (EFI_HII_STRING_PACKAGE_HDR));
    StringPackage->StringPkgHdr = AllocateZeroPool (HeaderSize);
    if (StringPackage->StringPkgHdr == NULL) {
      FreePool (StringPackage);
      return EFI_OUT_OF_RESOURCES;
    }
    StringPackage->StringPkgHdr->Header.Type      = EFI_HII_PACKAGE_STRINGS;
    StringPackage->StringPkgHdr->HdrSize          = HeaderSize;
    StringPackage->StringPkgHdr->StringInfoOffset = HeaderSize;
    CopyMem (StringPackage->StringPkgHdr->LanguageWindow, mLanguageWindow, 16 * sizeof (CHAR16));;
    StringPackage->StringPkgHdr->LanguageName     = 1;
    AsciiStrCpy (StringPackage->StringPkgHdr->Language, (CHAR8 *) Language);

    //
    // Calculate the length of the string blocks, including string block to record
    // printable language full name and EFI_HII_SIBT_END_BLOCK.
    //
    Ucs2BlockSize = (UINT32) (StrSize ((CHAR16 *) LanguageName) +
                              sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK) - sizeof (CHAR16));

    BlockSize     = Ucs2BlockSize + sizeof (EFI_HII_SIBT_END_BLOCK);
    StringPackage->StringBlock = (UINT8 *) AllocateZeroPool (BlockSize);
    if (StringPackage->StringBlock == NULL) {
      FreePool (StringPackage->StringPkgHdr);
      FreePool (StringPackage);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Insert the string block of printable language full name
    //
    BlockPtr  = StringPackage->StringBlock;
    *BlockPtr = EFI_HII_SIBT_STRING_UCS2;
    BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
    CopyMem (BlockPtr, (EFI_STRING) LanguageName, StrSize ((EFI_STRING) LanguageName));
    BlockPtr += StrSize ((EFI_STRING) LanguageName);

    //
    // Insert the end block
    //
    *BlockPtr = EFI_HII_SIBT_END;

    //
    // Append this string package node to string package array in this package list.
    //
    StringPackage->StringPkgHdr->Header.Length    = HeaderSize + BlockSize;
    PackageListNode->PackageListHdr.PackageLength += StringPackage->StringPkgHdr->Header.Length;
    InsertTailList (&PackageListNode->StringPkgHdr, &StringPackage->StringEntry);

  }

  //
  // Create a string block and corresponding font block if exists, then append them
  // to the end of the string package.
  //
  Status = FindStringBlock (
             Private,
             StringPackage,
             0,
             NULL,
             NULL,
             NULL,
             StringId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;

  if (StringFontInfo == NULL) {
    //
    // Create a EFI_HII_SIBT_STRING_UCS2_BLOCK since font info is not specified.
    //

    Ucs2BlockSize = (UINT32) (StrSize (String) + sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK)
                              - sizeof (CHAR16));

    StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + Ucs2BlockSize);
    if (StringBlock == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Copy original string blocks, except the EFI_HII_SIBT_END.
    //
    CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));
    //
    // Create a EFI_HII_SIBT_STRING_UCS2 block
    //
    BlockPtr  = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);
    *BlockPtr = EFI_HII_SIBT_STRING_UCS2;
    BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
    CopyMem (BlockPtr, (EFI_STRING) String, StrSize ((EFI_STRING) String));
    BlockPtr += StrSize ((EFI_STRING) String);

    //
    // Append a EFI_HII_SIBT_END block to the end.
    //
    *BlockPtr = EFI_HII_SIBT_END;
    FreePool (StringPackage->StringBlock);
    StringPackage->StringBlock = StringBlock;
    StringPackage->StringPkgHdr->Header.Length += Ucs2BlockSize;
    PackageListNode->PackageListHdr.PackageLength += Ucs2BlockSize;

  } else {
    //
    // StringFontInfo is specified here. If there is a EFI_HII_SIBT_FONT_BLOCK
    // which refers to this font info, create a EFI_HII_SIBT_STRING_UCS2_FONT block
    // only. Otherwise create a EFI_HII_SIBT_FONT block with a EFI_HII_SIBT_STRING
    // _UCS2_FONT block.
    //
    Ucs2FontBlockSize = (UINT32) (StrSize (String) + sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK) -
                                  sizeof (CHAR16));
    if (ReferFontInfoLocally (Private, StringPackage, StringPackage->FontId, FALSE, GlobalFont, &LocalFont)) {
      //
      // Create a EFI_HII_SIBT_STRING_UCS2_FONT block only.
      //
      StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + Ucs2FontBlockSize);
      if (StringBlock == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Copy original string blocks, except the EFI_HII_SIBT_END.
      //
      CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));
      //
      // Create a EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK
      //
      BlockPtr  = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);
      *BlockPtr = EFI_HII_SIBT_STRING_UCS2_FONT;
      BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
      *BlockPtr = LocalFont->FontId;
      BlockPtr  += sizeof (UINT8);
      CopyMem (BlockPtr, (EFI_STRING) String, StrSize ((EFI_STRING) String));
      BlockPtr += StrSize ((EFI_STRING) String);

      //
      // Append a EFI_HII_SIBT_END block to the end.
      //
      *BlockPtr = EFI_HII_SIBT_END;
      FreePool (StringPackage->StringBlock);
      StringPackage->StringBlock = StringBlock;
      StringPackage->StringPkgHdr->Header.Length += Ucs2FontBlockSize;
      PackageListNode->PackageListHdr.PackageLength += Ucs2FontBlockSize;

    } else {
      //
      // EFI_HII_SIBT_FONT_BLOCK does not exist in current string package, so
      // create a EFI_HII_SIBT_FONT block to record the font info, then generate
      // a EFI_HII_SIBT_STRING_UCS2_FONT block to record the incoming string.
      //
      FontBlockSize = (UINT32) (StrSize (((EFI_FONT_INFO *) StringFontInfo)->FontName) +
                                sizeof (EFI_HII_SIBT_FONT_BLOCK) - sizeof (CHAR16));
      StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + FontBlockSize + Ucs2FontBlockSize);
      if (StringBlock == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      //
      // Copy original string blocks, except the EFI_HII_SIBT_END.
      //
      CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));

      //
      // Create a EFI_HII_SIBT_FONT block firstly and then backup its info in string
      // package instance for future reference.
      //
      BlockPtr = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);

      Ext2.Header.BlockType = EFI_HII_SIBT_EXT2;
      Ext2.BlockType2       = EFI_HII_SIBT_FONT;
      Ext2.Length           = (UINT16) FontBlockSize;
      CopyMem (BlockPtr, &Ext2, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
      BlockPtr += sizeof (EFI_HII_SIBT_EXT2_BLOCK);

      *BlockPtr = LocalFont->FontId;
      BlockPtr += sizeof (UINT8);
      CopyMem (BlockPtr, &((EFI_FONT_INFO *) StringFontInfo)->FontSize, sizeof (UINT16));
      BlockPtr += sizeof (UINT16);
      CopyMem (BlockPtr, &((EFI_FONT_INFO *) StringFontInfo)->FontStyle, sizeof (EFI_HII_FONT_STYLE));
      BlockPtr += sizeof (EFI_HII_FONT_STYLE);
      CopyMem (
        BlockPtr,
        &((EFI_FONT_INFO *) StringFontInfo)->FontName,
        StrSize (((EFI_FONT_INFO *) StringFontInfo)->FontName)
        );

      //
      // Create a EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK
      //
      *BlockPtr = EFI_HII_SIBT_STRING_UCS2_FONT;
      BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
      *BlockPtr = LocalFont->FontId;
      BlockPtr  += sizeof (UINT8);
      CopyMem (BlockPtr, (EFI_STRING) String, StrSize ((EFI_STRING) String));
      BlockPtr += StrSize ((EFI_STRING) String);

      //
      // Append a EFI_HII_SIBT_END block to the end.
      //
      *BlockPtr = EFI_HII_SIBT_END;
      FreePool (StringPackage->StringBlock);
      StringPackage->StringBlock = StringBlock;
      StringPackage->StringPkgHdr->Header.Length += FontBlockSize + Ucs2FontBlockSize;
      PackageListNode->PackageListHdr.PackageLength += FontBlockSize + Ucs2FontBlockSize;

      //
      // Increase the FontId to make it unique since we already add 
      // a EFI_HII_SIBT_FONT block to this string package.
      //
      StringPackage->FontId++;
    }
  }

  return EFI_SUCCESS;
}


/**
  This function retrieves the string specified by StringId which is associated
  with the specified PackageList in the language Language and copies it into
  the buffer specified by String.

  @param  This                   A pointer to the EFI_HII_STRING_PROTOCOL instance.
  @param  Language               Points to the language for the retrieved string.
  @param  PackageList            The package list in the HII database to search for
                                 the  specified string.
  @param  StringId               The string's id, which is unique within
                                 PackageList.
  @param  String                 Points to the new null-terminated string.
  @param  StringSize             On entry, points to the size of the buffer pointed
                                 to by  String, in bytes. On return, points to the
                                 length of the string, in bytes.
  @param  StringFontInfo         If not NULL, points to the string's font
                                 information.  It's caller's responsibility to free
                                 this buffer.

  @retval EFI_SUCCESS            The string was returned successfully.
  @retval EFI_NOT_FOUND          The string specified by StringId is not available.
  @retval EFI_NOT_FOUND          The string specified by StringId is available but
                                                not in the specified language.
                                                The specified PackageList is not in the database.
  @retval EFI_INVALID_LANGUAGE   - The string specified by StringId is available but
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by StringSize is too small to
                                  hold the string.
  @retval EFI_INVALID_PARAMETER  The String or Language or StringSize was NULL.
  @retval EFI_OUT_OF_RESOURCES   There were insufficient resources to complete the
                                 request.

**/
EFI_STATUS
EFIAPI
HiiGetString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  CONST CHAR8                     *Language,
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize,
  OUT EFI_FONT_INFO                   **StringFontInfo OPTIONAL
  )
{
  EFI_STATUS                          Status;
  LIST_ENTRY                          *Link;
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_RECORD                 *DatabaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;

  if (This == NULL || Language == NULL || StringId < 1 || StringSize == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (String == NULL && *StringSize != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private = HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  PackageListNode = NULL;

  for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
    DatabaseRecord = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (DatabaseRecord->Handle == PackageList) {
      PackageListNode = DatabaseRecord->PackageList;
      break;
    }
  }

  if (PackageListNode != NULL) {
    //
    // First search: to match the StringId in the specified language.
    //
    for (Link =  PackageListNode->StringPkgHdr.ForwardLink;
         Link != &PackageListNode->StringPkgHdr;
         Link =  Link->ForwardLink
        ) {
        StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
        if (R8_EfiLibCompareLanguage (StringPackage->StringPkgHdr->Language, (CHAR8 *) Language)) {
          Status = GetStringWorker (Private, StringPackage, StringId, String, StringSize, StringFontInfo);
          if (Status != EFI_NOT_FOUND) {
            return Status;
          }
        }
      }
      //
      // Second search: to match the StringId in other available languages if exist.
      //
      for (Link =  PackageListNode->StringPkgHdr.ForwardLink; 
           Link != &PackageListNode->StringPkgHdr;
           Link =  Link->ForwardLink
          ) {
      StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);      
      Status = GetStringWorker (Private, StringPackage, StringId, String, StringSize, StringFontInfo);
      if (!EFI_ERROR (Status)) {
        return EFI_INVALID_LANGUAGE;
      }
    }    
  }

  return EFI_NOT_FOUND;
}



/**
  This function updates the string specified by StringId in the specified PackageList to the text
  specified by String and, optionally, the font information specified by StringFontInfo.

  @param  This                   A pointer to the EFI_HII_STRING_PROTOCOL instance.
  @param  PackageList            The package list containing the strings.
  @param  StringId               The string's id, which is unique within
                                 PackageList.
  @param  Language               Points to the language for the updated string.
  @param  String                 Points to the new null-terminated string.
  @param  StringFontInfo         Points to the string's font information or NULL if
                                 the  string font information is not changed.

  @retval EFI_SUCCESS            The string was updated successfully.
  @retval EFI_NOT_FOUND          The string specified by StringId is not in the
                                 database.
  @retval EFI_INVALID_PARAMETER  The String or Language was NULL.
  @retval EFI_INVALID_PARAMETER  The specified StringFontInfo does not exist in
                                 current database.
  @retval EFI_OUT_OF_RESOURCES   The system is out of resources to accomplish the
                                 task.

**/
EFI_STATUS
EFIAPI
HiiSetString (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_STRING_ID                    StringId,
  IN CONST CHAR8                      *Language,
  IN CONST EFI_STRING                 String,
  IN CONST EFI_FONT_INFO              *StringFontInfo OPTIONAL
  )
{
  EFI_STATUS                          Status;
  LIST_ENTRY                          *Link;
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_RECORD                 *DatabaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  UINT32                              OldPackageLen;

  if (This == NULL || Language == NULL || StringId < 1 || String == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private = HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  PackageListNode = NULL;

  for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
    DatabaseRecord = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (DatabaseRecord->Handle == PackageList) {
      PackageListNode = (HII_DATABASE_PACKAGE_LIST_INSTANCE *) (DatabaseRecord->PackageList);
    }
  }

  if (PackageListNode != NULL) {
    for (Link =  PackageListNode->StringPkgHdr.ForwardLink;
         Link != &PackageListNode->StringPkgHdr;
         Link =  Link->ForwardLink
        ) {
      StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
      if (R8_EfiLibCompareLanguage (StringPackage->StringPkgHdr->Language, (CHAR8 *) Language)) {
        OldPackageLen = StringPackage->StringPkgHdr->Header.Length;
        Status = SetStringWorker (
                   Private,
                   StringPackage,
                   StringId,
                   (EFI_STRING) String,
                   (EFI_FONT_INFO *) StringFontInfo
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
        PackageListNode->PackageListHdr.PackageLength += StringPackage->StringPkgHdr->Header.Length - OldPackageLen;
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}



/**
  This function returns the list of supported languages, in the format specified
  in Appendix M of UEFI 2.1 spec.

  @param  This                   A pointer to the EFI_HII_STRING_PROTOCOL instance.
  @param  PackageList            The package list to examine.
  @param  Languages              Points to the buffer to hold the returned string.
  @param  LanguagesSize          On entry, points to the size of the buffer pointed
                                 to by  Languages, in bytes. On  return, points to
                                 the length of Languages, in bytes.

  @retval EFI_SUCCESS            The languages were returned successfully.
  @retval EFI_INVALID_PARAMETER  The Languages or LanguagesSize was NULL.
  @retval EFI_BUFFER_TOO_SMALL   The LanguagesSize is too small to hold the list of
                                  supported languages. LanguageSize is updated to
                                 contain the required size.
  @retval EFI_NOT_FOUND          Could not find string package in specified
                                 packagelist.

**/
EFI_STATUS
EFIAPI
HiiGetLanguages (
  IN CONST EFI_HII_STRING_PROTOCOL    *This,
  IN EFI_HII_HANDLE                   PackageList,
  IN OUT CHAR8                        *Languages,
  IN OUT UINTN                        *LanguagesSize
  )
{
  LIST_ENTRY                          *Link;
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_RECORD                 *DatabaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  UINTN                               ResultSize;

  if (This == NULL || Languages == NULL || LanguagesSize == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private = HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  PackageListNode = NULL;
  for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
    DatabaseRecord  = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (DatabaseRecord->Handle == PackageList) {
      PackageListNode = DatabaseRecord->PackageList;
      break;
    }
  }
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Search the languages in the specified packagelist.
  //
  ResultSize = 0;
  for (Link = PackageListNode->StringPkgHdr.ForwardLink;
       Link != &PackageListNode->StringPkgHdr;
       Link = Link->ForwardLink
      ) {
    StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
    ResultSize += AsciiStrSize (StringPackage->StringPkgHdr->Language);
    if (ResultSize < *LanguagesSize) {
      AsciiStrCpy (Languages, StringPackage->StringPkgHdr->Language);
      Languages += AsciiStrSize (StringPackage->StringPkgHdr->Language);
      *(Languages - 1) = L';';
    }
  }
  if (ResultSize == 0) {
    return EFI_NOT_FOUND;
  }

  if (*LanguagesSize < ResultSize) {
    *LanguagesSize = ResultSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *(Languages - 1) = 0;
  return EFI_SUCCESS;
}


/**
  Each string package has associated with it a single primary language and zero
  or more secondary languages. This routine returns the secondary languages
  associated with a package list.

  @param  This                   A pointer to the EFI_HII_STRING_PROTOCOL instance.
  @param  PackageList            The package list to examine.
  @param  FirstLanguage          Points to the primary language.
  @param  SecondaryLanguages     Points to the buffer to hold the returned list of
                                 secondary languages for the specified
                                 FirstLanguage. If there are no secondary
                                 languages, the function  returns successfully, but
                                 this is set to NULL.
  @param  SecondaryLanguagesSize On entry, points to the size of the buffer pointed
                                 to  by SecondaryLanguages, in bytes. On return,
                                 points to the length of SecondaryLanguages in bytes.

  @retval EFI_SUCCESS            Secondary languages were correctly returned.
  @retval EFI_INVALID_PARAMETER  FirstLanguage or SecondaryLanguages or
                                 SecondaryLanguagesSize was NULL.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by SecondaryLanguagesSize is
                                 too small to hold the returned information.
                                 SecondLanguageSize is updated to hold the size of
                                 the buffer required.
  @retval EFI_INVALID_LANGUAGE           The language specified by FirstLanguage is not
                                  present in the specified package list.
  @retval EFI_NOT_FOUND          The specified PackageList is not in the Database.                                

**/
EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN CONST EFI_HII_STRING_PROTOCOL   *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN CONST CHAR8                     *FirstLanguage,
  IN OUT CHAR8                       *SecondaryLanguages,
  IN OUT UINTN                       *SecondaryLanguagesSize
  )
{
  LIST_ENTRY                          *Link;
  LIST_ENTRY                          *Link1;
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_RECORD                 *DatabaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  CHAR8                               *Languages;
  UINTN                               ResultSize;

  if (This == NULL || PackageList == NULL || FirstLanguage == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (SecondaryLanguages == NULL || SecondaryLanguagesSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private    = HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  PackageListNode = NULL;     
  for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
    DatabaseRecord  = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (DatabaseRecord->Handle == PackageList) {
      PackageListNode = (HII_DATABASE_PACKAGE_LIST_INSTANCE *) (DatabaseRecord->PackageList);
        break;
      }
    }
    if (PackageListNode == NULL) {
      return EFI_NOT_FOUND;
    }
      
    Languages  = NULL;
    ResultSize = 0;
    for (Link1 = PackageListNode->StringPkgHdr.ForwardLink;
         Link1 != &PackageListNode->StringPkgHdr;
         Link1 = Link1->ForwardLink
        ) {
    StringPackage = CR (Link1, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
    if (R8_EfiLibCompareLanguage (StringPackage->StringPkgHdr->Language, (CHAR8 *) FirstLanguage)) {
      Languages = StringPackage->StringPkgHdr->Language;
      //
      // Language is a series of ';' terminated strings, first one is primary
      // language and following with other secondary languages or NULL if no
      // secondary languages any more.
      //
      Languages = AsciiStrStr (Languages, ";");
      if (Languages == NULL) {
        break;
      }
      Languages++;

      ResultSize = AsciiStrSize (Languages);
      if (ResultSize <= *SecondaryLanguagesSize) {
        AsciiStrCpy (SecondaryLanguages, Languages);
      } else {
        *SecondaryLanguagesSize = ResultSize;
        return EFI_BUFFER_TOO_SMALL;
      }

      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_LANGUAGE;
}

