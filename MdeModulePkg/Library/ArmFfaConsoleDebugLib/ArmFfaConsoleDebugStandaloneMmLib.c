/** @file
  Base Debug library instance based on Arm FF-A Console log.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmStdSmc.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugPrintErrorLevelLib.h>

//
// Define the maximum debug and assert message length that this library supports
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

//
// Define Maxmium available number of registers per FF-A call.
//
#define MAX_REG64_COUNT  16

//
// VA_LIST can not initialize to NULL for all compiler, so we use this to
// indicate a null VA_LIST
//
STATIC VA_LIST  mVaListNull;

/**
  Convert FFA return code to RETURN_STATUS.

  @param [in] FfaStatus          Ffa return Status

  @retval RETURN_STATUS          return value correspond RETURN_STATUS to FfaStatus

**/
STATIC
RETURN_STATUS
EFIAPI
FfaStatusToRetStatus (
  IN UINTN  FfaStatus
  )
{
  switch ((UINT32)FfaStatus) {
    case ARM_FFA_RET_SUCCESS:
      return RETURN_SUCCESS;
    case ARM_FFA_RET_INVALID_PARAMETERS:
      return RETURN_INVALID_PARAMETER;
    case ARM_FFA_RET_NO_MEMORY:
      return RETURN_OUT_OF_RESOURCES;
    case ARM_FFA_RET_BUSY:
      return RETURN_NO_RESPONSE;
    case ARM_FFA_RET_DENIED:
      return RETURN_ACCESS_DENIED;
    case ARM_FFA_RET_ABORTED:
      return RETURN_ABORTED;
    case ARM_FFA_RET_NODATA:
      return RETURN_NOT_FOUND;
    case ARM_FFA_RET_NOT_READY:
      return RETURN_NOT_READY;
    case ARM_FFA_RET_RETRY:
      return RETURN_TIMEOUT;
    default:
      return RETURN_UNSUPPORTED;
  }
}

/**
  Convert FfArgs to RETURN_STATUS.

  @param [in] FfaArgs            Ffa arguments

  @retval RETURN_STATUS           return value correspond RETURN_STATUS to FfaStatus

**/
STATIC
RETURN_STATUS
EFIAPI
FfaArgsToRetStatus (
  IN ARM_FFA_ARGS  *FfaArgs
  )
{
  UINT32  FfaStatus;

  if (FfaArgs == NULL) {
    FfaStatus = ARM_FFA_RET_INVALID_PARAMETERS;
  } else if (IS_FID_FFA_ERROR (FfaArgs->Arg0)) {
    /*
     * In case of error, the Arg0 will be set to the fid FFA_ERROR.
     * and Error code is set in Arg2.
     */
    FfaStatus = FfaArgs->Arg2;
  } else if (FfaArgs->Arg0 == ARM_FFA_RET_NOT_SUPPORTED) {
    /*
     * If Some FF-A ABI doesn't support, it sets ARM_FFA_RET_NOT_SUPPORTED
     * in Arg0 and other register has no meaning.
     * In this case, set Arg2 as ARM_FFA_RET_NOT_SUPPORTED so that
     * FfaStatusToEfiStatus (FfaARgs.Arg2) returns proper EFI_STATUS.
     */
    FfaStatus = ARM_FFA_RET_NOT_SUPPORTED;
  } else {
    FfaStatus = ARM_FFA_RET_SUCCESS;
  }

  return FfaStatusToRetStatus (FfaStatus);
}

/**
  Trigger FF-A ABI call according to PcdFfaLibConduitSmc.

  @param [in, out]  FfaArgs        Ffa arguments

**/
STATIC
VOID
EFIAPI
ArmFfaConsoleCall (
  IN OUT ARM_FFA_ARGS  *FfaArgs
  )
{
  if (PcdGetBool (PcdFfaLibConduitSmc)) {
    ArmCallSmc ((ARM_SMC_ARGS *)FfaArgs);
  } else {
    ArmCallSvc ((ARM_SVC_ARGS *)FfaArgs);
  }
}

/**
  Log through FFA_CONSOLE_LOG supervisor call.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes written.
**/
STATIC
UINTN
EFIAPI
ArmFfaConsoleLog (
  IN CHAR8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  RETURN_STATUS  Status;
  UINTN          MaxCharsPerArmFfaConsoleCall;
  UINTN          RemainBytes;
  UINTN          WriteBytes;
  ARM_FFA_ARGS   FfaArgs;

  MaxCharsPerArmFfaConsoleCall = sizeof (UINT64) * MAX_REG64_COUNT;
  RemainBytes                  = NumberOfBytes;

  while (RemainBytes > 0) {
    ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));
    WriteBytes = (RemainBytes > MaxCharsPerArmFfaConsoleCall) ? MaxCharsPerArmFfaConsoleCall : RemainBytes;

    /*
     * StandaloneMm in Arm is supported 64-bit only so
     * SPMC must be in the 64-bit execution state and
     * according to FF-A specification Chapter 11, bullet 6
     * SPMC must implement both SMC32 and SMC64 conventions of the ABI
     * if it runs in the 64-bit execution state.
     *
     * Therefore, use FFA_CONSOLE_LOG_AARH64 only.
     */
    FfaArgs.Arg0 = ARM_FID_FFA_CONSOLE_LOG_AARCH64;
    FfaArgs.Arg1 = WriteBytes;
    CopyMem (&FfaArgs.Arg2, Buffer, WriteBytes);

    ArmFfaConsoleCall (&FfaArgs);
    Status = FfaArgsToRetStatus (&FfaArgs);

    if (Status == RETURN_TIMEOUT) {
      WriteBytes = FfaArgs.Arg3;
    } else if (RETURN_ERROR (Status)) {
      break;
    }

    RemainBytes -= WriteBytes;
    Buffer      += WriteBytes;
  }

  return NumberOfBytes - RemainBytes;
}

/**
  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and the
  associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  ...         Variable argument list whose contents are accessed
                      based on the format string specified by Format.

**/
VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  VA_LIST  Marker;

  VA_START (Marker, Format);
  DebugVPrint (ErrorLevel, Format, Marker);
  VA_END (Marker);
}

/**
  Prints a debug message to the debug output device if the specified
  error level is enabled base on Null-terminated format string and a
  VA_LIST argument list or a BASE_LIST argument list.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel      The error level of the debug message.
  @param  Format          Format string for the debug message to print.
  @param  VaListMarker    VA_LIST marker for the variable argument list.
  @param  BaseListMarker  BASE_LIST marker for the variable argument list.

**/
VOID
DebugPrintMarker (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker,
  IN  BASE_LIST    BaseListMarker
  )
{
  CHAR8  Buffer[MAX_DEBUG_MESSAGE_LENGTH];

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Check driver debug mask value and global mask
  //
  if ((ErrorLevel & GetDebugPrintErrorLevel ()) == 0) {
    return;
  }

  //
  // Convert the DEBUG() message to an ASCII String
  //
  if (BaseListMarker == NULL) {
    AsciiVSPrint (Buffer, sizeof (Buffer), Format, VaListMarker);
  } else {
    AsciiBSPrint (Buffer, sizeof (Buffer), Format, BaseListMarker);
  }

  //
  // Send the print string to the FF-A console for tracing.
  //
  ArmFfaConsoleLog (Buffer, AsciiStrLen (Buffer));
}

/**
  Prints a debug message to the debug output device if the specified
  error level is enabled.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel    The error level of the debug message.
  @param  Format        Format string for the debug message to print.
  @param  VaListMarker  VA_LIST marker for the variable argument list.

**/
VOID
EFIAPI
DebugVPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker
  )
{
  DebugPrintMarker (ErrorLevel, Format, VaListMarker, NULL);
}

/**
  Prints a debug message to the debug output device if the specified
  error level is enabled.
  This function use BASE_LIST which would provide a more compatible
  service than VA_LIST.

  If any bit in ErrorLevel is also set in DebugPrintErrorLevelLib function
  GetDebugPrintErrorLevel (), then print the message specified by Format and
  the associated variable argument list to the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel      The error level of the debug message.
  @param  Format          Format string for the debug message to print.
  @param  BaseListMarker  BASE_LIST marker for the variable argument list.

**/
VOID
EFIAPI
DebugBPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  IN  BASE_LIST    BaseListMarker
  )
{
  DebugPrintMarker (ErrorLevel, Format, mVaListNull, BaseListMarker);
}

/**
  Prints an assert message containing a filename, line number, and description.
  This may be followed by a breakpoint or a dead loop.

  Print a message of the form "ASSERT <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of
  PcdDebugProperyMask is set then CpuBreakpoint() is called. Otherwise, if
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugProperyMask is set then
  CpuDeadLoop() is called.  If neither of these bits are set, then this function
  returns immediately after the message is printed to the debug output device.
  DebugAssert() must actively prevent recursion.  If DebugAssert() is called while
  processing another DebugAssert(), then DebugAssert() must return immediately.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     The pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  The pointer to the description of the assert condition.

**/
VOID
EFIAPI
DebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  Buffer[MAX_DEBUG_MESSAGE_LENGTH];

  //
  // Generate the ASSERT() message in Ascii format
  //
  AsciiSPrint (Buffer, sizeof (Buffer), "ASSERT [%a] %a(%d): %a\n", gEfiCallerBaseName, FileName, LineNumber, Description);

  //
  // Send the print string to the FF-A Console.
  //
  ArmFfaConsoleLog (Buffer, AsciiStrLen (Buffer));

  //
  // Generate a Breakpoint, DeadLoop, or NOP based on PCD settings
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED) != 0) {
    CpuBreakpoint ();
  } else if ((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED) != 0) {
    CpuDeadLoop ();
  }
}

/**
  Fills a target buffer with PcdDebugClearMemoryValue, and returns the target buffer.

  This function fills Length bytes of Buffer with the value specified by
  PcdDebugClearMemoryValue, and returns Buffer.

  If Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param   Buffer  The pointer to the target buffer to be filled with PcdDebugClearMemoryValue.
  @param   Length  The number of bytes in Buffer to fill with zeros PcdDebugClearMemoryValue.

  @return  Buffer  The pointer to the target buffer filled with PcdDebugClearMemoryValue.

**/
VOID *
EFIAPI
DebugClearMemory (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  //
  // If Buffer is NULL, then ASSERT().
  //
  ASSERT (Buffer != NULL);

  //
  // SetMem() checks for the the ASSERT() condition on Length and returns Buffer
  //
  return SetMem (Buffer, Length, PcdGet8 (PcdDebugClearMemoryValue));
}

/**
  Returns TRUE if ASSERT() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  )
{
  return (BOOLEAN)((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED) != 0);
}

/**
  Returns TRUE if DEBUG() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  )
{
  return (BOOLEAN)((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_PRINT_ENABLED) != 0);
}

/**
  Returns TRUE if DEBUG_CODE() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  )
{
  return (BOOLEAN)((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_DEBUG_CODE_ENABLED) != 0);
}

/**
  Returns TRUE if DEBUG_CLEAR_MEMORY() macro is enabled.

  This function returns TRUE if the DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugClearMemoryEnabled (
  VOID
  )
{
  return (BOOLEAN)((PcdGet8 (PcdDebugPropertyMask) & DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED) != 0);
}

/**
  Returns TRUE if any one of the bit is set both in ErrorLevel and PcdFixedDebugPrintErrorLevel.

  This function compares the bit mask of ErrorLevel and PcdFixedDebugPrintErrorLevel.

  @retval  TRUE    Current ErrorLevel is supported.
  @retval  FALSE   Current ErrorLevel is not supported.

**/
BOOLEAN
EFIAPI
DebugPrintLevelEnabled (
  IN  CONST UINTN  ErrorLevel
  )
{
  return (BOOLEAN)((ErrorLevel & PcdGet32 (PcdFixedDebugPrintErrorLevel)) != 0);
}
