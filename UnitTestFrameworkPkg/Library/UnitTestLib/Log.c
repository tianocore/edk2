/**
  Implemnet UnitTestLib log services

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <UnitTestFrameworkTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>

#define UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH  (512)
#define UNIT_TEST_MAX_LOG_BUFFER                SIZE_16KB

struct _UNIT_TEST_LOG_PREFIX_STRING {
  UNIT_TEST_STATUS    LogLevel;
  CHAR8               *String;
};

struct _UNIT_TEST_LOG_PREFIX_STRING  mLogPrefixStrings[] = {
  { UNIT_TEST_LOG_LEVEL_ERROR,   "[ERROR]       " },
  { UNIT_TEST_LOG_LEVEL_WARN,    "[WARNING]     " },
  { UNIT_TEST_LOG_LEVEL_INFO,    "[INFO]        " },
  { UNIT_TEST_LOG_LEVEL_VERBOSE, "[VERBOSE]     " }
};

//
// Unit-Test Log helper functions
//

STATIC
CONST CHAR8 *
GetStringForStatusLogPrefix (
  IN UINTN  LogLevel
  )
{
  UINTN  Index;
  CHAR8  *Result;

  Result = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mLogPrefixStrings); Index++) {
    if (mLogPrefixStrings[Index].LogLevel == LogLevel) {
      Result = mLogPrefixStrings[Index].String;
      break;
    }
  }

  return Result;
}

STATIC
EFI_STATUS
AddStringToUnitTestLog (
  IN OUT UNIT_TEST    *UnitTest,
  IN     CONST CHAR8  *String
  )
{
  EFI_STATUS  Status;

  //
  // Make sure that you're cooking with gas.
  //
  if ((UnitTest == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // If this is the first log for the test allocate log space
  if (UnitTest->Log == NULL) {
    UnitTestLogInit (UnitTest, NULL, 0);
  }

  if (UnitTest->Log == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate space for unit test log\n"));
    ASSERT (UnitTest->Log != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AsciiStrnCatS (
             UnitTest->Log,
             UNIT_TEST_MAX_LOG_BUFFER / sizeof (CHAR8),
             String,
             UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to add unit test log string.  Status = %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function is responsible for initializing the log buffer for a single test. It can
  be used internally, but may also be consumed by the test framework to add pre-existing
  data to a log before it's used.

  @param[in,out]  TestHandle    A handle to the test being initialized.
  @param[in]      Buffer        [Optional] A pointer to pre-existing log data that should
                                be used to initialize the log. Should include a NULL terminator.
  @param[in]      BufferSize    [Optional] The size of the pre-existing log data.

**/
VOID
EFIAPI
UnitTestLogInit (
  IN OUT UNIT_TEST  *Test,
  IN     UINT8      *Buffer      OPTIONAL,
  IN     UINTN      BufferSize   OPTIONAL
  )
{
  //
  // Make sure that you're cooking with gas.
  //
  if (Test == NULL) {
    DEBUG ((DEBUG_ERROR, "%a called with invalid Test parameter\n", __FUNCTION__));
    return;
  }

  //
  // If this is the first log for the test allocate log space
  //
  if (Test->Log == NULL) {
    Test->Log = AllocateZeroPool (UNIT_TEST_MAX_LOG_BUFFER);
  }

  //
  // check again to make sure allocate worked
  //
  if (Test->Log == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for the log\n"));
    return;
  }

  if ((Buffer != NULL) && (BufferSize > 0) && (BufferSize <= UNIT_TEST_MAX_LOG_BUFFER)) {
    CopyMem (Test->Log, Buffer, BufferSize);
  }
}

/**
  Test logging function that records a messages in the test framework log.
  Record is associated with the currently executing test case.

  @param[in]  ErrorLevel  The error level of the unit test log message.
  @param[in]  Format      Formatting string following the format defined in the
                          MdePkg/Include/Library/PrintLib.h.
  @param[in]  ...         Print args.
**/
VOID
EFIAPI
UnitTestLog (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle;
  CHAR8                       NewFormatString[UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH];
  CHAR8                       LogString[UNIT_TEST_MAX_SINGLE_LOG_STRING_LENGTH];
  CONST CHAR8                 *LogTypePrefix;
  VA_LIST                     Marker;

  FrameworkHandle = GetActiveFrameworkHandle ();

  LogTypePrefix = NULL;

  //
  // Make sure that this unit test log level is enabled.
  //
  if ((ErrorLevel & (UINTN)PcdGet32 (PcdUnitTestLogLevel)) == 0) {
    return;
  }

  //
  // If we need to define a new format string...
  // well... get to it.
  //
  LogTypePrefix = GetStringForStatusLogPrefix (ErrorLevel);
  if (LogTypePrefix != NULL) {
    AsciiSPrint (NewFormatString, sizeof (NewFormatString), "%a%a", LogTypePrefix, Format);
  } else {
    AsciiStrCpyS (NewFormatString, sizeof (NewFormatString), Format);
  }

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (LogString, sizeof (LogString), NewFormatString, Marker);
  VA_END (Marker);

  //
  // Finally, add the string to the log.
  //
  AddStringToUnitTestLog (((UNIT_TEST_FRAMEWORK *)FrameworkHandle)->CurrentTest, LogString);
}
