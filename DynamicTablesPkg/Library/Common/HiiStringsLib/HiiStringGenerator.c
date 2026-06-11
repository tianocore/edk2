/** @file
  Dynamic Hii String and String Package generation API functions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "InternalHiiStringsLib.h"

/** Get the length occupied by all the strings in the string package.

  Add up the length of all the strings to be added in the string package,
  along with the block header(EFI_HII_SIBT_STRING_UCS2) length.

  @param [in]  StrList           List of strings.

  @retval Total length of all the strings in the package, including their
          respective block headers(EFI_HII_SIBT_STRING_UCS2).

**/
static
UINTN
DynHiiGetStrLength (
  IN   CONST LIST_ENTRY  *StrList
  )
{
  UINTN             StrLen;
  LIST_ENTRY        *List;
  DYN_HII_STR_INFO  *StrInfo;

  StrLen = 0;
  List   = GetFirstNode (StrList);
  while (!IsNull (StrList, List)) {
    StrInfo = BASE_CR (List, DYN_HII_STR_INFO, Link);

    StrLen += StrInfo->Length;

    List = GetNextNode (StrList, List);
  }

  return StrLen;
}

/** Copy all the strings into the string package.

  Copy all the added strings to the string package. Each string is prefixed
  with EFI_HII_SIBT_STRING_UCS2.

  @param [in]  StrList           List of strings to be added.
  @param [in]  StrPkg            Pointer to the string package where the
                                 strings are to be added.

  @retval Pointer to the location where the EFI_HII_SIBT_END_BLOCK would
          be added.

**/
static
UINT8 *
DynHiiSetStrings (
  IN   CONST LIST_ENTRY  *StrList,
  IN   UINT8             *StrPkg
  )
{
  LIST_ENTRY        *List;
  DYN_HII_STR_INFO  *StrInfo;

  List = GetFirstNode (StrList);
  while (!IsNull (StrList, List)) {
    StrInfo = BASE_CR (List, DYN_HII_STR_INFO, Link);

    CopyMem (StrPkg, StrInfo->String, StrInfo->Length);

    StrPkg += StrInfo->Length;

    List = GetNextNode (StrList, List);
  }

  return StrPkg;
}

/** Set the EFI_HII_SIBT_END_BLOCK in the string package.

  Set the EFI_HII_SIBT_END_BLOCK in the string package once all the
  strings have been added to the package.

  @param [in]  StrPkg           Pointer to the string package.

**/
static
VOID
DynHiiSetEndBlock (
  IN   UINT8  *StrPkg
  )
{
  EFI_HII_STRING_BLOCK  *Block;

  Block = (EFI_HII_STRING_BLOCK *)StrPkg;

  Block->BlockType = EFI_HII_SIBT_END;
}

/** Get the size of null terminated unicode string in bytes

  Returns the size of a possibly unaligned null terminated unicode
  string in bytes, including the null-terminator.

  @param [in]  String           Pointer to the unicode string.

  @retval  Size of the string.

**/
static
UINT32
DynHiiGetStrSize (
  IN  CONST CHAR16  *String
  )
{
  UINT32  StrLen;
  CHAR16  Str;

  for (StrLen = 1; (Str = ReadUnaligned16 (String++)) != L'\0'; StrLen++) {
  }

  return StrLen * sizeof (CHAR16);
}

/** Dump the contents of the end skip2 block.

  Dump the contents of a EFI_HII_SIBT_SKIP2_BLOCK string block.

  @param [in]  Buf               Pointer to the string block.

**/
static
VOID
DynHiiDumpSkip2Block (
  IN   UINT8  *Buf
  )
{
  UINT32  Idx;
  UINT32  BlockSize;

  BlockSize = sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
  for (Idx = 0; Idx < BlockSize; Idx++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "0x%02x, ",
      *Buf++
      ));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}

/** Dump the contents of the end string block.

  Dump the contents of a EFI_HII_SIBT_END_BLOCK string block.

  @param [in]  Buf               Pointer to the string block.

**/
static
VOID
DynHiiDumpEndBlock (
  IN   UINT8  *Buf
  )
{
  UINT32  Idx;
  UINT32  BlockSize;

  BlockSize = sizeof (EFI_HII_SIBT_END_BLOCK);
  for (Idx = 0; Idx < BlockSize; Idx++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "0x%02x, ",
      *Buf++
      ));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}

/** Dump the contents of a UCS2 string block.

  Dump the contents of a UCS2 string block.

  @param [in]  Buf               Pointer to the string block.
  @param [in]  StrLen            Length of the string associated
                                 with the block.

**/
static
VOID
DynHiiDumpUcs2Block (
  IN   UINT8   *Buf,
  IN   UINT32  StrLen
  )
{
  UINT32  Idx;
  UINT32  Idx1;

  DEBUG ((
    DEBUG_VERBOSE,
    "0x%02x, ",
    *Buf++
    ));

  for (Idx = 0; Idx < StrLen;) {
    for (Idx1 = 0; Idx < StrLen && Idx1 < 16; Idx1++, Idx++) {
      DEBUG ((
        DEBUG_VERBOSE,
        "0x%02x, ",
        *Buf++
        ));
    }
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}

/** Dump the contents of a String package.

  Dump the contents of a string package. Useful for debugging.

  @param [in]  StrPkg            Pointer to the string package.
  @param [in]  StrPkgLen         Length of the string package.

  @retval  EFI_INVALID_PARAMETER  If any of the checks fail.
  @retval  EFI_SUCCESS            No issues found.

**/
static
EFI_STATUS
DynHiiDumpStrPkg (
  IN          UINT8   *StrPkg,
  IN          UINT32  StrPkgLen
  )
{
  UINT8                 *Buf;
  UINT32                Idx;
  UINT32                Idx1;
  UINT32                HdrLen;
  UINT32                StrLen;
  EFI_STATUS            Status;
  EFI_HII_STRING_BLOCK  *StrBlock;

  Buf    = StrPkg;
  Status = EFI_SUCCESS;
  HdrLen = ReadUnaligned32 (Buf + sizeof (EFI_HII_PACKAGE_HEADER));

  for (Idx = 0; Idx < HdrLen;) {
    for (Idx1 = 0; Idx < HdrLen && Idx1 < 16; Idx1++, Idx++) {
      DEBUG ((
        DEBUG_VERBOSE,
        "0x%02x, ",
        *Buf++
        ));
    }

    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "\n\n"));

  while (StrPkgLen != 0) {
    StrBlock = (EFI_HII_STRING_BLOCK *)Buf;

    switch (StrBlock->BlockType) {
      case EFI_HII_SIBT_SKIP2:
        DynHiiDumpSkip2Block (Buf);
        Buf       += sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
        StrPkgLen -= sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
        break;

      case EFI_HII_SIBT_STRING_UCS2:
        StrLen = DynHiiGetStrSize (
                   (CHAR16 *)(Buf + sizeof (EFI_HII_STRING_BLOCK))
                   );
        DynHiiDumpUcs2Block (Buf, StrLen);
        Buf       += StrLen + sizeof (EFI_HII_STRING_BLOCK);
        StrPkgLen -= StrLen + sizeof (EFI_HII_STRING_BLOCK);
        break;

      case EFI_HII_SIBT_END:
        DynHiiDumpEndBlock (Buf);
        Buf       += sizeof (EFI_HII_SIBT_END_BLOCK);
        StrPkgLen -= sizeof (EFI_HII_SIBT_END_BLOCK);
        break;

      default:
        DEBUG ((DEBUG_VERBOSE, "Received Invalid String Block Type.\n"));
        Status = EFI_INVALID_PARAMETER;
        goto out;
    }
  }

out:
  DEBUG ((
    DEBUG_VERBOSE,
    "};\n\n"
    ));

  return Status;
}

/** Dump the contents of the String buffer.

  Dump the contents of all the string packages. Useful for debugging.

  @param [in]  StrPkg            Pointer to the string package array.
  @param [in]  StrPkgLen         Length of the string package array.

  @retval  EFI_INVALID_PARAMETER  If any of the checks fail.
  @retval  EFI_SUCCESS            No issues found.

**/
static
EFI_STATUS
DynHiiDumpStrPkgBuf (
  IN          UINT8   *StrPkg,
  IN          UINT32  StrPkgLen
  )
{
  UINT8       *Buf;
  UINT32      Idx;
  UINT32      PkgLen;
  EFI_STATUS  Status;

  if ((StrPkg == NULL) || (StrPkgLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "INFO: StrPkg => %p, StrPkgLen => 0x%x\n",
    StrPkg,
    StrPkgLen
    ));

  DEBUG ((
    DEBUG_VERBOSE,
    "***** Generated String Package *****\n"
    ));

  DEBUG ((
    DEBUG_VERBOSE,
    "StrByteStream[] = {\n"
    ));

  Buf = StrPkg;
  for (Idx = 0; Idx < 4; Idx++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "0x%02x, ",
      *Buf++
      ));
  }

  DEBUG ((DEBUG_VERBOSE, "\n\n"));

  while (StrPkgLen != 0) {
    PkgLen = ReadUnaligned24 (Buf);

    if (StrPkgLen < PkgLen) {
      return EFI_INVALID_PARAMETER;
    }

    Status = DynHiiDumpStrPkg (Buf, PkgLen);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Buf       += PkgLen;
    StrPkgLen -= PkgLen;
  }

  return EFI_SUCCESS;
}

/** Get number of string packages in the sting package array.

  The string package array may consists of multiple string packages, each
  associated with a language-code. Count the number of such string packages
  in the array, and perform basic checks on the string packages, like they
  actually are string packages.

  @param [in]  StrPkgArr         Array of string packages.
  @param [out] NumStrPkg         Number of string packages found in the
                                 array.

  @retval  EFI_INVALID_PARAMETER  If any of the checks fail.
  @retval  EFI_SUCCESS            No issues found.

**/
static
EFI_STATUS
DynHiiGetNumStrPkg (
  IN   CONST   UINT8   *StrPkgArr,
  OUT          UINT32  *NumStrPkg
  )
{
  CONST UINT8  *StrArr;
  UINT8        PkgType;
  UINT32       StrArrLen;
  UINT32       PkgLen;
  UINT32       TotalPkgLen;
  UINT32       ReadPkgLen;
  UINT32       PkgHdr;

  ReadPkgLen = ReadUnaligned32 (StrPkgArr);
  StrArrLen  = ReadPkgLen - sizeof (UINT32);

  // The string package array generated by the
  // AutoGen tool consists of a 4 byte array
  // length member which has to be discarded.
  StrArr = StrPkgArr + sizeof (UINT32);

  *NumStrPkg  = 0;
  TotalPkgLen = sizeof (UINT32);

  while (StrArrLen != 0) {
    PkgHdr  = ReadUnaligned32 (StrArr);
    PkgType = (PkgHdr & 0xFF000000) >> 24;
    PkgLen  = PkgHdr & 0x00FFFFFF;

    if (PkgType != EFI_HII_PACKAGE_STRINGS) {
      return EFI_INVALID_PARAMETER;
    }

    if (StrArrLen < PkgLen) {
      return EFI_INVALID_PARAMETER;
    }

    StrArrLen   -= PkgLen;
    StrArr      += PkgLen;
    TotalPkgLen += PkgLen;

    (*NumStrPkg)++;
  }

  if (ReadPkgLen != TotalPkgLen) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Build a list of strings under a given string package

  The string package consists of multiple string blocks. The supported
  types of string blocks are EFI_HII_SIBT_STRING_UCS2, EFI_HII_SIBT_SKIP2
  and EFI_HII_SIBT_END. This function iterates through the existing string
  package, and splits up the strings into a list of string blocks. The
  EFI_HII_SIBT_END is not added to the list.

  @param [in]  StrArr            String package array.
  @param [in]  StrPkgInstance    An instance of a string package.

  @retval  EFI_UNSUPPORTED        Unsupported string block type.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.
  @retval  EFI_SUCCESS            The string list was built successfully.

**/
static
EFI_STATUS
DynHiiGenStringList (
  IN CONST   UINT8                     *StrArr,
  IN         DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance
  )
{
  UINT32                          AllocLen;
  UINT32                          PkgLen;
  UINT32                          HdrLen;
  CONST UINT8                     *StrOffset;
  EFI_STRING_ID                   StringId;
  DYN_HII_STR_INFO                *StrInfo;
  CONST EFI_HII_STRING_BLOCK      *StrBlock;
  CONST EFI_HII_SIBT_SKIP2_BLOCK  *Skip2Block;

  PkgLen   = ReadUnaligned24 (StrArr);
  HdrLen   = ReadUnaligned32 (StrArr + sizeof (EFI_HII_PACKAGE_HEADER));
  StringId = 0;

  PkgLen   -= HdrLen;
  StrOffset = StrArr + HdrLen;

  while (PkgLen != 0) {
    StrBlock = (CONST EFI_HII_STRING_BLOCK *)StrOffset;
    if ((StrBlock->BlockType != EFI_HII_SIBT_STRING_UCS2) &&
        (StrBlock->BlockType != EFI_HII_SIBT_SKIP2) &&
        (StrBlock->BlockType != EFI_HII_SIBT_END))
    {
      return EFI_UNSUPPORTED;
    }

    if (StrBlock->BlockType == EFI_HII_SIBT_SKIP2) {
      AllocLen   = sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
      Skip2Block = (CONST EFI_HII_SIBT_SKIP2_BLOCK *)StrOffset;
      StringId  += Skip2Block->SkipCount;
    } else if (StrBlock->BlockType == EFI_HII_SIBT_END) {
      PkgLen -= sizeof (EFI_HII_SIBT_END_BLOCK);
      continue;
    } else {
      AllocLen = DynHiiGetStrSize (
                   (CONST CHAR16 *)(StrOffset +
                                    sizeof (EFI_HII_STRING_BLOCK))
                   ) +
                 sizeof (EFI_HII_STRING_BLOCK);
      StringId++;
    }

    StrInfo = AllocateZeroPool (sizeof (*StrInfo));
    if (StrInfo == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    StrInfo->String = AllocateZeroPool (AllocLen);
    if (StrInfo->String == NULL) {
      FreePool (StrInfo);
      return EFI_OUT_OF_RESOURCES;
    }

    StrInfo->StringId = StringId;
    StrInfo->Length   = AllocLen;
    CopyMem (StrInfo->String, StrOffset, AllocLen);
    StrPkgInstance->MaxStringId = StringId;

    InitializeListHead (&StrInfo->Link);
    InsertTailList (&StrPkgInstance->StringList, &StrInfo->Link);

    StrOffset += AllocLen;
    PkgLen    -= AllocLen;
  }

  return EFI_SUCCESS;
}

/** Initialize a string package instance.

  The string package array may contain multiple string packages, each
  associated with a language-code. Information about each such string
  package is stored in a string instance structure. Initialize and
  populate a string instance for a string package. Add the string
  instance to the list of string instance list.

  @param [in]  PkgIdx            Index of theString package.
  @param [in]  StrPkgArr         Array of string packages.
  @param [in]  StrPkg            Top-level string package information
                                 structure.

  @retval  EFI_UNSUPPORTED        Unsupported string block type.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.
  @retval  EFI_SUCCESS            The string list was built successfully.

**/
static
EFI_STATUS
DynHiiInitStrPkgInstance (
  IN          UINT32           PkgIdx,
  IN  CONST   UINT8            *StrPkgArr,
  IN          DYN_HII_STR_PKG  *StrPkg
  )
{
  CONST UINT8               *StrArr;
  UINT32                    Idx;
  UINT32                    PkgLen;
  UINT32                    HdrLen;
  UINT32                    LangLen;
  DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance;

  // The string package array generated by the
  // AutoGen tool consists of a 4 byte array
  // length member which has to be discarded.
  StrArr = StrPkgArr + sizeof (UINT32);

  for (Idx = 0; Idx < PkgIdx; Idx++) {
    PkgLen  = ReadUnaligned24 (StrArr);
    StrArr += PkgLen;
  }

  StrPkgInstance = AllocateZeroPool (sizeof (*StrPkgInstance));
  if (StrPkgInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrPkgInstance->MaxStringId = 0;
  InitializeListHead (&StrPkgInstance->StringList);
  InitializeListHead (&StrPkgInstance->Link);
  InsertTailList (&StrPkg->StrPkgList, &StrPkgInstance->Link);

  HdrLen                    = ReadUnaligned32 (StrArr + sizeof (EFI_HII_PACKAGE_HEADER));
  StrPkgInstance->StrPkgHdr = AllocateZeroPool (HdrLen);
  if (StrPkgInstance->StrPkgHdr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (StrPkgInstance->StrPkgHdr, StrArr, HdrLen);

  LangLen                      = HdrLen - OFFSET_OF (EFI_HII_STRING_PACKAGE_HDR, Language);
  StrPkgInstance->LanguageCode = AllocateZeroPool (LangLen);
  if (StrPkgInstance->LanguageCode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (
    StrPkgInstance->LanguageCode,
    LangLen,
    (CONST CHAR8 *)StrArr + OFFSET_OF (EFI_HII_STRING_PACKAGE_HDR, Language)
    );

  return DynHiiGenStringList (StrArr, StrPkgInstance);
}

/** Check if a language is supported in the string package list

  Check the list of string packages for a given language support.

  @param [in]  StrPkg            Top-level string package information
                                 structure.
  @param [in]  Language          Language to check for.

  @retval TRUE if the language is supported, FALSE otherwise.

**/
static
BOOLEAN
DynHiiIsLanguageSupported (
  IN  CONST  DYN_HII_STR_PKG  *StrPkg,
  IN  CONST  CHAR8            *Language
  )
{
  LIST_ENTRY                *List;
  DYN_HII_STR_PKG_INSTANCE  *Tmp;

  List = GetFirstNode (&StrPkg->StrPkgList);
  while (!IsNull (&StrPkg->StrPkgList, List)) {
    Tmp = BASE_CR (List, DYN_HII_STR_PKG_INSTANCE, Link);

    if (AsciiStrCmp (Tmp->LanguageCode, Language) == 0) {
      return TRUE;
    }

    List = GetNextNode (&StrPkg->StrPkgList, List);
  }

  return FALSE;
}

/** Append a string to the given string instance

  Append a string block, either consisting of a real string or a dummy
  EFI_HII_SIBT_STRING_UCS2 block to the list of strings under the string
  instance passed to the function.

  @param [in]  StrPkgInstance    An instance of a string package.
  @param [in]  String            A UCS2 string.
  @param [in]  DummyStr          Boolean flag indicating if a dummy
                                 string block is to be added.

  @retval  EFI_BAD_BUFFER_SIZE    Input is too large to fit under a
                                  string package.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.
  @retval  EFI_SUCCESS            The string was appended successfully.

**/
static
EFI_STATUS
EFIAPI
DynHiiStringAppend (
  IN        DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance,
  IN  CONST EFI_STRING                String,
  IN        BOOLEAN                   DummyStr
  )
{
  UINT8                           *Buf;
  UINTN                           StrBytes;
  DYN_HII_STR_INFO                *NewStr;
  EFI_HII_STRING_BLOCK            StrBlock;
  EFI_HII_SIBT_STRING_UCS2_BLOCK  *Ucs2Block;

  NewStr = AllocateZeroPool (sizeof (*NewStr));
  if (NewStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrBytes = DummyStr ? sizeof (*Ucs2Block) :
             DynHiiGetStrSize (String) + sizeof (EFI_HII_STRING_BLOCK);
  if (StrBytes > MAX_UINT32) {
    FreePool (NewStr);
    return EFI_BAD_BUFFER_SIZE;
  }

  NewStr->String = AllocateZeroPool (StrBytes);
  if (NewStr->String == NULL) {
    FreePool (NewStr);
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&NewStr->Link);
  InsertTailList (&StrPkgInstance->StringList, &NewStr->Link);
  NewStr->StringId = StrPkgInstance->MaxStringId;
  NewStr->Length   = (UINT32)StrBytes;

  if (DummyStr) {
    Ucs2Block                   = (EFI_HII_SIBT_STRING_UCS2_BLOCK *)NewStr->String;
    Ucs2Block->Header.BlockType = EFI_HII_SIBT_STRING_UCS2;
  } else {
    StrBlock.BlockType = EFI_HII_SIBT_STRING_UCS2;
    CopyMem (NewStr->String, &StrBlock, sizeof (EFI_HII_STRING_BLOCK));
    Buf = (UINT8 *)NewStr->String + sizeof (EFI_HII_STRING_BLOCK);
    CopyMem (Buf, String, StrBytes - sizeof (EFI_HII_STRING_BLOCK));
  }

  return EFI_SUCCESS;
}

/** Update the length of the string package header.

  Based on the total length of the string blocks computed, added
  to the size of EFI_HII_STRING_PACKAGE_HDR, update the string
  package size.

  @param [in]  StrPkgHdr         String package header.
  @param [in]  StrLen            Combined length of all string
                                 blocks for the string package.

  @retval  EFI_BAD_BUFFER_SIZE    Input is too large to fit under a
                                  string package.
  @retval  EFI_SUCCESS            The string package header was updated
                                  successfully.

**/
static
EFI_STATUS
DynHiiUpdateStrPkgHdrLen (
  IN  EFI_HII_STRING_PACKAGE_HDR  *StrPkgHdr,
  IN  UINT32                      StrLen
  )
{
  UINT8   *PkgHdr;
  UINT32  HdrLen;
  UINT32  PkgLen;

  PkgHdr = (UINT8 *)StrPkgHdr;
  HdrLen = ReadUnaligned32 (PkgHdr + sizeof (EFI_HII_PACKAGE_HEADER));

  PkgLen = HdrLen + StrLen;
  if (PkgLen >= SIZE_16MB) {
    return EFI_BAD_BUFFER_SIZE;
  }

  WriteUnaligned24 ((UINT8 *)StrPkgHdr, PkgLen);

  return EFI_SUCCESS;
}

/** Check if all the MaxStringId's match

  Iterate through all the string packages that have been added, and
  check that the MaxStrinId value in all the packages match.

  @param [in]  StrPkgList        List of string packages.

  @retval TRUE if the ID's match, FALSE otherwise.

**/
static
BOOLEAN
DynHiiStrIdMatch (
  IN   LIST_ENTRY  *StrPkgList
  )
{
  LIST_ENTRY                *List;
  EFI_STRING_ID             CachedStrId;
  EFI_STRING_ID             ReadStrId;
  DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance;

  CachedStrId = 0;
  ReadStrId   = 0;

  List = GetFirstNode (StrPkgList);
  while (!IsNull (StrPkgList, List)) {
    StrPkgInstance = BASE_CR (List, DYN_HII_STR_PKG_INSTANCE, Link);
    ReadStrId      = StrPkgInstance->MaxStringId;

    if (CachedStrId == 0) {
      CachedStrId = ReadStrId;
    }

    if (ReadStrId != CachedStrId) {
      return FALSE;
    }

    List = GetNextNode (StrPkgList, List);
  }

  return TRUE;
}

/** Initialize the string package structure.

  Initialize the DYN_HII_STR_PKG structure by allocating memory for
  certain members of the structure and then initializing them with data
  that were passed to the function.

  @param [in]  StrPkgArr        Existing string package array which might
                                consist of multiple language string arrays
  @param [out] StrPkgHandle     Pointer to the DYN_HII_STR_HANDLE to hold
                                the string package structure.

  @retval  EFI_SUCCESS            The structure was initialized successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiInitStringPkg (
  IN  CONST   UINT8               *StrPkgArr,
  OUT         DYN_HII_STR_HANDLE  *StrPkgHandle
  )
{
  UINT32           NumStrPkg;
  UINT32           Idx;
  EFI_STATUS       Status;
  DYN_HII_STR_PKG  *StrPkg;

  if ((StrPkgArr == NULL) || (StrPkgHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = DynHiiGetNumStrPkg (StrPkgArr, &NumStrPkg);
  if ((EFI_ERROR (Status)) || (NumStrPkg == 0)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Incorrect string package array received.\n",
      __func__
      ));
    return Status;
  }

  StrPkg = AllocateZeroPool (sizeof (DYN_HII_STR_PKG));
  if (StrPkg == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrPkg->LanguageCount = NumStrPkg;
  StrPkg->Signature     = DYN_HII_STR_PKG_SIGNATURE;
  InitializeListHead (&StrPkg->StrPkgList);

  for (Idx = 0; Idx < NumStrPkg; Idx++) {
    Status = DynHiiInitStrPkgInstance (Idx, StrPkgArr, StrPkg);
    if (EFI_ERROR (Status)) {
      goto err_out;
    }
  }

  if (!DynHiiStrIdMatch (&StrPkg->StrPkgList)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: MaxStrId mismatch found in string packages.\n",
      __func__
      ));

    Status = EFI_INVALID_PARAMETER;
    goto err_out;
  }

  *StrPkgHandle = StrPkg;
  return EFI_SUCCESS;

err_out:
  DynHiiFreeStringPackage (StrPkg);

  return Status;
}

/** Add a string to the string list.

  Add the string passed to the function to a list of strings that would be
  added to form the string package.

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.
  @param [in]   Language          Language for which string is to be added.
  @param [in]   String            The string that is to be added.
  @param [out]  StringId          String Identifier assigned for this string.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrList or String is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddString (
  IN  CONST DYN_HII_STR_HANDLE  StrPkgHandle,
  IN  CONST CHAR8               *Language,
  IN  CONST EFI_STRING          String,
  OUT       EFI_STRING_ID       *StringId
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                *List;
  DYN_HII_STR_PKG           *StrPkg;
  DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance;

  if ((StrPkgHandle == NULL) || (String == NULL) || (Language == NULL) ||
      (StringId == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  StrPkg = StrPkgHandle;
  if (StrPkg->Signature != DYN_HII_STR_PKG_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (!DynHiiIsLanguageSupported (StrPkg, Language)) {
    return EFI_NOT_FOUND;
  }

  List = GetFirstNode (&StrPkg->StrPkgList);
  while (!IsNull (&StrPkg->StrPkgList, List)) {
    StrPkgInstance = BASE_CR (List, DYN_HII_STR_PKG_INSTANCE, Link);

    StrPkgInstance->MaxStringId++;
    *StringId = StrPkgInstance->MaxStringId;

    Status = DynHiiStringAppend (
               StrPkgInstance,
               String,
               AsciiStrCmp (StrPkgInstance->LanguageCode, Language) != 0
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    List = GetNextNode (&StrPkg->StrPkgList, List);
  }

  return EFI_SUCCESS;
}

/** Generate string package.

  Generate the string package from the input parameter. The string package is
  subsequently added to the HII database through the HiiAddPackages() call.

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.
  @param [out]  StrPkgData        The generated array of string package(s).

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_ALREADY_STARTED    If StrPkgBuf is not NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateStringPackage (
  IN   DYN_HII_STR_HANDLE  StrPkgHandle,
  OUT  UINT8               **StrPkgData
  )
{
  UINT8                     *StrPkgBuf;
  UINT32                    *Arr;
  UINT32                    StrPkgLen;
  UINTN                     PkgLen;
  UINTN                     TmpLen;
  LIST_ENTRY                *List;
  EFI_STATUS                Status;
  DYN_HII_STR_PKG           *StrPkg;
  DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance;

  if ((StrPkgHandle == NULL) || (StrPkgData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  StrPkg = StrPkgHandle;
  if (StrPkg->Signature != DYN_HII_STR_PKG_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (!DynHiiStrIdMatch (&StrPkg->StrPkgList)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: MaxStrId mismatch found in string packages.\n",
      __func__
      ));

    return EFI_INVALID_PARAMETER;
  }

  TmpLen = 0;
  List   = GetFirstNode (&StrPkg->StrPkgList);
  while (!IsNull (&StrPkg->StrPkgList, List)) {
    PkgLen         = 0;
    StrPkgInstance = BASE_CR (List, DYN_HII_STR_PKG_INSTANCE, Link);

    PkgLen  = DynHiiGetStrLength (&StrPkgInstance->StringList);
    PkgLen += sizeof (EFI_HII_SIBT_END_BLOCK);

    if (PkgLen > MAX_UINT32) {
      return EFI_BAD_BUFFER_SIZE;
    }

    Status = DynHiiUpdateStrPkgHdrLen (
               StrPkgInstance->StrPkgHdr,
               (UINT32)PkgLen
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    PkgLen += StrPkgInstance->StrPkgHdr->HdrSize;

    TmpLen += PkgLen;

    List = GetNextNode (&StrPkg->StrPkgList, List);
  }

  TmpLen += sizeof (UINT32);
  if (TmpLen > MAX_UINT32) {
    return EFI_BAD_BUFFER_SIZE;
  }

  StrPkgLen = (UINT32)TmpLen;

  StrPkgBuf = AllocateZeroPool (StrPkgLen);
  if (StrPkgBuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Arr         = (UINT32 *)StrPkgBuf;
  *Arr        = StrPkgLen;
  *StrPkgData = StrPkgBuf;
  StrPkgBuf  += sizeof (UINT32);

  List = GetFirstNode (&StrPkg->StrPkgList);
  while (!IsNull (&StrPkg->StrPkgList, List)) {
    PkgLen         = 0;
    StrPkgInstance = BASE_CR (List, DYN_HII_STR_PKG_INSTANCE, Link);

    CopyMem (
      StrPkgBuf,
      StrPkgInstance->StrPkgHdr,
      StrPkgInstance->StrPkgHdr->HdrSize
      );
    StrPkgBuf += StrPkgInstance->StrPkgHdr->HdrSize;

    StrPkgBuf = DynHiiSetStrings (
                  &StrPkgInstance->StringList,
                  StrPkgBuf
                  );

    DynHiiSetEndBlock (StrPkgBuf);
    StrPkgBuf += sizeof (EFI_HII_SIBT_END_BLOCK);

    List = GetNextNode (&StrPkg->StrPkgList, List);
  }

  DynHiiDumpStrPkgBuf (StrPkgBuf, StrPkgLen);

  return EFI_SUCCESS;
}

/** Free all the strings in a string list.

  Iterate through the string list, and free up memory allocated for the
  string and the DYN_HII_STR_INFO structure that contains information about
  that string.

  @param [in]  StringList       The list of strings to be freed.

**/
VOID
EFIAPI
DynHiiFreeStrings (
  IN LIST_ENTRY  *StringList
  )
{
  LIST_ENTRY        *Link;
  LIST_ENTRY        *Next;
  DYN_HII_STR_INFO  *StrInfo;

  if (StringList == NULL) {
    return;
  }

  Link = GetFirstNode (StringList);
  while (!IsNull (StringList, Link)) {
    Next = GetNextNode (StringList, Link);

    StrInfo = BASE_CR (Link, DYN_HII_STR_INFO, Link);
    RemoveEntryList (Link);

    FreePool (StrInfo->String);
    FreePool (StrInfo);

    Link = Next;
  }

  InitializeListHead (StringList);
}

/** Free up the string package buffer.

  Free up the string package buffer along with any other allocations that
  were done for the string package information structure.

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.

**/
VOID
EFIAPI
DynHiiFreeStringPackage (
  IN   DYN_HII_STR_HANDLE  StrPkgHandle
  )
{
  LIST_ENTRY                *Link;
  LIST_ENTRY                *Next;
  DYN_HII_STR_PKG           *StrPkg;
  DYN_HII_STR_PKG_INSTANCE  *StrPkgInstance;

  if (StrPkgHandle == NULL) {
    return;
  }

  StrPkg = StrPkgHandle;
  if (StrPkg->Signature != DYN_HII_STR_PKG_SIGNATURE) {
    return;
  }

  Link = GetFirstNode (&StrPkg->StrPkgList);
  while (!IsNull (&StrPkg->StrPkgList, Link)) {
    Next           = GetNextNode (&StrPkg->StrPkgList, Link);
    StrPkgInstance = BASE_CR (Link, DYN_HII_STR_PKG_INSTANCE, Link);

    if (StrPkgInstance->StrPkgHdr != NULL) {
      FreePool (StrPkgInstance->StrPkgHdr);
    }

    if (StrPkgInstance->LanguageCode != NULL) {
      FreePool (StrPkgInstance->LanguageCode);
    }

    DynHiiFreeStrings (&StrPkgInstance->StringList);
    FreePool (StrPkgInstance);
    RemoveEntryList (Link);

    Link = Next;
  }

  FreePool (StrPkg);
}
