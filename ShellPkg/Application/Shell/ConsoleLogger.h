/** @file
  Provides interface to shell console logger.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _CONSOLE_LOGGER_HEADER_
#define _CONSOLE_LOGGER_HEADER_

#include "Shell.h"

#define CONSOLE_LOGGER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('c', 'o', 'P', 'D')

typedef struct _CONSOLE_LOGGER_PRIVATE_DATA{
  UINTN                             Signature;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   OurConOut;        ///< the protocol we installed onto the system table
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *OldConOut;       ///< old protocol to reinstall upon exiting
  EFI_HANDLE                        OldConHandle;     ///< old protocol handle
  UINTN                             ScreenCount;      ///< How many screens worth of data to save
  CHAR16                            *Buffer;          ///< Buffer to save data
  UINTN                             BufferSize;       ///< size of buffer in bytes

                                                      //  start row is the top of the screen
  UINTN                             OriginalStartRow; ///< What the originally visible start row was
  UINTN                             CurrentStartRow;  ///< what the currently visible start row is

  UINTN                             RowsPerScreen;    ///< how many rows the screen can display
  UINTN                             ColsPerScreen;    ///< how many columns the screen can display

  INT32                             *Attributes;      ///< Buffer for Attribute to be saved for each character
  UINTN                             AttribSize;       ///< Size of Attributes in bytes

  EFI_SIMPLE_TEXT_OUTPUT_MODE       HistoryMode;      ///< mode of the history log
  BOOLEAN                           Enabled;          ///< Set to FALSE when a break is requested.
  UINTN                             RowCounter;       ///< Initial row of each print job.
} CONSOLE_LOGGER_PRIVATE_DATA;

#define CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS(a) CR (a, CONSOLE_LOGGER_PRIVATE_DATA, OurConOut, CONSOLE_LOGGER_PRIVATE_DATA_SIGNATURE)

/**
  Install our intermediate ConOut into the system table to
  keep a log of all the info that is displayed to the user.

  @param[in] ScreensToSave  Sets how many screen-worths of data to save.
  @param[out] ConsoleInfo   The object to pass into later functions.

  @retval EFI_SUCCESS       The operation was successful.
  @return other             The operation failed.

  @sa ConsoleLoggerResetBuffers
  @sa InstallProtocolInterface
**/
EFI_STATUS
EFIAPI
ConsoleLoggerInstall(
  IN CONST UINTN ScreensToSave,
  OUT CONSOLE_LOGGER_PRIVATE_DATA **ConsoleInfo
  );

/**
  Return the system to the state it was before InstallConsoleLogger
  was installed.

  @param[in, out] ConsoleInfo   The object from the install function.

  @retval EFI_SUCCESS     The operation was successful
  @return other           The operation failed.  This was from UninstallProtocolInterface.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerUninstall(
  IN OUT CONSOLE_LOGGER_PRIVATE_DATA *ConsoleInfo
  );

/**
  Displays previously logged output back to the screen.

  This will scroll the screen forwards and backwards through the log of previous
  output.  If Rows is 0 then the size of 1/2 the screen will be scrolled.  If Rows
  is (UINTN)(-1) then the size of the screen will be scrolled.

  @param[in] Forward      If TRUE then the log will be displayed forwards (scroll to newer).
                          If FALSE then the log will be displayed backwards (scroll to older).
  @param[in] Rows         Determines how many rows the log should scroll.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerDisplayHistory(
  IN CONST BOOLEAN  Forward,
  IN CONST UINTN    Rows,
  IN CONSOLE_LOGGER_PRIVATE_DATA *ConsoleInfo
  );

/**
  Function to return to normal output whent he scrolling is complete.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS   The operation was successful.
  @return other         The operation failed.  See UpdateDisplayFromHistory.

  @sa UpdateDisplayFromHistory
**/
EFI_STATUS
EFIAPI
ConsoleLoggerStopHistory(
  IN CONSOLE_LOGGER_PRIVATE_DATA *ConsoleInfo
  );

/**
  Updates the hidden ConOut to be displaying the correct stuff.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS     The operation was successful.
  @return other           The operation failed.
**/
EFI_STATUS
EFIAPI
UpdateDisplayFromHistory(
  IN CONSOLE_LOGGER_PRIVATE_DATA *ConsoleInfo
  );

/**
  Reset the text output device hardware and optionaly run diagnostics

  @param This                 Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  @param ExtendedVerification Indicates that a more extensive test may be performed

  @retval EFI_SUCCESS         The text output device was reset.
  @retval EFI_DEVICE_ERROR    The text output device is not functioning correctly and
                              could not be reset.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  BOOLEAN                         ExtendedVerification
  );

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
ConsoleLoggerOutputString(
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  CHAR16                          *WString
  );

/**
  Verifies that all characters in a Unicode string can be output to the
  target device.

  @param[in] This     Protocol instance pointer.
  @param[in] WString  The NULL-terminated Unicode string to be examined for the output
                      device(s).

  @retval EFI_SUCCESS           The device(s) are capable of rendering the output string.
  @retval EFI_UNSUPPORTED       Some of the characters in the Unicode string cannot be
                                rendered by one or more of the output devices mapped
                                by the EFI handle.

**/
EFI_STATUS
EFIAPI
ConsoleLoggerTestString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  CHAR16                          *WString
  );

/**
  Returns information for an available text mode that the output device(s)
  supports.

  @param[in] This               Protocol instance pointer.
  @param[in] ModeNumber         The mode number to return information on.
  @param[out] Columns           Upon return, the number of columns in the selected geometry
  @param[out] Rows              Upon return, the number of rows in the selected geometry

  @retval EFI_SUCCESS           The requested mode information was returned.
  @retval EFI_DEVICE_ERROR      The device had an error and could not
                                complete the request.
  @retval EFI_UNSUPPORTED       The mode number was not valid.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerQueryMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber,
  OUT UINTN                            *Columns,
  OUT UINTN                            *Rows
  );

/**
  Sets the output device(s) to a specified mode.

  @param[in] This               Protocol instance pointer.
  @param[in] ModeNumber         The mode number to set.


  @retval EFI_SUCCESS           The requested text mode was set.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The mode number was not valid.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerSetMode (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            ModeNumber
  );

/**
  Sets the background and foreground colors for the OutputString () and
  ClearScreen () functions.

  @param[in] This               Protocol instance pointer.
  @param[in] Attribute          The attribute to set. Bits 0..3 are the foreground color, and
                                bits 4..6 are the background color. All other bits are undefined
                                and must be zero. The valid Attributes are defined in this file.

  @retval EFI_SUCCESS           The attribute was set.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The attribute requested is not defined.

**/
EFI_STATUS
EFIAPI
ConsoleLoggerSetAttribute (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
  IN  UINTN                           Attribute
  );

/**
  Clears the output device(s) display to the currently selected background
  color.

  @param[in] This               Protocol instance pointer.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerClearScreen (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This
  );

/**
  Sets the current coordinates of the cursor position.

  @param[in] This               Protocol instance pointer.
  @param[in] Column             Column to put the cursor in.  Must be between zero and Column returned from QueryMode
  @param[in] Row                Row to put the cursor in.  Must be between zero and Row returned from QueryMode

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device had an error and
                                could not complete the request.
  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode, or the
                                cursor position is invalid for the current mode.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerSetCursorPosition (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                         Column,
  IN  UINTN                         Row
  );

/**
    Makes the cursor visible or invisible

  @param[in] This       Protocol instance pointer.
  @param[in] Visible    If TRUE, the cursor is set to be visible. If FALSE, the cursor is
                        set to be invisible.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the
                                request, or the device does not support changing
                                the cursor mode.
  @retval EFI_UNSUPPORTED       The output device is not in a valid text mode.

**/
EFI_STATUS
EFIAPI
ConsoleLoggerEnableCursor (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          Visible
  );

/**
  Function to update and verify that the current buffers are correct.

  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  This will be used when a mode has changed or a reset ocurred to verify all
  history buffers.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerResetBuffers(
  IN CONSOLE_LOGGER_PRIVATE_DATA *ConsoleInfo
  );

#endif //_CONSOLE_LOGGER_HEADER_

