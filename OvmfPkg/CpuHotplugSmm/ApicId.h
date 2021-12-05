/** @file
  Type and macro definitions for representing and printing APIC IDs, compatibly
  with the LocalApicLib and PrintLib classes, respectively.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef APIC_ID_H_
#define APIC_ID_H_

//
// The type that LocalApicLib represents an APIC ID with.
//
typedef UINT32 APIC_ID;

//
// The PrintLib conversion specification for formatting an APIC_ID.
//
#define FMT_APIC_ID  "0x%08x"

#endif // APIC_ID_H_
