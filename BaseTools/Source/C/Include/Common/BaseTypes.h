/** @file
  Processor or Compiler specific defines for all supported processors.

  This file is stand alone self consistent set of definitions.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BASE_TYPES_H__
#define __BASE_TYPES_H__

//
// Include processor specific binding
//
#include <ProcessorBind.h>
#include <stdarg.h>

//
// Modifiers to abstract standard types to aid in debug of problems
//
#define CONST     const
#define STATIC    static
#define VOID      void

//
// Modifiers for Data Types used to self document code.
// This concept is borrowed for UEFI specification.
//
#ifndef IN
//
// Some other environments use this construct, so #ifndef to prevent
// multiple definition.
//
#define IN
#define OUT
#define OPTIONAL
#endif

//
// Constants. They may exist in other build structures, so #ifndef them.
//
#ifndef TRUE
//
// BugBug: UEFI specification claims 1 and 0. We are concerned about the
//  compiler portability so we did it this way.
//
#define TRUE  ((BOOLEAN)(1==1))
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN)(0==1))
#endif

#ifndef NULL
#define NULL  ((VOID *) 0)
#endif

#define PACKED

//
//  Support for variable length argument lists using the ANSI standard.
//
//  Since we are using the ANSI standard we used the standard naming and
//  did not follow the coding convention
//
//  VA_LIST  - typedef for argument list.
//  VA_START (VA_LIST Marker, argument before the ...) - Init Marker for use.
//  VA_END (VA_LIST Marker) - Clear Marker
//  VA_ARG (VA_LIST Marker, var arg size) - Use Marker to get an argument from
//    the ... list. You must know the size and pass it in this macro.
//
//  example:
//
//  UINTN
//  ExampleVarArg (
//    IN UINTN  NumberOfArgs,
//    ...
//    )
//  {
//    VA_LIST Marker;
//    UINTN   Index;
//    UINTN   Result;
//
//    //
//    // Initialize the Marker
//    //
//    VA_START (Marker, NumberOfArgs);
//    for (Index = 0, Result = 0; Index < NumberOfArgs; Index++) {
//      //
//      // The ... list is a series of UINTN values, so average them up.
//      //
//      Result += VA_ARG (Marker, UINTN);
//    }
//
//    VA_END (Marker);
//    return Result
//  }
//

#define _INT_SIZE_OF(n) ((sizeof (n) + sizeof (UINTN) - 1) &~(sizeof (UINTN) - 1))

//
// Also support coding convention rules for var arg macros
//
#ifndef VA_START

// typedef CHAR8 *VA_LIST;
// #define VA_START(ap, v) (ap = (VA_LIST) & (v) + _INT_SIZE_OF (v))
// #define VA_ARG(ap, t)   (*(t *) ((ap += _INT_SIZE_OF (t)) - _INT_SIZE_OF (t)))
// #define VA_END(ap)      (ap = (VA_LIST) 0)
// Use the native arguments for tools.
#define VA_START va_start
#define VA_ARG   va_arg
#define VA_END   va_end
#define VA_LIST  va_list

#endif

#ifndef GUID_DEFINED
#define GUID_DEFINED
///
/// 128 bit buffer containing a unique identifier value.
/// Unless otherwise specified, aligned on a 64 bit boundary.
///
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} GUID;
#endif

///
/// 4-byte buffer. An IPv4 internet protocol address.
///
typedef struct {
  UINT8 Addr[4];
} IPv4_ADDRESS;

///
/// 16-byte buffer. An IPv6 internet protocol address.
///
typedef struct {
  UINT8 Addr[16];
} IPv6_ADDRESS;

//
// Macro that returns the byte offset of a field in a data structure.
//
#define OFFSET_OF(TYPE, Field) ((UINTN) &(((TYPE *)0)->Field))

///
///  _CR - returns a pointer to the structure
///      from one of its elements.
///
#define _CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))

///
///  ALIGN_POINTER - aligns a pointer to the lowest boundary
///
#define ALIGN_POINTER(p, s) ((VOID *) ((UINTN)(p) + (((s) - ((UINTN) (p))) & ((s) - 1))))

///
///  ALIGN_VARIABLE - aligns a variable up to the next natural boundary for int size of a processor
///
#define ALIGN_VARIABLE(Value, Adjustment) \
  Adjustment = 0U; \
  if ((UINTN) (Value) % sizeof (UINTN)) { \
    (Adjustment) = (UINTN)(sizeof (UINTN) - ((UINTN) (Value) % sizeof (UINTN))); \
  } \
  (Value) = (UINTN)((UINTN) (Value) + (UINTN) (Adjustment))

//
// Return the maximum of two operands.
// This macro returns the maximum of two operand specified by a and b.
// Both a and b must be the same numerical types, signed or unsigned.
//
#define MAX(a, b)                       \
  (((a) > (b)) ? (a) : (b))


//
// Return the minimum of two operands.
// This macro returns the minimal of two operand specified by a and b.
// Both a and b must be the same numerical types, signed or unsigned.
//
#define MIN(a, b)                       \
  (((a) < (b)) ? (a) : (b))


//
// EFI Error Codes common to all execution phases
//

typedef UINTN RETURN_STATUS;

///
/// Set the upper bit to indicate EFI Error.
///
#define ENCODE_ERROR(a)              ((RETURN_STATUS)(MAX_BIT | (a)))

#define ENCODE_WARNING(a)            ((RETURN_STATUS)(a))
#define RETURN_ERROR(a)              (((RETURN_STATUS)(a)) >= MAX_BIT)

#define RETURN_SUCCESS               0
#define RETURN_LOAD_ERROR            ENCODE_ERROR (1)
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)
#define RETURN_UNSUPPORTED           ENCODE_ERROR (3)
#define RETURN_BAD_BUFFER_SIZE       ENCODE_ERROR (4)
#define RETURN_BUFFER_TOO_SMALL      ENCODE_ERROR (5)
#define RETURN_NOT_READY             ENCODE_ERROR (6)
#define RETURN_DEVICE_ERROR          ENCODE_ERROR (7)
#define RETURN_WRITE_PROTECTED       ENCODE_ERROR (8)
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)
#define RETURN_VOLUME_CORRUPTED      ENCODE_ERROR (10)
#define RETURN_VOLUME_FULL           ENCODE_ERROR (11)
#define RETURN_NO_MEDIA              ENCODE_ERROR (12)
#define RETURN_MEDIA_CHANGED         ENCODE_ERROR (13)
#define RETURN_NOT_FOUND             ENCODE_ERROR (14)
#define RETURN_ACCESS_DENIED         ENCODE_ERROR (15)
#define RETURN_NO_RESPONSE           ENCODE_ERROR (16)
#define RETURN_NO_MAPPING            ENCODE_ERROR (17)
#define RETURN_TIMEOUT               ENCODE_ERROR (18)
#define RETURN_NOT_STARTED           ENCODE_ERROR (19)
#define RETURN_ALREADY_STARTED       ENCODE_ERROR (20)
#define RETURN_ABORTED               ENCODE_ERROR (21)
#define RETURN_ICMP_ERROR            ENCODE_ERROR (22)
#define RETURN_TFTP_ERROR            ENCODE_ERROR (23)
#define RETURN_PROTOCOL_ERROR        ENCODE_ERROR (24)
#define RETURN_INCOMPATIBLE_VERSION  ENCODE_ERROR (25)
#define RETURN_SECURITY_VIOLATION    ENCODE_ERROR (26)
#define RETURN_CRC_ERROR             ENCODE_ERROR (27)
#define RETURN_END_OF_MEDIA          ENCODE_ERROR (28)
#define RETURN_END_OF_FILE           ENCODE_ERROR (31)

#define RETURN_WARN_UNKNOWN_GLYPH    ENCODE_WARNING (1)
#define RETURN_WARN_DELETE_FAILURE   ENCODE_WARNING (2)
#define RETURN_WARN_WRITE_FAILURE    ENCODE_WARNING (3)
#define RETURN_WARN_BUFFER_TOO_SMALL ENCODE_WARNING (4)

typedef UINT64 PHYSICAL_ADDRESS;

#define  BIT0     0x00000001
#define  BIT1     0x00000002
#define  BIT2     0x00000004
#define  BIT3     0x00000008
#define  BIT4     0x00000010
#define  BIT5     0x00000020
#define  BIT6     0x00000040
#define  BIT7     0x00000080
#define  BIT8     0x00000100
#define  BIT9     0x00000200
#define  BIT10    0x00000400
#define  BIT11    0x00000800
#define  BIT12    0x00001000
#define  BIT13    0x00002000
#define  BIT14    0x00004000
#define  BIT15    0x00008000
#define  BIT16    0x00010000
#define  BIT17    0x00020000
#define  BIT18    0x00040000
#define  BIT19    0x00080000
#define  BIT20    0x00100000
#define  BIT21    0x00200000
#define  BIT22    0x00400000
#define  BIT23    0x00800000
#define  BIT24    0x01000000
#define  BIT25    0x02000000
#define  BIT26    0x04000000
#define  BIT27    0x08000000
#define  BIT28    0x10000000
#define  BIT29    0x20000000
#define  BIT30    0x40000000
#define  BIT31    0x80000000
#define  BIT32    0x0000000100000000ULL
#define  BIT33    0x0000000200000000ULL
#define  BIT34    0x0000000400000000ULL
#define  BIT35    0x0000000800000000ULL
#define  BIT36    0x0000001000000000ULL
#define  BIT37    0x0000002000000000ULL
#define  BIT38    0x0000004000000000ULL
#define  BIT39    0x0000008000000000ULL
#define  BIT40    0x0000010000000000ULL
#define  BIT41    0x0000020000000000ULL
#define  BIT42    0x0000040000000000ULL
#define  BIT43    0x0000080000000000ULL
#define  BIT44    0x0000100000000000ULL
#define  BIT45    0x0000200000000000ULL
#define  BIT46    0x0000400000000000ULL
#define  BIT47    0x0000800000000000ULL
#define  BIT48    0x0001000000000000ULL
#define  BIT49    0x0002000000000000ULL
#define  BIT50    0x0004000000000000ULL
#define  BIT51    0x0008000000000000ULL
#define  BIT52    0x0010000000000000ULL
#define  BIT53    0x0020000000000000ULL
#define  BIT54    0x0040000000000000ULL
#define  BIT55    0x0080000000000000ULL
#define  BIT56    0x0100000000000000ULL
#define  BIT57    0x0200000000000000ULL
#define  BIT58    0x0400000000000000ULL
#define  BIT59    0x0800000000000000ULL
#define  BIT60    0x1000000000000000ULL
#define  BIT61    0x2000000000000000ULL
#define  BIT62    0x4000000000000000ULL
#define  BIT63    0x8000000000000000ULL

#endif
