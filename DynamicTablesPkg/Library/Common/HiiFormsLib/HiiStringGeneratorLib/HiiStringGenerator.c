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
#include <Library/PrintLib.h>

#include <HiiStringGeneratorLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/** Get the number of bytes occupied by the language code string.

  Get the number of bytes occupied by the language code string.

  @param [in]  LangCode          The language code string.

  @retval Number of bytes occupied by the string, including the null
          character.
**/
static
UINTN
DynHiiGetLangCodeSize (
  IN  CONST  CHAR8  *LangCode
  )
{
  return AsciiStrSize (LangCode);
}

/** Copy the language code string to the string package header.

  Copy the language code string to the string package header.

  @param [in]  StrPkg            Location where the language code is to
                                 be copied.
  @param [in]  LangCode          The language code to be copied.

**/
static
VOID
DynHiiSetLangCode (
  IN         UINT8  *StrPkg,
  IN  CONST  CHAR8  *LangCode
  )
{
  AsciiStrCpyS (
    (CHAR8 *)StrPkg,
    DynHiiGetLangCodeSize (LangCode),
    LangCode
    );
}

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
  LIST_ENTRY        *Link;
  DYN_HII_STR_INFO  *StrInfo;

  StrLen = 0;
  Link   = GetFirstNode (StrList);
  while (!IsNull (StrList, Link)) {
    StrInfo = (DYN_HII_STR_INFO *)Link;

    StrLen += StrInfo->Length;

    Link = GetNextNode (StrList, Link);
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
  LIST_ENTRY        *Link;
  DYN_HII_STR_INFO  *StrInfo;

  Link = GetFirstNode (StrList);
  while (!IsNull (StrList, Link)) {
    StrInfo = (DYN_HII_STR_INFO *)Link;

    *StrPkg++ = EFI_HII_SIBT_STRING_UCS2;
    CopyMem (StrPkg, StrInfo->String, StrSize (StrInfo->String));

    StrPkg += StrInfo->Length - 1;

    Link = GetNextNode (StrList, Link);
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

/** Initialize the string package header structure.

  Initialize the EFI_HII_STRING_PACKAGE_HDR structure. Some of the structure
  member values are computed at runtime, while the rest are obtained from
  StrPkgInfo.

  @param [in]  StrPkgInfo       Pointer to the DYN_HII_STR_PKG_INFO structure.
  @param [in]  StrPkgHdr        Pointer to the EFI_HII_STRING_PACKAGE_HDR
                                structure, to be initialized.
  @param [in]  StrPkgLen        Length of the string package.

**/
static
VOID
DynHiiInitStrPkgHdr (
  IN   CONST DYN_HII_STR_PKG_INFO  *StrPkgInfo,
  IN   EFI_HII_STRING_PACKAGE_HDR  *StrPkgHdr,
  IN   UINTN                       StrPkgLen
  )
{
  CHAR8  *LangCode;
  UINTN  TmpSize;

  LangCode = StrPkgInfo->LanguageCode;

  TmpSize = sizeof (*StrPkgHdr) + DynHiiGetLangCodeSize (LangCode);
  if (TmpSize > MAX_UINT32) {
    ASSERT (FALSE);
    StrPkgHdr->HdrSize = MAX_UINT32;
  } else {
    StrPkgHdr->HdrSize = (UINT32)TmpSize;
  }

  StrPkgHdr->StringInfoOffset = StrPkgHdr->HdrSize;
  SetMem (&StrPkgHdr->LanguageWindow, sizeof (StrPkgHdr->LanguageWindow), 0x0);
  StrPkgHdr->LanguageName = StrPkgInfo->LanguageId;

  TmpSize = StrPkgHdr->HdrSize + StrPkgLen;
  if (TmpSize > SIZE_16MB) {
    TmpSize = 0;
  }

  StrPkgHdr->Header.Length = (UINT32)TmpSize;
  StrPkgHdr->Header.Type   = EFI_HII_PACKAGE_STRINGS;
}

/** Dump the contents of the String buffer.

  Useful for debugging.

  @param [in]  StrPkg            Pointer to the string package.
  @param [in]  StrPkgLen         Length of the string package.
  @param [in]  StrList           List of strings in the string package.

**/
static
VOID
DumpStrPkgBuffer (
  IN          UINT8       *StrPkg,
  IN          UINT32      StrPkgLen,
  IN   CONST  LIST_ENTRY  *StrList
  )
{
  UINT8                       *Buf;
  UINT32                      Idx;
  UINT32                      Idx1;
  UINT32                      HdrLen;
  LIST_ENTRY                  *Link;
  DYN_HII_STR_INFO            *StrInfo;
  EFI_HII_STRING_PACKAGE_HDR  *StrPkgHdr;

  if ((StrPkg == NULL) || (StrList == NULL)) {
    return;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "INFO: %a: StrPkg => %p, StrPkgLen => 0x%x\n",
    __func__,
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
  StrPkgHdr = (EFI_HII_STRING_PACKAGE_HDR *)Buf;
  HdrLen    = StrPkgHdr->HdrSize;
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

  Link = GetFirstNode (StrList);
  while (!IsNull (StrList, Link)) {
    StrInfo = (DYN_HII_STR_INFO *)Link;

    for (Idx = 0; Idx < StrInfo->Length;) {
      for (Idx1 = 0; Idx < StrInfo->Length && Idx1 < 16; Idx1++, Idx++) {
        DEBUG ((
          DEBUG_VERBOSE,
          "0x%02x, ",
          *Buf++
          ));
      }

      DEBUG ((DEBUG_VERBOSE, "\n"));
    }

    DEBUG ((DEBUG_VERBOSE, "\n"));
    Link = GetNextNode (StrList, Link);
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "};\n\n"
    ));
}

/** Initialize the string package info structure.

  Initialize the DYN_HII_STR_PKG_INFO structure by allocating memory for
  certain members of the structure and then initializing them with data
  that were passed to the function.

  @param [in]  LanguageCode     LanguageCode string to be added.
  @param [in]  LangStrId        String ID of the Language string that has
                                been added to the string list.
  @param [out] StrPkgInfo       Pointer to the DYN_HII_STR_PKG_INFO structure
                                allocated by this function.

  @retval  EFI_SUCCESS            The structure was initialized successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiInitStringPkgInfo (
  IN  CONST   CHAR8                 *LanguageCode,
  IN  CONST   EFI_STRING_ID         LangStrId,
  OUT         DYN_HII_STR_PKG_INFO  **StrPkgInfo
  )
{
  UINTN                 LangSize;
  DYN_HII_STR_PKG_INFO  *Info;

  if ((LanguageCode == NULL) || (StrPkgInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *StrPkgInfo = NULL;
  Info        = AllocateZeroPool (sizeof (*Info));
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&Info->StringList);
  Info->LanguageId = LangStrId;

  LangSize           = AsciiStrSize (LanguageCode);
  Info->LanguageCode = AllocateZeroPool (LangSize);
  if (Info->LanguageCode == NULL) {
    FreePool (Info);
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiStrCpyS (Info->LanguageCode, LangSize, LanguageCode);

  *StrPkgInfo = Info;

  return EFI_SUCCESS;
}

/** Add a string to the string list.

  Add the string passed to the function to a list of strings that would be
  added to form the string package.

  @param [in]  StrList          String List.
  @param [in]  StringId         String Identifier that gets used in the forms.
  @param [in]  String           The string that is to be added.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrList or String is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddString (
  IN LIST_ENTRY     *StrList,
  IN EFI_STRING_ID  StringId,
  IN CHAR16         *String
  )
{
  DYN_HII_STR_INFO  *NewStr;
  UINTN             StrBytes;
  UINTN             TmpLen;

  if ((StrList == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NewStr = AllocateZeroPool (sizeof (*NewStr));
  if (NewStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrBytes = StrSize (String);
  TmpLen   = sizeof (EFI_HII_STRING_BLOCK) + StrBytes;
  if (TmpLen > MAX_UINT32) {
    FreePool (NewStr);
    return EFI_BAD_BUFFER_SIZE;
  }

  InitializeListHead (&NewStr->Link);
  NewStr->StringId = StringId;
  NewStr->Length   = (UINT32)TmpLen;

  NewStr->String = AllocateZeroPool (StrBytes);
  if (NewStr->String == NULL) {
    FreePool (NewStr);
    return EFI_OUT_OF_RESOURCES;
  }

  StrCpyS (NewStr->String, StrBytes / sizeof (CHAR16), String);

  InsertTailList (StrList, &NewStr->Link);

  return EFI_SUCCESS;
}

/** Generate string package.

  Generate the string package from the input parameter. The string package is
  subsequently added to the HII database through the HiiAddPackages() call.

  @param [in]  StrPkgInfo       The structure that contains all information
                                needed to generate the string package.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_ALREADY_STARTED    If StrPkgBuf is not NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateStringPackage (
  IN   DYN_HII_STR_PKG_INFO  *StrPkgInfo
  )
{
  UINT8                       *StrPkg;
  UINT32                      *Arr;
  UINT32                      StrPkgLen;
  UINTN                       TmpLen;
  EFI_HII_STRING_PACKAGE_HDR  StrPkgHdr;

  if ((StrPkgInfo == NULL) || (StrPkgInfo->LanguageCode == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (StrPkgInfo->StrPkgBuf != NULL) {
    return EFI_ALREADY_STARTED;
  }

  TmpLen  = DynHiiGetStrLength (&StrPkgInfo->StringList);
  TmpLen += sizeof (EFI_HII_SIBT_END_BLOCK);

  DynHiiInitStrPkgHdr (StrPkgInfo, &StrPkgHdr, TmpLen);
  TmpLen += StrPkgHdr.HdrSize;

  TmpLen += sizeof (UINT32);
  if (TmpLen > MAX_UINT32) {
    return EFI_BAD_BUFFER_SIZE;
  }

  StrPkgLen = (UINT32)TmpLen;

  StrPkg = AllocateZeroPool (StrPkgLen);
  if (StrPkg == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Arr                   = (UINT32 *)StrPkg;
  *Arr                  = StrPkgLen;
  StrPkgInfo->StrPkgBuf = StrPkg;
  StrPkg               += sizeof (UINT32);

  CopyMem (
    StrPkg,
    &StrPkgHdr,
    sizeof (EFI_HII_STRING_PACKAGE_HDR)
    );

  DynHiiSetLangCode (
    StrPkg + sizeof (EFI_HII_STRING_PACKAGE_HDR),
    StrPkgInfo->LanguageCode
    );

  StrPkg = DynHiiSetStrings (
             &StrPkgInfo->StringList,
             StrPkg + StrPkgHdr.StringInfoOffset
             );

  DynHiiSetEndBlock (StrPkg);

  DumpStrPkgBuffer (StrPkgInfo->StrPkgBuf, StrPkgLen, &StrPkgInfo->StringList);

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

    StrInfo = (DYN_HII_STR_INFO *)Link;
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

  @param [in]  StrPkgInfo       Pointer to the string package structure.

**/
VOID
EFIAPI
DynHiiFreeStringPackage (
  IN DYN_HII_STR_PKG_INFO  *StrPkgInfo
  )
{
  if (StrPkgInfo == NULL) {
    return;
  }

  if (StrPkgInfo->LanguageCode != NULL) {
    FreePool (StrPkgInfo->LanguageCode);
  }

  if (StrPkgInfo->StrPkgBuf != NULL) {
    FreePool (StrPkgInfo->StrPkgBuf);
  }

  FreePool (StrPkgInfo);
}
