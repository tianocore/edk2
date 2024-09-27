/** @file
  Provides interface to the shell protocols interactivity layer.

  Copyright (c) 2024, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SHELL_PROTOCOL_INTERACTIVITY_LIB__
#define __SHELL_PROTOCOL_INTERACTIVITY_LIB__

#include <Uefi.h>
#include <Library/ShellCommandLib.h>
#include <Protocol/ShellParameters.h>
#include <ShellInternals.h>

#define CONSOLE_LOGGER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('c', 'o', 'P', 'D')

typedef struct _CONSOLE_LOGGER_PRIVATE_DATA {
  UINTN                              Signature;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    OurConOut;       ///< the protocol we installed onto the system table
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *OldConOut;      ///< old protocol to reinstall upon exiting
  EFI_HANDLE                         OldConHandle;    ///< old protocol handle
  UINTN                              ScreenCount;     ///< How many screens worth of data to save
  CHAR16                             *Buffer;         ///< Buffer to save data
  UINTN                              BufferSize;      ///< size of buffer in bytes

  //  start row is the top of the screen
  UINTN                              OriginalStartRow; ///< What the originally visible start row was
  UINTN                              CurrentStartRow;  ///< what the currently visible start row is

  UINTN                              RowsPerScreen;   ///< how many rows the screen can display
  UINTN                              ColsPerScreen;   ///< how many columns the screen can display

  INT32                              *Attributes;     ///< Buffer for Attribute to be saved for each character
  UINTN                              AttribSize;      ///< Size of Attributes in bytes

  EFI_SIMPLE_TEXT_OUTPUT_MODE        HistoryMode;     ///< mode of the history log
  BOOLEAN                            Enabled;         ///< Set to FALSE when a break is requested.
  UINTN                              RowCounter;      ///< Initial row of each print job.
} CONSOLE_LOGGER_PRIVATE_DATA;

#define CONSOLE_LOGGER_PRIVATE_DATA_FROM_THIS(a)  CR (a, CONSOLE_LOGGER_PRIVATE_DATA, OurConOut, CONSOLE_LOGGER_PRIVATE_DATA_SIGNATURE)

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
ConsoleLoggerInstall (
  IN CONST UINTN                   ScreensToSave,
  OUT CONSOLE_LOGGER_PRIVATE_DATA  **ConsoleInfo
  );

/**
  Return the system to the state it was before InstallConsoleLogger
  was installed.

  @param[in, out] ConsoleInfo   The object from the install function.

  @retval EFI_SUCCESS     The operation was successful
  @return other           The operation failed.  This was from UninstallProtocolInterface.
**/
EFI_STATUS
ConsoleLoggerUninstall (
  IN OUT CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
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
ConsoleLoggerDisplayHistory (
  IN CONST BOOLEAN                Forward,
  IN CONST UINTN                  Rows,
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  );

/**
  Function to return to normal output whent he scrolling is complete.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS   The operation was successful.
  @return other         The operation failed.  See UpdateDisplayFromHistory.

  @sa UpdateDisplayFromHistory
**/
EFI_STATUS
ConsoleLoggerStopHistory (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  );

/**
  Updates the hidden ConOut to be displaying the correct stuff.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS     The operation was successful.
  @return other           The operation failed.
**/
EFI_STATUS
UpdateDisplayFromHistory (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  );

/**
  Reset the text output device hardware and optionally run diagnostics

  @param This                 Pointer to EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  @param ExtendedVerification Indicates that a more extensive test may be performed

  @retval EFI_SUCCESS         The text output device was reset.
  @retval EFI_DEVICE_ERROR    The text output device is not functioning correctly and
                              could not be reset.
**/
EFI_STATUS
EFIAPI
ConsoleLoggerReset (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  BOOLEAN                          ExtendedVerification
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
ConsoleLoggerOutputString (
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
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
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  CHAR16                           *WString
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
  IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
  IN  UINTN                            Attribute
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
  IN  UINTN                            Column,
  IN  UINTN                            Row
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

  This will be used when a mode has changed or a reset occurred to verify all
  history buffers.
**/
EFI_STATUS
ConsoleLoggerResetBuffers (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  );

/**
  Move the cursor position one character backward.

  @param[in] LineLength       Length of a line. Get it by calling QueryMode
  @param[in, out] Column      Current column of the cursor position
  @param[in, out] Row         Current row of the cursor position
**/
VOID
MoveCursorBackward (
  IN     UINTN  LineLength,
  IN OUT UINTN  *Column,
  IN OUT UINTN  *Row
  );

/**
  Move the cursor position one character forward.

  @param[in] LineLength       Length of a line.
  @param[in] TotalRow         Total row of a screen
  @param[in, out] Column      Current column of the cursor position
  @param[in, out] Row         Current row of the cursor position
**/
VOID
MoveCursorForward (
  IN     UINTN  LineLength,
  IN     UINTN  TotalRow,
  IN OUT UINTN  *Column,
  IN OUT UINTN  *Row
  );

/**
  Prints out each previously typed command in the command list history log.

  When each screen is full it will pause for a key before continuing.

  @param[in] TotalCols    How many columns are on the screen
  @param[in] TotalRows    How many rows are on the screen
  @param[in] StartColumn  which column to start at
**/
VOID
PrintCommandHistory (
  IN CONST UINTN  TotalCols,
  IN CONST UINTN  TotalRows,
  IN CONST UINTN  StartColumn
  );

typedef struct {
  LIST_ENTRY           Link;        ///< Standard linked list handler.
  SHELL_FILE_HANDLE    SplitStdOut; ///< ConsoleOut for use in the split.
  SHELL_FILE_HANDLE    SplitStdIn;  ///< ConsoleIn for use in the split.
} SPLIT_LIST;

typedef struct {
  BUFFER_LIST    CommandHistory;
  UINTN          VisibleRowNumber;
  UINTN          OriginalVisibleRowNumber;
  BOOLEAN        InsertMode;                        ///< Is the current typing mode insert (FALSE = overwrite).
} SHELL_VIEWING_SETTINGS;

typedef struct {
  BOOLEAN                      PageBreakEnabled;
  SHELL_VIEWING_SETTINGS       ViewingSettings;
  EFI_HII_HANDLE               HiiHandle;           ///< Handle from HiiLib.
  UINTN                        LogScreenCount;      ///< How many screens of log information to save.
  CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo;        ///< Pointer for ConsoleInformation.
  SPLIT_LIST                   SplitList;           ///< List of Splits in FILO stack.
  VOID                         *CtrlCNotifyHandle1; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlCNotifyHandle2; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlCNotifyHandle3; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlCNotifyHandle4; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlSNotifyHandle1; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlSNotifyHandle2; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlSNotifyHandle3; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                         *CtrlSNotifyHandle4; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  BOOLEAN                      HaltOutput;          ///< TRUE to start a CTRL-S halt.
} SHELL_PROTOCOL_INTERACTIVITY_INFO;

extern SHELL_PROTOCOL_INTERACTIVITY_INFO  ShellProtocolInteractivityInfoObject;

/**
  Determine if the UEFI Shell is currently running with nesting enabled or disabled.

  @retval FALSE   nesting is required
  @retval other   nesting is enabled
**/
BOOLEAN
EFIAPI
NestingEnabled (
  VOID
  );

EFI_STATUS
CleanUpInteractiveShellEnvironment (
  VOID
  );

/**
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  This will also update the system table.

  @param[in, out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine              The new command line to parse and use.
  @param[out] OldStdIn                   Pointer to old StdIn.
  @param[out] OldStdOut                  Pointer to old StdOut.
  @param[out] OldStdErr                  Pointer to old StdErr.
  @param[out] SystemTableInfo            Pointer to old system table information.

  @retval   EFI_SUCCESS                 Operation was successful, Argv and Argc are valid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
UpdateStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             *NewCommandLine,
  OUT SHELL_FILE_HANDLE                 *OldStdIn,
  OUT SHELL_FILE_HANDLE                 *OldStdOut,
  OUT SHELL_FILE_HANDLE                 *OldStdErr,
  OUT SYSTEM_TABLE_INFO                 *SystemTableInfo
  );

/**
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure with StdIn and StdOut.  The current values are de-allocated.

  @param[in, out] ShellParameters      Pointer to parameter structure to modify.
  @param[in] OldStdIn                  Pointer to old StdIn.
  @param[in] OldStdOut                 Pointer to old StdOut.
  @param[in] OldStdErr                 Pointer to old StdErr.
  @param[in] SystemTableInfo           Pointer to old system table information.
**/
EFI_STATUS
RestoreStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN  SHELL_FILE_HANDLE                 *OldStdIn,
  IN  SHELL_FILE_HANDLE                 *OldStdOut,
  IN  SHELL_FILE_HANDLE                 *OldStdErr,
  IN  SYSTEM_TABLE_INFO                 *SystemTableInfo
  );

#endif
