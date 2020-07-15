/** @file
  Define the FIRST_SMI_HANDLER_CONTEXT structure, which is an exchange area
  between the SMM Monarch and the hot-added CPU, for relocating the SMBASE of
  the hot-added CPU.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FIRST_SMI_HANDLER_CONTEXT_H_
#define FIRST_SMI_HANDLER_CONTEXT_H_

//
// The following structure is used to communicate between the SMM Monarch
// (running the root MMI handler) and the hot-added CPU (handling its first
// SMI). It is placed at SMM_DEFAULT_SMBASE, which is in SMRAM under QEMU's
// "SMRAM at default SMBASE" feature.
//
#pragma pack (1)
typedef struct {
  //
  // When ApicIdGate is MAX_UINT64, then no hot-added CPU may proceed with
  // SMBASE relocation.
  //
  // Otherwise, the hot-added CPU whose APIC ID equals ApicIdGate may proceed
  // with SMBASE relocation.
  //
  // This field is intentionally wider than APIC_ID (UINT32) because we need a
  // "gate locked" value that is different from all possible APIC_IDs.
  //
  UINT64 ApicIdGate;
  //
  // The new SMBASE value for the hot-added CPU to set in the SMRAM Save State
  // Map, before leaving SMM with the RSM instruction.
  //
  UINT32 NewSmbase;
  //
  // The hot-added CPU sets this field to 1 right before executing the RSM
  // instruction. This tells the SMM Monarch to proceed to polling the last
  // byte of the normal RAM reserved page (Post-SMM Pen).
  //
  UINT8 AboutToLeaveSmm;
} FIRST_SMI_HANDLER_CONTEXT;
#pragma pack ()

#endif // FIRST_SMI_HANDLER_CONTEXT_H_
