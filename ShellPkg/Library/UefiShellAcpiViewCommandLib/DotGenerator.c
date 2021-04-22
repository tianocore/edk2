/** @file
  Dot File Generator

  Copyright (c) 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include "DotGenerator.h"
#include "AcpiView.h"

#define MAX_DOT_BUFFER_SIZE   128

/**
  Writes a Null terminated ASCII string to the dot file handle.

  @param [in] String      Null terminated ascii string.
**/
STATIC
VOID
DotWriteFile (
  SHELL_FILE_HANDLE DotFileHandle,
  IN CHAR8* String
  )
{
  UINTN TransferBytes;
  EFI_STATUS Status;

  if (DotFileHandle == NULL) {
    Print ("ERROR: Failed to write to dot file\n");
    ASSERT (0);
    return;
  }

  TransferBytes = AsciiStrLen (String);
  Status = ShellWriteFile (
    DotFileHandle,
    &TransferBytes,
    String
    );
  ASSERT_EFI_ERROR (Status);
  ASSERT (AsciiStrLen (String) == TransferBytes);
}

/**
  Writes a new parameter to a previously started parameter list.

  @param [in] Name      Null terminated string of the parameter's name.
  @param [in] Value     Null terminated string of the parameter's value.
  @param [in] Quoted    True if value needs to be quoted.
**/
STATIC
VOID
DotAddParameter (
  SHELL_FILE_HANDLE DotFileHandle,
  IN CHAR16*  Name,
  IN CHAR16*  Value,
  IN BOOLEAN  Quoted
  )
{
  CHAR8 StringBuffer[MAX_DOT_BUFFER_SIZE];

  ASSERT(DotFileHandle != NULL);

  if (Quoted) {
    AsciiSPrint (
      StringBuffer,
      sizeof (StringBuffer),
      "[%s=\"%s\"]",
      Name,
      Value
      );
  } else {
    AsciiSPrint (
      StringBuffer,
      sizeof (StringBuffer),
      "[%s=%s]",
      Name,
      Value
      );
  }

  DotWriteFile (DotFileHandle, StringBuffer);
}

/**
  Writes the color argument of nodes or links according to flags.

  @param [in] Flags     Flags describing the color (one of DOT_COLOR_...)
**/
STATIC
VOID
WriteColor (
  SHELL_FILE_HANDLE DotFileHandle,
  IN UINT16 Flags
  )
{
  ASSERT(DotFileHandle != NULL);

  switch (Flags & DOT_COLOR_MASK) {
    case DOT_COLOR_GRAY:
      DotAddParameter (DotFileHandle, L"color", L"gray", FALSE);
      break;
    case DOT_COLOR_YELLOW:
      DotAddParameter (DotFileHandle, L"color", L"yellow", FALSE);
      break;
    case DOT_COLOR_BLUE:
      DotAddParameter (DotFileHandle, L"color", L"blue", FALSE);
      break;
    case DOT_COLOR_RED:
      DotAddParameter (DotFileHandle, L"color", L"red", FALSE);
      break;
    case DOT_COLOR_BLACK:
    default:
      DotAddParameter (DotFileHandle, L"color", L"black", FALSE);
      break;
  }
}

/**
  Opens a new dot file and writes a dot directional graph.

  @param [in] FileName    Null terminated unicode string.
**/
SHELL_FILE_HANDLE
DotOpenNewFile (
  IN CHAR16* FileName
  )
{
  SHELL_FILE_HANDLE DotFileHandle;
  EFI_STATUS Status;

  Status = ShellOpenFileByName (
             FileName,
             &DotFileHandle,
             EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
             0
             );
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Couldn't open dot file");
    return NULL;//Status;
  }
  Print (L"Creating DOT Graph in : %s... ", FileName);
  DotWriteFile (DotFileHandle, "digraph {\n\trankdir=BT\n");
  return DotFileHandle;
}

/**
  Writes a dot graph footer and closes the dot file.
**/
VOID
DotCloseFile (
  SHELL_FILE_HANDLE DotFileHandle
  )
{
  ASSERT(DotFileHandle != NULL);

  DotWriteFile (DotFileHandle, "}\n");
  ShellCloseFile (&DotFileHandle);
  Print (L"Done.\n");
}

/**
  Writes a line in the previously opened dot file describing a
  new node.

  @param [in] Id        A unique identifier for the node.
  @param [in] Flags     Flags describing the node's characteristics.
  @param [in] Label     Label to be shown on the graph node.
**/
VOID
DotAddNode (
  SHELL_FILE_HANDLE DotFileHandle,
  IN UINT32         Id,
  IN UINT16         Flags,
  IN CONST CHAR16*  Label
  )
{
  CHAR8  LineBuffer[64];
  CHAR16 LabelBuffer[MAX_DOT_BUFFER_SIZE];

  ASSERT ((Flags & ~DOT_BOX_FLAGS_MASK) == 0);
  ASSERT(DotFileHandle != NULL);

  AsciiSPrint (
    LineBuffer,
    sizeof (LineBuffer),
    "\tx%x",
    Id
    );
  DotWriteFile (DotFileHandle, LineBuffer);

  switch (Flags & DOT_BOX_TYPE_MASK) {
    case DOT_BOX_DIAMOND:
      DotAddParameter (DotFileHandle, L"shape", L"diamond", FALSE);
      break;
    case DOT_BOX_SQUARE:
    default:
      DotAddParameter (DotFileHandle, L"shape", L"box", FALSE);
      break;
  }

  if (Label != NULL) {
    if ((Flags & DOT_BOX_ADD_ID_TO_LABEL) != 0) {
      UnicodeSPrint (
        LabelBuffer,
        sizeof (LabelBuffer),
        L"%s\\n0x%x",
        Label,
        Id
        );
    } else {
      UnicodeSPrint (
        LabelBuffer,
        sizeof (LabelBuffer),
        L"%s",
        Label
        );
    }
    DotAddParameter (DotFileHandle, L"label", LabelBuffer, TRUE);
  }

  WriteColor (DotFileHandle, Flags);
  DotWriteFile (DotFileHandle, "\n");
}

/**
  Writes a line in the previously opened dot file describing a
  new link between two nodes.

  @param [in] IdSource    An identifier for the source node of the link.
  @param [in] IdTarget    An identifier for the target node of the link.
  @param [in] Flags       Flags describing the node's characteristics.
**/
VOID
DotAddLink (
  SHELL_FILE_HANDLE DotFileHandle,
  IN UINT32 IdSource,
  IN UINT32 IdTarget,
  IN UINT16 Flags
  )
{
  CHAR8 LineBuffer[64];

  ASSERT(DotFileHandle != NULL);
  ASSERT ((Flags & ~DOT_ARROW_FLAGS_MASK) == 0);

  AsciiSPrint (
    LineBuffer,
    sizeof (LineBuffer),
    "\tx%x -> x%x",
    IdSource,
    IdTarget
    );
  DotWriteFile (DotFileHandle, LineBuffer);

  if ((Flags & DOT_ARROW_RANK_REVERSE) != 0) {
    DotAddParameter (DotFileHandle, L"dir", L"back", FALSE);
  }

  switch (Flags & DOT_ARROW_TYPE_MASK) {
    case DOT_ARROW_DOTTED:
      DotAddParameter (DotFileHandle, L"style", L"dotted", FALSE);
      break;
    case DOT_ARROW_FULL:
    default:
      DotAddParameter (DotFileHandle, L"style", L"solid", FALSE);
      break;
  }

  WriteColor (DotFileHandle, Flags);
  DotWriteFile (DotFileHandle, "\n");
}
