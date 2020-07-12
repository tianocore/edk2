/** @file
  Header file for 'acpiview' logging and output facility

  Copyright (c) 2020, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_VIEW_LOG_H_
#define ACPI_VIEW_LOG_H_

/**
  Categories of errors that can be logged by AcpiView.
*/
typedef enum {
  ACPI_ERROR_NONE,        ///< Not an error
  ACPI_ERROR_GENERIC,     ///< An unspecified error
  ACPI_ERROR_CSUM,        ///< A checksum was invalid
  ACPI_ERROR_PARSE,       ///< Failed to parse item
  ACPI_ERROR_LENGTH,      ///< Size of a thing is incorrect
  ACPI_ERROR_VALUE,       ///< The value of a field was incorrect
  ACPI_ERROR_CROSS,       ///< A constraint on multiple items was violated
  ACPI_ERROR_MAX
} ACPI_ERROR_TYPE;

/**
  Different severities of events that can be logged.
*/
typedef enum {
  ACPI_DEBUG,       ///< Will not be shown unless specified on command line
  ACPI_INFO,        ///< Standard output
  ACPI_GOOD,        ///< Unspecified good outcome, green colour
  ACPI_BAD,         ///< Unspecified bad outcome, red colour
  ACPI_ITEM,        ///< Used when describing multiple items
  ACPI_HIGHLIGHT,   ///< A new context or section has been entered
  ACPI_WARN,        ///< An unusual event happened
  ACPI_ERROR,       ///< Acpi table is not conformant
  ACPI_FATAL        ///< This will abort program execution.
} ACPI_LOG_SEVERITY;

// Publicly accessible error and warning counters.
extern UINT32   mTableErrorCount;
extern UINT32   mTableWarningCount;

/**
  AcpiView output and logging function. Will log the event to
  configured output (currently screen) and annotate with colour
  and extra metadata.

  @param[in] FileName      The full filename of the source file where this
                           event occured.
  @param[in] FunctionName  The name of the function where this event occured.
  @param[in] LineNumber    The line number in the source code where this event
                           occured.
  @param[in] Severity      The severity of the event that occured.
  @param[in] Error         The type of the erorr reported. May be ACPI_ERROR_NONE if the event
                           is not an error.
  @param[in] Format        The format of the string describing the event.
  @param[in] ...           The variable number of parameters that will format the
                           string supplied in Format.
**/
VOID
EFIAPI
AcpiViewLog (
  IN const CHAR8       *FileName,
  IN const CHAR8       *FunctionName,
  IN UINTN             LineNumber,
  IN ACPI_ERROR_TYPE   Error,
  IN ACPI_LOG_SEVERITY Severity,
  IN const CHAR16      *Format,
  ...
  );

/**
  Formats and prints and ascii string to screen.

  @param[in] Format String that will be formatted and printed.
  @param[in] ...    A variable number of parameters that will be formatted.
**/
VOID
EFIAPI
AcpiViewOutput (
  IN const CHAR16 *Format,
  IN ...
  );

/**
  Check that a buffer has not been overrun by a member. Can be invoked
  using the BufferOverrun macro that fills in local source metadata
  (first three parameters) for logging purposes.

  @param[in] FileName        Source file where this invocation is made
  @param[in] FunctionName    Name of the local symbol
  @param[in] LineNumber      Source line number of the local call
  @param[in] ItemDescription User friendly name for the member being checked
  @param[in] Position        Memory address of the member
  @param[in] Length          Length of the member
  @param[in] Buffer          Memory address of the buffer where member resides
  @param[in] BufferSize      Size of the buffer where member resides

  @retval TRUE               Buffer was overrun
  @retval FALSE              Buffer is safe
**/
BOOLEAN
EFIAPI
MemberIntegrityInternal (
  IN const CHAR8  *FileName,
  IN const CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN const CHAR8 *ItemDescription,
  IN UINTN        Offset,
  IN UINTN        Length,
  IN VOID         *Buffer,
  IN UINTN        BufferSize
  );

// Determine if a member located at Offset into a Buffer lies entirely
// within the BufferSize given the member size is Length
// Evaluates to TRUE and logs error if buffer is overrun or if Length is zero
#define AssertMemberIntegrity(Offset, Length, Buffer, BufferSize) \
  MemberIntegrityInternal (__FILE__, __func__, __LINE__, #Length, Offset, Length, Buffer, BufferSize)


/**
  Checks that a boolean constraint evaluates correctly. Can be invoked
  using the CheckConstraint macro that fills in the source code metadata.

  @param[in] FileName        Source file where this invocation is made
  @param[in] FunctionName    Name of the local symbol
  @param[in] LineNumber      Source line number of the local call
  @param[in] ConstraintText  The Source code of the constraint
  @param[in] Specification   The specification that imposes the constraint
  @param[in] Constraint      The actual constraint
  @

  @retval TRUE               Constraint is violated
  @retval FALSE              Constraint is not violated
**/
BOOLEAN
EFIAPI
CheckConstraintInternal (
  IN const CHAR8       *FileName,
  IN const CHAR8       *FunctionName,
  IN UINTN             LineNumber,
  IN const CHAR8       *ConstraintText,
  IN const CHAR16      *Specification,
  IN BOOLEAN           Constraint,
  IN ACPI_LOG_SEVERITY Severity
  );

// Evaluates to TRUE and logs error if a constraint is violated
// Constraint internally cast to BOOLEAN using !!(Constraint)
#define AssertConstraint(Specification, Constraint) \
  CheckConstraintInternal (                         \
    __FILE__,                                       \
    __func__,                                       \
    __LINE__,                                       \
    #Constraint,                                    \
    Specification,                                  \
    !!(Constraint),                                 \
    ACPI_ERROR)

// Evaluates to TRUE and logs error if a constraint is violated
// Constraint internally cast to BOOLEAN using !!(Constraint)
#define WarnConstraint(Specification, Constraint) \
  CheckConstraintInternal (                       \
    __FILE__,                                     \
    __func__,                                     \
    __LINE__,                                     \
    #Constraint,                                  \
    Specification,                                \
    !!(Constraint),                               \
    ACPI_WARN)


// Maximum string size that can be printed
#define MAX_OUTPUT_SIZE 256

#define _AcpiLog(...) AcpiViewLog(__FILE__, __func__, __LINE__, __VA_ARGS__)

// Generic Loging macro, needs severity and formatted string
#define AcpiLog(Severity, ...) _AcpiLog(ACPI_ERROR_NONE, Severity, __VA_ARGS__)

// Log undecorated text, needs formatted string
#define AcpiInfo(...) _AcpiLog(ACPI_ERROR_NONE, ACPI_INFO, __VA_ARGS__)

// Log error and increment counter, needs error type and formatted string
#define AcpiError(Error, ...) _AcpiLog(Error, ACPI_ERROR, __VA_ARGS__)

// Log a FATAL error, needs formatted string
#define AcpiFatal(...) _AcpiLog(ACPI_ERROR_GENERIC, ACPI_FATAL, __VA_ARGS__)

#endif // ACPI_VIEW_LOG_H_
