/** @file
  Function definitions for shell simple text in and out on top of file handles.

  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

extern BOOLEAN  AsciiRedirection;

typedef struct {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL    SimpleTextIn;
  SHELL_FILE_HANDLE                 FileHandle;
  EFI_HANDLE                        TheHandle;
  UINT64                            RemainingBytesOfInputFile;
} SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    SimpleTextOut;
  SHELL_FILE_HANDLE                  FileHandle;
  EFI_HANDLE                         TheHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *OriginalSimpleTextOut;
} SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/**
  Event notification function for EFI_SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey event
  Signal the event if there is key available

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.

**/
VOID
EFIAPI
ConInWaitForKey (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  gBS->SignalEvent (Event);
}

/**
  Reset function for the fake simple text input.

  @param[in] This     A pointer to the SimpleTextIn structure.
  @param[in] ExtendedVerification TRUE for extra validation, FALSE otherwise.

  @retval   EFI_SUCCESS The reset was successful.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextInReset (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN BOOLEAN                         ExtendedVerification
  )
{
  return (EFI_SUCCESS);
}

/**
  ReadKeyStroke function for the fake simple text input.

  @param[in] This      A pointer to the SimpleTextIn structure.
  @param[in, out] Key  A pointer to the Key structure to fill.

  @retval EFI_SUCCESS      The read was successful.
  @retval EFI_UNSUPPORTED  The device does not support the ability to read
                           keystroke data.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextInReadKeyStroke (
  IN      EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
  IN OUT  EFI_INPUT_KEY                   *Key
  )
{
  UINTN  Size;
  UINTN  CharSize;

  //
  // Verify the parameters
  //
  if ((Key == NULL) || (This == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Check if we have any characters left in the stream.
  //
  if (((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)This)->RemainingBytesOfInputFile == 0) {
    return (EFI_NOT_READY);
  }

  Size = sizeof (CHAR16);

  if (!AsciiRedirection) {
    CharSize = sizeof (CHAR16);
  } else {
    CharSize = sizeof (CHAR8);
  }

  //
  // Decrement the amount of free space by Size or set to zero (for odd length files)
  //
  if (((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)This)->RemainingBytesOfInputFile > CharSize) {
    ((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)This)->RemainingBytesOfInputFile -= CharSize;
  } else {
    ((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)This)->RemainingBytesOfInputFile = 0;
  }

  Key->ScanCode = 0;
  return (ShellInfoObject.NewEfiShellProtocol->ReadFile (
                                                 ((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)This)->FileHandle,
                                                 &Size,
                                                 &Key->UnicodeChar
                                                 ));
}

/**
  Function to create a EFI_SIMPLE_TEXT_INPUT_PROTOCOL on top of a
  SHELL_FILE_HANDLE to support redirecting input from a file.

  @param[in]  FileHandleToUse The pointer to the SHELL_FILE_HANDLE to use.
  @param[in]  HandleLocation  The pointer of a location to copy handle with protocol to.

  @retval NULL                There was insufficient memory available.
  @return                     A pointer to the allocated protocol structure;
**/
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *
CreateSimpleTextInOnFile (
  IN SHELL_FILE_HANDLE  FileHandleToUse,
  IN EFI_HANDLE         *HandleLocation
  )
{
  SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *ProtocolToReturn;
  EFI_STATUS                            Status;
  UINT64                                CurrentPosition;
  UINT64                                FileSize;

  if ((HandleLocation == NULL) || (FileHandleToUse == NULL)) {
    return (NULL);
  }

  ProtocolToReturn = AllocateZeroPool (sizeof (SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL));
  if (ProtocolToReturn == NULL) {
    return (NULL);
  }

  ShellGetFileSize (FileHandleToUse, &FileSize);
  ShellGetFilePosition (FileHandleToUse, &CurrentPosition);

  //
  // Initialize the protocol members
  //
  ProtocolToReturn->RemainingBytesOfInputFile  = FileSize - CurrentPosition;
  ProtocolToReturn->FileHandle                 = FileHandleToUse;
  ProtocolToReturn->SimpleTextIn.Reset         = FileBasedSimpleTextInReset;
  ProtocolToReturn->SimpleTextIn.ReadKeyStroke = FileBasedSimpleTextInReadKeyStroke;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  ConInWaitForKey,
                  &ProtocolToReturn->SimpleTextIn,
                  &ProtocolToReturn->SimpleTextIn.WaitForKey
                  );

  if (EFI_ERROR (Status)) {
    FreePool (ProtocolToReturn);
    return (NULL);
  }

  /// @todo possibly also install SimpleTextInputEx on the handle at this point.
  Status = gBS->InstallProtocolInterface (
                  &(ProtocolToReturn->TheHandle),
                  &gEfiSimpleTextInProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &(ProtocolToReturn->SimpleTextIn)
                  );
  if (!EFI_ERROR (Status)) {
    *HandleLocation = ProtocolToReturn->TheHandle;
    return ((EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)ProtocolToReturn);
  } else {
    FreePool (ProtocolToReturn);
    return (NULL);
  }
}

/**
  Function to close a EFI_SIMPLE_TEXT_INPUT_PROTOCOL on top of a
  SHELL_FILE_HANDLE to support redirecting input from a file.

  @param[in]  SimpleTextIn    The pointer to the SimpleTextIn to close.

  @retval EFI_SUCCESS         The object was closed.
**/
EFI_STATUS
CloseSimpleTextInOnFile (
  IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *SimpleTextIn
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  Status1;

  if (SimpleTextIn == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = gBS->CloseEvent (((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)SimpleTextIn)->SimpleTextIn.WaitForKey);

  Status1 = gBS->UninstallProtocolInterface (
                   ((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)SimpleTextIn)->TheHandle,
                   &gEfiSimpleTextInProtocolGuid,
                   &(((SHELL_EFI_SIMPLE_TEXT_INPUT_PROTOCOL *)SimpleTextIn)->SimpleTextIn)
                   );

  FreePool (SimpleTextIn);
  if (!EFI_ERROR (Status)) {
    return (Status1);
  } else {
    return (Status);
  }
}

/**
  Reset the text output device hardware and optionally run diagnostics.

  @param  This                pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  @param ExtendedVerification Indicates that a more extensive test may be performed

  @retval EFI_SUCCESS         The text output device was reset.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
  )
{
  return (EFI_SUCCESS);
}

/**
  Verifies that all characters in a Unicode string can be output to the
  target device.

  @param[in] This     Protocol instance pointer.
  @param[in] WString  The NULL-terminated Unicode string to be examined.

  @retval EFI_SUCCESS The device(s) are capable of rendering the output string.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  return (EFI_SUCCESS);
}

/**
  Returns information for an available text mode that the output device(s)
  supports.

  @param[in] This               Protocol instance pointer.
  @param[in] ModeNumber         The mode number to return information on.
  @param[out] Columns           Upon return, the number of columns in the selected geometry
  @param[out] Rows              Upon return, the number of rows in the selected geometry

  @retval EFI_UNSUPPORTED       The mode number was not valid.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *PassThruProtocol;

  PassThruProtocol = ((SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)This)->OriginalSimpleTextOut;

  // Pass the QueryMode call thru to the original SimpleTextOutProtocol
  return (PassThruProtocol->QueryMode (
                              PassThruProtocol,
                              ModeNumber,
                              Columns,
                              Rows
                              ));
}

/**
  Sets the output device(s) to a specified mode.

  @param[in] This               Protocol instance pointer.
  @param[in] ModeNumber         The mode number to set.

  @retval EFI_UNSUPPORTED       The mode number was not valid.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  )
{
  return (EFI_UNSUPPORTED);
}

/**
  Sets the background and foreground colors for the OutputString () and
  ClearScreen () functions.

  @param[in] This               Protocol instance pointer.
  @param[in] Attribute          The attribute to set. Bits 0..3 are the foreground color, and
                                bits 4..6 are the background color. All other bits are undefined
                                and must be zero. The valid Attributes are defined in this file.

  @retval EFI_SUCCESS           The attribute was set.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
  )
{
  return (EFI_SUCCESS);
}

/**
  Clears the output device(s) display to the currently selected background
  color.

  @param[in] This               Protocol instance pointer.

  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  )
{
  return (EFI_SUCCESS);
}

/**
  Sets the current coordinates of the cursor position

  @param[in] This               Protocol instance pointer.
  @param[in] Column             Column to put the cursor in.  Must be between zero and Column returned from QueryMode
  @param[in] Row                Row to put the cursor in.  Must be between zero and Row returned from QueryMode

  @retval EFI_SUCCESS           The operation completed successfully.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Column,
  IN  UINTN                            Row
  )
{
  return (EFI_SUCCESS);
}

/**
  Makes the cursor visible or invisible

  @param[in] This       Protocol instance pointer.
  @param[in] Visible    If TRUE, the cursor is set to be visible. If FALSE, the cursor is
                        set to be invisible.

  @retval EFI_SUCCESS           The operation completed successfully.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  )
{
  return (EFI_SUCCESS);
}

/**
  Write a Unicode string to the output device.

  @param[in] This                 Protocol instance pointer.
  @param[in] WString              The NULL-terminated Unicode string to be displayed on the output
                                  device(s). All output devices must also support the Unicode
                                  drawing defined in this file.
  @retval EFI_SUCCESS             The string was output to the device.
  @retval EFI_DEVICE_ERROR        The device reported an error while attempting to output
                                  the text.
  @retval EFI_UNSUPPORTED         The output device's mode is not currently in a
                                  defined text mode.
  @retval EFI_WARN_UNKNOWN_GLYPH  This warning code indicates that some of the
                                  characters in the Unicode string could not be
                                  rendered and were skipped.
**/
EFI_STATUS
EFIAPI
FileBasedSimpleTextOutOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
  )
{
  UINTN  Size;

  Size = StrLen (WString) * sizeof (CHAR16);
  return (ShellInfoObject.NewEfiShellProtocol->WriteFile (
                                                 ((SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)This)->FileHandle,
                                                 &Size,
                                                 WString
                                                 ));
}

/**
  Function to create a EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL on top of a
  SHELL_FILE_HANDLE to support redirecting output from a file.

  @param[in]  FileHandleToUse  The pointer to the SHELL_FILE_HANDLE to use.
  @param[in]  HandleLocation   The pointer of a location to copy handle with protocol to.
  @param[in]  OriginalProtocol The pointer to the original output protocol for pass thru of functions.

  @retval NULL                There was insufficient memory available.
  @return                     A pointer to the allocated protocol structure;
**/
EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *
CreateSimpleTextOutOnFile (
  IN SHELL_FILE_HANDLE                FileHandleToUse,
  IN EFI_HANDLE                       *HandleLocation,
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *OriginalProtocol
  )
{
  SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ProtocolToReturn;
  EFI_STATUS                             Status;

  if ((HandleLocation == NULL) || (FileHandleToUse == NULL)) {
    return (NULL);
  }

  ProtocolToReturn = AllocateZeroPool (sizeof (SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL));
  if (ProtocolToReturn == NULL) {
    return (NULL);
  }

  ProtocolToReturn->FileHandle                      = FileHandleToUse;
  ProtocolToReturn->OriginalSimpleTextOut           = OriginalProtocol;
  ProtocolToReturn->SimpleTextOut.Reset             = FileBasedSimpleTextOutReset;
  ProtocolToReturn->SimpleTextOut.TestString        = FileBasedSimpleTextOutTestString;
  ProtocolToReturn->SimpleTextOut.QueryMode         = FileBasedSimpleTextOutQueryMode;
  ProtocolToReturn->SimpleTextOut.SetMode           = FileBasedSimpleTextOutSetMode;
  ProtocolToReturn->SimpleTextOut.SetAttribute      = FileBasedSimpleTextOutSetAttribute;
  ProtocolToReturn->SimpleTextOut.ClearScreen       = FileBasedSimpleTextOutClearScreen;
  ProtocolToReturn->SimpleTextOut.SetCursorPosition = FileBasedSimpleTextOutSetCursorPosition;
  ProtocolToReturn->SimpleTextOut.EnableCursor      = FileBasedSimpleTextOutEnableCursor;
  ProtocolToReturn->SimpleTextOut.OutputString      = FileBasedSimpleTextOutOutputString;
  ProtocolToReturn->SimpleTextOut.Mode              = AllocateZeroPool (sizeof (EFI_SIMPLE_TEXT_OUTPUT_MODE));
  if (ProtocolToReturn->SimpleTextOut.Mode == NULL) {
    FreePool (ProtocolToReturn);
    return (NULL);
  }

  ProtocolToReturn->SimpleTextOut.Mode->MaxMode       = OriginalProtocol->Mode->MaxMode;
  ProtocolToReturn->SimpleTextOut.Mode->Mode          = OriginalProtocol->Mode->Mode;
  ProtocolToReturn->SimpleTextOut.Mode->Attribute     = OriginalProtocol->Mode->Attribute;
  ProtocolToReturn->SimpleTextOut.Mode->CursorColumn  = OriginalProtocol->Mode->CursorColumn;
  ProtocolToReturn->SimpleTextOut.Mode->CursorRow     = OriginalProtocol->Mode->CursorRow;
  ProtocolToReturn->SimpleTextOut.Mode->CursorVisible = OriginalProtocol->Mode->CursorVisible;

  Status = gBS->InstallProtocolInterface (
                  &(ProtocolToReturn->TheHandle),
                  &gEfiSimpleTextOutProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &(ProtocolToReturn->SimpleTextOut)
                  );
  if (!EFI_ERROR (Status)) {
    *HandleLocation = ProtocolToReturn->TheHandle;
    return ((EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)ProtocolToReturn);
  } else {
    SHELL_FREE_NON_NULL (ProtocolToReturn->SimpleTextOut.Mode);
    SHELL_FREE_NON_NULL (ProtocolToReturn);
    return (NULL);
  }
}

/**
  Function to close a EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL on top of a
  SHELL_FILE_HANDLE to support redirecting output from a file.

  @param[in] SimpleTextOut    The pointer to the SimpleTextOUT to close.

  @retval EFI_SUCCESS         The object was closed.
**/
EFI_STATUS
CloseSimpleTextOutOnFile (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *SimpleTextOut
  )
{
  EFI_STATUS  Status;

  if (SimpleTextOut == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  Status = gBS->UninstallProtocolInterface (
                  ((SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)SimpleTextOut)->TheHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  &(((SHELL_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *)SimpleTextOut)->SimpleTextOut)
                  );
  FreePool (SimpleTextOut->Mode);
  FreePool (SimpleTextOut);
  return (Status);
}
