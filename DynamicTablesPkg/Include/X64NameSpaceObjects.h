/** @file

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - X64        - X64 processor architecture
  **/

#ifndef X64_NAMESPACE_OBJECT_H_
#define X64_NAMESPACE_OBJECT_H_

/** The EX64_OBJECT_ID enum describes the Object IDs
    in the X64 Namespace
*/
typedef enum X64ObjectID {
  EX64ObjReserved,                         ///<  0 - Reserved
  EX64ObjMadtLocalInterruptInfo,           ///<  1 - MADT Local Interrupt Information
  EX64ObjMadtProcessorLocalApicX2ApicInfo, ///<  2 - MADT Local APIC/x2APIC Controller Information
  EX64ObjMadtIoApicInfo,                   ///<  3 - MADT IOAPIC Information
  EX64ObjMadtLocalApicX2ApicNmiInfo,       ///<  4 - MADT Local APIC/x2APIC NMI Information
  EX64ObjMadtInterruptSourceOverrideInfo,  ///<  5 - MADT Interrupt Override Information
  EX64ObjMax
} E_X64_OBJECT_ID;

/** A structure that describes the
    MADT Local Interrupt Information for the Platform.

    ID: EX64ObjMadtLocalInterruptInfo
*/
typedef struct CmX64MadtLocalInterruptInfo {
  UINT32    LocalApicAddress; ///< Local Interrupt Controller Address
  UINT32    Flags;            ///< Flags
} CM_X64_MADT_LOCAL_INTERRUPT_INFO;

/** A structure that describes the
    MADT Interrupt controller type information for the platform.

    ID: EX64ObjMadtInterruptControllerTypeInfo
*/
typedef struct CmX64MadtInterruptControllerTypeInfo {
  VOID     *InterruptControllerTypeInfo; ///< Interrupt Controller Type Information
  UINTN    Size;                         ///< Size of the Interrupt Controller Type Information
} CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO;
#endif
